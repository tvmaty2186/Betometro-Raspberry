// Vigilante de memoria y trabas (Matías, 08/11/2026: la Pi se trababa y el reporte no decía nada).
// - Memoria: la lee cada 10 s (un archivo del sistema, menos de 1 ms). Si pasa del 70 % manda un reporte con
//   el estado del equipo, los procesos que más usan, cuánto crece Betómetro, las últimas pantallas y la
//   memoria de la última media hora (un número cada 30 s, solo en memoria: no gasta la tarjeta).
// - Pantalla congelada: un tic cada medio segundo; si llega más de 1,5 s tarde, el programa estuvo trabado.
// - Tirones: en carrera, 5 o más trabas cortas (0,4 a 1,5 s) en un minuto.
// Cada motivo avisa como mucho una vez cada 10 minutos. La lista de procesos se arma solo al avisar.
// - Despejar (Chequeo previo): reinicia el programa (no la Raspberry) y vuelve al Chequeo previo.
#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <functional>

class Reportes;

class Vigilante : public QObject {
    Q_OBJECT
    Q_PROPERTY(int memoria READ memoria NOTIFY cambio)               // % usado (−1 si no se sabe)

public:
    struct Mem { qint64 total = 0, disponible = 0, swapTotal = 0, swapLibre = 0; };   // en kB
    struct Proceso { int pid = 0; QString nombre; qint64 rssKb = 0; qint64 ticks = 0; };
    // Públicos para probarlos
    static Mem leerMem(const QByteArray& meminfo);
    static int porcentaje(const Mem& m);
    static bool leerStat(const QByteArray& stat, QString* nombre, qint64* ticks);

    Vigilante(const QString& carpeta, Reportes* reportes, QObject* padre = nullptr);
    int memoria() const { return memoria_; }
    int despejeAntes() const { return despejeAntes_; }

    Q_INVOKABLE void pantalla(const QString& nombre);   // Main.qml avisa cada cambio de pantalla
    Q_INVOKABLE void despejar();                         // reinicia el programa y vuelve al Chequeo previo
    Q_INVOKABLE int tomarDespeje() { const int a = despejeAntes_; despejeAntes_ = -1; return a; }   // para el aviso, una vez

    std::function<QStringList()> andando;   // qué está haciendo: carrera, cámara, Drive…
    std::function<bool()> enCarrera;

signals:
    void cambio();

private:
    void revisar();
    void latido();
    void alarma(const QString& motivo, const QString& titulo);
    void escribirReporte(const QString& titulo, const QHash<int, Proceso>& antes);
    static QHash<int, Proceso> procesos();
    static qint64 rssPropioKb();
    Reportes* reportes_;
    QString carpeta_;
    bool activo_ = false;              // en la Raspberry (o con BETO_VIGILANTE_UMBRAL para probar)
    int umbral_ = 70;
    int memoria_ = -1, despejeAntes_ = -1;
    bool memoriaAlta_ = false;
    qint64 rssInicialKb_ = 0;
    QVector<int> historia_;            // % cada 30 s, la última media hora
    QVector<qint64> historiaRss_;      // memoria de Betómetro (kB), igual
    int vueltas_ = 0;
    QStringList pantallas_;            // las últimas 10, con la hora
    QString pantallaActual_;
    QHash<QString, qint64> ultimaAlarma_;   // motivo → cuándo (ms desde que arrancó)
    QVector<qint64> trabasCortas_;
    QElapsedTimer reloj_, tic_;
    QTimer cada_, latido_;
};
