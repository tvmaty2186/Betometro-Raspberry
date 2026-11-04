#include "wifi.h"

#include <QHostAddress>
#include <QNetworkInterface>
#include <QDateTime>
#include <QProcess>

#include <unistd.h>

#include <algorithm>
#include <memory>

namespace {
int rayitas(int porcentaje) { return porcentaje >= 75 ? 4 : porcentaje >= 55 ? 3 : porcentaje >= 35 ? 2 : porcentaje > 0 ? 1 : 0; }
}

Wifi::Wifi(QObject* padre) : QObject(padre), real_(qEnvironmentVariableIsSet("BETO_RASPBERRY")) {
    if (!real_) {   // Simulado: lo que se vería en casa
        redes_ = {{"Casa", 3, true}, {"Vecino_2.4G", 4, true}, {"Fibertel WiFi 552", 2, true}, {"Invitados", 1, false}};
        guardadas_ = {"Casa", "Celu de Matías"};
        conectada_ = "Casa";
        senalConectada_ = 3;
        return;
    }
    connect(&cada_, &QTimer::timeout, this, &Wifi::refrescar);
    connect(&cada_, &QTimer::timeout, this, &Wifi::revisarPropia);
    cada_.start(10000);
    refrescar();
    QTimer::singleShot(3000, this, &Wifi::buscar);
}

QStringList Wifi::campos(const QString& linea) {
    QStringList r;
    QString actual;
    for (int i = 0; i < linea.size(); i++) {
        const QChar ch = linea[i];
        if (ch == '\\' && i + 1 < linea.size()) { actual += linea[++i]; continue; }
        if (ch == ':') { r << actual; actual.clear(); continue; }
        actual += ch;
    }
    r << actual;
    return r;
}

void Wifi::nmcli(const QStringList& args, std::function<void(int, const QString&, const QString&)> listo) {
    auto* p = new QProcess(this);
    p->setChildProcessModifier([] { [[maybe_unused]] const int r = ::nice(10); });   // el wifi espera: primero GPS, cronómetro y pulsador
    connect(p, &QProcess::finished, this, [p, listo](int codigo, QProcess::ExitStatus) {
        listo(codigo, QString::fromUtf8(p->readAllStandardOutput()), QString::fromUtf8(p->readAllStandardError()).trimmed());
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this, [p, listo](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) { listo(-1, {}, QStringLiteral("No está nmcli")); p->deleteLater(); }
    });
    p->start(QStringLiteral("nmcli"), args);
}

namespace { const QString kConPropia = QStringLiteral("betometro-propia"); }

void Wifi::ponerPropia(bool permitida, const QString& numeroAuto) {
    const QString n = numeroAuto.trimmed().isEmpty() ? QStringLiteral("101") : numeroAuto.trimmed();
    if (n != numero_) { numero_ = n; emit cambio(); }
    propiaPermitida_ = permitida;
    if (!permitida && propia_) apagarPropia();
}

void Wifi::apagarPropia() {
    if (!real_) { propia_ = false; emit cambio(); return; }
    nmcli({"connection", "down", kConPropia}, [this](int, const QString&, const QString&) { propia_ = false; sinRedDesde_ = 0; refrescar(); emit cambio(); });
}

// Si en 90 s no hay ninguna red guardada (casa, celular), se arma la wifi propia para entrar desde el
// celular a los informes. Se apaga sola al tocar «Buscar redes» o al apagar la opción.
void Wifi::revisarPropia() {
    if (!real_ || propia_ || conectando_) return;
    const qint64 ahora = QDateTime::currentMSecsSinceEpoch();
    if (!conectada_.isEmpty()) { sinRedDesde_ = 0; return; }
    if (sinRedDesde_ == 0) { sinRedDesde_ = ahora; return; }
    if (!propiaPermitida_ || ahora - sinRedDesde_ < 90000) return;
    for (const auto& r : redes_) if (guardadas_.contains(r.nombre)) return;   // hay una guardada cerca: se conecta sola
    nmcli({"dev", "wifi", "hotspot", "ifname", "wlan0", "con-name", kConPropia, "ssid", nombrePropia(), "password", clavePropia()},
          [this](int codigo, const QString&, const QString& err) {
              if (codigo != 0) { qWarning("Wifi propia: no se pudo armar (%s)", qPrintable(err)); sinRedDesde_ = 0; return; }
              propia_ = true;
              emit cambio();
          });
}

