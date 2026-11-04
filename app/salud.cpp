#include "salud.h"

#include <QDir>
#include <QFile>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace {
const char* kEstado = "/sys/devices/platform/soc/soc:firmware/get_throttled";
const char* kTemperatura = "/sys/class/thermal/thermal_zone0/temp";

QByteArray leer(const QString& ruta) {
    QFile f(ruta);
    return f.open(QIODevice::ReadOnly) ? f.readAll().trimmed() : QByteArray();
}
bool escribir(const QString& ruta, const QByteArray& valor) {
    QFile f(ruta);
    return f.open(QIODevice::WriteOnly) && f.write(valor) == valor.size();
}
QStringList nucleos() {
    QStringList r;
    const QDir d(QStringLiteral("/sys/devices/system/cpu"));
    for (const QString& n : d.entryList({QStringLiteral("cpu[0-9]*")}, QDir::Dirs))
        if (QFile::exists(d.filePath(n + QStringLiteral("/cpufreq/scaling_governor")))) r << d.filePath(n + QStringLiteral("/cpufreq/scaling_governor"));
    return r;
}
}  // namespace

unsigned Salud::leerEstado(const QByteArray& texto) {
    bool ok = false;
    const QByteArray t = texto.trimmed().startsWith("throttled=") ? texto.trimmed().mid(10) : texto.trimmed();
    const unsigned v = t.toUInt(&ok, 16);
    return ok ? v : 0;
}

Salud::Salud(QObject* padre) : QObject(padre), raspberry_(qEnvironmentVariableIsSet("BETO_RASPBERRY")) {
    if (!raspberry_) return;
    connect(&cada_, &QTimer::timeout, this, &Salud::revisar);
    cada_.start(2000);
    revisar();
    // Vigilante: «sigo vivo» cada 5 s desde el hilo principal. Si el programa se traba, deja de
    // llegar y systemd lo reinicia (WatchdogSec=30 en betometro.service).
    connect(&vivo_, &QTimer::timeout, this, [] { avisarSystemd("WATCHDOG=1"); });
    if (qEnvironmentVariableIsSet("WATCHDOG_USEC")) vivo_.start(5000);
}

Salud::~Salud() { modoCarrera(false); }

void Salud::listo() {
    if (raspberry_) { avisarSystemd("READY=1"); avisarSystemd("WATCHDOG=1"); }
}

void Salud::avisarSystemd(const char* mensaje) {
    const QByteArray ruta = qgetenv("NOTIFY_SOCKET");
    if (ruta.isEmpty() || ruta.size() >= static_cast<int>(sizeof(sockaddr_un::sun_path))) return;
    const int s = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (s < 0) return;
    sockaddr_un a{};
    a.sun_family = AF_UNIX;
    std::memcpy(a.sun_path, ruta.constData(), ruta.size());
    if (a.sun_path[0] == '@') a.sun_path[0] = '\0';   // socket abstracto
    const socklen_t largo = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + ruta.size());
    ::sendto(s, mensaje, std::strlen(mensaje), MSG_NOSIGNAL, reinterpret_cast<sockaddr*>(&a), largo);
    ::close(s);
}

void Salud::revisar() {
    const QByteArray e = leer(QString::fromLatin1(kEstado));
    const unsigned nuevo = leerEstado(e);
    const QByteArray t = leer(QString::fromLatin1(kTemperatura));
    const int temp = t.isEmpty() ? -1 : t.toInt() / 1000;
    const bool sabe = !e.isEmpty() || temp >= 0;
    const bool baja0 = tensionBaja(), calor0 = calor();
    const bool cambio_ = nuevo != estado_ || temp != temperatura_ || sabe != sabe_;
    estado_ = nuevo;
    temperatura_ = temp;
    sabe_ = sabe;
    if (tensionBaja() && !baja0) emit aviso(QStringLiteral("Tensión baja: la fuente no da 5 V suficientes (revisar el convertidor y el cable)"));
    if (calor() && !calor0) emit aviso(QStringLiteral("Calor: el procesador se frena por temperatura (%1 °C)").arg(temp));
    if (cambio_) emit cambio();
    if (carrera_) aplicar();
}

void Salud::modoCarrera(bool siCarrera) {
    if (!raspberry_ || siCarrera == carrera_) return;
    carrera_ = siCarrera;
    if (siCarrera) {
        gobernadores_.clear();
        huboProblema_ = false;
        for (const QString& c : nucleos()) gobernadores_ << QString::fromLatin1(leer(c));
    }
    aplicar();
    if (!siCarrera) gobernadores_.clear();
}

// En carrera, al máximo; salvo con calor o tensión baja: ahí se lo deja bajar (como estaba antes de la
// carrera) para aguantar, y vuelve al máximo después de 30 s sin problemas (Matías, 04/11/2026).
void Salud::aplicar() {
    const bool problema = calor() || tensionBaja();
    if (problema) { huboProblema_ = true; sinProblemaDesde_.invalidate(); }
    else if (!sinProblemaDesde_.isValid()) sinProblemaDesde_.start();
    const bool alMaximo = carrera_ && !problema && (!huboProblema_ || sinProblemaDesde_.elapsed() >= 30000);
    if (alMaximo == alMaximo_) return;
    alMaximo_ = alMaximo;
    const QStringList cpus = nucleos();
    for (int i = 0; i < cpus.size(); ++i) {
        if (alMaximo) escribir(cpus[i], "performance");
        else if (i < gobernadores_.size() && !gobernadores_[i].isEmpty()) escribir(cpus[i], gobernadores_[i].toLatin1());
    }
    if (carrera_ && problema) qWarning("Procesador: se deja bajar por %s", calor() ? "calor" : "tensión baja");
}
