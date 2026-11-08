// «Betómetro en la red»: una página para el celular o la compu, en la misma wifi.
// Lista de carreras, el informe de cada una (el mismo HTML de la app) para ver o descargar, y
// todos juntos en un .zip, y los videos de la cámara (de a pedazos, para poder adelantar y atrasar).
// Solo lectura, salvo /control: el celu como control remoto del modo tele (docs/video.md). Se apaga durante la carrera (no gasta nada del equipo).
#pragma once

#include <QByteArray>
#include <QObject>
#include <QTcpServer>

class ControlRemoto;
class Datos;
class Drive;
class PruebaCompleta;
class Contexto;
class Reportes;
class QTcpSocket;

class Servidor : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool activo READ activo NOTIFY cambio)
    Q_PROPERTY(int puerto READ puerto CONSTANT)
    // Código de 4 cifras que muestra la Pi (Ajustes → Google Drive) y hay que escribir en /drive para subir el cliente
    // de Google: así nadie de la wifi lo puede cambiar sin ver la pantalla (auditoría 12). Cambia después de cada uso.
    Q_PROPERTY(QString codigoDrive READ codigoDrive NOTIFY codigoDriveChanged)
public:
    Servidor(Datos* datos, quint16 puerto, QObject* padre = nullptr);
    bool activo() const { return tcp_.isListening(); }
    int puerto() const { return puerto_; }
    QString codigoDrive() const { return codigoDrive_; }
    void prender(bool si);
    void ponerReportes(Reportes* r) { reportes_ = r; }
    void ponerDrive(Drive* d) { drive_ = d; }
    void ponerControl(ControlRemoto* c) { control_ = c; }   // el celu como control de la tele (/control)
    void ponerPrueba(PruebaCompleta* p) { prueba_ = p; }    // la Prueba completa: verla y arrancarla desde el celu (/prueba)
    void ponerContexto(Contexto* c) { contexto_ = c; }      // Personalizar: el tema y la foto del inicio
    // Personalizar desde el celu (08/11/2026): la dirección del QR lleva una llave que cambia en cada arranque;
    // sin ella la página no deja guardar (que nadie de la wifi te cambie el equipo sin ver la pantalla)
    Q_PROPERTY(QString llavePersonalizar READ llavePersonalizar CONSTANT)
    QString llavePersonalizar() const { return llaveP_; }
    QByteArray recibirPersonalizar(const QByteArray& cuerpo);   // POST /personalizar (JSON): lo aplica y contesta JSON

    // POST /drive: el JSON del cliente de Google (desde la página /drive). Devuelve la página de respuesta.
    QByteArray recibirDrive(const QByteArray& cuerpo);

    // Para probar sin red: arma la respuesta HTTP completa de un pedido «GET ruta»
    QByteArray responder(const QByteArray& ruta, bool post = false) const;   // post: llegó por POST (/control/accion y /prueba/empezar solo así)
    // .zip sin comprimir (los HTML ya son chicos); público para probarlo
    static QByteArray zip(const QList<QPair<QString, QByteArray>>& archivos);
    // «Range: bytes=a-b» de un archivo de `tam` bytes → [desde, hasta] (inclusive). false si no se puede cumplir.
    static bool rango(const QByteArray& pedido, qint64 tam, qint64& desde, qint64& hasta);

signals:
    void cambio();
    void codigoDriveChanged();

private:
    QByteArray pagina() const;
    QByteArray paginaDrive(const QString& aviso = {}, bool bien = false) const;
    QByteArray paginaControl() const;
    // GET /video/<archivo>: manda el archivo de a pedazos (nunca entero en memoria)
    void enviarVideo(QTcpSocket* s, const QString& nombre, const QByteArray& rangoPedido, bool descargar);
    Datos* datos_;
    Reportes* reportes_ = nullptr;
    Drive* drive_ = nullptr;
    ControlRemoto* control_ = nullptr;
    PruebaCompleta* prueba_ = nullptr;
    Contexto* contexto_ = nullptr;
    QString llaveP_;
    quint16 puerto_;
    QString codigoDrive_;
    void nuevoCodigoDrive();
    QTcpServer tcp_;
};
