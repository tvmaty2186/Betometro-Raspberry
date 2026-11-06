#include "imagen_video.h"

#include <QFile>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>
#include <QSGTexture>

#ifdef BETO_VIDEO_CHIP
#include <rhi/qrhi.h>
#endif

#include <algorithm>
#include <cstring>

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

// Por el procesador. `paso`: 1 entero, 2 a la mitad, 4 a la cuarta parte
template <bool completo>
QImage convertirCon(const CuadroYuv& c, int paso) {
    const int w = c.ancho, h = c.alto;
    QImage img(w / paso, h / paso, QImage::Format_RGB32);
    if (img.isNull()) return img;
    if (c.formato == CuadroYuv::I420) {
        for (int fy = 0; fy < img.height(); ++fy) {
            const int y = fy * paso;
            const uchar* fY = c.plano[0] + y * c.paso[0];
            const uchar* fU = c.plano[1] + (y / 2) * c.paso[1];
            const uchar* fV = c.plano[2] + (y / 2) * c.paso[2];
            quint32* o = reinterpret_cast<quint32*>(img.scanLine(fy));
            for (int fx = 0; fx < img.width(); ++fx) {
                const int x = fx * paso;
                o[fx] = rgb<completo>(fY[x], fU[x / 2], fV[x / 2]);
            }
        }
    } else {
        const bool nv21 = c.formato == CuadroYuv::NV21;
        for (int fy = 0; fy < img.height(); ++fy) {
            const int y = fy * paso;
            const uchar* fY = c.plano[0] + y * c.paso[0];
            const uchar* fUV = c.plano[1] + (y / 2) * c.paso[1];
            quint32* o = reinterpret_cast<quint32*>(img.scanLine(fy));
            for (int fx = 0; fx < img.width(); ++fx) {
                const int x = fx * paso, k = (x / 2) * 2;
                const int u = fUV[k + (nv21 ? 1 : 0)], v = fUV[k + (nv21 ? 0 : 1)];
                o[fx] = rgb<completo>(fY[x], u, v);
            }
        }
    }
    return img;
}

// Arma el cuadro (sin copiar) de un QVideoFrame: lo deja abierto hasta que se suelta
CuadroPtr desdeFrame(const QVideoFrame& original) {
    auto c = std::make_shared<CuadroYuv>();
    c->ancho = original.width();
    c->alto = original.height();
    c->completo = original.surfaceFormat().colorRange() == QVideoFrameFormat::ColorRange_Full;
    const auto fmt = original.pixelFormat();
    const bool i420 = fmt == QVideoFrameFormat::Format_YUV420P || fmt == QVideoFrameFormat::Format_YV12;
    const bool nv = fmt == QVideoFrameFormat::Format_NV12 || fmt == QVideoFrameFormat::Format_NV21;
    if (!i420 && !nv) {
        c->otro = original;
        return c;
    }
    QVideoFrame f(original);
    if (!f.map(QVideoFrame::ReadOnly)) return nullptr;
    if (i420) {
        const bool yv12 = fmt == QVideoFrameFormat::Format_YV12;
        c->formato = CuadroYuv::I420;
        for (int p = 0; p < 3; ++p) {
            const int q = yv12 && p > 0 ? 3 - p : p;   // YV12: V antes que U
            c->plano[p] = f.bits(q);
            c->paso[p] = f.bytesPerLine(q);
        }
    } else {
        c->formato = fmt == QVideoFrameFormat::Format_NV21 ? CuadroYuv::NV21 : CuadroYuv::NV12;
        for (int p = 0; p < 2; ++p) { c->plano[p] = f.bits(p); c->paso[p] = f.bytesPerLine(p); }
    }
    c->soltar = [f]() mutable { f.unmap(); };
    return c;
}

// Que entre entero, sin deformar (como PreserveAspectFit)
QRectF encajar(const QSizeF& video, qreal ancho, qreal alto) {
    if (video.isEmpty()) return {};
    const qreal k = qMin(ancho / video.width(), alto / video.height());
    const QSizeF s(video.width() * k, video.height() * k);
    return QRectF((ancho - s.width()) / 2, (alto - s.height()) / 2, s.width(), s.height());
}

#ifdef BETO_VIDEO_CHIP
// ---------- Por el chip gráfico ----------
// Los programas, armados por qsb (CMake, qt_add_shaders) con variante GLSL ES 100 para la Pi 3
const QString kVert = QStringLiteral(":/shaders/yuv.vert.qsb");
const QString kFrag = QStringLiteral(":/shaders/yuv.frag.qsb");

