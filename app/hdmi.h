// Modo tele: ¿hay una tele enchufada en el HDMI de la Raspberry?
//
// Qt (eglfs_kms, 6.8) arma las pantallas una sola vez, al abrir el programa: si la tele se enchufa
// después, Qt no la ve (no hay «enchufar en caliente»). Acá se mira lo que dice el sistema
// (/sys/class/drm/card*-HDMI-A-*/status) cada 2 s:
//  - si dice «connected» pero Qt tiene una sola pantalla, el informe ofrece reiniciar Betómetro;
//  - si dice «disconnected» con la ventana de la tele abierta, la ventana se cierra (no se dibuja
//    en una salida desenchufada).
// En la PC no hay nada que mirar (vigila = false) y manda solo lo que ve Qt.
#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>

class Hdmi : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool vigila READ vigila CONSTANT)                 // hay un HDMI que mirar (Raspberry)
    Q_PROPERTY(bool conectada READ conectada NOTIFY cambio)      // el sistema dice que hay tele
public:
    // simular: para capturas en la PC (--prueba tele-reiniciar): como si hubiera tele sin pantalla de Qt
    explicit Hdmi(bool simular, QObject* padre = nullptr);
    bool vigila() const { return vigila_; }
    bool conectada() const { return conectada_; }
    // Cierra Betómetro con el código 3: systemd (Restart=always) lo vuelve a abrir a los 2 s y Qt arma
    // las dos pantallas. Se cierra prolijo (el reporte de errores no lo toma como una caída).
    Q_INVOKABLE void reiniciar();

signals:
    void cambio();

private:
    void mirar();
    QStringList archivos_;   // /sys/class/drm/card*-HDMI-A-*/status
    QTimer cada_;
    bool vigila_ = false;
    bool conectada_ = false;
};
