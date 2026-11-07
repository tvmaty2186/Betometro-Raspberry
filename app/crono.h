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

class Camara;

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

    // Elegir la carrera (id), una práctica nueva (id vacío) o un recorrido libre («recorrido») antes de largar
    Q_INVOKABLE void preparar(const QString& id);
    // Marca: la hora se toma lo primero de todo. `edadMs`: cuánto hace que se apretó (pulsador)
    Q_INVOKABLE QString marcar(const QString& fuente = QStringLiteral("pantalla"), double edadMs = 0);
    Q_INVOKABLE void terminar();             // a mano: guarda la vuelta interrumpida
    Q_INVOKABLE void salir();                // sin carrera en curso: suelta la carrera elegida
    Q_INVOKABLE QString carreraId() const { return carreraId_; }
    void refrescarAhora() { refrescar(); }
    void ponerReloj(std::function<double()> r) { reloj_ms_ = std::move(r); }   // para pruebas
    void ponerCamara(Camara* c);             // video de la carrera (puede no haber)
    Q_INVOKABLE void grabarAMano();          // botón REC (modo «a mano»): empieza o para

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
    bool camaraActiva() const;
    void empezarVideo();
    void asociarVideo();
    void alEvento(beto::Evento e, const beto::DatosEvento& d);
    void alMuestra(const beto::Muestra& m);
    void refrescar();
    void voz();
    void terminarCon(const QString& motivo, bool aMano);

    Datos* datos_;
    FuenteGps* fuenteGps_;
    QString carreraId_;
    bool practicaSinGuardar_ = false;
    // Recorrido libre: sin vueltas; MARCAR deja marcas; GPS a 1 lectura por segundo en la traza (entran 5 h)
    bool recorrido_ = false;
    beto::Json marcas_ = beto::Json::lista();
    double paradoMs_ = 0;          // tiempo con menos de 3 km/h
    double ultMuestraT_ = 0;
    double ultTrazaT_ = -1e12;
    double ultMarcaT_ = -1e12;
    double ultParcialT_ = -1e12;
    bool sinGpsAvisado_ = false;           // pitido al cortarse el GPS en carrera y al volver (07-2.3)
    double sinGpsCambioT_ = 0;   // recorrido libre: última vez que se guardó lo andado (por si se corta la luz)
    void guardarParcial(double t);
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
    Camara* camara_ = nullptr;
    QTimer reintentoVideo_;                   // en carrera, si la cámara se cortó (desenchufada, colgada): probar cada 10 s (06-1.4)
    double largadaT_ = 0;          // hora de la largada (reloj de la carrera)
    bool videoPendiente_ = false;  // falta anotar en qué momento del video fue la largada
    QVariantMap info_, estado_, ultima_, gpsInfo_;
    std::vector<double> tiemposGps_;
};
