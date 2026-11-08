#include "camara.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QTextStream>
#include <QUrl>

#include "datos.h"
#include "gps.h"

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace {
constexpr double kMBporHora = 2048;   // 720p H.264 + sonido: unos 2 GB por hora (480p la mitad)
constexpr qint64 kMinimoLibre = 2LL * 1024 * 1024 * 1024;   // con menos de 2 GB libres se hace lugar

struct Dispositivo { QString ruta, nombre; bool h264 = false, mjpeg = false; QList<QSize> tamH264, tamMjpeg; };

QList<Dispositivo> buscarCamaras() {
    QList<Dispositivo> r;
#ifdef Q_OS_LINUX
    for (int i = 0; i < 16; ++i) {
        const QByteArray ruta = "/dev/video" + QByteArray::number(i);
        const int fd = ::open(ruta.constData(), O_RDWR | O_NONBLOCK);
        if (fd < 0) continue;
        v4l2_capability cap{};
        if (::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
            const quint32 caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
            if ((caps & V4L2_CAP_VIDEO_CAPTURE) && QByteArray(reinterpret_cast<const char*>(cap.driver)) == "uvcvideo") {
                Dispositivo d;
                d.ruta = QString::fromLatin1(ruta);
                d.nombre = QString::fromUtf8(reinterpret_cast<const char*>(cap.card)).trimmed();
                v4l2_fmtdesc f{};
                f.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                for (f.index = 0; ::ioctl(fd, VIDIOC_ENUM_FMT, &f) == 0; ++f.index) {
                    if (f.pixelformat == V4L2_PIX_FMT_H264) d.h264 = true;
                    if (f.pixelformat == V4L2_PIX_FMT_MJPEG) d.mjpeg = true;
                    if (f.pixelformat != V4L2_PIX_FMT_H264 && f.pixelformat != V4L2_PIX_FMT_MJPEG) continue;
                    // Tamaños que da en ese formato (para 480p: no todas lo tienen)
                    v4l2_frmsizeenum t{};
                    t.pixel_format = f.pixelformat;
                    for (t.index = 0; ::ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &t) == 0 && t.type == V4L2_FRMSIZE_TYPE_DISCRETE; ++t.index)
                        (f.pixelformat == V4L2_PIX_FMT_H264 ? d.tamH264 : d.tamMjpeg) << QSize(int(t.discrete.width), int(t.discrete.height));
                }
                r << d;
            }
        }
        ::close(fd);
    }
#endif
    return r;
}

// Micrófonos USB de /proc/asound/cards: « 1 [Camera         ]: USB-Audio - USB Camera»
QVariantList buscarMicrofonos() {
    QVariantList r;
    QFile f(QStringLiteral("/proc/asound/cards"));
    if (!f.open(QIODevice::ReadOnly)) return r;
    static const QRegularExpression re(QStringLiteral("^\\s*(\\d+)\\s+\\[(\\S+)\\s*\\]:\\s*(\\S+)\\s+-\\s+(.*)$"));
    for (const QString& l : QString::fromUtf8(f.readAll()).split('\n')) {
        const auto m = re.match(l);
        if (!m.hasMatch() || m.captured(3) != QLatin1String("USB-Audio")) continue;
        if (!QFile::exists(QStringLiteral("/proc/asound/card%1/pcm0c").arg(m.captured(1)))) continue;   // sin captura
        r << QVariantMap{{"id", m.captured(2)}, {"nombre", m.captured(4).trimmed()},
                         {"dispositivo", QStringLiteral("plughw:CARD=%1").arg(m.captured(2))}};
    }
    return r;
}

bool parecido(const QString& a, const QString& b) {
    for (const QString& p : a.toLower().split(QRegularExpression(QStringLiteral("\\W+")), Qt::SkipEmptyParts))
        if (p.size() >= 3 && b.toLower().contains(p)) return true;
    return false;
}
}  // namespace