void Wifi::refrescar() {
    // ¿Está armada la wifi propia?
    nmcli({"-t", "-f", "NAME", "connection", "show", "--active"}, [this](int, const QString& out, const QString&) {
        const bool p = out.split('\n').contains(kConPropia);
        if (p != propia_) { propia_ = p; emit cambio(); }
    });
    // Red conectada y su señal
    nmcli({"-t", "-f", "ACTIVE,SIGNAL,SSID", "dev", "wifi", "list", "--rescan", "no"}, [this](int, const QString& out, const QString&) {
        QString red;
        int s = 0;
        for (const auto& l : out.split('\n', Qt::SkipEmptyParts)) {
            const QStringList c = campos(l);
            if (c.size() >= 3 && c[0] == QLatin1String("yes")) { red = c.mid(2).join(':'); s = c[1].toInt(); }
        }
        if (red != conectada_ || rayitas(s) != senalConectada_) {
            const bool nueva = !red.isEmpty() && red != conectada_;
            conectada_ = red;
            senalConectada_ = rayitas(s);
            emit cambio();
            if (nueva) emit conectado(red);
        }
    });
    // Redes guardadas: el nombre de la conexión puede no ser el de la red (Raspberry Pi Imager la
    // guarda como «preconfigured»), así que se pregunta el SSID de cada una
    nmcli({"-t", "-f", "NAME,TYPE", "connection", "show"}, [this](int, const QString& out, const QString&) {
        QStringList nombres;
        for (const auto& l : out.split('\n', Qt::SkipEmptyParts)) {
            const QStringList c = campos(l);
            if (c.size() >= 2 && c[1] == QLatin1String("802-11-wireless") && c[0] != kConPropia) nombres << c[0];
        }
        auto pendientes = std::make_shared<int>(static_cast<int>(nombres.size()));
        auto nuevo = std::make_shared<QHash<QString, QString>>();
        if (nombres.isEmpty() && !conexion_.isEmpty()) { conexion_.clear(); guardadas_.clear(); emit cambio(); }
        for (const QString& n : nombres) {
            nmcli({"-g", "802-11-wireless.ssid", "connection", "show", n}, [this, n, pendientes, nuevo](int, const QString& o, const QString&) {
                const QString ssid = o.trimmed().replace(QLatin1String("\\:"), QLatin1String(":"));
                if (!ssid.isEmpty()) nuevo->insert(ssid, n);
                if (--*pendientes == 0 && *nuevo != conexion_) {
                    conexion_ = *nuevo;
                    guardadas_ = conexion_.keys();
                    guardadas_.sort(Qt::CaseInsensitive);
                    emit cambio();
                }
            });
        }
    });
}

int Wifi::senal() const {
    if (real_) return conectada_.isEmpty() ? 0 : senalConectada_;
    for (const auto& r : redes_) if (r.nombre == conectada_) return r.senal;
    return 0;
}

QString Wifi::direccion() const {
    if (!real_) return QStringLiteral("betometro.local");
    for (const auto& i : QNetworkInterface::allInterfaces()) {
        if (!(i.flags() & QNetworkInterface::IsUp) || (i.flags() & QNetworkInterface::IsLoopBack)) continue;
        for (const auto& e : i.addressEntries())
            if (e.ip().protocol() == QAbstractSocket::IPv4Protocol) return e.ip().toString();
    }
    return QStringLiteral("betometro.local");
}

