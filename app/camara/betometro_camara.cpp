// betometro-camara: el ayudante de video y sonido de Betómetro (GStreamer).
// Corre aparte y con prioridad baja: si algo se traba acá, la carrera sigue igual.
// Habla con Betómetro por la salida estándar, una línea JSON por evento.
//
//   grabar   --video /dev/video0 --ancho 1280 --alto 720 [--escala 848x480] [--audio hw:CARD=X|no] --salida f.mkv
//            La cámara ya entrega H.264: se guarda tal cual (no se vuelve a comprimir).
//            Avisa {"primerCuadro": ms} con la hora monotónica del primer cuadro (para sincronizar).
//            Termina ordenado con SIGINT/SIGTERM (cierra el archivo).
//   vista    --video /dev/video0 --salida /run/betometro/vista.jpg
//            Foto de la cámara 5 veces por segundo (MJPEG de la cámara, sin descomprimir).
//   nivel    --audio hw:CARD=X       Nivel del micrófono: {"nivel": 0..1} 10 veces por segundo.
//   probar   --audio hw:CARD=X --segundos 5 --salida f.wav    Graba unos segundos de sonido.
//   mp4      --entrada f.mkv --salida f.mp4 [--sonido si|no]
//            Pasa el video a MP4 (lo abren todos los teléfonos, también el iPhone). La imagen no se vuelve
//            a comprimir (es rápido); el sonido pasa de Opus a AAC.
//   --prueba: en la PC, sin cámara: imagen y sonido de prueba.
// El archivo de video es Matroska «para transmitir»: si se corta la luz, lo grabado se puede ver igual.
#include <glib-unix.h>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <unistd.h>

