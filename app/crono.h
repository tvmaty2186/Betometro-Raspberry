// Crono: la carrera en curso. Une el motor (E1), el GPS, la base y la pantalla.
// Reglas de la app: docs/app/lo-aprendido.md, secciones 5.6, 7 y 8.
#pragma once

#include <QObject>
#include <QTimer>
#include <QVariant>
#include <functional>
#include <memory>
#include <optional>

#include "datos.h"
#include "gps.h"
#include "modelo.h"
#include "motor.h"
#include "programador.h"

class Crono : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool listo READ listo NOTIFY carreraChanged)           // hay carrera elegida
    Q_PROPERTY(bool enCurso READ enCurso NOTIFY enCursoChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY carreraChanged)      // nombre, ideal, distancia, total…
    Q_PROPERTY(QVariantMap estado READ estado NOTIFY estadoChanged)   // lo de motor.estado() + extras, 20 veces por segundo
    Q_PROPERTY(QVariantMap ultima READ ultima NOTIFY ultimaChanged)   // última vuelta cerrada
    Q_PROPERTY(QVariantMap gps READ gps NOTIFY gpsChanged)            // lecturas por segundo, precisión

public:
    Crono(Datos* datos, FuenteGps* gps, QObject* padre = nullptr);

    bool listo() const { return datosLargada_.has_value(); }
    bool enCurso() const { return motor_ && motor_->enCurso(); }
    QVariantMap info() const { return info_; }
    QVariantMap estado() const { return estado_; }
    QVariantMap ultima() const { return ultima_; }
    QVariantMap gps() const { return gpsInfo_; }

    // Elegir la carrera (id) o una práctica nueva (id vacío) antes de largar
    Q_INVOKABLE void preparar(const QString& id);
    // Marca: la hora se toma lo primero de todo. `edadMs`: cuánto hace que se apretó (pulsador)
    Q_INVOKABLE QString marcar(const QString& fuente = QStringLiteral("pantalla"), double edadMs = 0);
    Q_INVOKABLE void terminar();             // a mano: guarda la vuelta interrumpida
    Q_INVOKABLE void salir();                // sin carrera en curso: suelta la carrera elegida
    Q_INVOKABLE QString carreraId() const { return carreraId_; }
    void refrescarAhora() { refrescar(); }
    void ponerReloj(std::function<double()> r) { reloj_ms_ = std::move(r); }   // para pruebas

signals:
    void carreraChanged();
    void enCursoChanged();
    void estadoChanged();
    void ultimaChanged();
    void gpsChanged();
    void aviso(const QString& texto);
    void marcado(const QString& resultado);  // largada, marca, ignorada
    void terminada(const QString& motivo, int vueltas, const QString& id);
    void sonido(const QString& nombre);      // «n05», «clic», «vuelta_03_m»… (lo reproduce Audio)
    void frase(const QString& texto);        // «Vuelta 3, medida», «Carrera completa»

private:
    void alEvento(beto::Evento e, const beto::DatosEvento& d);
    void alMuestra(const beto::Muestra& m);
    void refrescar();
    void voz();
    void terminarCon(const QString& motivo, bool aMano);

    Datos* datos_;
    FuenteGps* fuenteGps_;
    QString carreraId_;
    bool practicaSinGuardar_ = false;
    std::optional<beto::DatosLargada> datosLargada_;
    std::unique_ptr<beto::Motor> motor_;
    int largada_ = 1;
    int vueltasGuardadas_ = 0;
    beto::Traza traza_;
    std::optional<beto::Muestra> ultimaMuestra_;
    double velMaxVuelta_ = 0;
    QTimer reloj_;
    QTimer relojVoz_;
    beto::ProgramadorVoz programador_;
    std::function<double()> reloj_ms_ = ahoraMs;
    QVariantMap info_, estado_, ultima_, gpsInfo_;
    std::vector<double> tiemposGps_;
};
