// El celu como control remoto del modo tele (docs/video.md).
// Es el puente entre «Betómetro en la red» (/control) y las pantallas:
//  - las pantallas le cuentan cómo está la tele (ponerEstado, ponerCarreras);
//  - el servidor le pasa lo que se tocó en el celu (hacer); él lo revisa y avisa (accion).
// Nunca en carrera: el servidor está apagado y, además, las pantallas no hacen caso.
#pragma once

#include <QByteArray>
#include <QObject>
#include <QVariant>

class ControlRemoto : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    // {activa, carrera, carreraId, vueltas: [{nombre, tiempo, desdeMs, video}], vuelta, pos, total, andando, marcas}
    Q_INVOKABLE void ponerEstado(const QVariantMap& e) { estado_ = e; }
    // Las carreras con video: [{id, nombre, fecha, vueltas, videos}]
    Q_INVOKABLE void ponerCarreras(const QVariantList& l) { carreras_ = l; }

    bool activa() const { return estado_.value(QStringLiteral("activa")).toBool(); }
    // GET /control/estado
    QByteArray estadoJson() const;
    // GET o POST /control/accion?tipo=..&valor=.. Revisa todo y avisa. Devuelve el problema (vacío: bien).
    // tipo: play, pausa, alternar, saltar (±ms), ir (ms), vuelta (índice), carrera (id), salir
    QString hacer(const QString& tipo, const QString& valor);

signals:
    void accion(const QString& tipo, const QVariant& valor);

private:
    QVariantMap estado_;
    QVariantList carreras_;
};