QVariantList Wifi::cerca() const {
    QVariantList l;
    for (const auto& r : redes_)
        if (!guardadas_.contains(r.nombre)) l << QVariantMap{{"nombre", r.nombre}, {"senal", r.senal}, {"clave", r.clave}};
    return l;
}

QVariantList Wifi::guardadas() const {
    QVariantList l;
    for (const auto& g : guardadas_) {
        int s = 0;
        for (const auto& r : redes_) if (r.nombre == g) s = r.senal;
        if (g == conectada_ && s == 0) s = senal();
        l << QVariantMap{{"nombre", g}, {"senal", s}, {"cerca", s > 0}, {"conectada", g == conectada_}};
    }
    return l;
}

bool Wifi::tieneClave(const QString& red) const {
    for (const auto& r : redes_) if (r.nombre == red) return r.clave;
    return true;
}

void Wifi::buscar() {
    if (buscando_) return;
    if (propia_) { apagarPropia(); QTimer::singleShot(3000, this, &Wifi::buscar); return; }
    buscando_ = true;
    emit cambio();
    if (!real_) {
        QTimer::singleShot(1200, this, [this] { buscando_ = false; emit cambio(); });
        return;
    }
    nmcli({"-t", "-f", "SSID,SIGNAL,SECURITY", "dev", "wifi", "list", "--rescan", "yes"}, [this](int, const QString& out, const QString&) {
        QHash<QString, Red> mejor;   // la misma red puede verse varias veces (2,4 y 5 GHz): queda la más fuerte
        for (const auto& l : out.split('\n', Qt::SkipEmptyParts)) {
            const QStringList c = campos(l);
            if (c.size() < 3 || c[0].isEmpty()) continue;
            const Red r{c[0], rayitas(c[1].toInt()), !c[2].isEmpty() && c[2] != QLatin1String("--")};
            if (!mejor.contains(r.nombre) || mejor[r.nombre].senal < r.senal) mejor.insert(r.nombre, r);
        }
        redes_ = mejor.values();
        std::sort(redes_.begin(), redes_.end(), [](const Red& a, const Red& b) { return a.senal > b.senal; });
        buscando_ = false;
        emit cambio();
    });
}

void Wifi::conectar(const QString& red, const QString& clave) {
    error_.clear();
    const bool guardada = conexion_.contains(red);
    if (!guardada && tieneClave(red) && clave.size() < 8) {
        error_ = QStringLiteral("La clave tiene que tener al menos 8 letras o números");
        emit cambio();
        return;
    }
    conectando_ = true;
    emit cambio();
    if (!real_) {
        QTimer::singleShot(1500, this, [this, red] {
            conectando_ = false;
            conectada_ = red;
            if (!guardadas_.contains(red)) guardadas_.prepend(red);
            emit cambio();
            emit conectado(red);
        });
        return;
    }
    QStringList args;
    if (guardada && clave.isEmpty()) args = {"connection", "up", conexion_.value(red)};
    else {
        args = {"dev", "wifi", "connect", red};
        if (!clave.isEmpty()) args << "password" << clave;
    }
    nmcli(args, [this, red](int codigo, const QString&, const QString& err) {
        conectando_ = false;
        if (codigo != 0) {
            error_ = err.contains(QLatin1String("Secrets were required"), Qt::CaseInsensitive) || err.contains(QLatin1String("password"), Qt::CaseInsensitive)
                         ? QStringLiteral("La clave no es correcta")
                         : QStringLiteral("No se pudo conectar a «%1»").arg(red);
            emit cambio();
            return;
        }
        refrescar();
        emit cambio();
    });
}

void Wifi::olvidar(const QString& red) {
    if (!real_) {
        guardadas_.removeAll(red);
        if (conectada_ == red) conectada_.clear();
        emit cambio();
        return;
    }
    const QString n = conexion_.value(red);
    if (n.isEmpty()) return;
    nmcli({"connection", "delete", n}, [this](int, const QString&, const QString&) { refrescar(); });
}