Camara::Camara(Datos* datos, const QString& carpetaDatos, QObject* padre)
    : QObject(padre), datos_(datos), carpeta_(carpetaDatos + QStringLiteral("/videos")),
      prueba_(qEnvironmentVariableIsSet("BETO_CAMARA_PRUEBA")) {
    // La cámara después del GPS, el cronómetro y el pulsador
    for (QProcess* p : {&vistaProc_, &micProc_}) p->setChildProcessModifier([] { [[maybe_unused]] const int r = ::nice(5); });
    grab_.setChildProcessModifier([] { [[maybe_unused]] const int r = ::nice(10); });
    QDir().mkpath(carpeta_);
    connect(this, &Camara::grabandoChanged, this, &Camara::enProcesoChanged);
    connect(this, &Camara::convirtiendoChanged, this, &Camara::enProcesoChanged);
    detenerTimer_.setSingleShot(true);
    connect(&detenerTimer_, &QTimer::timeout, this, [this] { if (grabando()) { aMano_ = true; grab_.terminate(); matarTimer_.start(); } });
    reintento_.setSingleShot(true);
    reintento_.setInterval(2000);
    connect(&reintento_, &QTimer::timeout, this, [this] {
        if (grabando() || !hay() || !datos_->config()["camara"].aBool(false)) { emit grabandoChanged(); return; }
        ++reintentos_;
        const QString nombre = nombreBase_ + '-' + QChar('a' + reintentos_);
        qWarning("Cámara: no arrancó, se vuelve a probar (%d de 2) en %s", reintentos_, qPrintable(nombre));
        if (arrancar(nombre)) {
            if (quedabanMs_ > 0) detenerTimer_.start(quedabanMs_);
            emit reintento();
        } else {
            emit grabandoChanged();
        }
    });
    matarTimer_.setSingleShot(true);
    matarTimer_.setInterval(12000);
    connect(&matarTimer_, &QTimer::timeout, this, [this] {
        if (!grabando()) return;
        qWarning("Cámara: no terminó al pedírselo, se cortó a la fuerza");
        grab_.kill();
    });

    leer(grab_, [this](const QVariantMap& m) {
        if (m.contains("primerCuadro")) {
            primerCuadro_ = monotonicoAAhora(m.value("primerCuadro").toDouble());
            emit primerCuadroListo(primerCuadro_);
        }
        if (m.contains("error")) { falla_ = m.value("error").toString(); emit grabandoChanged(); }
        if (m.contains("cuadros")) {   // el último conteo (cada 30 s y al terminar)
            cuadros_ = m;
            const qlonglong n = m.value("cuadros").toLongLong();
            // Cámara colgada sin error (cable, corriente): en 30 s no llegó ningún cuadro (06-1.4)
            if (grabando() && !matarTimer_.isActive() && cuadrosAntes_ >= 0 && n <= cuadrosAntes_) cortar(QStringLiteral("La cámara se colgó"));
            cuadrosAntes_ = n;
            // Tarjeta casi llena: se para el video y la carrera sigue (Matías, 07/11; 06-1.3)
            const QStorageInfo d(carpeta_);
            if (grabando() && !matarTimer_.isActive() && d.isValid() && d.bytesAvailable() < 300LL * 1024 * 1024)
                cortar(QStringLiteral("Poco lugar: se paró el video"));
        }
    });
    // Al terminar de grabar: si la webcam mandó cuadros cortados o dañados, queda en el reporte
    connect(&grab_, &QProcess::finished, this, [this] {
        const qlonglong n = cuadros_.value("cuadros").toLongLong(), malos = cuadros_.value("cortados").toLongLong(),
                        danados = cuadros_.value("avisosJpeg").toLongLong();
        if (n > 0 && (malos > 0 || danados > 0))
            qWarning("Cámara: de %lld cuadros, %lld llegaron cortados (salteados) y %lld con daños", n, malos, danados);
        ultimosCuadros_ = cuadros_;
        cuadros_.clear();
    });
    connect(&grab_, &QProcess::stateChanged, this, [this](QProcess::ProcessState s) {
        if (s == QProcess::NotRunning) matarTimer_.stop();
        // El conteo de cuadros se guarda acá: «finished» llega después de este aviso y la Prueba completa leía el
        // de la grabación anterior (salía «no se cuentan» o un número viejo)
        if (s == QProcess::NotRunning) ultimosCuadros_ = cuadros_;
        if (s == QProcess::NotRunning && grab_.exitCode() != 0 && falla_.isEmpty()) falla_ = QStringLiteral("La cámara dejó de grabar");
        // Falló antes del primer cuadro («failed during initialization», el USB que se reinicia): a los 2 s se
        // vuelve a probar sola, en otro archivo. Pasaba en una de cada dos pruebas completas (07/11/2026).
        if (s == QProcess::NotRunning && !aMano_ && primerCuadro_ <= 0 && reintentos_ < 2 && !falla_.startsWith(QLatin1String("Poco lugar"))) {
            quedabanMs_ = detenerTimer_.isActive() ? detenerTimer_.interval() : 0;   // el tiempo entero de nuevo (la prueba graba 30 s)
            detenerTimer_.stop();
            const QFileInfo f(archivo_);   // el que no llegó a tener cuadros
            if (f.exists() && f.size() < 1024 * 1024) QFile::remove(f.absoluteFilePath());
            reintento_.start();
        }
        emit grabandoChanged();
        if (s != QProcess::Starting) actualizarLugar();   // la lista de videos marca el que se está grabando
    });
    leer(vistaProc_, [this](const QVariantMap& m) {
        if (!m.value("foto").toBool()) return;
        vista_ = QUrl::fromLocalFile(QDir::tempPath() + QStringLiteral("/betometro-vista.jpg"));
        vista_.setQuery(QStringLiteral("n=%1").arg(++fotos_));   // otra dirección: la imagen se vuelve a leer
        emit vistaChanged();
    });
    connect(&vistaProc_, &QProcess::stateChanged, this, &Camara::vistaChanged);
    // MP4: al terminar de grabar (y al prender, por si quedó alguno) se pasa el .mkv a .mp4, que abren
    // todos los teléfonos. Se graba en .mkv porque si se corta la luz no se pierde lo grabado.
    mp4_.setChildProcessModifier([] { [[maybe_unused]] const int r = ::nice(15); });
    leer(mp4_, [this](const QVariantMap& m) { if (m.contains("error")) mp4Error_ = true; });
    connect(&mp4_, &QProcess::finished, this, [this](int codigo, QProcess::ExitStatus estado) {
        const QString mkv = mp4Entrada_, mp4 = mkv.left(mkv.size() - 4) + QStringLiteral(".mp4"), parte = mp4 + QStringLiteral(".parte");
        mp4Entrada_.clear();
        if (estado == QProcess::NormalExit && codigo == 0 && !mp4Error_ && QFileInfo(parte).size() > 0) {
            QFile::remove(mp4);
            if (QFile::rename(parte, mp4)) {
                datos_->renombrarVideo(QFileInfo(mkv).fileName(), QFileInfo(mp4).fileName());
                QFile::remove(mkv);
            }
        } else {
            QFile::remove(parte);
            // Falló en carrera: se reintenta después. Si falló fuera de carrera, no (queda el .mkv, que igual se ve)
            if (!enCarrera_) mp4Fallados_ << mkv;
        }
        emit convirtiendoChanged();
        actualizarLugar();
        QTimer::singleShot(1000, this, &Camara::revisarMp4);
    });
    connect(&grab_, &QProcess::finished, this, [this] { QTimer::singleShot(3000, this, &Camara::revisarMp4); });
    QTimer::singleShot(30000, this, &Camara::revisarMp4);
    leer(nivelProc_, [this](const QVariantMap& m) {
        if (m.contains("nivel")) { nivel_ = m.value("nivel").toDouble(); emit nivelChanged(); }
    });
    connect(&micProc_, &QProcess::finished, this, [this] {
        const QString wav = QDir::tempPath() + QStringLiteral("/betometro-prueba-mic.wav");
        if (pruebaMic_ == QLatin1String("grabando") && QFile::exists(wav)) {
            pruebaMic_ = QStringLiteral("escuchando");
            emit pruebaMicChanged();
            micProc_.start(QStringLiteral("aplay"), {QStringLiteral("-q"), wav});   // por los auriculares
            return;
        }
        pruebaMic_.clear();
        emit pruebaMicChanged();
    });
    // Si no arranca (falta aplay o el ayudante), «finished» nunca llega: el botón quedaba trabado (auditoría 12)
    connect(&micProc_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart) return;
        qWarning("Cámara: la prueba del micrófono no pudo arrancar (%s)", qPrintable(micProc_.program()));
        pruebaMic_.clear();
        emit pruebaMicChanged();
    });
    detectar();
}

