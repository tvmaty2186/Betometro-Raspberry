// Reproductor de video con GStreamer, para usar el descompresor de video de la Raspberry (chip aparte,
// «v4l2h264dec», mueve 1080p sin cansar al procesador). El MediaPlayer de Qt (FFmpeg) no lo usa en la Pi
// y descomprime con el procesador. Pedido de Matías, 06/11/2026: «no podemos desperdiciar ese recurso».
//
// Se usa en QML igual que MediaPlayer (mismos nombres y mismos números de estado):
//   VideoGst { source: url; videoOutput: imagen; position; duration; playbackState; mediaStatus; play(); pause() }
// La imagen va directo a ImagenVideo (recibir(), sin copiar el cuadro). Otra salida: por su videoSink.
// El sonido va por el sistema (PulseAudio). Si no está el chip (en la PC), GStreamer usa otro descompresor.
#pragma once

#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVideoSink>

#include <atomic>

typedef struct _GstElement GstElement;
typedef struct _GstAppSink GstAppSink;
typedef struct _GstSample GstSample;
class ImagenVideo;

class VideoGst : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QObject* videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int playbackState READ playbackState NOTIFY playbackStateChanged)   // 0 parado, 1 andando, 2 en pausa
    Q_PROPERTY(int mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)         // los de MediaPlayer
    Q_PROPERTY(QString descompresor READ descompresor NOTIFY descompresorChanged)   // «v4l2h264dec» = el chip

public:
    // Los mismos números que MediaPlayer (así el QML compara con MediaPlayer.LoadedMedia, etc.)
    enum Estado { Parado = 0, Andando = 1, Pausa = 2 };
    enum Medio { SinMedio = 0, Cargando = 1, Cargado = 2, Terminado = 6, Invalido = 7 };

    explicit VideoGst(QObject* padre = nullptr);
    ~VideoGst() override;

    QUrl source() const { return source_; }
    void setSource(const QUrl& url);
    QObject* videoOutput() const { return salida_; }
    void setVideoOutput(QObject* o);
    qint64 position() const { return posicion_; }
    void setPosition(qint64 ms);
    qint64 duration() const { return duracion_; }
    int playbackState() const { return estado_; }
    int mediaStatus() const { return medio_; }
    QString descompresor() const { return descompresor_; }

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();

    // Le da prioridad al chip de la Raspberry sobre los descompresores de procesador (una vez al empezar)
    static void preferirChip();

    // Para los avisos de GStreamer (otros hilos)
    void entregar(GstSample* muestra);               // un cuadro: al videoSink
    void ponerDescompresor(const QString& nombre);   // en el hilo de la pantalla
    void ponerRangoCompleto(bool si) { rangoCompleto_ = si; }   // la etiqueta de color original del video

signals:
    void sourceChanged();
    void videoOutputChanged();
    void positionChanged(qint64 position);
    void durationChanged();
    void playbackStateChanged();
    void mediaStatusChanged();
    void descompresorChanged();
    void errorOccurred(int error, const QString& errorString);

private:
    void cerrar();
    void abrir();
    void revisarBus();
    void ponerEstado(int e);
    void ponerMedio(int m);
    void leerPosicion();

    QUrl source_;
    QPointer<QObject> salida_;
    QMutex mutexSink_;                 // cuida sink_ e imagen_ (el hilo del video los usa)
    QPointer<QVideoSink> sink_;
    ImagenVideo* imagen_ = nullptr;    // se borra al irse la ImagenVideo (seVa)
    QMetaObject::Connection seVa_;
    GstElement* tubo_ = nullptr;
    QTimer bus_, reloj_;
    qint64 posicion_ = 0, duracion_ = 0, buscando_ = -1;
    int estado_ = Parado, medio_ = SinMedio;
    bool listo_ = false, quiereAndar_ = false;
    QString descompresor_;
    std::atomic<bool> rangoCompleto_{false};
};
