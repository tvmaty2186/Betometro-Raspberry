// Pulsador por cable en el GPIO17 (pin 11) y GND (pin 9). Solo en la Raspberry.
// Pide la línea al kernel (interfaz de caracteres de GPIO, v2) como entrada, con la resistencia de
// pull-up interna prendida y aviso de los dos flancos con la hora del kernel (reloj monotónico).
// Cada flanco pasa por FiltroPulsador (antirrebote y antirruido); una marca confirmada sale por
// `marca(edadMs)`: cuánto hace que se apoyó el dedo, para que la marca valga ese instante.
#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QTimer>
#include <memory>

#include "pulsador.h"

constexpr int kAvisoLargoMs = 2000;   // «Seguí apretando para salir»
constexpr int kLargoMs = 5000;        // sale de la carrera (Matías, 09/11/2026)

class PulsadorGpio : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool activo READ activo CONSTANT)           // se pudo pedir el pin
    Q_PROPERTY(bool apretado READ apretado NOTIFY cambio)   // nivel ahora (para el chequeo)
    Q_PROPERTY(QString error READ error CONSTANT)
public:
    explicit PulsadorGpio(int linea = 17, QObject* padre = nullptr);
    ~PulsadorGpio() override;
    bool activo() const { return fd_ >= 0; }
    bool apretado() const { return apretado_; }
    QString error() const { return error_; }

signals:
    void marca(double edadMs);
    void cambio();
    // Mantenido apretado (Salir con el pulsador): a los 2 s el aviso, a los 5 s salir. Soltar antes los cancela.
    void avisoLargo();
    void largo();

private:
    void leer();
    void revisar();
    static double ahoraMs();
    int fd_ = -1;
    bool apretado_ = false;
    QString error_;
    beto::FiltroPulsador filtro_;
    std::unique_ptr<QSocketNotifier> aviso_;
    QTimer reloj_;
    QTimer tAviso_, tLargo_;
    void sostener();   // empieza o corta la cuenta de «mantenido» según el nivel
};
