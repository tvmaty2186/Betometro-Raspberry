#include "gps.h"

#include <QRandomGenerator>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
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

GpsNeo::GpsNeo(const QString& dispositivo, QObject* padre) : FuenteGps(padre), dispositivo_(dispositivo) {
    fd_ = ::open(dispositivo.toLocal8Bit().constData(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0) {
        qWarning("No pude abrir el GPS en %s", qPrintable(dispositivo));
        return;
    }
    termios t{};
    if (tcgetattr(fd_, &t) == 0) {
        cfmakeraw(&t);
        cfsetispeed(&t, B115200);
        cfsetospeed(&t, B115200);
        tcsetattr(fd_, TCSANOW, &t);
    }
    aviso_ = new QSocketNotifier(fd_, QSocketNotifier::Read, this);
    connect(aviso_, &QSocketNotifier::activated, this, &GpsNeo::leer);
}

GpsNeo::~GpsNeo() {
    if (fd_ >= 0) ::close(fd_);
}

void GpsNeo::leer() {
    const double llegada = ahoraMs();
    uint8_t b[1024];
    const ssize_t n = ::read(fd_, b, sizeof b);
    if (n <= 0) return;
    for (const auto& l : dec_.agregar(b, static_cast<size_t>(n))) {
        QDateTime utc;
        if (l.horaValida) utc = QDateTime(QDate(l.anio, l.mes, l.dia), QTime(l.hora, l.minuto, l.segundo), QTimeZone::UTC);
        emit datos(l.satelites, utc);
        if (!l.fix3d) continue;   // G6: solo posición 3D buena
        double t = llegada;
        if (l.iTOW > 0) {         // G2: hora del GPS atada al reloj del equipo
            const bool salto = hayDesfase_ && (l.iTOW < ultimoITOW_ || l.iTOW - ultimoITOW_ > 5000);
            const double d = llegada - l.iTOW;
            if (!hayDesfase_ || salto || d < desfase_) desfase_ = d;
            if (!hayDesfase_ || salto) hayDesfase_ = true;
            desfase_ += 0.001;    // se deja subir de a poco por si el reloj del equipo se corre
            ultimoITOW_ = l.iTOW;
            t = l.iTOW + desfase_;
        }
        beto::Muestra m;
        m.t = t;
        m.velMs = l.velMs;
        m.precisionM = l.precisionM;
        m.lat = l.lat;
        m.lon = l.lon;
        emit muestra(m);
    }
}
