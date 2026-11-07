// Wifi. En la Raspberry es real: usa NetworkManager (nmcli), que es el que trae Raspberry Pi OS.
// En la PC es simulado (para ver las pantallas).
#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <functional>

class Wifi : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString conectada READ conectada NOTIFY cambio)        // nombre de la red, vacío si no hay
    Q_PROPERTY(int senal READ senal NOTIFY cambio)                    // 0 a 4 rayitas
    Q_PROPERTY(bool conectando READ conectando NOTIFY cambio)
    Q_PROPERTY(bool buscando READ buscando NOTIFY cambio)
    Q_PROPERTY(QVariantList cerca READ cerca NOTIFY cambio)           // [{nombre, senal, clave}] sin las guardadas
    Q_PROPERTY(QVariantList guardadas READ guardadas NOTIFY cambio)   // [{nombre, senal, cerca, conectada}]
    Q_PROPERTY(bool horaInternet READ horaInternet NOTIFY cambio)
    Q_PROPERTY(QString direccion READ direccion NOTIFY cambio)        // para el celular: la IP (Android no entiende «.local»)
    Q_PROPERTY(QString error READ error NOTIFY cambio)
    // Wifi propia: si no hay ninguna red guardada cerca, la Raspberry arma la suya («Betometro-101»)
    Q_PROPERTY(bool propia READ propia NOTIFY cambio)
    Q_PROPERTY(QString nombrePropia READ nombrePropia NOTIFY cambio)
    Q_PROPERTY(QString clavePropia READ clavePropia CONSTANT)
    Q_PROPERTY(int conectadosPropia READ conectadosPropia NOTIFY cambio)   // celus conectados a la wifi propia
    Q_PROPERTY(QString errorPropia READ errorPropia NOTIFY cambio)         // por qué no se pudo armar (en palabras)
    Q_PROPERTY(bool armandoPropia READ armandoPropia NOTIFY cambio)
    // «Conectarse a redes wifi» (Conectividad). Apagado: se desconecta, no se conecta a ninguna red y arma la propia
    Q_PROPERTY(bool redes READ redes NOTIFY cambio)

public:
    explicit Wifi(QObject* padre = nullptr);
    QString conectada() const { return conectada_; }
    int senal() const;
    bool conectando() const { return conectando_; }
    bool buscando() const { return buscando_; }
    QVariantList cerca() const;
    QVariantList guardadas() const;
    bool horaInternet() const { return !conectada_.isEmpty() && !propia_; }
    bool propia() const { return propia_; }
    QString nombrePropia() const { return QStringLiteral("Betometro-") + numero_; }
    QString clavePropia() const { return QStringLiteral("betometro"); }
    void ponerPropia(bool permitida, const QString& numeroAuto);   // desde Ajustes → Conectividad
    int conectadosPropia() const { return conectadosPropia_; }
    QString errorPropia() const { return errorPropia_; }
    bool armandoPropia() const { return armandoPropia_; }
    bool redes() const { return redes_permitidas_; }
    void ponerRedes(bool permitidas);   // desde la configuración (wifiRedes)
    Q_INVOKABLE void armarPropia();   // ya, sin esperar (botón en Conectividad)
    QString direccion() const;
    QString error() const { return error_; }

    Q_INVOKABLE void buscar();
    void modoCarrera(bool enCarrera);   // en carrera se revisa cada 60 s en vez de cada 10 s (02-1.3, 04-3.3)
    Q_INVOKABLE bool tieneClave(const QString& red) const;
    Q_INVOKABLE void conectar(const QString& red, const QString& clave);
    Q_INVOKABLE void olvidar(const QString& red);

    // Separa una línea de «nmcli -t» (los «:» de adentro vienen como «\:»). Público para probarlo.
    static QStringList campos(const QString& linea);

signals:
    void cambio();
    void conectado(const QString& red);

private:
    struct Red { QString nombre; int senal; bool clave; };
    void nmcli(const QStringList& args, std::function<void(int, const QString&, const QString&)> listo);
    void refrescar();             // red conectada, señal y redes guardadas
    void revisarPropia();         // ¿hay que armar la wifi propia?
    void apagarPropia();
    void cortarYArmar();
    void contarConectados();

    bool real_ = false;
    QList<Red> redes_;            // las que se ven
    QStringList guardadas_;
    QHash<QString, QString> conexion_;   // red → nombre de la conexión guardada en NetworkManager
    QString conectada_, error_;
    int senalConectada_ = 0;
    bool conectando_ = false, buscando_ = false;
    bool propia_ = false, propiaPermitida_ = true;
    QString numero_ = QStringLiteral("101");
    qint64 sinRedDesde_ = 0;      // ms desde que no hay red (para esperar antes de armar la propia)
    int conectadosPropia_ = 0;
    QString errorPropia_;
    bool armandoPropia_ = false;
    bool redes_permitidas_ = true;
    qint64 fallaPropia_ = 0;      // ms de la última vez que no se pudo armar (para no reintentar sin parar)
    QTimer cada_;
    int enVuelo_ = 0;             // nmcli/iw que todavía no terminaron: no se lanza otra vuelta encima
};
