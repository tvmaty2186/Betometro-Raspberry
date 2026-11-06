#include "imagen_video.h"

#include <QQuickWindow>
#include <QSGSimpleTextureNode>

#include <algorithm>

namespace {

inline quint32 armar(int r, int g, int b) {
    return 0xff000000u | (quint32(std::clamp(r, 0, 255)) << 16) | (quint32(std::clamp(g, 0, 255)) << 8) | quint32(std::clamp(b, 0, 255));
}

// BT.601. `completo`: rango de las fotos JPEG (0–255, webcams MJPEG); si no, el de video (16–235)
template <bool completo>
inline quint32 rgb(int y, int u, int v) {
    const int d = u - 128, e = v - 128;
    if (completo) {
        const int c = y * 256;
        return armar((c + 359 * e + 128) >> 8, (c - 88 * d - 183 * e + 128) >> 8, (c + 454 * d + 128) >> 8);
    }
    const int c = (y - 16) * 298;
    return armar((c + 409 * e + 128) >> 8, (c - 100 * d - 208 * e + 128) >> 8, (c + 516 * d + 128) >> 8);
}

}  // namespace

ImagenVideo::ImagenVideo(QQuickItem* padre) : QQuickItem(padre), sink_(new QVideoSink(this)) {
    setFlag(ItemHasContents, true);
    // El cuadro llega en el hilo del video: se convierte ahí (no traba la pantalla)
    connect(sink_, &QVideoSink::videoFrameChanged, this, &ImagenVideo::alCuadro, Qt::DirectConnection);
}

ImagenVideo::~ImagenVideo() {
    // Que no llegue un cuadro mientras se borra
    disconnect(sink_, nullptr, this, nullptr);
}

void ImagenVideo::geometryChange(const QRectF& nueva, const QRectF& vieja) {
    QQuickItem::geometryChange(nueva, vieja);
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    anchoLugar_ = qMax(1, qRound(nueva.width() * dpr));
    update();
}

template <bool completo>
static QImage convertirCon(QVideoFrame& cuadro, int paso);

QImage ImagenVideo::convertir(const QVideoFrame& original, int paso) {
    QVideoFrame cuadro(original);
    return cuadro.surfaceFormat().colorRange() == QVideoFrameFormat::ColorRange_Full ? convertirCon<true>(cuadro, paso)
                                                                                      : convertirCon<false>(cuadro, paso);
}

template <bool completo>
static QImage convertirCon(QVideoFrame& cuadro, int paso) {
    if (!cuadro.isValid() || !cuadro.map(QVideoFrame::ReadOnly)) return {};
    const int w = cuadro.width(), h = cuadro.height();
    const auto fmt = cuadro.pixelFormat();
    paso = paso == 2 ? 2 : 1;
    QImage img;
    if (fmt == QVideoFrameFormat::Format_YUV420P || fmt == QVideoFrameFormat::Format_YV12) {
        const bool yv12 = fmt == QVideoFrameFormat::Format_YV12;
        const uchar *Y = cuadro.bits(0), *U = cuadro.bits(yv12 ? 2 : 1), *V = cuadro.bits(yv12 ? 1 : 2);
        const int sy = cuadro.bytesPerLine(0), su = cuadro.bytesPerLine(yv12 ? 2 : 1), sv = cuadro.bytesPerLine(yv12 ? 1 : 2);
        img = QImage(w / paso, h / paso, QImage::Format_RGB32);
        for (int fy = 0; fy < img.height(); ++fy) {
            const int y = fy * paso;
            const uchar* fY = Y + y * sy;
            const uchar* fU = U + (y / 2) * su;
            const uchar* fV = V + (y / 2) * sv;
            quint32* o = reinterpret_cast<quint32*>(img.scanLine(fy));
            for (int fx = 0; fx < img.width(); ++fx) {
                const int x = fx * paso;
                o[fx] = rgb<completo>(fY[x], fU[x / 2], fV[x / 2]);
            }
        }
    } else if (fmt == QVideoFrameFormat::Format_NV12 || fmt == QVideoFrameFormat::Format_NV21) {
        const bool nv21 = fmt == QVideoFrameFormat::Format_NV21;
        const uchar *Y = cuadro.bits(0), *UV = cuadro.bits(1);
        const int sy = cuadro.bytesPerLine(0), suv = cuadro.bytesPerLine(1);
        img = QImage(w / paso, h / paso, QImage::Format_RGB32);
        for (int fy = 0; fy < img.height(); ++fy) {
            const int y = fy * paso;
            const uchar* fY = Y + y * sy;
            const uchar* fUV = UV + (y / 2) * suv;
            quint32* o = reinterpret_cast<quint32*>(img.scanLine(fy));
            for (int fx = 0; fx < img.width(); ++fx) {
                const int x = fx * paso, c = (x / 2) * 2;
                const int u = fUV[c + (nv21 ? 1 : 0)], v = fUV[c + (nv21 ? 0 : 1)];
                o[fx] = rgb<completo>(fY[x], u, v);
            }
        }
    } else {
        // Otros formatos (raros acá): la conversión de Qt
        cuadro.unmap();
        img = cuadro.toImage();
        if (paso == 2 && !img.isNull()) img = img.scaled(img.width() / 2, img.height() / 2);
        return img;
    }
    cuadro.unmap();
    return img;
}

void ImagenVideo::alCuadro(const QVideoFrame& cuadro) {
    if (!cuadro.isValid()) return;
    // A la mitad si el video es al menos el doble de ancho que el lugar donde se muestra
    const int paso = cuadro.width() >= 2 * anchoLugar_ ? 2 : 1;
    QImage img = convertir(cuadro, paso);
    if (img.isNull()) return;
    {
        QMutexLocker l(&mutex_);
        nueva_ = std::move(img);
        hayNueva_ = true;
    }
    QMetaObject::invokeMethod(this, [this] {
        if (!hayImagen_) { hayImagen_ = true; emit hayImagenChanged(); }
        update();
    }, Qt::QueuedConnection);
}

QSGNode* ImagenVideo::updatePaintNode(QSGNode* viejo, UpdatePaintNodeData*) {
    auto* nodo = static_cast<QSGSimpleTextureNode*>(viejo);
    QImage img;
    {
        QMutexLocker l(&mutex_);
        if (hayNueva_) { img = std::move(nueva_); nueva_ = QImage(); hayNueva_ = false; }
    }
    if (!img.isNull() && window()) {
        if (!nodo) {
            nodo = new QSGSimpleTextureNode;
            nodo->setOwnsTexture(true);
            nodo->setFiltering(QSGTexture::Linear);
        }
        // Con ownsTexture, setTexture borra la anterior
        nodo->setTexture(window()->createTextureFromImage(img, QQuickWindow::TextureIsOpaque));
    }
    if (!nodo || !nodo->texture()) return nodo;
    // Que entre entero, sin deformar (como PreserveAspectFit)
    const QSizeF t = nodo->texture()->textureSize();
    const qreal k = qMin(width() / t.width(), height() / t.height());
    const QSizeF s(t.width() * k, t.height() * k);
    nodo->setRect(QRectF((width() - s.width()) / 2, (height() - s.height()) / 2, s.width(), s.height()));
    return nodo;
}
