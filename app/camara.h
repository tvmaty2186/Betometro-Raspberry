// Cámara y micrófono: encuentra la cámara USB (con H.264) y los micrófonos, y maneja al ayudante
// «betometro-camara» (GStreamer, en un proceso aparte con prioridad baja) para la vista previa,
// la prueba del micrófono y la grabación de la carrera. La carrera nunca espera por la cámara.
// En la PC, con BETO_CAMARA_PRUEBA=1, usa una cámara y un micrófono de prueba.
#pragma once

#include <QObject>
#include <QProcess>
#include <QSize>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariant>

class Datos;

class Camara : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hay READ hay NOTIFY dispositivosChanged)            // hay una cámara que sirve (H.264)
    Q_PROPERTY(QString nombre READ nombre NOTIFY dispositivosChanged)   // «ELP 1080p · H.264 · lista»
    Q_PROPERTY(QString problema READ problema NOTIFY dispositivosChanged)
    Q_PROPERTY(QVariantList microfonos READ microfonos NOTIFY dispositivosChanged)
    Q_PROPERTY(bool grabando READ grabando NOTIFY grabandoChanged)
    Q_PROPERTY(QString falla READ falla NOTIFY grabandoChanged)        // texto si la grabación se cortó
    Q_PROPERTY(QUrl vista READ vista NOTIFY vistaChanged)               // foto de la vista previa (cambia sola)
    Q_PROPERTY(bool viendo READ viendo NOTIFY vistaChanged)
    Q_PROPERTY(double nivel READ nivel NOTIFY nivelChanged)             // micrófono, 0..1
    Q_PROPERTY(QString pruebaMic READ pruebaMic NOTIFY pruebaMicChanged) // "", grabando, escuchando
    Q_PROPERTY(QVariantMap lugar READ lugar NOTIFY lugarChanged)        // usadoGB, videos, horas
    Q_PROPERTY(QVariantList videos READ videos NOTIFY lugarChanged)
    Q_PROPERTY(bool convirtiendo READ convirtiendo NOTIFY convirtiendoChanged)   // pasando un video a MP4   // los de Datos::videosGuardados, + «grabando»

public:
    void hacerLugar();                       // borra videos viejos si falta lugar (al abrir la carrera, no al largar)
    Camara(Datos* datos, const QString& carpetaDatos, QObject* padre = nullptr);
    ~Camara() override;

    bool hay() const { return !dispVideo_.isEmpty(); }
    QString nombre() const { return nombre_; }
    QString problema() const { return problema_; }
    QVariantList microfonos() const { return microfonos_; }
    bool grabando() const { return grab_.state() != QProcess::NotRunning; }
    bool convirtiendo() const { return mp4_.state() != QProcess::NotRunning; }
    // En carrera no se pasa nada a MP4 (y si estaba pasando uno, se corta y se sigue después)
    void ponerEnCarrera(bool enCarrera);
    QString falla() const { return falla_; }
    QUrl vista() const { return vista_; }
    bool viendo() const { return vistaProc_.state() != QProcess::NotRunning; }
    double nivel() const { return nivel_; }
    QString pruebaMic() const { return pruebaMic_; }
    QVariantMap lugar() const { return lugar_; }
    QVariantList videos() const;

    Q_INVOKABLE void detectar();
    Q_INVOKABLE void empezarVista();
    Q_INVOKABLE void pararVista();
    Q_INVOKABLE void empezarNivel();
    Q_INVOKABLE void pararNivel();
    Q_INVOKABLE void probarMicrofono();       // graba 5 s y los reproduce
    Q_INVOKABLE void probarGrabacion();       // 10 s de video, para ver que anda
    // Borra un video (archivo y lo de la carrera) y actualiza el lugar. Nunca mientras graba.
    Q_INVOKABLE bool borrarVideo(const QString& archivo);
    Q_INVOKABLE int borrarPruebas();          // borra los prueba-*; devuelve cuántos
    // Dirección (file://) de un video de la carpeta, para el reproductor del informe. Vacía si no está el archivo.
    Q_INVOKABLE QString urlVideo(const QString& archivo) const;

    // Para la carrera (Crono)
    bool empezar(const QString& nombreBase);  // false si no hay cámara o está apagada
    void detener(int retrasoS);
    QString archivo() const { return archivo_; }
    double primerCuadro() const { return primerCuadro_; }   // en el reloj de la carrera (ahoraMs); 0 si todavía no

    QString ayudante() const;
    QString dispositivoAudio() const;         // según el ajuste camaraSonido; "no" si no hay

signals:
    void dispositivosChanged();
    void grabandoChanged();
    void vistaChanged();
    void nivelChanged();
    void pruebaMicChanged();
    void lugarChanged();
    void convirtiendoChanged();
    void primerCuadroListo(double ahora);    // hora del primer cuadro, en el reloj de la carrera

private:
    void leer(QProcess& p, const std::function<void(const QVariantMap&)>& f);
    void revisarMp4();                       // pasa a MP4 el .mkv más viejo, si se puede ahora
    void actualizarLugar();
    QStringList base() const;                // opciones comunes (prueba en la PC)

    Datos* datos_;
    QString carpeta_;                         // /datos/videos
    bool prueba_ = false;
    QString formatoVideo_ = QStringLiteral("h264");   // h264 o mjpeg (webcams comunes)
    QList<QSize> tamanos_;                    // tamaños que da la cámara en ese formato
    QString dispVideo_, nombre_, problema_, falla_, archivo_, pruebaMic_;
    QVariantList microfonos_;
    QProcess grab_, vistaProc_, nivelProc_, micProc_, mp4_;
    QString mp4Entrada_;                      // el .mkv que se está pasando
    bool mp4Error_ = false, enCarrera_ = false;
    QStringList mp4Fallados_;                 // no se reintentan hasta reiniciar
    QTimer detenerTimer_;
    QTimer matarTimer_;                       // si 12 s después de terminate() sigue vivo, kill() (02-1.2)
    qlonglong cuadrosAntes_ = -1;             // para ver si la cámara se colgó (no suben los cuadros)
    void cortar(const QString& motivo);       // corta la grabación por un problema (queda en falla())
    QUrl vista_;
    int fotos_ = 0;
    double nivel_ = 0, primerCuadro_ = 0;
    QVariantMap lugar_;
    QVariantMap cuadros_;                     // conteo de cuadros de la grabación (cortados, dañados)
};