// Un plano del video (un canal: RED_OR_ALPHA8 = GL_ALPHA en GLES 2, que la Pi 3 sí tiene)
class TexturaPlano : public QSGTexture {
public:
    TexturaPlano() {
        setFiltering(QSGTexture::Linear);
        setMipmapFiltering(QSGTexture::None);
        setHorizontalWrapMode(QSGTexture::ClampToEdge);
        setVerticalWrapMode(QSGTexture::ClampToEdge);
    }
    ~TexturaPlano() override { if (tex_) tex_->deleteLater(); }
    qint64 comparisonKey() const override { return qint64(quintptr(this)); }
    QRhiTexture* rhiTexture() const override { return tex_; }
    QSize textureSize() const override { return tam_; }
    bool hasAlphaChannel() const override { return false; }
    bool hasMipmaps() const override { return false; }

    // Los datos tienen que seguir vivos hasta que se dibuje (los guarda el nodo, en `actual`)
    void poner(const uchar* datos, int ancho, int alto, int paso) {
        datos_ = datos;
        tam_ = QSize(ancho, alto);
        paso_ = paso;
        sucio_ = true;
    }

    void commitTextureOperations(QRhi* rhi, QRhiResourceUpdateBatch* cambios) override {
        if (!sucio_ || !datos_ || tam_.isEmpty()) return;
        sucio_ = false;
        if (!tex_ || tex_->pixelSize() != tam_) {
            if (tex_) tex_->deleteLater();
            tex_ = rhi->newTexture(QRhiTexture::RED_OR_ALPHA8, tam_);
            if (!tex_->create()) { delete tex_; tex_ = nullptr; return; }
        }
        const int w = tam_.width(), h = tam_.height();
        QRhiTextureSubresourceUploadDescription sub;
        if (paso_ == w) {
            sub.setData(QByteArray::fromRawData(reinterpret_cast<const char*>(datos_), w * h));
        } else if (rhi->isFeatureSupported(QRhi::ImageDataStride)) {
            sub.setData(QByteArray::fromRawData(reinterpret_cast<const char*>(datos_), paso_ * (h - 1) + w));
            sub.setDataStride(quint32(paso_));
        } else {
            // GLES 2 no sabe saltar el relleno de cada fila: se copia justo (raro: anchos que no son múltiplo de 4)
            copia_.resize(w * h);
            for (int f = 0; f < h; ++f) std::memcpy(copia_.data() + f * w, datos_ + f * paso_, size_t(w));
            sub.setData(QByteArray::fromRawData(copia_.constData(), copia_.size()));
        }
        cambios->uploadTexture(tex_, QRhiTextureUploadEntry(0, 0, sub));
    }

private:
    QRhiTexture* tex_ = nullptr;
    QSize tam_;
    const uchar* datos_ = nullptr;
    int paso_ = 0;
    bool sucio_ = false;
    QByteArray copia_;
};

class MaterialYuv : public QSGMaterial {
public:
    QSGMaterialType* type() const override { static QSGMaterialType t; return &t; }
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    int compare(const QSGMaterial* otro) const override { return this == otro ? 0 : (this < otro ? -1 : 1); }
    TexturaPlano plano[3];
    float completo = 0, rojo = 0;
};

class ShaderYuv : public QSGMaterialShader {
public:
    ShaderYuv() {
        setShaderFileName(VertexStage, kVert);
        setShaderFileName(FragmentStage, kFrag);
    }
    bool updateUniformData(RenderState& estado, QSGMaterial* nuevo, QSGMaterial*) override {
        // Como en el programa: matriz (64), opacidad, completo, rojo
        QByteArray* b = estado.uniformData();
        if (estado.isMatrixDirty()) {
            const QMatrix4x4 m = estado.combinedMatrix();
            std::memcpy(b->data(), m.constData(), 64);
        }
        if (estado.isOpacityDirty()) {
            const float o = estado.opacity();
            std::memcpy(b->data() + 64, &o, 4);
        }
        const auto* m = static_cast<MaterialYuv*>(nuevo);
        std::memcpy(b->data() + 68, &m->completo, 4);
        std::memcpy(b->data() + 72, &m->rojo, 4);
        return true;
    }
    void updateSampledImage(RenderState&, int binding, QSGTexture** t, QSGMaterial* nuevo, QSGMaterial*) override {
        if (binding >= 1 && binding <= 3) *t = &static_cast<MaterialYuv*>(nuevo)->plano[binding - 1];
    }
};

