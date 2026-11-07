#include "video_gst.h"

#include "imagen_video.h"

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <QFile>
#include <QVariant>
#include <QVideoFrame>
#include <QVideoFrameFormat>

#include <cstring>

namespace {
GstFlowReturn alMuestra(GstAppSink* s, gpointer yo) {
    if (GstSample* m = gst_app_sink_pull_sample(s)) { static_cast<VideoGst*>(yo)->entregar(m); gst_sample_unref(m); }
    return GST_FLOW_OK;
}
GstFlowReturn alPreroll(GstAppSink* s, gpointer yo) {
    // El cuadro de cuando está en pausa (al abrir o al saltar a una vuelta)
    if (GstSample* m = gst_app_sink_pull_preroll(s)) { static_cast<VideoGst*>(yo)->entregar(m); gst_sample_unref(m); }
    return GST_FLOW_OK;
}
}  // namespace

VideoGst::VideoGst(QObject* padre) : QObject(padre) {
    preferirChip();
    bus_.setInterval(30);
    connect(&bus_, &QTimer::timeout, this, &VideoGst::revisarBus);
    reloj_.setInterval(100);
    connect(&reloj_, &QTimer::timeout, this, &VideoGst::leerPosicion);
}

VideoGst::~VideoGst() { cerrar(); }

void VideoGst::preferirChip() {
    static bool hecho = false;
    if (hecho) return;
    hecho = true;
    if (!gst_is_initialized()) gst_init(nullptr, nullptr);
    // El descompresor del chip de la Raspberry primero; los de procesador quedan de repuesto
    if (GstPluginFeature* f = gst_registry_find_feature(gst_registry_get(), "v4l2h264dec", GST_TYPE_ELEMENT_FACTORY)) {
        gst_plugin_feature_set_rank(f, GST_RANK_PRIMARY + 100);
        gst_object_unref(f);
    }
}

void VideoGst::setSource(const QUrl& url) {
    if (url == source_) return;
    source_ = url;
    emit sourceChanged();
    cerrar();
    if (!source_.isEmpty()) abrir();
}

void VideoGst::setVideoOutput(QObject* o) {
    if (o == salida_) return;
    salida_ = o;
    QVideoSink* s = qobject_cast<QVideoSink*>(o);
    if (!s && o) s = o->property("videoSink").value<QVideoSink*>();
    auto* imagen = qobject_cast<ImagenVideo*>(o);
    disconnect(seVa_);
    {
        QMutexLocker l(&mutexSink_);
        sink_ = s;
        imagen_ = imagen;
    }
    // Si la imagen se borra, que el hilo del video deje de mandarle (espera a que termine de entregar)
    if (imagen) seVa_ = connect(imagen, &ImagenVideo::seVa, this, [this] {
        QMutexLocker l(&mutexSink_);
        imagen_ = nullptr;
        sink_ = nullptr;
    }, Qt::DirectConnection);
    emit videoOutputChanged();
    // En pausa: que la salida nueva (por ejemplo la tele) muestre el cuadro de ahora
    if (tubo_ && listo_ && estado_ != Andando) setPosition(posicion_);
}

void VideoGst::cerrar() {
    bus_.stop();
    reloj_.stop();
    if (tubo_) {
        gst_element_set_state(tubo_, GST_STATE_NULL);
        gst_object_unref(tubo_);
        tubo_ = nullptr;
    }
    listo_ = quiereAndar_ = false;
    buscando_ = -1;
    if (posicion_ != 0) { posicion_ = 0; emit positionChanged(0); }
    if (duracion_ != 0) { duracion_ = 0; emit durationChanged(); }
    ponerEstado(Parado);
    ponerMedio(SinMedio);
}

