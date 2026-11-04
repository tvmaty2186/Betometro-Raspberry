// Salud del equipo (pedido de Matías, 04/11/2026). Solo hace algo en la Raspberry.
// - Tensión baja y calor: los lee del sistema cada 2 s. Si aparecen, se ve un aviso en pantalla y
//   queda anotado en el reporte de errores (con la vuelta, si hay carrera). Antes pasaba y no se veía.
// - Vigilante: le avisa a systemd que el programa sigue vivo. Si se congela (la pantalla queda
//   quieta), systemd lo reinicia solo (WatchdogSec en betometro.service).
// - Procesador al máximo durante la carrera (sin subir y bajar la velocidad, que da tirones);
//   fuera de la carrera vuelve a como estaba. Con calor o tensión baja se lo deja bajar para aguantar.
#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>
#include <QTimer>

class Salud : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool tensionBaja READ tensionBaja NOTIFY cambio)      // ahora mismo
    Q_PROPERTY(bool calor READ calor NOTIFY cambio)                  // ahora mismo: se frena por temperatura
    Q_PROPERTY(bool huboTensionBaja READ huboTensionBaja NOTIFY cambio)   // desde que se prendió
    Q_PROPERTY(bool huboCalor READ huboCalor NOTIFY cambio)
    Q_PROPERTY(int temperatura READ temperatura NOTIFY cambio)      // °C del procesador (−1 si no se sabe)
    Q_PROPERTY(bool sabe READ sabe NOTIFY cambio)                    // si el sistema da estos datos

public:
    explicit Salud(QObject* padre = nullptr);
    ~Salud() override;
    bool tensionBaja() const { return estado_ & 0x1; }
    bool calor() const { return (estado_ & 0x8) || temperatura_ >= 80; }   // bit 3: límite por temperatura
    bool huboTensionBaja() const { return estado_ & 0x10000; }
    bool huboCalor() const { return estado_ & 0x80000; }
    int temperatura() const { return temperatura_; }
    bool sabe() const { return sabe_; }

    void modoCarrera(bool siCarrera);       // procesador al máximo en carrera
    void listo();                           // el programa terminó de arrancar (para systemd)

    // Lee «get_throttled» en hexadecimal («0x50005»). Público para probarlo.
    static unsigned leerEstado(const QByteArray& texto);

signals:
    void cambio();
    void aviso(const QString& texto);       // tensión baja o calor nuevos: para el reporte

private:
    void revisar();
    void aplicar();                         // procesador al máximo o como estaba
    static void avisarSystemd(const char* mensaje);
    bool raspberry_ = false;
    unsigned estado_ = 0;
    int temperatura_ = -1;
    bool sabe_ = false;
    bool carrera_ = false;
    bool alMaximo_ = false, huboProblema_ = false;
    QElapsedTimer sinProblemaDesde_;
    QStringList gobernadores_;              // cómo estaba cada núcleo antes de la carrera
    QTimer cada_, vivo_;
};
