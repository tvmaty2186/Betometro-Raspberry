// Google Drive (pedido de Matías, 05/11/2026; docs/google-drive.md).
// Sube solo los informes y los videos de cada carrera a «Betómetro/AAAA-MM-DD Nombre» en su Drive.
// - Conectar (una vez): flujo de Google para equipos sin teclado. La Raspberry muestra un código;
//   él lo pone en google.com/device desde el celu. Permiso «drive.file»: Betómetro solo ve lo que subió él.
// - El cliente de Google (client_id y client_secret) lo sube Matías desde la compu en http://<IP>/drive:
//   nunca está en el programa. Queda en <datos>/drive-cliente.json (solo lo lee el dueño).
// - El permiso y lo ya subido quedan en <datos>/drive.json (así nada se sube dos veces).
// - Sube solo con wifi, sin carrera y sin la cámara grabando. Para apenas empieza una carrera.
//   Los videos, solo con las wifi elegidas (la de casa): una hora de video son ~2 GB.
// - De a un archivo y de a pedazos de 4 MB (subida «resumable»: si se corta, sigue donde quedó),
//   con curl en un proceso con prioridad baja y una pausa entre pedazos.
// En la PC (sin BETO_RASPBERRY) no sale a internet: «--prueba drive-…» inventa los estados para las capturas.
#pragma once

#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <functional>

#include "drive_logica.h"

class Datos;
class QNetworkReply;
class QFile;

class Drive : public QObject {
    Q_OBJECT
    // falta (no está el cliente de Google), desconectado, pidiendo (pidiendo el código), codigo, conectado
    Q_PROPERTY(QString estado READ estado NOTIFY cambio)
    Q_PROPERTY(QString codigo READ codigo NOTIFY cambio)      // el que se escribe en google.com/device
    Q_PROPERTY(QString enlace READ enlace NOTIFY cambio)      // https://www.google.com/device
    Q_PROPERTY(QString cuenta READ cuenta NOTIFY cambio)      // x@gmail.com
    Q_PROPERTY(QString texto READ texto NOTIFY cambio)        // «Subiendo Santa Clara · video 45 %», «Todo subido»…
    Q_PROPERTY(bool subiendo READ subiendo NOTIFY cambio)
    Q_PROPERTY(double progreso READ progreso NOTIFY cambio)   // 0 a 1, del archivo que se sube
    Q_PROPERTY(QString error READ error NOTIFY cambio)        // el último problema, en palabras
    Q_PROPERTY(bool listo READ listo NOTIFY cambio)           // todo subido

public:
    Drive(Datos* datos, const QString& carpetaDatos, const QString& prueba, QObject* padre = nullptr);
    ~Drive() override;

    QString estado() const { return estado_; }
    QString codigo() const { return codigo_; }
    QString enlace() const { return enlace_; }
    QString cuenta() const { return cuenta_; }
    QString texto() const { return texto_; }
    bool subiendo() const { return subiendo_; }
    double progreso() const { return progreso_; }
    QString error() const { return error_; }
    bool listo() const { return listo_; }

    Q_INVOKABLE void conectar();      // pide el código
    Q_INVOKABLE void cancelar();      // deja de esperar el código
    Q_INVOKABLE void desconectar();   // le saca el permiso en Google y borra el de acá
    Q_INVOKABLE void revisar();       // ¿hay algo para subir? (también solo, cada 10 min)

    // Desde main: red conectada (vacía si no hay), si es la wifi propia, y si hay carrera o la cámara graba
    void ponerCondiciones(const QString& red, bool propia, bool ocupado);
    // Cómo borrar un video ya subido (Camara::borrarVideo: nunca el que se está grabando)
    void ponerBorrador(std::function<bool(const QString&)> f) { borrador_ = std::move(f); }
    // Desde la página http://<IP>/drive: revisa y guarda el JSON del cliente. "" si quedó bien; si no, el problema.
    QString guardarCliente(const QByteArray& json);
    bool tieneCliente() const { return !clienteId_.isEmpty(); }

signals:
    void cambio();

private:
    using Listo = std::function<void(int http, const QByteArray& cuerpo, QNetworkReply* r)>;
    // deCola: si se para la subida (empieza una carrera), la respuesta ya no hace nada
    void pedir(const QString& verbo, const QNetworkRequest& req, const QByteArray& cuerpo, Listo listo, bool deCola = true);
    QNetworkRequest req(const QString& url, bool conToken = true) const;
    void conToken(std::function<void()> seguir, std::function<void(const QString&)> mal);
    void esperarCodigo();
    void buscarCuenta();

    // La cola de subida
    bool puede() const;
    drive::Opciones opciones() const;
    QList<drive::CarreraLocal> carrerasLocales();
    void borrarSubidos();
    void seguirCola();
    void conCarpeta(const drive::Tarea& t, std::function<void(const QString&)> seguir);
    void buscarOCrearCarpeta(const QString& nombre, const QString& padre, std::function<void(const QString&)> seguir);
    void subir(const drive::Tarea& t, const QString& carpeta);
    void empezarSesion();
    void preguntarSesion();
    void mandarPedazo(qint64 desde);
    void alimentar();   // le pasa a curl el pedazo, de a 256 KB
    void terminoArchivo(const QByteArray& cuerpo);
    void olvidarCarpeta();   // la carpeta de la carrera de la tarea (la borraron en Drive)
    void fallar(const QString& texto);
    void parar();
    void actualizarTexto();

    void cargar();
    void guardar() const;
    void avisar() { emit cambio(); }
    QString url(const QString& camino) const;   // https://www.googleapis.com/… (o el servidor de prueba)

    Datos* datos_;
    QString carpeta_, archivoEstado_, archivoCliente_;
    bool real_ = false, pruebaFija_ = false;
    QString base_;   // BETO_DRIVE_SERVIDOR (pruebas con un Google de mentira); vacío: Google de verdad
    QNetworkAccessManager red_;

    QString estado_ = QStringLiteral("falta"), codigo_, enlace_, cuenta_, texto_, error_;
    bool subiendo_ = false, listo_ = false;
    double progreso_ = 0;

    QString clienteId_, clienteSecreto_;
    QString refresh_, access_;
    qint64 venceAccess_ = 0;
    QString deviceCode_;
    int intervalo_ = 5;
    qint64 venceCodigo_ = 0;
    QTimer sondeo_, cada_, pronto_;

    QJsonObject estadoGuardado_;   // drive.json sin el permiso: raiz, carreras, sesion
    QHash<QString, QString> huellas_;   // id de carrera → huella del informe (se borra si cambian las carreras)
    // Las huellas se arman de a una carrera por vez (QTimer): armar todos los informes de una trababa la pantalla (04-3.6)
    QStringList porHuellar_;
    QTimer huellar_;
    void huellarUna();

    QString red_ssid_;
    bool propia_ = false, ocupado_ = false;
    std::function<bool(const QString&)> borrador_;

    // Lo que se está subiendo
    int gen_ = 0;                  // sube al parar: las respuestas viejas no hacen nada
    bool trabajando_ = false;
    drive::Tarea tarea_;
    QString carpetaTarea_, rutaTarea_, mime_, nombreArchivo_, versionTarea_;
    qint64 total_ = 0;
    QPointer<QNetworkReply> enVuelo_;
    QProcess* curl_ = nullptr;
    QFile* leyendo_ = nullptr;
    qint64 faltaMandar_ = 0;
    bool reintento401_ = false;
    int cortes_ = 0;               // cortes seguidos sin avanzar
};