Camara::~Camara() {
    for (QProcess* p : {&grab_, &vistaProc_, &nivelProc_, &micProc_}) {
        if (p->state() == QProcess::NotRunning) continue;
        p->terminate();
        if (!p->waitForFinished(3000)) p->kill();
    }
}

void Camara::leer(QProcess& p, const std::function<void(const QVariantMap&)>& f) {
    connect(&p, &QProcess::readyReadStandardOutput, this, [&p, f] {
        while (p.canReadLine()) {
            const QVariantMap m = QJsonDocument::fromJson(p.readLine()).toVariant().toMap();
            if (!m.isEmpty()) f(m);
        }
    });
}

QStringList Camara::enProceso() const {
    QStringList r;
    if (grabando() && !archivo_.isEmpty()) r << QFileInfo(archivo_).fileName();
    if (convirtiendo() && !mp4Entrada_.isEmpty()) r << QFileInfo(mp4Entrada_).fileName();
    return r;
}

QString Camara::ayudante() const {
    const QString junto = QCoreApplication::applicationDirPath() + QStringLiteral("/betometro-camara");
    return QFile::exists(junto) ? junto : QStringLiteral("betometro-camara");
}

QStringList Camara::base() const { return prueba_ ? QStringList{QStringLiteral("--prueba")} : QStringList{}; }

