// Reproductor de la voz y los sonidos. Todos los archivos se cargan en memoria al arrancar,
// así suenan sin demora. Los sonidos (pitidos, clic) están en la carpeta de la voz y las frases
// en una subcarpeta por voz (elena, tomas); si no hay subcarpeta, se usan las de la carpeta. En la Raspberry salen por los auriculares (3,5 mm).
#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QSoundEffect>

class Audio : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool lista READ lista NOTIFY cambio)
    Q_PROPERTY(int cargados READ cargados NOTIFY cambio)
public:
    explicit Audio(QObject* padre = nullptr);
    bool lista() const { return !efectos_.isEmpty(); }
    int cargados() const { return static_cast<int>(efectos_.size()); }

    Q_INVOKABLE void reproducir(const QString& nombre);     // «n05», «clic», «vuelta_03_m»…
    Q_INVOKABLE void ponerVolumen(double porcentaje);       // 0 a 100
    Q_INVOKABLE void ponerVoz(const QString& voz);          // elena, tomas
    Q_INVOKABLE void probar() { reproducir(QStringLiteral("prueba")); }
    // «Vuelta 3, medida» → el archivo que corresponde
    static QString archivoDeFrase(const QString& frase);

signals:
    void cambio();

private:
    QString carpeta() const;
    void cargar(const QString& dir);
    QString voz_;
    QHash<QString, QSoundEffect*> efectos_;
    QSet<QString> faltantes_;   // audios que faltaron (se avisa una vez por nombre)
    double volumen_ = 1;
};
