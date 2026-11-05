// «Betómetro en la red»: una página para el celular o la compu, en la misma wifi.
// Lista de carreras, el informe de cada una (el mismo HTML de la app) para ver o descargar, y
// todos juntos en un .zip, y los videos de la cámara (de a pedazos, para poder adelantar y atrasar).
// Solo lectura. Se apaga durante la carrera (no gasta nada del equipo).
#pragma once

#include <QByteArray>
#include <QObject>
#include <QTcpServer>

class Datos;
class Drive;
class Reportes;
class QTcpSocket;

class Servidor : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool activo READ activo NOTIFY cambio)
    Q_PROPERTY(int puerto READ puerto CONSTANT)
public:
    Servidor(Datos* datos, quint16 puerto, QObject* padre = nullptr);
    bool activo() const { return tcp_.isListening(); }
    int puerto() const { return puerto_; }
    void prender(bool si);
    void ponerReportes(Reportes* r) { reportes_ = r; }
    void ponerDrive(Drive* d) { drive_ = d; }

    // POST /drive: el JSON del cliente de Google (desde la página /drive). Devuelve la página de respuesta.
    QByteArray recibirDrive(const QByteArray& cuerpo);

    // Para probar sin red: arma la respuesta HTTP completa de un pedido «GET ruta»
    QByteArray responder(const QByteArray& ruta) const;
    // .zip sin comprimir (los HTML ya son chicos); público para probarlo
    static QByteArray zip(const QList<QPair<QString, QByteArray>>& archivos);
    // «Range: bytes=a-b» de un archivo de `tam` bytes → [desde, hasta] (inclusive). false si no se puede cumplir.
    static bool rango(const QByteArray& pedido, qint64 tam, qint64& desde, qint64& hasta);

signals:
    void cambio();

private:
    QByteArray pagina() const;
    QByteArray paginaDrive(const QString& aviso = {}, bool bien = false) const;
    // GET /video/<archivo>: manda el archivo de a pedazos (nunca entero en memoria)
    void enviarVideo(QTcpSocket* s, const QString& nombre, const QByteArray& rangoPedido, bool descargar);
    Datos* datos_;
    Reportes* reportes_ = nullptr;
    Drive* drive_ = nullptr;
    quint16 puerto_;
    QTcpServer tcp_;
};
