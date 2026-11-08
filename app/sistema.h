// Cosas del equipo: apagar y brillo. En la PC solo cierra el programa; en la Raspberry apaga ordenado.
#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <cmath>
#include <QObject>
#include <QProcess>

class Sistema : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool raspberry READ raspberry CONSTANT)
public:
    using QObject::QObject;
    bool raspberry() const { return qEnvironmentVariableIsSet("BETO_RASPBERRY"); }
    Q_INVOKABLE void apagar() {
        if (raspberry()) QProcess::startDetached(QStringLiteral("/sbin/poweroff"), {});
        QCoreApplication::quit();
    }
    // Reiniciar la Raspberry entera (Ajustes → Apagar → Reiniciar). En la PC solo cierra el programa.
    Q_INVOKABLE void reiniciar() {
        if (raspberry()) QProcess::startDetached(QStringLiteral("/sbin/reboot"), {});
        QCoreApplication::quit();
    }
    // Brillo de la pantalla, 10 a 100 % (100 = el máximo que da la pantalla). Se escribe en la luz de fondo
    // del sistema (/sys/class/backlight/…: la Touch Display 2 la tiene). Antes el brillo solo se guardaba.
    Q_INVOKABLE void brillo(int porcentaje) {
        if (!raspberry()) return;
        const int p = std::max(10, std::min(100, porcentaje));
        const QDir d(QStringLiteral("/sys/class/backlight"));
        for (const QString& n : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System)) {
            QFile fmax(d.filePath(n + QStringLiteral("/max_brightness")));
            if (!fmax.open(QIODevice::ReadOnly)) continue;
            const int maximo = QString::fromLatin1(fmax.readAll()).trimmed().toInt();
            if (maximo <= 0) continue;
            QFile apagado(d.filePath(n + QStringLiteral("/bl_power")));   // 0 = prendida
            if (apagado.open(QIODevice::WriteOnly)) apagado.write("0");
            QFile f(d.filePath(n + QStringLiteral("/brightness")));
            if (f.open(QIODevice::WriteOnly))
                f.write(QByteArray::number(std::max(1, static_cast<int>(std::lround(p / 100.0 * maximo)))));
            else qWarning("Brillo: no pude escribir en %s", qPrintable(f.fileName()));
        }
    }
};
