#include "actualizador.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QVersionNumber>

namespace {
// Donde se publican las versiones (repo público, solo los paquetes). Se puede cambiar con BETO_ACTUALIZACIONES.
const char* kUrl = "https://api.github.com/repos/tvmaty2186/Betometro-actualizaciones/releases/latest";
// Paquete: betometro-pi-X.Y.Z.tar.gz con la carpeta del instalador (instalar.sh y lo demás)
const QString kPatron = QStringLiteral("betometro-pi-*.tar.gz");
const QString kPendrive = QStringLiteral("/run/betometro/pendrive");

QString versionDeNombre(const QString& n) {   // betometro-pi-1.2.0.tar.gz → 1.2.0
    static const QRegularExpression re(QStringLiteral("(\\d+\\.\\d+\\.\\d+)"));
    return re.match(n).captured(1);
}
}  // namespace

Actualizador::Actualizador(const QString& carpetaDatos, QObject* padre)
    : QObject(padre), carpeta_(carpetaDatos + QStringLiteral("/actualizacion")),
      url_(qEnvironmentVariable("BETO_ACTUALIZACIONES", QString::fromLatin1(kUrl))),
      raspberry_(qEnvironmentVariableIsSet("BETO_RASPBERRY")) {
    if (!raspberry_) {   // en la PC: una versión de muestra para ver la pantalla
        versionNueva_ = QStringLiteral("1.1.0");
        notas_ = QStringLiteral("Arreglos y mejoras de la pantalla de carrera");
        origen_ = QStringLiteral("internet");
    }
    connect(&rauc_, &QProcess::readyReadStandardOutput, this, [this] { rauc_.readAllStandardOutput(); });
    connect(&rauc_, &QProcess::finished, this, [this](int codigo, QProcess::ExitStatus s) {
        if (!archivoPendrive_.isEmpty()) { QProcess::execute(QStringLiteral("timeout"), {QStringLiteral("10"), QStringLiteral("umount"), kPendrive}); archivoPendrive_.clear(); }
        if (s == QProcess::NormalExit && codigo == 0) {
            // El instalador corre aparte (systemd-run) y al terminar reinicia el equipo solo
            poner(QStringLiteral("lista"), 100, QStringLiteral("Instalando… no la apagues: se reinicia sola"));
        } else {
            const QString err = QString::fromUtf8(rauc_.readAllStandardError()).trimmed().section('\n', -1);
            poner(QStringLiteral("error"), 0, QStringLiteral("No se pudo instalar") + (err.isEmpty() ? QString() : ": " + err));
        }
    });
    sim_.setInterval(120);
    connect(&sim_, &QTimer::timeout, this, &Actualizador::simular);
    QDir().mkpath(carpeta_);
    // Lo que dejó el instalador (o la vuelta atrás automática): se muestra una vez y se borra
    {
        QFile r(carpeta_ + QStringLiteral("/resultado"));
        if (r.open(QIODevice::ReadOnly)) {
            resultado_ = QString::fromUtf8(r.readAll()).trimmed();
            r.close();
            r.remove();
            if (!resultado_.startsWith(QLatin1String("Se instaló"))) qWarning("Actualización: %s", qPrintable(resultado_));
        }
    }
    if (raspberry_) {
        // Si la versión anda bien un minuto, ya no se vuelve sola a la anterior (betometro-volver.service)
        QTimer::singleShot(60000, this, [this] { QFile::remove(carpeta_ + QStringLiteral("/a-probar")); });
        // Al rato de prender y después cada media hora (cuando aparezca la wifi o se ponga la hora)
        // Al prender el wifi tarda en conectar: se busca a los 20 s, y si no hubo respuesta se reintenta
        // cada minuto hasta que la haya; después, cada 15 minutos. También apenas conecta el wifi
        // (lo pide main.cpp). Antes, si fallaba la primera, no volvía a buscar hasta 30 minutos después
        // y el «· Actualizar» amarillo del inicio no aparecía.
        QTimer::singleShot(20000, this, &Actualizador::buscar);
        auto* cada = new QTimer(this);
        connect(cada, &QTimer::timeout, this, [this] { if (versionNueva_.isEmpty()) buscar(); });
        cada->start(15 * 60 * 1000);
        reintento_.setSingleShot(true);
        reintento_.setInterval(60 * 1000);
        connect(&reintento_, &QTimer::timeout, this, &Actualizador::buscar);
    }
}

