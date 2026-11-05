#include "hdmi.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

Hdmi::Hdmi(bool simular, QObject* padre) : QObject(padre) {
    if (simular) { vigila_ = true; conectada_ = true; return; }
    if (!qEnvironmentVariableIsSet("BETO_RASPBERRY")) return;
    const QDir drm(QStringLiteral("/sys/class/drm"));
    for (const QString& n : drm.entryList({QStringLiteral("card*-HDMI-A-*")}, QDir::Dirs | QDir::System | QDir::NoDotAndDotDot))
        archivos_ << drm.filePath(n + QStringLiteral("/status"));
    vigila_ = !archivos_.isEmpty();
    if (!vigila_) return;
    mirar();
    connect(&cada_, &QTimer::timeout, this, &Hdmi::mirar);
    cada_.start(2000);
}

void Hdmi::mirar() {
    bool hay = false;
    for (const QString& a : std::as_const(archivos_)) {
        QFile f(a);
        if (f.open(QIODevice::ReadOnly) && f.readAll().trimmed() == "connected") hay = true;
    }
    if (hay != conectada_) { conectada_ = hay; emit cambio(); }
}

void Hdmi::reiniciar() {
    QCoreApplication::exit(3);
}
