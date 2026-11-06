#include "video_gst.h"

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video.h>

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
void alElemento(GstElement*, GstElement* e, gpointer yo) {
    // Qué descompresor eligió (para saber si usa el chip). Llega en otro hilo.
    GstElementFactory* f = gst_element_get_factory(e);
    if (!f) return;
    const char* clase = gst_element_factory_get_metadata(f, GST_ELEMENT_METADATA_KLASS);
    if (!clase || !strstr(clase, "Decoder") || !strstr(clase, "Video")) return;
    const QString nombre = QString::fromUtf8(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(f)));
    auto* v = static_cast<VideoGst*>(yo);
    QMetaObject::invokeMethod(v, [v, nombre] { v->ponerDescompresor(nombre); }, Qt::QueuedConnection);
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
    {
        QMutexLocker l(&mutexSink_);
        sink_ = s;
    }
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

void VideoGst::abrir() {
    tubo_ = gst_element_factory_make("playbin", nullptr);
    GstElement* imagen = gst_element_factory_make("appsink", "imagen");
    if (!tubo_ || !imagen) {
        if (imagen) gst_object_unref(imagen);
        emit errorOccurred(1, QStringLiteral("Falta GStreamer (playbin o appsink)"));
        ponerMedio(Invalido);
        return;
    }
    // La imagen en YUV (lo que da el chip); se pasa a colores en ImagenVideo
    GstCaps* caps = gst_caps_from_string("video/x-raw,format=(string){I420,NV12}");
    g_object_set(imagen, "caps", caps, "max-buffers", 2, "drop", TRUE, "sync", TRUE, nullptr);
    gst_caps_unref(caps);
    GstAppSinkCallbacks cb{};
    cb.new_sample = alMuestra;
    cb.new_preroll = alPreroll;
    gst_app_sink_set_callbacks(GST_APP_SINK(imagen), &cb, this, nullptr);
    g_object_set(tubo_, "uri", source_.toString(QUrl::FullyEncoded).toUtf8().constData(), "video-sink", imagen, nullptr);
    // Sin subtítulos ni visualizaciones
    g_object_set(tubo_, "flags", 0x1 | 0x2 | 0x10, nullptr);   // video | audio | soft-volume
    g_signal_connect(tubo_, "element-setup", G_CALLBACK(alElemento), this);
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
    qInfo("Video: descompresor %s", qPrintable(nombre));
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
    QVideoSink* sink;
    {
        QMutexLocker l(&mutexSink_);
        sink = sink_;
    }
    if (!sink) return;
    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, gst_sample_get_caps(muestra))) return;
    const bool nv12 = GST_VIDEO_INFO_FORMAT(&info) == GST_VIDEO_FORMAT_NV12;
    if (!nv12 && GST_VIDEO_INFO_FORMAT(&info) != GST_VIDEO_FORMAT_I420) return;
    GstVideoFrame g;
    if (!gst_video_frame_map(&g, &info, gst_sample_get_buffer(muestra), GST_MAP_READ)) return;
    const int w = GST_VIDEO_INFO_WIDTH(&info), h = GST_VIDEO_INFO_HEIGHT(&info);
    QVideoFrame cuadro(QVideoFrameFormat(QSize(w, h), nv12 ? QVideoFrameFormat::Format_NV12 : QVideoFrameFormat::Format_YUV420P));
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
        sink->setVideoFrame(cuadro);
    }
    gst_video_frame_unmap(&g);
}