namespace {
bool hay(const char* e) {
    GstElementFactory* f = gst_element_factory_find(e);
    if (f) gst_object_unref(f);
    return f != nullptr;
}
// La etiqueta de color que trae el video (antes de cambiarla): «1:...» es rango completo (como las fotos JPEG)
GstPadProbeReturn alCaps(GstPad*, GstPadProbeInfo* info, gpointer yo) {
    GstEvent* e = GST_PAD_PROBE_INFO_EVENT(info);
    if (e && GST_EVENT_TYPE(e) == GST_EVENT_CAPS) {
        GstCaps* c = nullptr;
        gst_event_parse_caps(e, &c);
        const GstStructure* s = c ? gst_caps_get_structure(c, 0) : nullptr;
        const char* col = s ? gst_structure_get_string(s, "colorimetry") : nullptr;
        static_cast<VideoGst*>(yo)->ponerRangoCompleto(col && (col[0] == '1' && col[1] == ':'));
    }
    return GST_PAD_PROBE_OK;
}
}  // namespace

namespace {
// La salida de sonido del HDMI (la tele), si el sistema la tiene: en la Pi 3 la placa «vc4hdmi»
// (por PulseAudio o directo). Si no hay, nullptr (queda la de siempre).
GstElement* salidaHdmi() {
    GstDeviceMonitor* m = gst_device_monitor_new();
    gst_device_monitor_add_filter(m, "Audio/Sink", nullptr);
    GstElement* r = nullptr;
    if (gst_device_monitor_start(m)) {
        GList* lista = gst_device_monitor_get_devices(m);
        for (GList* l = lista; l && !r; l = l->next) {
            GstDevice* d = GST_DEVICE(l->data);
            gchar* nombre = gst_device_get_display_name(d);
            GstStructure* p = gst_device_get_properties(d);
            gchar* props = p ? gst_structure_to_string(p) : nullptr;
            const QString texto = (QString::fromUtf8(nombre ? nombre : "") + QLatin1Char(' ') + QString::fromUtf8(props ? props : "")).toLower();
            g_free(props);
            if (texto.contains(QLatin1String("hdmi"))) {
                r = gst_device_create_element(d, nullptr);
                qWarning("Video: sonido por %s", nombre ? nombre : "HDMI");
            }
            if (p) gst_structure_free(p);
            g_free(nombre);
        }
        g_list_free_full(lista, gst_object_unref);
        gst_device_monitor_stop(m);
    }
    gst_object_unref(m);
    return r;
}
}  // namespace

void VideoGst::setSonidoTele(bool si) {
    if (si == sonidoTele_) return;
    sonidoTele_ = si;
    emit sonidoTeleChanged();
    if (!tubo_) return;
    // Cambiar la salida: se vuelve a abrir en el mismo lugar (un instante)
    const qint64 pos = posicion_;
    const bool andaba = estado_ == Andando;
    cerrar();
    abrir();
    setPosition(pos);
    if (andaba) play(); else pause();
}