namespace {

GMainLoop* bucle = nullptr;
GstElement* tubo = nullptr;
bool primerVisto = false;
// Cuadros de la webcam (MJPEG): cuántos llegaron y cuántos llegaron cortados (se saltean, no se graban rotos)
long cuadrosTotal = 0, cuadrosMalos = 0, avisosJpeg = 0;
std::string formatoGrabar;
gboolean alContar(gpointer);   // más abajo
std::string salidaVista;
int segundosProbar = 0;

double monoMs() {
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

void decir(const std::string& json) {
    std::printf("%s\n", json.c_str());
    std::fflush(stdout);
}

std::string escapar(const char* s) {
    std::string r;
    for (; s && *s; ++s) {
        if (*s == '"' || *s == '\\') r += '\\';
        if (*s == '\n') { r += "\\n"; continue; }
        r += *s;
    }
    return r;
}

gboolean alSenal(gpointer) {
    // Cerrar ordenado: fin de flujo, así el archivo queda completo
    if (tubo) gst_element_send_event(tubo, gst_event_new_eos());
    // Si la cámara está trabada, el fin de flujo no llega nunca: a los 8 s se corta igual (el .mkv sirve) (02-1.2)
    g_timeout_add_seconds(8, +[](gpointer) -> gboolean {
        decir("{\"error\":\"La cámara no respondió al cerrar\"}");
        g_main_loop_quit(bucle);
        return G_SOURCE_REMOVE;
    }, nullptr);
    return G_SOURCE_REMOVE;
}

gboolean alBus(GstBus*, GstMessage* m, gpointer) {
    switch (GST_MESSAGE_TYPE(m)) {
        case GST_MESSAGE_EOS:
            if (cuadrosTotal) alContar(nullptr);
            decir("{\"fin\":true}");
            g_main_loop_quit(bucle);
            break;
        case GST_MESSAGE_ERROR: {
            GError* e = nullptr;
            gst_message_parse_error(m, &e, nullptr);
            decir(std::string("{\"error\":\"") + escapar(e ? e->message : "?") + "\"}");
            if (e) g_error_free(e);
            g_main_loop_quit(bucle);
            break;
        }
        case GST_MESSAGE_WARNING: {
            // jpegdec avisa cuando una foto viene dañada por dentro (aunque tenga principio y fin)
            ++avisosJpeg;
            break;
        }
        case GST_MESSAGE_ELEMENT: {
            const GstStructure* s = gst_message_get_structure(m);
            if (s && gst_structure_has_name(s, "level")) {
                // pico del primer canal, en dB → 0..1 (−60 dB = 0)
                const GValue* picos = gst_structure_get_value(s, "peak");
                if (picos && G_VALUE_HOLDS_BOXED(picos)) {
                    const GValueArray* a = static_cast<const GValueArray*>(g_value_get_boxed(picos));
                    if (a && a->n_values > 0) {
                        const double db = g_value_get_double(g_value_array_get_nth(const_cast<GValueArray*>(a), 0));
                        const double n = std::isfinite(db) ? std::fmax(0.0, std::fmin(1.0, (db + 60.0) / 60.0)) : 0.0;
                        char b[64];
                        std::snprintf(b, sizeof b, "{\"nivel\":%.3f}", n);
                        decir(b);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return TRUE;
}

// Primer cuadro de video: su hora en el reloj monotónico (el mismo que usa la carrera)
GstPadProbeReturn alCuadro(GstPad*, GstPadProbeInfo* info, gpointer) {
    if (primerVisto) return GST_PAD_PROBE_REMOVE;
    GstBuffer* b = GST_PAD_PROBE_INFO_BUFFER(info);
    double ms = monoMs();
    if (b && GST_BUFFER_PTS_IS_VALID(b) && tubo) {
        // El reloj del tubo es el monotónico del sistema: hora = base + pts
        const GstClockTime base = gst_element_get_base_time(tubo);
        GstClock* reloj = gst_element_get_clock(tubo);
        if (reloj) {
            gint tipo = 0;
            g_object_get(reloj, "clock-type", &tipo, nullptr);
            if (tipo == GST_CLOCK_TYPE_MONOTONIC) ms = static_cast<double>(base + GST_BUFFER_PTS(b)) / 1e6;
            gst_object_unref(reloj);
        }
    }
    primerVisto = true;
    char t[96];
    std::snprintf(t, sizeof t, "{\"primerCuadro\":%.1f}", ms);
    decir(t);
    return GST_PAD_PROBE_REMOVE;
}

// Webcam MJPEG: cada foto tiene que empezar con FFD8 y terminar con FFD9 (a veces con ceros después).
// Si llegó cortada se saltea: queda un cuadro menos en vez de una imagen rota (Matías, 06/11/2026: «la
// imagen se rompe» al grabar). Se cuenta para el reporte.
GstPadProbeReturn alFotoCamara(GstPad*, GstPadProbeInfo* info, gpointer) {
    GstBuffer* b = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!b) return GST_PAD_PROBE_OK;
    ++cuadrosTotal;
    GstMapInfo m;
    if (!gst_buffer_map(b, &m, GST_MAP_READ)) return GST_PAD_PROBE_OK;
    bool bien = m.size > 4 && m.data[0] == 0xFF && m.data[1] == 0xD8;
    if (bien) {
        bien = false;
        const gsize desde = m.size > 512 ? m.size - 512 : 0;
        for (gsize i = m.size - 1; i > desde; --i)
            if (m.data[i - 1] == 0xFF && m.data[i] == 0xD9) { bien = true; break; }
    }
    gst_buffer_unmap(b, &m);
    if (bien) return GST_PAD_PROBE_OK;
    ++cuadrosMalos;
    return GST_PAD_PROBE_DROP;
}

gboolean alContar(gpointer) {
    char t[160];
    std::snprintf(t, sizeof t, "{\"cuadros\":%ld,\"cortados\":%ld,\"avisosJpeg\":%ld}", cuadrosTotal, cuadrosMalos, avisosJpeg);
    decir(t);
    return G_SOURCE_CONTINUE;
}

// Vista: cada foto se escribe aparte y se renombra (la pantalla nunca lee una a medias)
GstFlowReturn alFoto(GstAppSink* s, gpointer) {
    GstSample* m = gst_app_sink_pull_sample(s);
    if (!m) return GST_FLOW_OK;
    GstBuffer* b = gst_sample_get_buffer(m);
    GstMapInfo map;
    if (b && gst_buffer_map(b, &map, GST_MAP_READ)) {
        const std::string tmp = salidaVista + ".tmp";
        if (FILE* f = std::fopen(tmp.c_str(), "wb")) {
            std::fwrite(map.data, 1, map.size, f);
            std::fclose(f);
            std::rename(tmp.c_str(), salidaVista.c_str());
            decir("{\"foto\":true}");
        }
        gst_buffer_unmap(b, &map);
    }
    gst_sample_unref(m);
    return GST_FLOW_OK;
}

gboolean alCumplir(gpointer) {
    alSenal(nullptr);
    return G_SOURCE_REMOVE;
}

const char* arg(int argc, char** argv, const char* nombre, const char* def) {
    for (int i = 2; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], nombre) == 0) return argv[i + 1];
    return def;
}

bool bandera(int argc, char** argv, const char* nombre) {
    for (int i = 2; i < argc; ++i)
        if (std::strcmp(argv[i], nombre) == 0) return true;
    return false;
}

std::string fuenteAudio(const std::string& audio, bool prueba) {
    if (prueba) return "audiotestsrc is-live=true wave=pink-noise volume=0.2";
    return "alsasrc device=" + audio;
}

// Webcam común (MJPEG): el procesador descomprime (jpegdec) y el chip de video de la Raspberry vuelve a
// comprimir en H.264 (v4l2h264enc). Probado en la Pi de Matías el 05/11/2026 con su webcam a 1280×720:
//   A) v4l2jpegdec ! v4l2h264enc                → se traba después del primer cuadro (36 KB)
//   B) jpegdec ! videoconvert ! v4l2h264enc     → anda (2,5 MB en 5 s)   ← este
//   C) v4l2jpegdec ! v4l2convert ! v4l2h264enc  → se traba (40 KB)
// Si no está el compresor del chip, uno de software.
// `escala` ("848x480"): achicar antes de comprimir (480p con una cámara que no lo da). En 480p, menos bitrate.
std::string compresorMjpeg(const std::string& escala, int alto) {
    auto hay = [](const char* e) { GstElementFactory* f = gst_element_factory_find(e); if (f) gst_object_unref(f); return f != nullptr; };
    std::string d = "jpegdec ! videoconvert";
    const auto x = escala.find('x');
    // Un solo filtro de caps: dos seguidos («…,height=480 ! video/x-raw,format=I420») GStreamer no los acepta y
    // la grabación en 480p no arrancaba (06-1.2)
    if (x != std::string::npos) {
        d += " ! videoscale ! video/x-raw,format=I420,width=" + escala.substr(0, x) + ",height=" + escala.substr(x + 1);
        alto = std::atoi(escala.substr(x + 1).c_str());
    } else {
        d += " ! video/x-raw,format=I420";
    }
    const int kbps = alto <= 600 ? 2000 : 4000;   // 480p y 800×600: 2 Mbit/s
    if (hay("v4l2h264enc"))
        d += " ! v4l2h264enc extra-controls=controls,video_bitrate=" + std::to_string(kbps * 1000) + ",repeat_sequence_header=1 ! video/x-h264,level=(string)4";
    else if (hay("x264enc"))
        d += " ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=" + std::to_string(kbps) + " key-int-max=30";
    else
        d += " ! openh264enc bitrate=" + std::to_string(kbps * 1000);
    std::fprintf(stderr, "MJPEG: %s\n", d.c_str());
    return d;
}

}  // namespace

int main(int argc, char** argv) {
    gst_init(&argc, &argv);
    if (argc < 2) {
        std::fprintf(stderr, "uso: betometro-camara grabar|vista|nivel|probar|mp4 [opciones]\n");
        return 2;
    }
    const std::string modo = argv[1];
    const bool prueba = bandera(argc, argv, "--prueba");
    const std::string video = arg(argc, argv, "--video", "/dev/video0");
    const std::string audio = arg(argc, argv, "--audio", "no");
    const std::string salida = arg(argc, argv, "--salida", "");
    const int ancho = std::atoi(arg(argc, argv, "--ancho", "1280"));
    const int alto = std::atoi(arg(argc, argv, "--alto", "720"));
    segundosProbar = std::atoi(arg(argc, argv, "--segundos", "5"));

    std::string desc;
    const std::string tam = "width=" + std::to_string(ancho) + ",height=" + std::to_string(alto);
    if (modo == "grabar") {
        if (salida.empty()) return 2;
        const std::string formato = arg(argc, argv, "--formato", "h264");
        formatoGrabar = formato;
        const std::string escala = arg(argc, argv, "--escala", "");
        std::string fuente;
        if (prueba)
            fuente = "videotestsrc name=fuente is-live=true pattern=ball ! video/x-raw," + tam + ",framerate=30/1"
                     + (escala.find('x') != std::string::npos ? " ! videoscale ! video/x-raw,width=" + escala.substr(0, escala.find('x')) + ",height=" + escala.substr(escala.find('x') + 1) : std::string())
                     + " ! x264enc tune=zerolatency speed-preset=ultrafast key-int-max=30";
        else if (formato == "mjpeg")
            // 1080p: la Pi 3 no llega a descomprimir y comprimir 30 por segundo y el video salía con cuadros
            // dañados; se graban 15 (se tiran antes de descomprimir). 720p y menos: 30.
            fuente = "v4l2src name=fuente device=" + video + " do-timestamp=true ! image/jpeg," + tam + ",framerate=30/1"
                     + (alto > 720 && escala.empty() ? " ! videorate drop-only=true ! image/jpeg,framerate=15/1" : "")
                     + " ! " + compresorMjpeg(escala, alto);
        else
            fuente = "v4l2src name=fuente device=" + video + " do-timestamp=true ! video/x-h264," + tam + ",framerate=30/1";
        desc = fuente + " ! h264parse name=cuadros config-interval=-1 ! queue max-size-time=3000000000 ! "
               "matroskamux name=m streamable=true ! filesink location=\"" + salida + "\" sync=false";
        if (audio != "no")
            desc += " " + fuenteAudio(audio, prueba) + " ! queue ! audioconvert ! audioresample ! audio/x-raw,rate=48000,channels=1 ! "
                    "opusenc bitrate=48000 ! queue ! m.";
    } else if (modo == "vista") {
        if (salida.empty()) return 2;
        salidaVista = salida;
        const std::string fuente = prueba
            ? "videotestsrc is-live=true pattern=smpte ! video/x-raw,width=640,height=360,framerate=5/1 ! jpegenc"
            // Vista previa: cualquier tamaño chico que dé la cámara (no todas dan 640×360)
            : "v4l2src device=" + video + " ! image/jpeg,width=[320,1280],height=[180,720] ! videorate drop-only=true ! image/jpeg,framerate=5/1";
        desc = fuente + " ! appsink name=fotos emit-signals=true max-buffers=1 drop=true sync=false";
    } else if (modo == "nivel") {
        desc = fuenteAudio(audio, prueba) + " ! audioconvert ! level interval=100000000 post-messages=true ! fakesink sync=false";
    } else if (modo == "probar") {
        if (salida.empty()) return 2;
        desc = fuenteAudio(audio, prueba) + " ! audioconvert ! audioresample ! audio/x-raw,rate=48000,channels=1 ! wavenc ! filesink location=\"" + salida + "\"";
    } else if (modo == "mp4") {
        const std::string entrada = arg(argc, argv, "--entrada", "");
        if (entrada.empty() || salida.empty()) return 2;
        const bool sonido = std::strcmp(arg(argc, argv, "--sonido", "si"), "no") != 0;
        desc = "filesrc location=\"" + entrada + "\" ! matroskademux name=d "
               "mp4mux name=m faststart=true ! filesink location=\"" + salida + "\" "
               "d.video_0 ! queue ! h264parse ! m.";
        if (sonido) {
            const auto hay = [](const char* e) { GstElementFactory* f = gst_element_factory_find(e); if (f) gst_object_unref(f); return f != nullptr; };
            const std::string aac = hay("avenc_aac") ? "avenc_aac bitrate=64000" : hay("fdkaacenc") ? "fdkaacenc" : hay("voaacenc") ? "voaacenc" : "";
            if (aac.empty()) desc += " d.audio_0 ! queue ! opusparse ! m.";   // sin AAC: queda Opus (no anda en iPhone)
            else desc += " d.audio_0 ! queue ! opusdec ! audioconvert ! audioresample ! " + aac + " ! aacparse ! m.";
        }
    } else {
        std::fprintf(stderr, "modo desconocido: %s\n", modo.c_str());
        return 2;
    }

    GError* e = nullptr;
    tubo = gst_parse_launch(desc.c_str(), &e);
    if (!tubo || e) {
        decir(std::string("{\"error\":\"") + escapar(e ? e->message : "no se pudo armar") + "\"}");
        return 1;
    }
    if (modo == "grabar") {
        // Reloj del tubo: el monotónico del sistema (no el de la placa de sonido), así la hora
        // de cada cuadro se compara con la de las marcas
        GstClock* reloj = gst_system_clock_obtain();
        g_object_set(reloj, "clock-type", GST_CLOCK_TYPE_MONOTONIC, nullptr);
        gst_pipeline_use_clock(GST_PIPELINE(tubo), reloj);
        gst_object_unref(reloj);
        // La hora se toma a la salida de la cámara (antes de cualquier codificador, que corre los tiempos)
        if (GstElement* c = gst_bin_get_by_name(GST_BIN(tubo), "fuente")) {
            GstPad* p = gst_element_get_static_pad(c, "src");
            gst_pad_add_probe(p, GST_PAD_PROBE_TYPE_BUFFER, alCuadro, nullptr, nullptr);
            if (formatoGrabar == "mjpeg" && !prueba) gst_pad_add_probe(p, GST_PAD_PROBE_TYPE_BUFFER, alFotoCamara, nullptr, nullptr);
            gst_object_unref(p);
            gst_object_unref(c);
        }
        g_timeout_add_seconds(30, alContar, nullptr);
    } else if (modo == "vista") {
        if (GstElement* s = gst_bin_get_by_name(GST_BIN(tubo), "fotos")) {
            g_signal_connect(s, "new-sample", G_CALLBACK(alFoto), nullptr);
            gst_object_unref(s);
        }
    } else if (modo == "probar") {
        g_timeout_add_seconds(static_cast<guint>(std::max(1, std::min(30, segundosProbar))), alCumplir, nullptr);
    }

    bucle = g_main_loop_new(nullptr, FALSE);
    GstBus* bus = gst_element_get_bus(tubo);
    gst_bus_add_watch(bus, alBus, nullptr);
    gst_object_unref(bus);
    g_unix_signal_add(SIGINT, alSenal, nullptr);
    g_unix_signal_add(SIGTERM, alSenal, nullptr);

    if (gst_element_set_state(tubo, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        decir("{\"error\":\"no arranca (¿cámara o micrófono desconectados?)\"}");
        return 1;
    }
    decir("{\"andando\":true}");
    g_main_loop_run(bucle);
    gst_element_set_state(tubo, GST_STATE_NULL);
    gst_object_unref(tubo);
    g_main_loop_unref(bucle);
    return 0;
}
