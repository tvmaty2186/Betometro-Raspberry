// Simulador de carrera del Chequeo previo (pedido de Matías, 07/11/2026).
// Corre por detrás una carrera simulada (GPS de mentira, reloj adelantado) con el mismo motor, cronómetro,
// guardado e informe que la carrera de verdad, en una carpeta aparte (nunca toca las carreras de verdad).
// Revisa todo y da: verde «Simulador OK», naranja (falla leve: revisar) o rojo (hay que avisar al desarrollador).
// Lo que falla queda en el reporte de errores, que el celu baja con el QR.
#pragma once

#include <QObject>
#include <QVariantList>

class Audio;
class Reportes;

class Simulacion : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString estado READ estado NOTIFY cambio)       // "", corriendo, ok, leve, grave
    Q_PROPERTY(QVariantList pruebas READ pruebas NOTIFY cambio) // [{nombre, ok, grave, detalle}]
    Q_PROPERTY(int total READ total NOTIFY cambio)
    Q_PROPERTY(int fallas READ fallas NOTIFY cambio)
    Q_PROPERTY(QString cuando READ cuando NOTIFY cambio)
public:
    Simulacion(const QString& carpetaDatos, Audio* audio, Reportes* reportes, QObject* padre = nullptr);
    QString estado() const { return estado_; }
    QVariantList pruebas() const { return pruebas_; }
    int total() const { return static_cast<int>(pruebas_.size()); }
    int fallas() const;
    QString cuando() const { return cuando_; }
    Q_INVOKABLE void correr();
    Q_INVOKABLE bool mandar();             // por internet si hay llave; si no, false (queda el QR)

signals:
    void cambio();

private:
    void hacer();
    void anotar(const QString& nombre, bool ok, bool grave, const QString& detalle = {});
    QString carpeta_;
    Audio* audio_;
    Reportes* reportes_;
    QString estado_, cuando_;
    QVariantList pruebas_;
};
