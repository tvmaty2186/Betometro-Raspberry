// Reproductor de la voz y los sonidos. Todos los archivos se cargan en memoria al arrancar,
// así suenan sin demora. En la Raspberry salen por los auriculares (3,5 mm).
#pragma once

#include <QHash>
#include <QObject>
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
    Q_INVOKABLE void probar() { reproducir(QStringLiteral("prueba")); }
    // «Vuelta 3, medida» → el archivo que corresponde
    static QString archivoDeFrase(const QString& frase);

signals:
    void cambio();

private:
    QString carpeta() const;
    QHash<QString, QSoundEffect*> efectos_;
    double volumen_ = 1;
};