QSGMaterialShader* MaterialYuv::createShader(QSGRendererInterface::RenderMode) const { return new ShaderYuv; }

class NodoYuv : public QSGGeometryNode {
public:
    explicit NodoYuv(bool rojo) : geo_(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4) {
        geo_.setDrawingMode(QSGGeometry::DrawTriangleStrip);
        setGeometry(&geo_);
        mat_.rojo = rojo ? 1.f : 0.f;
        setMaterial(&mat_);
    }
    void poner(CuadroPtr c) {
        const int w = c->ancho, h = c->alto, cw = (w + 1) / 2, ch = (h + 1) / 2;
        mat_.plano[0].poner(c->plano[0], w, h, c->paso[0]);
        if (c->formato == CuadroYuv::I420) {
            mat_.plano[1].poner(c->plano[1], cw, ch, c->paso[1]);
            mat_.plano[2].poner(c->plano[2], cw, ch, c->paso[2]);
        } else {
            // NV12/NV21: se separan U y V (dos canales no es seguro en GLES 2). Con el chip de la Pi llega I420.
            u_.resize(cw * ch);
            v_.resize(cw * ch);
            const bool nv21 = c->formato == CuadroYuv::NV21;
            for (int f = 0; f < ch; ++f) {
                const uchar* o = c->plano[1] + f * c->paso[1];
                uchar* du = reinterpret_cast<uchar*>(u_.data()) + f * cw;
                uchar* dv = reinterpret_cast<uchar*>(v_.data()) + f * cw;
                for (int x = 0; x < cw; ++x) {
                    du[x] = o[2 * x + (nv21 ? 1 : 0)];
                    dv[x] = o[2 * x + (nv21 ? 0 : 1)];
                }
            }
            mat_.plano[1].poner(reinterpret_cast<const uchar*>(u_.constData()), cw, ch, cw);
            mat_.plano[2].poner(reinterpret_cast<const uchar*>(v_.constData()), cw, ch, cw);
        }
        mat_.completo = c->completo ? 1.f : 0.f;
        actual_ = std::move(c);   // suelta el anterior: ya se dibujó (estamos en la sincronización siguiente)
        markDirty(QSGNode::DirtyMaterial);
    }
    void ponerRect(const QRectF& r) {
        if (r == rect_) return;
        rect_ = r;
        QSGGeometry::updateTexturedRectGeometry(&geo_, r, QRectF(0, 0, 1, 1));
        markDirty(QSGNode::DirtyGeometry);
    }

private:
    QSGGeometry geo_;
    MaterialYuv mat_;
    CuadroPtr actual_;
    QByteArray u_, v_;
    QRectF rect_;
};

// ¿El chip gráfico arma nuestro programa? Se prueba de verdad (en GLES 2 compila y une los programas):
// si no, la imagen va por el procesador en vez de quedar negra.
bool probarPrograma(QRhi* rhi) {
    auto cargar = [](const QString& n) {
        QFile f(n);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    };
    const QShader vs = cargar(kVert), fs = cargar(kFrag);
    if (!vs.isValid() || !fs.isValid()) return false;
    if (!rhi->isTextureFormatSupported(QRhiTexture::RED_OR_ALPHA8)) return false;
    std::unique_ptr<QRhiTexture> destino(rhi->newTexture(QRhiTexture::RGBA8, QSize(16, 16), 1, QRhiTexture::RenderTarget));
    if (!destino->create()) return false;
    std::unique_ptr<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({destino.get()}));
    std::unique_ptr<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rp.get());
    if (!rt->create()) return false;
    std::unique_ptr<QRhiBuffer> ub(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 80));
    if (!ub->create()) return false;
    std::unique_ptr<QRhiSampler> smp(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    if (!smp->create()) return false;
    std::unique_ptr<QRhiTexture> t[3];
    for (auto& x : t) {
        x.reset(rhi->newTexture(QRhiTexture::RED_OR_ALPHA8, QSize(2, 2)));
        if (!x->create()) return false;
    }
    const auto ambos = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    std::unique_ptr<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, ambos, ub.get()),
                      QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, t[0].get(), smp.get()),
                      QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, t[1].get(), smp.get()),
                      QRhiShaderResourceBinding::sampledTexture(3, QRhiShaderResourceBinding::FragmentStage, t[2].get(), smp.get())});
    if (!srb->create()) return false;
    std::unique_ptr<QRhiGraphicsPipeline> ps(rhi->newGraphicsPipeline());
    ps->setShaderStages({{QRhiShaderStage::Vertex, vs}, {QRhiShaderStage::Fragment, fs}});
    QRhiVertexInputLayout capas;
    capas.setBindings({{4 * sizeof(float)}});
    capas.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}, {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)}});
    ps->setVertexInputLayout(capas);
    ps->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    ps->setShaderResourceBindings(srb.get());
    ps->setRenderPassDescriptor(rp.get());
    return ps->create();
}
#endif  // BETO_VIDEO_CHIP

}  // namespace