QString Actualizador::version() const { return QStringLiteral(BETO_VERSION); }

bool Actualizador::masNueva(const QString& a, const QString& b) {
    return QVersionNumber::compare(QVersionNumber::fromString(a), QVersionNumber::fromString(b)) > 0;
}

void Actualizador::poner(const QString& estado, int progreso, const QString& mensaje) {
    estado_ = estado;
    progreso_ = progreso;
    mensaje_ = mensaje;
    emit cambio();
}

void Actualizador::buscar() {
    if (!raspberry_ || estado_ == QLatin1String("descargando") || estado_ == QLatin1String("instalando")) return;
    QNetworkRequest q{QUrl(url_)};
    q.setRawHeader("Accept", "application/vnd.github+json");
    q.setRawHeader("User-Agent", "Betometro/" BETO_VERSION);   // GitHub rechaza pedidos sin User-Agent
    q.setTransferTimeout(15000);
    busqueda_ = QStringLiteral("buscando");
    emit cambio();
    QNetworkReply* r = red_.get(q);
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) {
            // La API de GitHub deja buscar 60 veces por hora por casa (todas las compus juntas) y después
            // contesta «prohibido». Por eso, si falla, se pregunta a la página común de GitHub, que no tiene
            // ese límite (04/11/2026: la Pi decía «sin internet» con el wifi andando).
            // Sin internet (no encuentra el nombre: wifi sin salida, o sin wifi): ni la página, ni aviso cada vez.
            // Se vuelve a buscar al conectarse a una wifi (main.cpp) o en 15 minutos (10/11/2026: llenaba el reporte)
            if (r->error() == QNetworkReply::HostNotFoundError) { sinInternet(r->errorString()); return; }
            qWarning("Actualizar: la API no contestó (%s), pruebo la página", qPrintable(r->errorString()));
            buscarPagina();
            return;
        }
        sinRedAvisado_ = false;
        const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        QString v = o.value(QStringLiteral("tag_name")).toString().remove(QLatin1Char('v'));
        // Solo números de versión (va a un nombre de archivo): «1.0.36» (auditoría 12)
        if (!QRegularExpression(QStringLiteral("^\\d+\\.\\d+\\.\\d+$")).match(v).hasMatch()) v.clear();
        ultima_ = v;
        busqueda_ = QStringLiteral("aldia");
        emit cambio();
        if (!masNueva(v, version())) return;
        for (const auto& a : o.value(QStringLiteral("assets")).toArray()) {
            const QJsonObject x = a.toObject();
            const QString nombre = x.value(QStringLiteral("name")).toString();
            if (!nombre.startsWith(QLatin1String("betometro-pi-")) || !nombre.endsWith(QLatin1String(".tar.gz"))) continue;
            versionNueva_ = v;
            notas_ = o.value(QStringLiteral("body")).toString().section('\n', 0, 2).trimmed();
            origen_ = QStringLiteral("internet");
            descarga_ = x.value(QStringLiteral("browser_download_url")).toString();
            tamano_ = x.value(QStringLiteral("size")).toInteger();
            emit cambio();
            return;
        }
    });
}

// Plan B: https://github.com/<repo>/releases/latest redirige a …/releases/tag/vX.Y.Z. El paquete se baja de
// …/releases/download/vX.Y.Z/betometro-pi-X.Y.Z.tar.gz (siempre se publica con ese nombre).
void Actualizador::buscarPagina() {
    const QString web = QString(url_).replace(QLatin1String("https://api.github.com/repos/"), QLatin1String("https://github.com/"))
                            .remove(QLatin1String("/latest")).remove(QRegularExpression(QStringLiteral("/releases$")));
    QNetworkRequest q{QUrl(web + QStringLiteral("/releases/latest"))};
    q.setRawHeader("User-Agent", "Betometro/" BETO_VERSION);
    q.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    q.setTransferTimeout(15000);
    QNetworkReply* r = red_.get(q);
    connect(r, &QNetworkReply::finished, this, [this, r, web] {
        r->deleteLater();
        const QString destino = r->header(QNetworkRequest::LocationHeader).toUrl().toString();
        const QString v = destino.section(QLatin1String("/tag/v"), 1, 1).section('/', 0, 0);
        if (v.isEmpty() || !QRegularExpression(QStringLiteral("^\\d+\\.\\d+\\.\\d+$")).match(v).hasMatch()) {   // ahora sí: sin internet
            if (r->error() == QNetworkReply::HostNotFoundError) { sinInternet(r->errorString()); return; }
            qWarning("Actualizar: no pude buscar (%s)", qPrintable(r->errorString()));
            busqueda_ = QStringLiteral("sinred");
            emit cambio();
            reintento_.start();   // otro problema (GitHub lento, se cortó): de nuevo en un minuto
            return;
        }
        sinRedAvisado_ = false;
        ultima_ = v;
        busqueda_ = QStringLiteral("aldia");
        if (masNueva(v, version())) {
            versionNueva_ = v;
            notas_.clear();
            origen_ = QStringLiteral("internet");
            descarga_ = web + QStringLiteral("/releases/download/v%1/betometro-pi-%1.tar.gz").arg(v);
            tamano_ = 0;
        }
        emit cambio();
    });
}

