// Fuentes de GPS. Todas entregan la misma muestra que usa el motor ({t, velMs, precisionM, lat, lon}).
// - GpsSimulado: solo para la PC, da vueltas a un circuito a 10 Hz. (El simulador de los mensajes
//   del NEO-M8N por una terminal virtual viene en la parte del GPS.)
// - El GPS real por UART se agrega en E4.
#pragma once

#include <QElapsedTimer>
#include <QTimeZone>
#include <QObject>
#include <QDateTime>
#include <QSocketNotifier>
#include <QTimer>

#include "neo.h"

#include "motor.h"

// Reloj monotónico común a todo el programa, en ms (como performance.now()).
double ahoraMs();
// Hora del reloj monotónico del sistema (ms, como CLOCK_MONOTONIC) pasada al reloj de la carrera
double monotonicoAAhora(double monoMs);

class FuenteGps : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
signals:
    void muestra(const beto::Muestra& m);
    // Datos de cada lectura que no usa el motor: satélites y hora UTC del GPS
    void datos(int satelites, const QDateTime& horaUtc);
    // Qué módulo es (UBX-MON-VER) y cómo quedó configurado
    void modulo(const QString& modelo, const QString& firmware, bool original, int hz);
};

// GPS real (UART de la Raspberry) o el simulador por puerto virtual: lee UBX NAV-PVT o NMEA.
// - Solo pasan las lecturas con posición 3D buena (mejora G6).
// - La hora de cada lectura sale del reloj del GPS (iTOW), atada al reloj del equipo (mejora G2):
//   las lecturas quedan separadas exactamente 100 ms aunque lleguen con demoras distintas por el cable.
// Al arrancar: busca la velocidad del puerto (115200 si ya estaba configurado, 9600 de fábrica), pregunta la
// versión del módulo, lo pasa a 115200, lo configura (GPS + GLONASS a 5 Hz, o GPS solo a 10 Hz) y lo guarda.
// Si después no llegan NAV-PVT (un módulo que no es M8), vuelve a NMEA, que el lector también entiende.
class GpsNeo : public FuenteGps {
    Q_OBJECT
public:
    // `familia`: «auto» (se detecta con MON-VER), «m8», «7» o «6» (Ajustes → Módulo GPS)
    explicit GpsNeo(const QString& dispositivo, bool soloGps = false, const QString& familia = QStringLiteral("auto"),
                    QObject* padre = nullptr);
    ~GpsNeo() override;
    bool abierto() const { return fd_ >= 0; }

private:
    void leer();
    void velocidad(int baudios);
    void escribir(const std::vector<uint8_t>& m);
    void detectar(int baudios);
    void configurar(bool desde9600);
    void revisar();
    void enviarConfiguracion(int desde);
    bool soloGps_ = false;
    beto::FamiliaNeo familiaElegida_ = beto::FamiliaNeo::Desconocida;
    beto::FamiliaNeo familia_ = beto::FamiliaNeo::M8;
    int validosAntes_ = 0;
    bool configurado_ = false;
    QString dispositivo_;
    int fd_ = -1;
    QSocketNotifier* aviso_ = nullptr;
    beto::DecodificadorNeo dec_;
    double desfase_ = 0;          // ms: hora del equipo − iTOW (la menor vista: la de menos demora)
    uint32_t ultimoITOW_ = 0;
    bool hayDesfase_ = false;
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
