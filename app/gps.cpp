#include "gps.h"

#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#include <cmath>

static const QElapsedTimer& relojCarrera() {
    static QElapsedTimer reloj = [] { QElapsedTimer r; r.start(); return r; }();
    return reloj;
}

double ahoraMs() {
    return static_cast<double>(relojCarrera().nsecsElapsed()) / 1e6 + 1000;   // como performance.now(), nunca 0
}

double monotonicoAAhora(double monoMs) {
    // QElapsedTimer usa el reloj monotónico del sistema: su referencia es la hora en que arrancó
    return monoMs - static_cast<double>(relojCarrera().msecsSinceReference()) + 1000;
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

GpsNeo::GpsNeo(const QString& dispositivo, bool soloGps, const QString& familia, QObject* padre)
    : FuenteGps(padre), soloGps_(soloGps), familiaElegida_(beto::familiaDeTexto(familia.toStdString())), dispositivo_(dispositivo),
      automatico_(dispositivo == QLatin1String("auto")) {
    if (!automatico_) { abrir(dispositivo); return; }
    connect(&vigia_, &QTimer::timeout, this, &GpsNeo::vigilar);
    vigia_.start(2000);
    vigilar();
}

QStringList GpsNeo::candidatos(const QString& carpetaDev) {
    QStringList r;
    const QDir d(carpetaDev);
    for (const QString& patron : {QStringLiteral("ttyACM*"), QStringLiteral("ttyUSB*")})
        for (const QString& n : d.entryList({patron}, QDir::System | QDir::Files | QDir::Readable | QDir::Writable, QDir::Name))
            r << d.filePath(n);
    r << d.filePath(QStringLiteral("ttyAMA0"));   // los pines (UART), siempre al final
    return r;
}

void GpsNeo::abrir(const QString& ruta) {
    cerrar();
    ++vuelta_;
    actual_ = ruta;
    dec_ = beto::DecodificadorNeo();
    validosVistos_ = 0;
    ultimoDato_ = ahoraMs();
    configurado_ = false;
    hayDesfase_ = false;
    fd_ = ::open(ruta.toLocal8Bit().constData(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0) {
        if (!automatico_) qWarning("No pude abrir el GPS en %s", qPrintable(ruta));
        return;
    }
    aviso_ = new QSocketNotifier(fd_, QSocketNotifier::Read, this);
    connect(aviso_, &QSocketNotifier::activated, this, &GpsNeo::leer);
    detectar(115200);
}

void GpsNeo::cerrar() {
    if (aviso_) { aviso_->setEnabled(false); aviso_->deleteLater(); aviso_ = nullptr; }
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
}

void GpsNeo::vigilar() {
    QStringList lista = candidatos();
    const QString pines = lista.last();
    // Los USB que ya se probaron y no hablan (otro aparato enchufado) se saltean mientras sigan ahí
    for (int i = mudos_.size() - 1; i >= 0; --i) if (!lista.contains(mudos_[i])) mudos_.removeAt(i);
    for (const QString& m : std::as_const(mudos_)) lista.removeAll(m);
    const bool esUsb = !actual_.isEmpty() && actual_ != pines;
    const bool hayUsb = lista.size() > 1;
    const bool callado = fd_ < 0 || ahoraMs() - ultimoDato_ > 6000;
    if (!callado && (esUsb || !hayUsb)) return;   // anda (o recién empieza) y no hay nada mejor que probar
    if (callado && esUsb && lista.contains(actual_)) { mudos_ << actual_; lista.removeAll(actual_); }
    if (callado && !esUsb && !actual_.isEmpty() && !hayUsb && !mudos_.isEmpty()) { mudos_.clear(); lista = candidatos(); }
    const QString siguiente = lista.first();   // el primer USB que queda, o los pines
    if (siguiente == actual_ && fd_ >= 0) { ultimoDato_ = ahoraMs(); return; }   // es el único: seguir esperando
    if (!actual_.isEmpty()) qWarning("GPS: pruebo %s", qPrintable(siguiente));
    abrir(siguiente);
}

void GpsNeo::velocidad(int baudios) {
    termios t{};
    if (tcgetattr(fd_, &t) != 0) return;
    cfmakeraw(&t);
    const speed_t v = baudios == 9600 ? B9600 : B115200;
    cfsetispeed(&t, v);
    cfsetospeed(&t, v);
    tcsetattr(fd_, TCSANOW, &t);
}

void GpsNeo::escribir(const std::vector<uint8_t>& m) {
    if (fd_ >= 0 && ::write(fd_, m.data(), m.size()) < 0) qWarning("No pude escribirle al GPS");
}

void GpsNeo::detectar(int baudios) {
    velocidad(baudios);
    validosAntes_ = dec_.validos();
    QTimer::singleShot(1500, this, [this, baudios, v = vuelta_] {
        if (v != vuelta_ || fd_ < 0) return;
        if (dec_.validos() - validosAntes_ >= 2) { configurar(baudios == 9600); return; }
        detectar(baudios == 115200 ? 9600 : 115200);   // nada sano: probar la otra velocidad
    });
}

void GpsNeo::configurar(bool desde9600) {
    // Primero preguntar qué módulo es; con la respuesta se elige la configuración
    escribir(beto::pedirVersion());
    QTimer::singleShot(500, this, [this, desde9600, v = vuelta_] {
        if (v != vuelta_) return;
        familia_ = familiaElegida_;
        if (familia_ == beto::FamiliaNeo::Desconocida)
            familia_ = dec_.version() ? beto::familiaDe(*dec_.version()) : beto::FamiliaNeo::Desconocida;
        if (familia_ == beto::FamiliaNeo::Desconocida) familia_ = beto::FamiliaNeo::M8;   // si no contesta: como M8 (y si no anda, NMEA)
        int t = 0;
        if (desde9600) {
            escribir(beto::cambioA115200());
            t += 150;
            QTimer::singleShot(t, this, [this, v = vuelta_] { if (v == vuelta_) velocidad(115200); });   // el módulo ya cambió
            t += 200;
        }
        enviarConfiguracion(t);
    });
}

void GpsNeo::enviarConfiguracion(int t) {
    const auto cfg = beto::configuracionNeo(soloGps_, familia_);
    for (size_t i = 0; i < cfg.size(); ++i) {
        QTimer::singleShot(t, this, [this, m = cfg[i], v = vuelta_] { if (v == vuelta_) escribir(m); });
        t += i == 0 ? 600 : 80;   // después de CFG-GNSS, medio segundo (recomendación de u-blox)
    }
    QTimer::singleShot(t + 100, this, [this, v = vuelta_] { if (v == vuelta_) escribir(beto::pedirVersion()); });
    QTimer::singleShot(t + 3000, this, [this, v = vuelta_] { if (v == vuelta_) revisar(); });
}

void GpsNeo::revisar() {
    if (dec_.navPvt() == 0 && familia_ != beto::FamiliaNeo::U6) {   // no aceptó la configuración: seguir con NMEA
        qWarning("El GPS no manda NAV-PVT: se usa NMEA");
        for (const auto& m : beto::volverANmea()) escribir(m);
    }
    configurado_ = true;
    const auto& v = dec_.version();
    QString modelo = v && !v->modelo().empty() ? QString::fromStdString(v->modelo()) : QString::fromLatin1(beto::nombreFamilia(familia_));
    if (automatico_)
        modelo += actual_.contains(QLatin1String("ttyAMA")) ? QStringLiteral(" · pines") : QStringLiteral(" · USB");
    // «original» solo se puede afirmar en el M8N (los NEO-6M y 7M no tienen flash ni dicen su modelo)
    const bool original = familia_ != beto::FamiliaNeo::M8 || (v && v->pareceOriginal());
    emit modulo(modelo, v ? QString::fromStdString(v->firmware()) : QString(), original, beto::hzDe(soloGps_, familia_));
}

GpsNeo::~GpsNeo() { cerrar(); }

void GpsNeo::leer() {
    const double llegada = ahoraMs();
    uint8_t b[1024];
    const ssize_t n = ::read(fd_, b, sizeof b);
    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) {   // desenchufado (USB)
        // Desenchufado: se cierra (con puerto fijo también: si no, el aviso quedaba prendido y el procesador al 100 %)
        // y se vuelve a probar a los 2 s (02-2)
        cerrar(); ultimoDato_ = 0;
        if (!automatico_) QTimer::singleShot(2000, this, [this] { if (fd_ < 0) abrir(dispositivo_); });
        return;
    }
    if (n < 0) return;
    for (const auto& l : dec_.agregar(b, static_cast<size_t>(n))) {
        QDateTime utc;
        if (l.horaValida) utc = QDateTime(QDate(l.anio, l.mes, l.dia), QTime(l.hora, l.minuto, l.segundo), QTimeZone::UTC);
        ultimoDato_ = llegada;
        ++validosVistos_;
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
