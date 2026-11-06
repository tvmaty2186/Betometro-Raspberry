// Muestra el video como imagen común (para la Pi 3).
// El VideoOutput de Qt dibuja el video con un programa del chip gráfico que la Pi 3 (VideoCore IV, solo
// OpenGL ES 2) no puede armar: «Failed to build graphics pipeline state» y la pantalla queda negra
// (visto en la Pi de Matías el 06/11/2026). Acá el procesador pasa cada cuadro (YUV) a una imagen RGB
// y la pantalla la dibuja como una foto, que la Pi 3 sí sabe.
// Se usa igual que VideoOutput: MediaPlayer { videoOutput: imagen } (toma la propiedad videoSink).
// Si el video es grande para el lugar (720p o más en la mitad de la pantalla), se achica a la mitad al
// convertir: cuesta 4 veces menos y en la pantalla de 5" no se nota.
#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickItem>
#include <QVideoFrame>
#include <QVideoSink>

#include <atomic>

class ImagenVideo : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QVideoSink* videoSink READ videoSink CONSTANT)
    Q_PROPERTY(bool hayImagen READ hayImagen NOTIFY hayImagenChanged)

public:
    explicit ImagenVideo(QQuickItem* padre = nullptr);
    ~ImagenVideo() override;
    Q_INVOKABLE QVideoSink* videoSink() const { return sink_; }   // MediaPlayer lo busca por nombre
    bool hayImagen() const { return hayImagen_; }

    // Pasa un cuadro a imagen RGB; `paso` 2 = a la mitad de tamaño. Pública para las pruebas.
    static QImage convertir(const QVideoFrame& cuadro, int paso);

signals:
    void hayImagenChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* viejo, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& nueva, const QRectF& vieja) override;

private:
    void alCuadro(const QVideoFrame& cuadro);   // en el hilo del video

    QVideoSink* sink_;
    QMutex mutex_;
    QImage nueva_;           // la última convertida, esperando a la pantalla
    bool hayNueva_ = false, hayImagen_ = false;
    std::atomic<int> anchoLugar_{1280};  // ancho del lugar en la pantalla (en píxeles), para decidir si se achica
};