void Camara::detectar() {
    dispVideo_.clear();
    problema_.clear();
    if (prueba_) {
        dispVideo_ = QStringLiteral("prueba");
        nombre_ = QStringLiteral("Cámara de prueba · H.264 · lista");
        microfonos_ = {QVariantMap{{"id", "prueba"}, {"nombre", "Micrófono de prueba"}, {"dispositivo", "prueba"}}};
    } else {
        const auto cams = buscarCamaras();
        // Primero una que dé H.264 (no hay que comprimir); si no, una común con MJPEG: la Raspberry la
        // pasa a H.264 con su chip de video (05/11/2026, para la webcam genérica de Matías)
        for (const auto& d : cams) if (d.h264) { dispVideo_ = d.ruta; formatoVideo_ = QStringLiteral("h264"); tamanos_ = d.tamH264; nombre_ = d.nombre + QStringLiteral(" · H.264 · lista"); break; }
        if (dispVideo_.isEmpty())
            for (const auto& d : cams) if (d.mjpeg) { dispVideo_ = d.ruta; formatoVideo_ = QStringLiteral("mjpeg"); tamanos_ = d.tamMjpeg; nombre_ = d.nombre + QStringLiteral(" · MJPEG · lista"); break; }
        if (dispVideo_.isEmpty()) {
            nombre_.clear();
            problema_ = cams.isEmpty() ? QStringLiteral("No se encontró la cámara")
                                       : QStringLiteral("«%1» no entrega H.264 ni MJPEG: no sirve para grabar").arg(cams.first().nombre);
        }
        microfonos_ = buscarMicrofonos();
    }
    emit dispositivosChanged();
    actualizarLugar();
}

