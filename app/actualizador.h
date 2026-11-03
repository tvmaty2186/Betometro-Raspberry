// Actualizador: busca una versión nueva (por internet en el repo público Betometro-actualizaciones,
// o en un pendrive), la baja y corre su instalador (raspios/instalar.sh), que guarda la versión
// anterior, pone la nueva y reinicia. Nunca durante una carrera (lo frena la pantalla).
// En la PC simula la descarga y la instalación (para ver la pantalla).
#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>

class QFile;
class QNetworkReply;

class Actualizador : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString versionNueva READ versionNueva NOTIFY cambio)
    Q_PROPERTY(QString notas READ notas NOTIFY cambio)          // «Qué trae»
    Q_PROPERTY(QString origen READ origen NOTIFY cambio)        // internet, pendrive
    Q_PROPERTY(QString estado READ estado NOTIFY cambio)        // nada, buscando, descargando, instalando, lista, error
    Q_PROPERTY(int progreso READ progreso NOTIFY cambio)        // 0..100
    Q_PROPERTY(QString mensaje READ mensaje NOTIFY cambio)

public:
    explicit Actualizador(const QString& carpetaDatos, QObject* padre = nullptr);

    QString version() const;
    QString versionNueva() const { return versionNueva_; }
    QString notas() const { return notas_; }
    QString origen() const { return origen_; }
    QString estado() const { return estado_; }
    int progreso() const { return progreso_; }
    QString mensaje() const { return mensaje_; }

    Q_INVOKABLE void buscar();             // por internet (si hay wifi)
    Q_INVOKABLE bool buscarPendrive();     // un betometro-pi-X.Y.Z.tar.gz en un pendrive
    Q_INVOKABLE void actualizar();         // baja (si hace falta) e instala
    Q_INVOKABLE void reiniciar();

    static bool masNueva(const QString& a, const QString& b);   // a > b (1.10.0 > 1.9.3)

signals:
    void cambio();

private:
    void poner(const QString& estado, int progreso, const QString& mensaje = {});
    void instalar(const QString& archivo);
    void simular();

    QString carpeta_;                      // /datos/actualizacion
    QString url_;                          // última versión publicada (JSON de GitHub)
    QString versionNueva_, notas_, origen_, estado_ = QStringLiteral("nada"), mensaje_;
    QString descarga_, archivoPendrive_;
    int progreso_ = 0;
    qint64 tamano_ = 0;
    bool raspberry_ = false;
    QNetworkAccessManager red_;
    QPointer<QNetworkReply> respuesta_;
    QFile* destino_ = nullptr;
    QProcess rauc_;                        // desarma el paquete y lanza el instalador
    QTimer sim_;
};