ImagenVideo::ImagenVideo(QQuickItem* padre) : QQuickItem(padre), sink_(new QVideoSink(this)) {
    setFlag(ItemHasContents, true);
    // El cuadro llega en el hilo del video: se guarda y listo (no traba ni al video ni a la pantalla)
    connect(sink_, &QVideoSink::videoFrameChanged, this, &ImagenVideo::alCuadro, Qt::DirectConnection);
    obrero_ = std::thread([this] { trabajar(); });
}

ImagenVideo::~ImagenVideo() {
    emit seVa();
    // Que no llegue un cuadro mientras se borra
    disconnect(sink_, nullptr, this, nullptr);
    {
        std::lock_guard<std::mutex> l(mutex_);
        fin_ = true;
    }
    hayTrabajo_.notify_all();
    obrero_.join();
}

void ImagenVideo::geometryChange(const QRectF& nueva, const QRectF& vieja) {
    QQuickItem::geometryChange(nueva, vieja);
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    anchoLugar_ = qMax(1, qRound(nueva.width() * dpr));
    update();
}

QImage ImagenVideo::convertir(const QVideoFrame& original, int paso) {
    if (!original.isValid()) return {};
    const CuadroPtr c = desdeFrame(original);
    return c ? convertir(*c, paso) : QImage();
}

QImage ImagenVideo::convertir(const CuadroYuv& c, int paso) {
    paso = qMax(1, paso);
    if (c.formato == CuadroYuv::Otro) {
        // Otros formatos (raros acá): la conversión de Qt
        QImage img = c.otro.toImage();
        if (paso > 1 && !img.isNull()) img = img.scaled(img.width() / paso, img.height() / paso);
        return img;
    }
    if (c.ancho < paso || c.alto < paso) return {};
    return c.completo ? convertirCon<true>(c, paso) : convertirCon<false>(c, paso);
}

bool ImagenVideo::sirveChip(const CuadroYuv& c) const {
    const int max = maxTextura_;
    return c.formato != CuadroYuv::Otro && c.ancho >= 2 && c.alto >= 2 && c.ancho <= max && c.alto <= max;
}

void ImagenVideo::alCuadro(const QVideoFrame& cuadro) {
    if (!cuadro.isValid()) return;
    recibir(desdeFrame(cuadro));
}

void ImagenVideo::recibir(CuadroPtr cuadro) {
    if (!cuadro || cuadro->ancho <= 0 || cuadro->alto <= 0) return;
    CuadroPtr viejo;   // se suelta afuera del candado
    bool alChip;
    {
        std::lock_guard<std::mutex> l(mutex_);
        cuadro->numero = ++numero_;
        const int m = modo_;
        alChip = m == SinSaber || (m == Chip && sirveChip(*cuadro));
        CuadroPtr& lugar = alChip ? paraChip_ : paraProcesador_;
        viejo = std::move(lugar);
        lugar = std::move(cuadro);
    }
    if (alChip) avisar();
    else hayTrabajo_.notify_one();
}

void ImagenVideo::avisar() {
    if (avisando_.exchange(true)) return;   // ya hay un aviso en camino
    QMetaObject::invokeMethod(this, [this] {
        avisando_ = false;
        if (!hayImagen_) { hayImagen_ = true; emit hayImagenChanged(); }
        update();
    }, Qt::QueuedConnection);
}