void VideoGst::abrir() {
    cuadros_ = 0;
    const QString ruta = source_.toLocalFile();
    QFile a(ruta);
    if (ruta.isEmpty() || !a.open(QIODevice::ReadOnly)) {
        emit errorOccurred(1, QStringLiteral("No está el archivo del video"));
        ponerMedio(Invalido);
        return;
    }
    // ¿Tiene sonido? (sin sonido, una rama de audio vacía no dejaría arrancar). El índice está al principio.
    const QByteArray cabeza = a.read(1024 * 1024);
    a.close();
    const bool sonido = cabeza.contains("mp4a") || cabeza.contains("A_OPUS") || cabeza.contains("A_AAC") || cabeza.contains("Opus");
    // MP4 o Matroska: por lo que tiene adentro (no por el nombre)
    const bool mp4 = cabeza.mid(4, 4) == "ftyp" || (!cabeza.startsWith("\x1a\x45\xdf\xa3") && ruta.endsWith(QLatin1String(".mp4"), Qt::CaseInsensitive));
    // Descompresor: el chip de la Raspberry si está; si no, el que haya.
    // La etiqueta de color: las webcams MJPEG graban con la de las fotos (rango completo, «1:4:0:0») y el chip
    // no la acepta: se queda esperando sin decir nada (probado en la Pi de Matías, 06/11/2026). Se le pone
    // «bt601» antes del chip; los colores se arreglan después en ImagenVideo (rango completo).
    // BETO_VIDEO_DEC: otro descompresor, para probar (por ejemplo «avdec_h264», el del procesador)
    const QString pedido = qEnvironmentVariable("BETO_VIDEO_DEC");
    const QString chip = !pedido.isEmpty() ? pedido : hay("v4l2h264dec") ? QStringLiteral("v4l2h264dec") : QString();
    const QString dec = !chip.isEmpty()
        ? QStringLiteral("capssetter caps=\"video/x-h264,colorimetry=(string)bt601\" ! ") + chip
        : QStringLiteral("decodebin");
    QString desc = QStringLiteral("filesrc name=archivo ! %1 name=d d.video_0 ! queue ! h264parse name=parser ! %2 ! videoconvert ! "
                                  "appsink name=imagen")
                       .arg(mp4 ? QStringLiteral("qtdemux") : QStringLiteral("matroskademux"), dec);
    // La salida de sonido se agrega después (la de la Pi o, en modo tele, la del HDMI)
    if (sonido) desc += QStringLiteral(" d.audio_0 ! queue ! decodebin ! audioconvert ! audioresample name=antesSonido");
    GError* e = nullptr;
    tubo_ = gst_parse_launch(desc.toUtf8().constData(), &e);
    if (!tubo_ || e) {
        const QString texto = QString::fromUtf8(e ? e->message : "no se pudo armar");
        if (e) g_error_free(e);
        if (tubo_) { gst_object_unref(tubo_); tubo_ = nullptr; }
        emit errorOccurred(1, QStringLiteral("Video: ") + texto);
        ponerMedio(Invalido);
        return;
    }
    if (GstElement* antes = gst_bin_get_by_name(GST_BIN(tubo_), "antesSonido")) {
        GstElement* salida = sonidoTele_ ? salidaHdmi() : nullptr;
        if (!salida) salida = gst_element_factory_make("autoaudiosink", nullptr);
        if (salida) { gst_bin_add(GST_BIN(tubo_), salida); gst_element_link(antes, salida); }
        gst_object_unref(antes);
    }
    if (GstElement* f = gst_bin_get_by_name(GST_BIN(tubo_), "archivo")) {
        g_object_set(f, "location", QFile::encodeName(ruta).constData(), nullptr);
        gst_object_unref(f);
    }
    if (GstElement* p = gst_bin_get_by_name(GST_BIN(tubo_), "parser")) {
        GstPad* sal = gst_element_get_static_pad(p, "src");
        gst_pad_add_probe(sal, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, alCaps, this, nullptr);
        gst_object_unref(sal);
        gst_object_unref(p);
    }
    if (GstElement* imagen = gst_bin_get_by_name(GST_BIN(tubo_), "imagen")) {
        // La imagen en YUV (lo que da el chip); se pasa a colores en ImagenVideo
        GstCaps* caps = gst_caps_from_string("video/x-raw,format=(string){I420,NV12}");
        // Sin «last-sample»: el cuadro mostrado ya lo guarda ImagenVideo (uno menos retenido del chip)
        g_object_set(imagen, "caps", caps, "max-buffers", 2, "drop", TRUE, "sync", TRUE, "enable-last-sample", FALSE, nullptr);
        gst_caps_unref(caps);
        GstAppSinkCallbacks cb{};
        cb.new_sample = alMuestra;
        cb.new_preroll = alPreroll;
        gst_app_sink_set_callbacks(GST_APP_SINK(imagen), &cb, this, nullptr);
        gst_object_unref(imagen);
    }
    ponerDescompresor(chip.isEmpty() ? QStringLiteral("procesador") : chip);
    ponerMedio(Cargando);
    bus_.start();
    if (gst_element_set_state(tubo_, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        emit errorOccurred(1, QStringLiteral("No se pudo abrir el video"));
        ponerMedio(Invalido);
    }
}

void VideoGst::ponerDescompresor(const QString& nombre) {
    if (descompresor_ == nombre) return;
    descompresor_ = nombre;
    qWarning("Video: descompresor %s", qPrintable(nombre));   // warning: así queda en el registro de la Pi
    emit descompresorChanged();
}

void VideoGst::play() {
    quiereAndar_ = true;
    if (!tubo_) return;
    if (medio_ == Terminado) { setPosition(0); ponerMedio(Cargado); }
    if (listo_) gst_element_set_state(tubo_, GST_STATE_PLAYING);
    ponerEstado(Andando);
    reloj_.start();
}

void VideoGst::pause() {
    quiereAndar_ = false;
    if (!tubo_) return;
    gst_element_set_state(tubo_, GST_STATE_PAUSED);
    reloj_.stop();
    leerPosicion();
    ponerEstado(Pausa);
}

void VideoGst::stop() {
    quiereAndar_ = false;
    if (!tubo_) return;
    gst_element_set_state(tubo_, GST_STATE_PAUSED);
    reloj_.stop();
    ponerEstado(Parado);
}

void VideoGst::setPosition(qint64 ms) {
    ms = qMax<qint64>(0, ms);
    if (duracion_ > 0) ms = qMin(ms, duracion_);
    if (!tubo_) return;
    if (!listo_) { buscando_ = ms; return; }   // al terminar de cargar
    // Exacto (no al cuadro clave anterior): el chip descomprime rápido
    gst_element_seek_simple(tubo_, GST_FORMAT_TIME, GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), ms * GST_MSECOND);
    if (medio_ == Terminado) ponerMedio(Cargado);
    if (posicion_ != ms) { posicion_ = ms; emit positionChanged(ms); }
}