QString Camara::dispositivoAudio() const {
    const QString modo = datos_->config()["camaraSonido"].aTexto("camara") == "usb" ? QStringLiteral("usb")
                       : datos_->config()["camaraSonido"].aTexto("camara") == "no" ? QStringLiteral("no") : QStringLiteral("camara");
    if (modo == QLatin1String("no") || microfonos_.isEmpty()) return QStringLiteral("no");
    if (prueba_) return QStringLiteral("prueba");
    const QString cam = nombre_.section(QStringLiteral(" · "), 0, 0);
    // de la cámara: el que se parece al nombre de la cámara; USB: otro (o el único que haya)
    for (const auto& v : microfonos_) {
        const auto m = v.toMap();
        const bool esDeLaCamara = parecido(cam, m.value("nombre").toString()) || parecido(cam, m.value("id").toString());
        if ((modo == QLatin1String("camara")) == esDeLaCamara) return m.value("dispositivo").toString();
    }
    return microfonos_.first().toMap().value("dispositivo").toString();
}

void Camara::empezarVista() {
    if (!hay() || viendo() || grabando()) return;
    QStringList a{QStringLiteral("vista"), QStringLiteral("--video"), dispVideo_,
                  QStringLiteral("--salida"), QDir::tempPath() + QStringLiteral("/betometro-vista.jpg")};
    vistaProc_.start(ayudante(), a + base());
}

void Camara::pararVista() { if (viendo()) vistaProc_.terminate(); }

void Camara::empezarNivel() {
    const QString audio = dispositivoAudio();
    if (audio == QLatin1String("no") || nivelProc_.state() != QProcess::NotRunning) return;
    nivelProc_.start(ayudante(), QStringList{QStringLiteral("nivel"), QStringLiteral("--audio"), audio} + base());
}

void Camara::pararNivel() {
    if (nivelProc_.state() != QProcess::NotRunning) nivelProc_.terminate();
    nivel_ = 0;
    emit nivelChanged();
}

void Camara::probarMicrofono() {
    const QString audio = dispositivoAudio();
    if (audio == QLatin1String("no") || !pruebaMic_.isEmpty()) return;
    pararNivel();
    pruebaMic_ = QStringLiteral("grabando");
    emit pruebaMicChanged();
    micProc_.start(ayudante(), QStringList{QStringLiteral("probar"), QStringLiteral("--audio"), audio, QStringLiteral("--segundos"),
                                           QStringLiteral("5"), QStringLiteral("--salida"),
                                           QDir::tempPath() + QStringLiteral("/betometro-prueba-mic.wav")} + base());
}

