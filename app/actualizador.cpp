#include "actualizador.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QVersionNumber>

namespace {
// Donde se publican las versiones (paquetes .raucb firmados). Se puede cambiar con BETO_ACTUALIZACIONES.
const char* kUrl = "https://api.github.com/repos/tvmaty2186/Betometro-Actualizaciones/releases/latest";
const QString kPendrive = QStringLiteral("/run/betometro/pendrive");

QString versionDeNombre(const QString& n) {   // betometro-1.2.0.raucb → 1.2.0
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
    connect(&rauc_, &QProcess::readyReadStandardOutput, this, [this] {
        // rauc install: «  40% Copying image to rootfs.1»
        static const QRegularExpression re(QStringLiteral("(\\d+)%"));
        while (rauc_.canReadLine()) {
            const auto m = re.match(QString::fromUtf8(rauc_.readLine()));
            if (m.hasMatch()) poner(QStringLiteral("instalando"), m.captured(1).toInt());
        }
    });
    connect(&rauc_, &QProcess::finished, this, [this](int codigo, QProcess::ExitStatus s) {
        if (!archivoPendrive_.isEmpty()) { QProcess::execute(QStringLiteral("umount"), {kPendrive}); archivoPendrive_.clear(); }
        if (s == QProcess::NormalExit && codigo == 0) {
            QFile::remove(descarga_);
            poner(QStringLiteral("lista"), 100, QStringLiteral("Lista: se usa al reiniciar el equipo"));
        } else {
            const QString err = QString::fromUtf8(rauc_.readAllStandardError()).trimmed().section('\n', -1);
            poner(QStringLiteral("error"), 0, QStringLiteral("No se pudo instalar") + (err.isEmpty() ? QString() : ": " + err));
        }
    });
    sim_.setInterval(120);
    connect(&sim_, &QTimer::timeout, this, &Actualizador::simular);
    QDir().mkpath(carpeta_);
    if (raspberry_) {
        // Al rato de prender y después cada media hora (cuando aparezca la wifi o se ponga la hora)
        QTimer::singleShot(20000, this, &Actualizador::buscar);
        auto* cada = new QTimer(this);
        connect(cada, &QTimer::timeout, this, [this] { if (versionNueva_.isEmpty()) buscar(); });
        cada->start(30 * 60 * 1000);
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
    q.setTransferTimeout(15000);
    QNetworkReply* r = red_.get(q);
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) return;   // sin internet: no pasa nada
        const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        const QString v = o.value(QStringLiteral("tag_name")).toString().remove(QLatin1Char('v'));
        if (!masNueva(v, version())) return;
        for (const auto& a : o.value(QStringLiteral("assets")).toArray()) {
            const QJsonObject x = a.toObject();
            if (!x.value(QStringLiteral("name")).toString().endsWith(QLatin1String(".raucb"))) continue;
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

bool Actualizador::buscarPendrive() {
    if (!raspberry_) return false;
    // Primera partición de cada pendrive (o el disco entero si no tiene particiones)
    QDir().mkpath(kPendrive);
    QProcess::execute(QStringLiteral("umount"), {kPendrive});
    const QStringList discos = QDir(QStringLiteral("/dev")).entryList({QStringLiteral("sd?1"), QStringLiteral("sd?")}, QDir::System);
    for (const QString& d : discos) {
        if (QProcess::execute(QStringLiteral("mount"), {QStringLiteral("-o"), QStringLiteral("ro"), "/dev/" + d, kPendrive}) != 0) continue;
        QString mejor, mejorV;
        for (const QString& f : QDir(kPendrive).entryList({QStringLiteral("*.raucb")}, QDir::Files)) {
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
        QProcess::execute(QStringLiteral("umount"), {kPendrive});
    }
    poner(QStringLiteral("error"), 0, QStringLiteral("No hay ninguna actualización (.raucb) en el pendrive"));
    return false;
}

void Actualizador::actualizar() {
    if (estado_ == QLatin1String("descargando") || estado_ == QLatin1String("instalando")) return;
    if (!raspberry_) { progreso_ = 0; poner(QStringLiteral("descargando"), 0); sim_.start(); return; }
    if (origen_ == QLatin1String("pendrive") && !archivoPendrive_.isEmpty()) { instalar(archivoPendrive_); return; }
    if (descarga_.isEmpty()) { poner(QStringLiteral("error"), 0, QStringLiteral("No hay versión nueva para bajar")); return; }

    const QString archivo = carpeta_ + QStringLiteral("/betometro-") + versionNueva_ + QStringLiteral(".raucb");
    delete destino_;
    destino_ = new QFile(archivo + QStringLiteral(".parte"), this);
    if (!destino_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        poner(QStringLiteral("error"), 0, QStringLiteral("No hay lugar para bajar la actualización"));
        return;
    }
    QNetworkRequest q{QUrl(descarga_)};
    q.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    q.setRawHeader("Accept", "application/octet-stream");
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
        descarga_ = archivo;   // desde ahora, la ruta del archivo bajado
        instalar(archivo);
    });
}

void Actualizador::instalar(const QString& archivo) {
    poner(QStringLiteral("instalando"), 0);
    // RAUC verifica la firma, escribe la copia que no se usa y le pasa el kernel (gancho del paquete)
    rauc_.start(QStringLiteral("rauc"), {QStringLiteral("install"), archivo});
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
