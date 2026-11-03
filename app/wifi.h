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

public:
    explicit Wifi(QObject* padre = nullptr);
    QString conectada() const { return conectada_; }
    int senal() const;
    bool conectando() const { return conectando_; }
    bool buscando() const { return buscando_; }
    QVariantList cerca() const;
    QVariantList guardadas() const;
    bool horaInternet() const { return !conectada_.isEmpty(); }
    QString direccion() const;
    QString error() const { return error_; }

    Q_INVOKABLE void buscar();
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

    bool real_ = false;
    QList<Red> redes_;            // las que se ven
    QStringList guardadas_;
    QHash<QString, QString> conexion_;   // red → nombre de la conexión guardada en NetworkManager
    QString conectada_, error_;
    int senalConectada_ = 0;
    bool conectando_ = false, buscando_ = false;
    QTimer cada_;
};
