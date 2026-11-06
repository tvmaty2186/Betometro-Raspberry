#include "control_remoto.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

QByteArray ControlRemoto::estadoJson() const {
    QVariantMap e = activa() ? estado_ : QVariantMap{{QStringLiteral("activa"), false}};
    e[QStringLiteral("carreras")] = carreras_;
    return QJsonDocument(QJsonObject::fromVariantMap(e)).toJson(QJsonDocument::Compact);
}

QString ControlRemoto::hacer(const QString& tipo, const QString& valor) {
    if (!activa()) return QStringLiteral("La tele no está en uso");
    if (tipo == QLatin1String("play") || tipo == QLatin1String("pausa") || tipo == QLatin1String("alternar") || tipo == QLatin1String("salir")) {
        emit accion(tipo, QVariant());
        return {};
    }
    bool ok = false;
    if (tipo == QLatin1String("saltar")) {   // de a ±10 s (como mucho 10 minutos)
        const qlonglong ms = valor.toLongLong(&ok);
        if (!ok || ms == 0 || std::llabs(ms) > 600000) return QStringLiteral("Salto fuera de rango");
        emit accion(tipo, ms);
        return {};
    }
    if (tipo == QLatin1String("ir")) {   // a un momento del video
        const double ms = valor.toDouble(&ok);
        const double total = estado_.value(QStringLiteral("total")).toDouble();
        if (!ok || !std::isfinite(ms) || ms < 0 || ms > total) return QStringLiteral("Momento fuera del video");
        emit accion(tipo, ms);
        return {};
    }
    if (tipo == QLatin1String("vuelta")) {   // al empezar esa vuelta (tiene que tener video)
        const int i = valor.toInt(&ok);
        const QVariantList vs = estado_.value(QStringLiteral("vueltas")).toList();
        if (!ok || i < 0 || i >= vs.size()) return QStringLiteral("No está esa vuelta");
        if (!vs[i].toMap().value(QStringLiteral("video")).toBool()) return QStringLiteral("Esa vuelta no tiene video");
        emit accion(tipo, i);
        return {};
    }
    if (tipo == QLatin1String("carrera")) {   // solo una de la lista (las que tienen video)
        for (const QVariant& c : carreras_)
            if (!valor.isEmpty() && c.toMap().value(QStringLiteral("id")).toString() == valor) {
                emit accion(tipo, valor);
                return {};
            }
        return QStringLiteral("No está esa carrera");
    }
    return QStringLiteral("No sé hacer eso");
}