void VideoGst::leerPosicion() {
    if (!tubo_ || !listo_) return;
    gint64 p = 0;
    if (gst_element_query_position(tubo_, GST_FORMAT_TIME, &p) && p >= 0) {
        const qint64 ms = p / GST_MSECOND;
        if (ms != posicion_) { posicion_ = ms; emit positionChanged(ms); }
    }
}

void VideoGst::revisarBus() {
    if (!tubo_) return;
    GstBus* bus = gst_element_get_bus(tubo_);
    while (GstMessage* m = gst_bus_pop(bus)) {
        switch (GST_MESSAGE_TYPE(m)) {
            case GST_MESSAGE_ASYNC_DONE:
                if (!listo_) {
                    listo_ = true;
                    gint64 d = 0;
                    if (gst_element_query_duration(tubo_, GST_FORMAT_TIME, &d) && d > 0) { duracion_ = d / GST_MSECOND; emit durationChanged(); }
                    ponerMedio(Cargado);
                    if (buscando_ >= 0) { const qint64 b = buscando_; buscando_ = -1; setPosition(b); }
                    if (quiereAndar_) gst_element_set_state(tubo_, GST_STATE_PLAYING);
                }
                break;
            case GST_MESSAGE_DURATION_CHANGED: {
                gint64 d = 0;
                if (gst_element_query_duration(tubo_, GST_FORMAT_TIME, &d) && d > 0 && d / GST_MSECOND != duracion_) {
                    duracion_ = d / GST_MSECOND;
                    emit durationChanged();
                }
                break;
            }
            case GST_MESSAGE_EOS:
                reloj_.stop();
                quiereAndar_ = false;
                gst_element_set_state(tubo_, GST_STATE_PAUSED);   // queda el último cuadro
                if (duracion_ > 0 && posicion_ != duracion_) { posicion_ = duracion_; emit positionChanged(posicion_); }
                ponerMedio(Terminado);
                ponerEstado(Parado);
                break;
            case GST_MESSAGE_ERROR: {
                GError* e = nullptr;
                gst_message_parse_error(m, &e, nullptr);
                const QString texto = QString::fromUtf8(e ? e->message : "error");
                if (e) g_error_free(e);
                qWarning("Video: %s", qPrintable(texto));
                emit errorOccurred(1, texto);
                ponerMedio(Invalido);
                ponerEstado(Parado);
                break;
            }
            default:
                break;
        }
        gst_message_unref(m);
        if (!tubo_) break;
    }
    gst_object_unref(bus);
}