bool Actualizador::buscarPendrive() {
    if (!raspberry_) return false;
    // Primera partición de cada pendrive (o el disco entero si no tiene particiones). mount/umount con tiempo
    // límite: un pendrive malo trababa la pantalla más de 30 s y el vigilante reiniciaba Betómetro (02-1.4)
    QDir().mkpath(kPendrive);
    QProcess::execute(QStringLiteral("timeout"), {QStringLiteral("10"), QStringLiteral("umount"), kPendrive});
    const QStringList discos = QDir(QStringLiteral("/dev")).entryList({QStringLiteral("sd?1"), QStringLiteral("sd?")}, QDir::System);
    for (const QString& d : discos) {
        if (QProcess::execute(QStringLiteral("timeout"), {QStringLiteral("10"), QStringLiteral("mount"), QStringLiteral("-o"), QStringLiteral("ro"), "/dev/" + d, kPendrive}) != 0) continue;
        QString mejor, mejorV;
        for (const QString& f : QDir(kPendrive).entryList({kPatron}, QDir::Files)) {
            const QString v = versionDeNombre(f);
            if (mejor.isEmpty() || masNueva(v, mejorV)) { mejor = f; mejorV = v; }
        }
        if (!mejor.isEmpty()) {
            archivoPendrive_ = kPendrive + '/' + mejor;
            versionNueva_ = mejorV.isEmpty() ? QStringLiteral("?") : mejorV;
            notas_ = QStringLiteral("Desde el pendrive: ") + mejor;
            origen_ = QStringLiteral("pendrive");
            emit cambio();
            return true;
        }
        QProcess::execute(QStringLiteral("timeout"), {QStringLiteral("10"), QStringLiteral("umount"), kPendrive});
    }
    poner(QStringLiteral("error"), 0, QStringLiteral("No hay ninguna actualización (betometro-pi-….tar.gz) en el pendrive"));
    return false;
}

void Actualizador::actualizar() {
    if (estado_ == QLatin1String("descargando") || estado_ == QLatin1String("instalando")) return;
    if (!raspberry_) { progreso_ = 0; poner(QStringLiteral("descargando"), 0); sim_.start(); return; }
    if (origen_ == QLatin1String("pendrive") && !archivoPendrive_.isEmpty()) { instalar(archivoPendrive_); return; }
    if (descarga_.isEmpty()) { poner(QStringLiteral("error"), 0, QStringLiteral("No hay versión nueva para bajar")); return; }

    const QString archivo = carpeta_ + QStringLiteral("/betometro-pi-") + versionNueva_ + QStringLiteral(".tar.gz");
    delete destino_;
    destino_ = new QFile(archivo + QStringLiteral(".parte"), this);
    if (!destino_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        poner(QStringLiteral("error"), 0, QStringLiteral("No hay lugar para bajar la actualización"));
        return;
    }
    QNetworkRequest q{QUrl(descarga_)};
    q.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    q.setRawHeader("Accept", "application/octet-stream");
    q.setRawHeader("User-Agent", "Betometro/" BETO_VERSION);
    q.setTransferTimeout(30000);   // si el wifi se cae a mitad, a los 30 s sin datos se corta (no queda «descargando») (02-3)
    respuesta_ = red_.get(q);
    poner(QStringLiteral("descargando"), 0);
    connect(respuesta_, &QNetworkReply::readyRead, this, [this] { destino_->write(respuesta_->readAll()); });
    connect(respuesta_, &QNetworkReply::downloadProgress, this, [this](qint64 r, qint64 t) {
        if (t <= 0) t = tamano_;
        if (t > 0) poner(QStringLiteral("descargando"), static_cast<int>(r * 100 / t));
    });
    connect(respuesta_, &QNetworkReply::finished, this, [this, archivo] {
        respuesta_->deleteLater();
        destino_->write(respuesta_->readAll());
        destino_->close();
        if (respuesta_->error() != QNetworkReply::NoError) {
            destino_->remove();
            poner(QStringLiteral("error"), 0, QStringLiteral("Se cortó la descarga. Probá de nuevo."));
            return;
        }
        QFile::remove(archivo);
        destino_->rename(archivo);
        const QString urlSha = descarga_ + QStringLiteral(".sha256");
        descarga_ = archivo;   // desde ahora, la ruta del archivo bajado
        comprobarEInstalar(archivo, urlSha);
    });
}

