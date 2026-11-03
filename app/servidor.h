// «Betómetro en la red»: una página para el celular o la compu, en la misma wifi.
// Lista de carreras, el informe de cada una (el mismo HTML de la app) para ver o descargar, y
// todos juntos en un .zip. Solo lectura. Se apaga durante la carrera (no gasta nada del equipo).
#pragma once

#include <QByteArray>
#include <QObject>
#include <QTcpServer>

class Datos;

class Servidor : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool activo READ activo NOTIFY cambio)
    Q_PROPERTY(int puerto READ puerto CONSTANT)
public:
    Servidor(Datos* datos, quint16 puerto, QObject* padre = nullptr);
    bool activo() const { return tcp_.isListening(); }
    int puerto() const { return puerto_; }
    void prender(bool si);

    // Para probar sin red: arma la respuesta HTTP completa de un pedido «GET ruta»
    QByteArray responder(const QByteArray& ruta) const;
    // .zip sin comprimir (los HTML ya son chicos); público para probarlo
    static QByteArray zip(const QList<QPair<QString, QByteArray>>& archivos);

signals:
    void cambio();

private:
    QByteArray pagina() const;
    Datos* datos_;
    quint16 puerto_;
    QTcpServer tcp_;
};