void VideoGst::ponerEstado(int e) {
    if (e == estado_) return;
    estado_ = e;
    emit playbackStateChanged();
}

void VideoGst::ponerMedio(int m) {
    if (m == medio_) return;
    medio_ = m;
    emit mediaStatusChanged();
}

void VideoGst::entregar(GstSample* muestra) {
    ++cuadros_;
    QMutexLocker l(&mutexSink_);
    if (!imagen_ && !sink_) return;
    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, gst_sample_get_caps(muestra))) return;
    const bool nv12 = GST_VIDEO_INFO_FORMAT(&info) == GST_VIDEO_FORMAT_NV12;
    if (!nv12 && GST_VIDEO_INFO_FORMAT(&info) != GST_VIDEO_FORMAT_I420) return;
    const int w = GST_VIDEO_INFO_WIDTH(&info), h = GST_VIDEO_INFO_HEIGHT(&info);
    if (imagen_) {
        // Directo, sin copiar: el cuadro queda abierto (y con su referencia) hasta que ImagenVideo lo suelta
        auto* g = new GstVideoFrame;
        if (!gst_video_frame_map(g, &info, gst_sample_get_buffer(muestra), GST_MAP_READ)) { delete g; return; }
        auto c = std::make_shared<CuadroYuv>();
        c->ancho = w;
        c->alto = h;
        c->formato = nv12 ? CuadroYuv::NV12 : CuadroYuv::I420;
        c->completo = rangoCompleto_;
        for (int p = 0; p < (nv12 ? 2 : 3); ++p) {
            c->plano[p] = static_cast<const uchar*>(GST_VIDEO_FRAME_PLANE_DATA(g, p));
            c->paso[p] = GST_VIDEO_FRAME_PLANE_STRIDE(g, p);
        }
        c->soltar = [g] { gst_video_frame_unmap(g); delete g; };
        imagen_->recibir(std::move(c));
        return;
    }
    // Otra salida (un VideoOutput, por ejemplo): copia a un QVideoFrame
    GstVideoFrame g;
    if (!gst_video_frame_map(&g, &info, gst_sample_get_buffer(muestra), GST_MAP_READ)) return;
    QVideoFrameFormat formato(QSize(w, h), nv12 ? QVideoFrameFormat::Format_NV12 : QVideoFrameFormat::Format_YUV420P);
    formato.setColorRange(rangoCompleto_ ? QVideoFrameFormat::ColorRange_Full : QVideoFrameFormat::ColorRange_Video);
    QVideoFrame cuadro(formato);
    if (cuadro.map(QVideoFrame::WriteOnly)) {
        const int planos = nv12 ? 2 : 3;
        for (int p = 0; p < planos && p < cuadro.planeCount(); ++p) {
            const int filas = p == 0 ? h : (h + 1) / 2;
            const int bytes = p == 0 || nv12 ? w : (w + 1) / 2;
            const uchar* o = static_cast<const uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&g, p));
            const int so = GST_VIDEO_FRAME_PLANE_STRIDE(&g, p);
            uchar* d = cuadro.bits(p);
            const int sd = cuadro.bytesPerLine(p);
            for (int f = 0; f < filas; ++f) std::memcpy(d + f * sd, o + f * so, size_t(qMin(bytes, qMin(so, sd))));
        }
        cuadro.unmap();
        const GstClockTime t = GST_BUFFER_PTS(gst_sample_get_buffer(muestra));
        if (GST_CLOCK_TIME_IS_VALID(t)) cuadro.setStartTime(qint64(t / 1000));
        sink_->setVideoFrame(cuadro);
    }
    gst_video_frame_unmap(&g);
}
