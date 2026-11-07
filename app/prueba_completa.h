// Prueba completa (pedido de Matías, 07/11/2026, para la 1.0.30): la Pi se revisa sola de punta a punta, mide
// el rendimiento y le manda el resultado al desarrollador (que no se puede conectar a la Pi).
// Corre en pasos, sin trabar la pantalla (cada paso avisa cuando termina, con relojes y procesos aparte):
// sistema, simulador de carrera, voz (con «¿Se escuchó?»), pulsador («Apretá el pulsador», 20 s), GPS, cámara
// (30 s de grabación y el pase a MP4), rendimiento mientras graba, reproducción, tarjeta, wifi e internet, Drive
// y que todas las pantallas se puedan armar (en la 1.0.29 un módulo que faltaba dejó todo en negro).
// Cada paso queda verde (ok), naranja (leve) o rojo (grave), con el detalle en números. El resultado se guarda
// en <datos>/prueba-completa.json y .txt, se suma al reporte de errores (y se manda si hay llave) y se ve en
// «Betómetro en la red» (/prueba, desde donde también se puede arrancar). No corre durante una carrera.
#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <functional>
#include <vector>

class Audio;
class Camara;
class Contexto;
class Crono;
class Datos;
class Drive;
class PulsadorGpio;
class QQmlEngine;
class Reportes;
class Simulacion;
class Wifi;

class PruebaCompleta : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString estado READ estado NOTIFY cambio)        // "", corriendo, ok, leve, grave
    Q_PROPERTY(QVariantList pasos READ pasos NOTIFY cambio)     // [{nombre, estado, detalle}]: estado espera, corriendo, ok, leve, grave, saltado
    Q_PROPERTY(int paso READ paso NOTIFY cambio)                // el que está corriendo (1 a total), 0 si no corre
    Q_PROPERTY(int total READ total NOTIFY cambio)
    Q_PROPERTY(QString haciendo READ haciendo NOTIFY cambio)    // qué está haciendo ahora («Grabando 30 s… 12»)
    Q_PROPERTY(QString pregunta READ pregunta NOTIFY cambio)    // "", voz («¿Se escuchó?») o pulsador («Apretá el pulsador»)
    Q_PROPERTY(int segundos READ segundos NOTIFY cambio)        // los que quedan para contestar
    Q_PROPERTY(QString cuando READ cuando NOTIFY cambio)
    Q_PROPERTY(QString resumen READ resumen NOTIFY cambio)      // «2 de 12 pasos con avisos»
    Q_PROPERTY(int fallas READ fallas NOTIFY cambio)
public:
    struct Piezas {
        QString carpetaDatos;
        Contexto* contexto = nullptr;
        Datos* datos = nullptr;
        Crono* crono = nullptr;
        Audio* audio = nullptr;
        Camara* camara = nullptr;
        Reportes* reportes = nullptr;
        Wifi* wifi = nullptr;
        Drive* drive = nullptr;
        Simulacion* simulacion = nullptr;
        PulsadorGpio* pulsador = nullptr;   // null en la PC
    };
    explicit PruebaCompleta(const Piezas& p, QObject* padre = nullptr);
    void ponerMotor(QQmlEngine* motor) { motor_ = motor; }   // para revisar las pantallas

    QString estado() const { return estado_; }
    QVariantList pasos() const;
    int paso() const { return estado_ == QLatin1String("corriendo") ? actual_ + 1 : 0; }
    int total() const { return static_cast<int>(pasos_.size()); }
    QString haciendo() const { return haciendo_; }
    QString pregunta() const { return pregunta_; }
    int segundos() const { return segundos_; }
    QString cuando() const { return cuando_; }
    QString resumen() const;
    int fallas() const;

    Q_INVOKABLE bool correr();                          // false si no se puede (carrera en curso o ya corriendo)
    Q_INVOKABLE void responder(bool si);                // «¿Se escuchó?» Sí / No; en el pulsador, «Saltar»
    Q_INVOKABLE void pulsadorRecibido(const QString& por);   // llegó una marca (el pulsador por cable, o el teclado)
    Q_INVOKABLE bool mandar();                          // por internet si hay llave; si no, false (queda el QR)
    QByteArray json() const;                            // el resultado (para /prueba/estado y el archivo)
    QByteArray texto() const;                           // legible (prueba-completa.txt)
    Q_INVOKABLE QString porQueNo() const;               // por qué no arrancó («Hay una carrera en curso»)

signals:
    void cambio();
    void empezo();                                      // para que la pantalla se abra sola (desde el celu)
    void termino();

private:
    struct Paso {
        QString nombre;
        std::function<void()> hacer;
        QString estado = QStringLiteral("espera");
        QString detalle;
        qint64 ms = 0;
    };
    void siguiente();
    void terminarPaso(const QString& estado, const QString& detalle);
    void preguntar(const QString& que, int segundos);
    void correrComando(const QString& programa, const QStringList& args, int topeMs,
                      const std::function<void(int, const QString&)>& listo);
    void medirCarga();          // una lectura de procesador, temperatura y freno (mientras graba la cámara)
    void guardar();
    bool rapida() const;        // BETO_PRUEBA_RAPIDA=1: esperas cortas (capturas)
    bool raspberry() const;

    // Los pasos
    void pasoSistema();
    void pasoSimulador();
    void pasoVoz();
    void pasoPulsador();
    void pasoGps();
    void pasoCamara();
    void pasoRendimiento();
    void pasoReproduccion();
    void pasoTarjeta();
    void pasoWifi();
    void pasoDrive();
    void pasoPantallas();

    Piezas p_;
    QQmlEngine* motor_ = nullptr;
    std::vector<Paso> pasos_;
    int actual_ = -1;
    int vuelta_ = 0;            // sube con cada paso: los avisos atrasados del paso anterior se ignoran
    QString estado_, haciendo_, pregunta_, cuando_;
    int segundos_ = 0;
    qint64 duracionS_ = 0;      // cuánto duró la última prueba
    QTimer cuenta_, tope_, carga_;
    QElapsedTimer relojPaso_, relojTotal_;
    QVariantMap sistema_;       // lo medido en el paso Sistema (versión, arranque, temperatura…)
    // Rendimiento bajo carga: lecturas mientras graba
    struct Carga { double cpuBeto = 0, cpuAyudante = 0; int tempMax = -1; unsigned freno = 0; int lecturas = 0; qint64 ticksBeto = -1, ticksAyu = -1; qint64 ultimaMs = 0; double maxBeto = 0, maxAyu = 0; } carga_m_;
    QString videoPrueba_;       // archivo (.mkv o .mp4) que grabó la cámara, para reproducirlo y después borrarlo
    QVariantMap camaraMedida_;
    QMetaObject::Connection espera_;   // la conexión del paso que está esperando una señal
    QObject* vivo_ = nullptr;   // objetos temporales del paso (se borran al terminar)
};