void Camara::probarGrabacion(int segundos) {
    if (empezar(QStringLiteral("prueba-") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")))) detener(qMax(1, segundos));
}

bool Camara::empezar(const QString& nombreBase) {
    if (!hay() || grabando()) return false;
    reintento_.stop();
    reintentos_ = 0;
    quedabanMs_ = 0;
    nombreBase_ = nombreBase;
    return arrancar(nombreBase);
}

bool Camara::arrancar(const QString& nombreBase) {
    if (!hay() || grabando()) return false;
    aMano_ = false;
    pararVista();
    pararNivel();
    // La vista previa y la barra del micrófono tienen tomados la cámara y el micrófono: hay que esperar a
    // que los suelten, si no la grabación arranca con «dispositivo ocupado» y falla enseguida (05/11/2026)
    if (vistaProc_.state() != QProcess::NotRunning && !vistaProc_.waitForFinished(2000)) vistaProc_.kill();
    if (nivelProc_.state() != QProcess::NotRunning && !nivelProc_.waitForFinished(2000)) nivelProc_.kill();
    hacerLugar();
    falla_.clear();
    primerCuadro_ = 0;
    cuadrosAntes_ = -1;
    detenerTimer_.stop();
    matarTimer_.stop();
    archivo_ = carpeta_ + '/' + nombreBase + QStringLiteral(".mkv");
    const QString calidad = QString::fromStdString(datos_->config()["camaraCalidad"].aTexto("720p"));
    QSize tam(1280, 720);   // 720p o 480p (sin 1080p)
    QString escala;
    if (calidad == QLatin1String("480p")) {
        // 480p: si la cámara lo da, directo (16:9 primero, si no 4:3). Si no, una común (MJPEG) se toma en 720p y
        // se achica antes de comprimir; una H.264 sin 480p graba en 720p (achicarla costaría mucho a la Pi).
        tam = QSize(1280, 720);
        // 800×600 (la webcam de Matías no da 480p): directo de la cámara, 2,6 veces menos trabajo que achicar 720p (06-2.1)
        for (const QSize& s : {QSize(848, 480), QSize(854, 480), QSize(800, 600), QSize(640, 480)})
            if (tamanos_.contains(s)) { tam = s; break; }
        if (tam.height() != 480 && (formatoVideo_ == QLatin1String("mjpeg") || prueba_)) escala = QStringLiteral("848x480");
    }
    QStringList a{QStringLiteral("grabar"), QStringLiteral("--video"), dispVideo_, QStringLiteral("--formato"), formatoVideo_,
                  QStringLiteral("--ancho"), QString::number(tam.width()), QStringLiteral("--alto"), QString::number(tam.height()),
                  QStringLiteral("--audio"), dispositivoAudio(), QStringLiteral("--salida"), archivo_};
    if (!escala.isEmpty()) a << QStringLiteral("--escala") << escala;
    // Prioridad: la carrera primero. Un solo nice (el 5 del ayudante, sumado al «nice -n 15» de antes, daba 19 y en
    // carrera se quedaba sin procesador y perdía cuadros) (06-3.1)
    grab_.setProgram(ayudante());
    grab_.setArguments(a + base());
    grab_.start();
    emit grabandoChanged();
    return true;
}

void Camara::detener(int retrasoS) {
    if (reintento_.isActive()) { reintento_.stop(); emit grabandoChanged(); }   // iba a volver a probar: ya no
    if (!grabando()) return;
    if (retrasoS <= 0) { aMano_ = true; grab_.terminate(); matarTimer_.start(); return; }
    detenerTimer_.start(retrasoS * 1000);
}

void Camara::cortar(const QString& motivo) {
    qWarning("Cámara: %s", qPrintable(motivo));
    falla_ = motivo;
    aMano_ = true;
    detenerTimer_.stop();
    grab_.terminate();
    matarTimer_.start();
    emit grabandoChanged();
}

void Camara::hacerLugar() {
    if (!datos_->config()["camaraBorrarViejos"].aBool(true)) return;
    QStorageInfo d(carpeta_);
    if (!d.isValid() || d.bytesAvailable() >= kMinimoLibre) return;
    // Del más viejo al más nuevo, hasta tener el doble del mínimo. Nunca toca los informes.
    QFileInfoList l = QDir(carpeta_).entryInfoList({QStringLiteral("*.mkv"), QStringLiteral("*.mp4")}, QDir::Files, QDir::Time | QDir::Reversed);
    for (const QFileInfo& f : l) {
        if (QStorageInfo(carpeta_).bytesAvailable() >= 2 * kMinimoLibre) break;
        if (convirtiendo() && f.absoluteFilePath() == mp4Entrada_) continue;   // el que se está pasando a MP4, no (06-3.2)
        // También se saca de su carrera: si no, el informe muestra «Video» de un archivo que ya no está
        if (QFile::remove(f.absoluteFilePath())) datos_->olvidarVideo(f.fileName());
    }
}

QVariantList Camara::videos() const {
    QVariantList r = datos_->videosGuardados();
    const QString actual = grabando() ? QFileInfo(archivo_).fileName() : QString();
    for (QVariant& v : r) {
        QVariantMap m = v.toMap();
        m["grabando"] = !actual.isEmpty() && m.value("archivo").toString() == actual;
        v = m;
    }
    return r;
}

void Camara::ponerEnCarrera(bool enCarrera) {
    enCarrera_ = enCarrera;
    // La carrera primero, pero el pase que ya estaba no se corta (se perdía y volvía a empezar de cero): sigue con
    // la prioridad más baja; al terminar la carrera vuelve a la de siempre (1.0.31)
    if (convirtiendo()) { [[maybe_unused]] const int r = ::setpriority(PRIO_PROCESS, static_cast<id_t>(mp4_.processId()), enCarrera_ ? 19 : 15); }
    if (!enCarrera_) QTimer::singleShot(5000, this, &Camara::revisarMp4);
}

void Camara::revisarMp4() {
    if (enCarrera_ || grabando() || convirtiendo()) return;
    const QFileInfoList l = QDir(carpeta_).entryInfoList({QStringLiteral("*.mkv")}, QDir::Files, QDir::Time | QDir::Reversed);
    for (const QFileInfo& f : l) {
        if (mp4Fallados_.contains(f.absoluteFilePath())) continue;
        // ¿Tiene sonido? Los videos de Betómetro llevan Opus y el encabezado lo dice al principio
        QFile a(f.absoluteFilePath());
        const bool sonido = a.open(QIODevice::ReadOnly) && a.read(256 * 1024).contains("A_OPUS");
        mp4Entrada_ = f.absoluteFilePath();
        mp4Error_ = false;
        const QString salida = mp4Entrada_.left(mp4Entrada_.size() - 4) + QStringLiteral(".mp4.parte");
        mp4_.setProgram(ayudante());
        mp4_.setArguments(QStringList{QStringLiteral("mp4"), QStringLiteral("--entrada"), mp4Entrada_, QStringLiteral("--salida"), salida,
                                      QStringLiteral("--sonido"), sonido ? QStringLiteral("si") : QStringLiteral("no")});
        mp4_.start();
        emit convirtiendoChanged();
        return;
    }
}

bool Camara::borrarVideo(const QString& archivo) {
    if (grabando() || !Datos::nombreVideoValido(archivo)) return false;
    if (convirtiendo() && QFileInfo(mp4Entrada_).fileName() == archivo) return false;
    const bool ok = datos_->borrarVideo(archivo);
    actualizarLugar();
    return ok;
}

int Camara::borrarPruebas() {
    if (grabando()) return 0;
    int n = 0;
    for (const QFileInfo& f : QDir(carpeta_).entryInfoList({QStringLiteral("prueba-*.mkv"), QStringLiteral("prueba-*.mp4")}, QDir::Files))
        if (datos_->borrarVideo(f.fileName())) ++n;
    actualizarLugar();
    return n;
}

QString Camara::urlVideo(const QString& archivo) const {
    if (archivo.isEmpty() || archivo.contains('/')) return {};
    const QString ruta = carpeta_ + '/' + archivo;
    return QFileInfo::exists(ruta) ? QUrl::fromLocalFile(ruta).toString() : QString();
}

void Camara::actualizarLugar() {
    qint64 usado = 0;
    int n = 0;
    for (const QFileInfo& f : QDir(carpeta_).entryInfoList({QStringLiteral("*.mkv"), QStringLiteral("*.mp4")}, QDir::Files)) { usado += f.size(); ++n; }
    QStorageInfo d(carpeta_);
    const double libreMB = d.isValid() ? d.bytesAvailable() / 1048576.0 : 0;
    lugar_ = {{"usadoGB", QString::number(usado / 1073741824.0, 'f', 1).replace('.', ',')}, {"videos", n},
              {"horas", qMax(0, qRound(libreMB / (kMBporHora * (datos_->config()["camaraCalidad"].aTexto("720p") == "480p" ? 0.5 : 1.0))))},
              {"porcentaje", d.isValid() && d.bytesTotal() > 0 ? double(usado) / d.bytesTotal() : 0.0}};
    emit lugarChanged();
}
