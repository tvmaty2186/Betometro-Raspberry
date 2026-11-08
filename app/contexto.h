// Lo que la pantalla (QML) lee del programa: orientación, tema, versión y estado del equipo.
// Por ahora con datos de muestra; en las próximas partes de E2 se conecta con el núcleo.
#pragma once

#include <QFile>
#include <QObject>
#include <QRegularExpression>
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
    Q_PROPERTY(bool fijo READ fijo CONSTANT)          // BETO_FIJO=1 (capturas comparadas): hora y GPS quietos
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString versionNueva READ versionNueva NOTIFY versionNuevaChanged)
    Q_PROPERTY(QString numeroAuto READ numeroAuto NOTIFY numeroAutoChanged)
    Q_PROPERTY(QString nombreAuto READ nombreAuto NOTIFY numeroAutoChanged)
    Q_PROPERTY(int satelites READ satelites NOTIFY gpsChanged)
    Q_PROPERTY(QString redWifi READ redWifi NOTIFY wifiChanged)
    Q_PROPERTY(QString horaFuente READ horaFuente NOTIFY horaFuenteChanged)
    // Foto propia del inicio (Personalizar, 08/11/2026): dirección del archivo acostada / parada, "" = la de fábrica
    Q_PROPERTY(QString fotoH READ fotoH NOTIFY fotoChanged)
    Q_PROPERTY(QString fotoV READ fotoV NOTIFY fotoChanged)   // de dónde salió la hora: internet, gps o ""

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
    bool fijo() const { return fijo_; }
    void setFijo(bool f) { fijo_ = f; }
    QString idInicial() const { return idInicial_; }
    void setIdInicial(const QString& i) { idInicial_ = i; }
    QString version() const { return QStringLiteral(BETO_VERSION); }
    QString versionNueva() const { return versionNueva_; }
    void setVersionNueva(const QString& v) { if (v != versionNueva_) { versionNueva_ = v; emit versionNuevaChanged(); } }
    QString numeroAuto() const { return numeroAuto_; }
    QString nombreAuto() const { return nombreAuto_; }
    int satelites() const { return satelites_; }
    QString redWifi() const { return redWifi_; }

    // El manual del navegante (recursos/web/ayuda.html, el mismo de /ayuda en el celu) en el HTML simple que entiende
    // un Text de QML: sin índice ni cabecera, con los estilos puestos a mano (hoja clara, se lee con sol)
    Q_INVOKABLE QString manual() const {
        QFile f(QStringLiteral(":/recursos/web/ayuda.html"));
        if (!f.open(QIODevice::ReadOnly)) return QStringLiteral("Falta el manual");
        QString h = QString::fromUtf8(f.readAll());
        const int a = h.indexOf(QLatin1String("</nav>")), b = h.indexOf(QLatin1String("<p class=pie>"));
        if (a < 0 || b < a) return QStringLiteral("Falta el manual");
        h = h.mid(a + 6, b - a - 6);
        using R = QRegularExpression;
        h.replace(R(QStringLiteral("<section[^>]*>|</section>")), QString());
        h.replace(R(QStringLiteral("<a [^>]*>([^<]*)</a>")), QStringLiteral("\\1"));
        h.replace(R(QStringLiteral("<h2><span class=n>([0-9]+)</span>")), QStringLiteral("<h2><span style=\"color:#b8860b\">\\1 · </span>"));
        h.replace(QLatin1String("</span> <span class=pt>"), QStringLiteral("</span> · <span class=pt>"));   // los nombres en fila
        h.replace(QLatin1String("<span class=bt>"), QStringLiteral("<span style=\"background-color:#f4c531;color:#14240f;font-style:italic\">&nbsp;"));
        h.replace(QLatin1String("<span class=pt>"), QStringLiteral("<span style=\"color:#1d4a2c;font-style:italic\">"));
        // Los recuadros: una tabla de una celda con fondo (el Text de QML no pinta el fondo de un div)
        h.replace(R(QStringLiteral("<div class=\"?ojo( mal)?\"?>(.*?)</div>"), R::DotMatchesEverythingOption), QStringLiteral("<table width=\"100%\" cellpadding=\"12\"><tr><td bgcolor=\"#f3ecd2\">\\2</td></tr></table>"));
        h.replace(QLatin1String("<table>"), QStringLiteral("<table width=\"100%\" cellpadding=\"6\" cellspacing=\"0\">"));
        h.replace(QLatin1String("<th>"), QStringLiteral("<th align=\"left\" bgcolor=\"#e8e2cc\">"));
        h.replace(QLatin1String("<h2>"), QStringLiteral("<h2 style=\"color:#1d4a2c\">"));
        h.replace(QLatin1String("<h3>"), QStringLiteral("<h3 style=\"color:#8a6a00\">"));
        return h;
    }
    void ponerCarpeta(const QString& c) { carpeta_ = c; emit fotoChanged(); }
    QString archivoFoto(bool h) const { return carpeta_ + (h ? QStringLiteral("/inicio-h.jpg") : QStringLiteral("/inicio-v.jpg")); }
    QString fotoH() const { return url(archivoFoto(true)); }
    QString fotoV() const { return url(archivoFoto(false)); }
    void fotoCambio() { ++fotoVersion_; emit fotoChanged(); }
    QString horaFuente() const { return horaFuente_; }
    void setHoraFuente(const QString& f) { if (f != horaFuente_) { horaFuente_ = f; emit horaFuenteChanged(); } }
signals:
    void fotoChanged();
    void horaFuenteChanged();
    void horizontalChanged();
    void temaChanged();
    void solChanged();
    void versionNuevaChanged();
    void numeroAutoChanged();
    void gpsChanged();
    void wifiChanged();

private:
    QString url(const QString& f) const {
        if (carpeta_.isEmpty() || !QFile::exists(f)) return {};
        return QStringLiteral("file://") + f + QStringLiteral("?v=") + QString::number(fotoVersion_);   // otra dirección: se vuelve a leer
    }
    QString carpeta_;
    int fotoVersion_ = 0;
    QString horaFuente_ = QStringLiteral("internet");
    bool horizontal_ = true;
    bool fisica_ = false;
    bool sol_ = false;
    QString tema_ = QStringLiteral("avispon");
    QString pantallaInicial_ = QStringLiteral("inicio");
    QString idInicial_;
    QString prueba_;
    bool fijo_ = false;
    QString versionNueva_;                              // la pone el Actualizador
    QString numeroAuto_ = QStringLiteral("101");
    QString nombreAuto_ = QStringLiteral("Avispón Verde");
    int satelites_ = 9;                                 // muestra
    QString redWifi_ = QStringLiteral("Celu de Matías"); // muestra
};