void Actualizador::comprobarEInstalar(const QString& archivo, const QString& urlSha) {
    QNetworkRequest q{QUrl(urlSha)};
    q.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    q.setRawHeader("User-Agent", "Betometro/" BETO_VERSION);
    q.setTransferTimeout(30000);
    QNetworkReply* r = red_.get(q);
    connect(r, &QNetworkReply::finished, this, [this, r, archivo] {
        r->deleteLater();
        const QByteArray esperado = r->readAll().trimmed().left(64).toLower();
        QFile f(archivo);
        QCryptographicHash h(QCryptographicHash::Sha256);
        const bool leido = f.open(QIODevice::ReadOnly) && h.addData(&f);
        f.close();
        if (r->error() != QNetworkReply::NoError || esperado.size() != 64) {
            QFile::remove(archivo);
            poner(QStringLiteral("error"), 0, QStringLiteral("No se pudo comprobar la actualización. Probá de nuevo."));
            return;
        }
        if (!leido || h.result().toHex() != esperado) {
            QFile::remove(archivo);
            qWarning("Actualizar: el paquete bajado no coincide con el publicado (sha256)");
            poner(QStringLiteral("error"), 0, QStringLiteral("La actualización llegó dañada. Probá de nuevo."));
            return;
        }
        instalar(archivo);
    });
}

void Actualizador::instalar(const QString& archivo) {
    poner(QStringLiteral("instalando"), 0);
    // Se desarma el paquete en una carpeta propia y se corre su instalar.sh como servicio aparte:
    // el instalador detiene Betómetro, guarda la versión anterior, pone la nueva y reinicia.
    const QString dir = carpeta_ + QStringLiteral("/nueva");
    QDir(dir).removeRecursively();
    QDir().mkpath(dir);
    // Los nombres van como argumentos ($1, $2), no dentro del texto: un nombre con «'» no rompe la orden (10-3)
    const QString orden = QStringLiteral("tar -xzf \"$1\" -C \"$2\" && f=$(ls -d \"$2\"/*/instalar.sh | head -1) && "
                                         "systemd-run --unit=betometro-actualizar --collect bash \"$f\"");
    rauc_.start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), orden, QStringLiteral("sh"), archivo, dir});
}

void Actualizador::reiniciar() {
    if (raspberry_) QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("reboot")});
    else poner(estado_, progreso_, QStringLiteral("En la Raspberry reinicia en la versión nueva"));
}

void Actualizador::simular() {
    if (estado_ == QLatin1String("descargando")) {
        if (progreso_ + 2 >= 100) poner(QStringLiteral("instalando"), 0); else poner(estado_, progreso_ + 2);
    } else if (estado_ == QLatin1String("instalando")) {
        if (progreso_ + 4 >= 100) { sim_.stop(); poner(QStringLiteral("lista"), 100, QStringLiteral("Lista: se usa al reiniciar el equipo")); }
        else poner(estado_, progreso_ + 4);
    } else {
        sim_.stop();
    }
}

// Sin internet: se anota una sola vez hasta que vuelva a andar, y no se reintenta cada minuto
void Actualizador::sinInternet(const QString& error) {
    if (!sinRedAvisado_) qWarning("Actualizar: sin internet (%s); se busca al conectarse a una wifi", qPrintable(error));
    sinRedAvisado_ = true;
    busqueda_ = QStringLiteral("sinred");
    emit cambio();
}
