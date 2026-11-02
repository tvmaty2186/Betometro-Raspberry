// Lo que la pantalla (QML) lee del programa: orientación, tema, versión y estado del equipo.
// Por ahora con datos de muestra; en las próximas partes de E2 se conecta con el núcleo.
#pragma once

#include <QObject>
#include <QString>

class Contexto : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool horizontal READ horizontal WRITE setHorizontal NOTIFY horizontalChanged)
    Q_PROPERTY(bool fisica READ fisica CONSTANT)
    Q_PROPERTY(QString tema READ tema WRITE setTema NOTIFY temaChanged)
    Q_PROPERTY(bool sol READ sol WRITE setSol NOTIFY solChanged)   // modo sol: fondo claro, tinta negra
    Q_PROPERTY(QString pantallaInicial READ pantallaInicial CONSTANT)
    Q_PROPERTY(QString idInicial READ idInicial CONSTANT)
    Q_PROPERTY(QString prueba READ prueba CONSTANT)   // solo para capturas: abre algo al empezar
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString versionNueva READ versionNueva NOTIFY versionNuevaChanged)
    Q_PROPERTY(QString numeroAuto READ numeroAuto NOTIFY numeroAutoChanged)
    Q_PROPERTY(QString nombreAuto READ nombreAuto NOTIFY numeroAutoChanged)
    Q_PROPERTY(int satelites READ satelites NOTIFY gpsChanged)
    Q_PROPERTY(QString redWifi READ redWifi NOTIFY wifiChanged)

public:
    bool horizontal() const { return horizontal_; }
    void setHorizontal(bool h) { if (h != horizontal_) { horizontal_ = h; emit horizontalChanged(); } }
    bool fisica() const { return fisica_; }
    void setFisica(bool f) { fisica_ = f; }
    QString tema() const { return tema_; }
    void setTema(const QString& t) { if (t != tema_) { tema_ = t; emit temaChanged(); } }
    bool sol() const { return sol_; }
    void setSol(bool s) { if (s != sol_) { sol_ = s; emit solChanged(); } }
    QString pantallaInicial() const { return pantallaInicial_; }
    void setPantallaInicial(const QString& p) { pantallaInicial_ = p; }
    QString prueba() const { return prueba_; }
    void setPrueba(const QString& p) { prueba_ = p; }
    QString idInicial() const { return idInicial_; }
    void setIdInicial(const QString& i) { idInicial_ = i; }
    QString version() const { return QStringLiteral(BETO_VERSION); }
    QString versionNueva() const { return versionNueva_; }
    void setVersionNueva(const QString& v) { if (v != versionNueva_) { versionNueva_ = v; emit versionNuevaChanged(); } }
    QString numeroAuto() const { return numeroAuto_; }
    QString nombreAuto() const { return nombreAuto_; }
    int satelites() const { return satelites_; }
    QString redWifi() const { return redWifi_; }

signals:
    void horizontalChanged();
    void temaChanged();
    void solChanged();
    void versionNuevaChanged();
    void numeroAutoChanged();
    void gpsChanged();
    void wifiChanged();

private:
    bool horizontal_ = true;
    bool fisica_ = false;
    bool sol_ = false;
    QString tema_ = QStringLiteral("avispon");
    QString pantallaInicial_ = QStringLiteral("inicio");
    QString idInicial_;
    QString prueba_;
    QString versionNueva_;                              // la pone el Actualizador
    QString numeroAuto_ = QStringLiteral("101");
    QString nombreAuto_ = QStringLiteral("Avispón Verde");
    int satelites_ = 9;                                 // muestra
    QString redWifi_ = QStringLiteral("Celu de Matías"); // muestra
};
