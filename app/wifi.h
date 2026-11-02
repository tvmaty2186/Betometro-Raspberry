// Wifi y «Betómetro en la red». En la PC es simulado; el real (wpa_supplicant, mDNS, servidor web) llega en E5.
#pragma once

#include <QObject>
#include <QTimer>
#include <QVariant>

class Wifi : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString conectada READ conectada NOTIFY cambio)        // nombre de la red, vacío si no hay
    Q_PROPERTY(int senal READ senal NOTIFY cambio)                    // 0 a 4 rayitas
    Q_PROPERTY(bool conectando READ conectando NOTIFY cambio)
    Q_PROPERTY(bool buscando READ buscando NOTIFY cambio)
    Q_PROPERTY(QVariantList cerca READ cerca NOTIFY cambio)           // [{nombre, senal, clave}] sin las guardadas
    Q_PROPERTY(QVariantList guardadas READ guardadas NOTIFY cambio)   // [{nombre, senal, cerca}]
    Q_PROPERTY(bool horaInternet READ horaInternet NOTIFY cambio)
    Q_PROPERTY(QString direccion READ direccion CONSTANT)
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
    QString direccion() const { return QStringLiteral("betometro.local"); }
    QString error() const { return error_; }

    Q_INVOKABLE void buscar();
    Q_INVOKABLE bool tieneClave(const QString& red) const;
    Q_INVOKABLE void conectar(const QString& red, const QString& clave);
    Q_INVOKABLE void olvidar(const QString& red);

signals:
    void cambio();
    void conectado(const QString& red);

private:
    struct Red { QString nombre; int senal; bool clave; };
    QList<Red> redes_;            // las que se ven
    QStringList guardadas_;
    QString conectada_, error_;
    bool conectando_ = false, buscando_ = false;
};
