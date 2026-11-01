// Fuentes de GPS. Todas entregan la misma muestra que usa el motor ({t, velMs, precisionM, lat, lon}).
// - GpsSimulado: solo para la PC, da vueltas a un circuito a 10 Hz. (El simulador de los mensajes
//   del NEO-M8N por una terminal virtual viene en la parte del GPS.)
// - El GPS real por UART se agrega en E4.
#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include "motor.h"

// Reloj monotónico común a todo el programa, en ms (como performance.now()).
double ahoraMs();

class FuenteGps : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
signals:
    void muestra(const beto::Muestra& m);
};

class GpsSimulado : public FuenteGps {
    Q_OBJECT
public:
    explicit GpsSimulado(QObject* padre = nullptr);
    // Ajusta la velocidad a la de una vuelta ideal (distancia / tiempo), con variaciones
    void ponerVuelta(double distanciaM, double tiempoMs);

private:
    void tic();
    QTimer reloj_;
    double base_ = 13.3;      // m/s
    double t0_ = 0;
    double recorrido_ = 0;    // m, para la posición
    double circuitoM_ = 3200;
};
