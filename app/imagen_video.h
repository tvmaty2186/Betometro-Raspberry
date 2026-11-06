// Muestra el video (para la Pi 3).
// El VideoOutput de Qt dibuja el video con un programa del chip gráfico que la Pi 3 (VideoCore IV, solo
// OpenGL ES 2) no puede armar: «Failed to build graphics pipeline state» y la pantalla queda negra
// (visto en la Pi de Matías el 06/11/2026). Acá hay dos caminos:
//  - Chip gráfico (el de siempre): los tres planos YUV suben tal cual como texturas de un canal y un
//    programa propio, hecho para GLES 2 (app/shaders/yuv.*), pasa a colores. El procesador casi no trabaja.
//  - Procesador (repuesto): pasa cada cuadro a una imagen RGB, como en la 1.0.19. Se usa con el dibujo por
//    software, si el programa propio no arma, o con BETO_VIDEO_CPU=1.
// Los cuadros llegan en el hilo del video y se guarda solo el último: el video nunca espera a la pantalla.
// Se usa igual que VideoOutput: MediaPlayer { videoOutput: imagen } (toma la propiedad videoSink).
// VideoGst entrega directo con recibir() (sin copiar el cuadro).
#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickItem>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

// Un cuadro YUV 4:2:0 para leer sin copiar: los datos son de quien lo descomprimió.
// `soltar` devuelve la memoria al borrarse (en cualquier hilo).
struct CuadroYuv {
    enum Formato { I420, NV12, NV21, Otro };
    int ancho = 0, alto = 0;
    Formato formato = Otro;
    bool completo = false;            // rango completo (0–255)
    const uchar* plano[3] = {};       // I420: Y, U, V. NV12/NV21: Y, UV
    int paso[3] = {};                 // bytes por fila de cada plano
    QVideoFrame otro;                 // formato raro: lo convierte Qt (por el procesador)
    std::function<void()> soltar;
    quint64 numero = 0;               // orden de llegada (lo pone ImagenVideo)
    CuadroYuv() = default;
    CuadroYuv(const CuadroYuv&) = delete;
    CuadroYuv& operator=(const CuadroYuv&) = delete;
    ~CuadroYuv() { if (soltar) soltar(); }
};
using CuadroPtr = std::shared_ptr<CuadroYuv>;

class ImagenVideo : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QVideoSink* videoSink READ videoSink CONSTANT)
    Q_PROPERTY(bool hayImagen READ hayImagen NOTIFY hayImagenChanged)

public:
    explicit ImagenVideo(QQuickItem* padre = nullptr);
    ~ImagenVideo() override;
    Q_INVOKABLE QVideoSink* videoSink() const { return sink_; }   // MediaPlayer lo busca por nombre
    bool hayImagen() const { return hayImagen_; }

    // Un cuadro nuevo (cualquier hilo, vuelve enseguida). Reemplaza al que estaba esperando.
    void recibir(CuadroPtr cuadro);

    // Pasa un cuadro a imagen RGB; `paso` 2 = a la mitad de tamaño. Pública para las pruebas.
    static QImage convertir(const QVideoFrame& cuadro, int paso);
    static QImage convertir(const CuadroYuv& cuadro, int paso);

signals:
    void hayImagenChanged();
    void seVa();   // al empezar a borrarse (VideoGst deja de mandar)

protected:
    QSGNode* updatePaintNode(QSGNode* viejo, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& nueva, const QRectF& vieja) override;

private:
    enum Modo { SinSaber = 0, Chip = 1, Procesador = 2 };
    void alCuadro(const QVideoFrame& cuadro);   // del videoSink, en el hilo del video
    void avisar();                              // que la pantalla se actualice (una vez por vez)
    void trabajar();                            // hilo que convierte por el procesador
    bool sirveChip(const CuadroYuv& c) const;
    int decidirModo();                          // en el hilo que dibuja, la primera vez

    QVideoSink* sink_;
    std::mutex mutex_;
    std::condition_variable hayTrabajo_;
    CuadroPtr paraChip_, paraProcesador_;       // el último que llegó, para cada camino
    QImage nueva_;                              // la última convertida por el procesador
    quint64 numeroNueva_ = 0, numero_ = 0;
    bool fin_ = false;
    std::thread obrero_;
    std::atomic<int> modo_{SinSaber};
    std::atomic<int> maxTextura_{2048};
    std::atomic<bool> avisando_{false};
    bool hayImagen_ = false;
    QTimer soltar_;
    std::atomic<int> anchoLugar_{1280};  // ancho del lugar en la pantalla (en píxeles), para decidir si se achica
    // Del hilo que dibuja (render)
    quint64 numeroMostrado_ = 0;
    QSizeF tamVideo_;
    bool rojo_ = false;                  // el plano de un canal se lee en .r (si no, en .a: GL_ALPHA)
};
