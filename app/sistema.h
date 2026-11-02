// Cosas del equipo: apagar. En la PC solo cierra el programa; en la Raspberry (E3) apaga ordenado.
#pragma once

#include <QCoreApplication>
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
};
