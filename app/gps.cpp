#include "gps.h"

#include <QRandomGenerator>
#include <cmath>

double ahoraMs() {
    static QElapsedTimer reloj = [] { QElapsedTimer r; r.start(); return r; }();
    return static_cast<double>(reloj.nsecsElapsed()) / 1e6 + 1000;   // como performance.now(), nunca 0
}

GpsSimulado::GpsSimulado(QObject* padre) : FuenteGps(padre) {
    t0_ = ahoraMs();
    reloj_.setTimerType(Qt::PreciseTimer);
    reloj_.setInterval(100);   // 10 Hz, como el NEO-M8N
    connect(&reloj_, &QTimer::timeout, this, &GpsSimulado::tic);
    reloj_.start();
}

void GpsSimulado::ponerVuelta(double distanciaM, double tiempoMs) {
    if (distanciaM > 0 && tiempoMs > 0) {
        base_ = distanciaM / (tiempoMs / 1000);
        circuitoM_ = distanciaM;
    }
}

void GpsSimulado::tic() {
    const double t = ahoraMs(), s = (t - t0_) / 1000;
    auto* azar = QRandomGenerator::global();
    double v = base_ * (1 + 0.12 * std::sin(s / 9) + 0.06 * std::sin(s / 2.7)) + (azar->generateDouble() - 0.5) * 0.3;
    v = std::max(0.0, v);
    recorrido_ += v * 0.1;
    // Circuito redondo cerca de Buenos Aires
    const double r = circuitoM_ / (2 * M_PI), a = recorrido_ / r;
    beto::Muestra m;
    m.t = t;
    m.velMs = v;
    m.precisionM = 1.5 + azar->generateDouble() * 1.5;
    m.lat = -34.6 + (r * std::sin(a)) / 111320.0;
    m.lon = -58.4 + (r * std::cos(a)) / (111320.0 * std::cos(34.6 * M_PI / 180));
    emit muestra(m);
}