void ImagenVideo::trabajar() {
    std::unique_lock<std::mutex> l(mutex_);
    for (;;) {
        hayTrabajo_.wait(l, [this] { return fin_ || paraProcesador_; });
        if (fin_) return;
        CuadroPtr c = std::move(paraProcesador_);
        l.unlock();
        // A la mitad si el video es al menos el doble de ancho que el lugar donde se muestra
        int paso = c->ancho >= 2 * anchoLugar_ ? 2 : 1;
        const int max = qMax(64, int(maxTextura_));
        while (c->ancho / paso > max || c->alto / paso > max) paso *= 2;
        QImage img = convertir(*c, paso);
        const quint64 n = c->numero;
        c.reset();   // devuelve el cuadro al descompresor
        l.lock();
        if (img.isNull() || n <= numeroNueva_) continue;
        nueva_ = std::move(img);
        numeroNueva_ = n;
        l.unlock();
        avisar();
        l.lock();
    }
}

int ImagenVideo::decidirModo() {
    if (modo_ != SinSaber) return modo_;
    int m = Procesador;
    const char* por = "procesador";
#ifdef BETO_VIDEO_CHIP
    QQuickWindow* v = window();
    QSGRendererInterface* ri = v ? v->rendererInterface() : nullptr;
    if (qEnvironmentVariableIntValue("BETO_VIDEO_CPU") == 1) {
        por = "procesador (BETO_VIDEO_CPU)";
    } else if (ri && QSGRendererInterface::isApiRhiBased(ri->graphicsApi())) {
        auto* rhi = static_cast<QRhi*>(ri->getResource(v, QSGRendererInterface::RhiResource));
        if (rhi && probarPrograma(rhi)) {
            m = Chip;
            maxTextura_ = rhi->resourceLimit(QRhi::TextureSizeMax);
            rojo_ = rhi->isFeatureSupported(QRhi::RedOrAlpha8IsRed);
            por = "chip gráfico";
        } else {
            por = "procesador (el chip gráfico no armó el programa)";
        }
    }
#endif
    qInfo("Video: colores por el %s", por);
    std::lock_guard<std::mutex> l(mutex_);
    modo_ = m;
    // Los que llegaron antes de saber: al camino que corresponde
    if (paraChip_ && (m == Procesador || !sirveChip(*paraChip_))) {
        paraProcesador_ = std::move(paraChip_);
        hayTrabajo_.notify_one();
    }
    return m;
}

QSGNode* ImagenVideo::updatePaintNode(QSGNode* viejo, UpdatePaintNodeData*) {
    if (!window()) return viejo;
    decidirModo();
    CuadroPtr chip;
    QImage img;
    quint64 nImg = 0;
    {
        std::lock_guard<std::mutex> l(mutex_);
        chip = std::move(paraChip_);
        if (!nueva_.isNull()) { img = std::move(nueva_); nueva_ = QImage(); nImg = numeroNueva_; }
    }
    // Gana el más nuevo; los viejos no se muestran
    if (chip && chip->numero < numeroMostrado_) chip.reset();
    if (!img.isNull() && nImg < numeroMostrado_) img = QImage();
    if (chip && !img.isNull()) {
        if (nImg > chip->numero) chip.reset();
        else img = QImage();
    }

    QSGNode* nodo = viejo;
#ifdef BETO_VIDEO_CHIP
    if (chip) {
        auto* n = dynamic_cast<NodoYuv*>(viejo);
        if (!n) { delete viejo; n = new NodoYuv(rojo_); }
        numeroMostrado_ = chip->numero;
        tamVideo_ = QSizeF(chip->ancho, chip->alto);
        n->poner(std::move(chip));
        nodo = n;
    }
#endif
    if (!img.isNull()) {
        auto* n = dynamic_cast<QSGSimpleTextureNode*>(viejo);
        if (!n) {
            delete viejo;
            n = new QSGSimpleTextureNode;
            n->setOwnsTexture(true);
            n->setFiltering(QSGTexture::Linear);
        }
        // Con ownsTexture, setTexture borra la anterior
        n->setTexture(window()->createTextureFromImage(img, QQuickWindow::TextureIsOpaque));
        numeroMostrado_ = nImg;
        tamVideo_ = img.size();
        nodo = n;
    }
    if (!nodo) return nullptr;
    const QRectF r = encajar(tamVideo_, width(), height());
#ifdef BETO_VIDEO_CHIP
    if (auto* n = dynamic_cast<NodoYuv*>(nodo)) { n->ponerRect(r); return nodo; }
#endif
    if (auto* n = static_cast<QSGSimpleTextureNode*>(nodo)) n->setRect(r);
    return nodo;
}
