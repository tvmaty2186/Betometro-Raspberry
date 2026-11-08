#include "vigilante.h"

#include "reportes.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <algorithm>
#include <cmath>
#include <unistd.h>

namespace {
QByteArray leer(const QString& ruta) {
    QFile f(ruta);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
QString mb(qint64 kb) { return QString::number((kb + 512) / 1024) + QStringLiteral(" MB"); }
QString duracion(qint64 s) {
    const qint64 h = s / 3600, m = (s % 3600) / 60;
    return h > 0 ? QStringLiteral("%1 h %2 min").arg(h).arg(m) : QStringLiteral("%1 min").arg(m);
}
const qint64 kCadaAlarma = 10 * 60 * 1000;   // un aviso por motivo cada 10 minutos
}  // namespace

Vigilante::Mem Vigilante::leerMem(const QByteArray& meminfo) {
    Mem m;
    for (const QByteArray& l : meminfo.split('\n')) {
        const QList<QByteArray> p = l.simplified().split(' ');
        if (p.size() < 2) continue;
        const qint64 v = p[1].toLongLong();
        if (p[0] == "MemTotal:") m.total = v;
        else if (p[0] == "MemAvailable:") m.disponible = v;
        else if (p[0] == "SwapTotal:") m.swapTotal = v;
        else if (p[0] == "SwapFree:") m.swapLibre = v;
    }
    return m;
}

int Vigilante::porcentaje(const Mem& m) {
    if (m.total <= 0) return -1;
    return static_cast<int>(std::lround(100.0 * (m.total - m.disponible) / m.total));
}

// «123 (nombre con espacios) S 1 …»: el nombre va entre paréntesis; utime y stime son los campos 14 y 15
bool Vigilante::leerStat(const QByteArray& stat, QString* nombre, qint64* ticks) {
    const int a = stat.indexOf('('), b = stat.lastIndexOf(')');
    if (a < 0 || b < a) return false;
    const QList<QByteArray> c = stat.mid(b + 2).simplified().split(' ');   // empieza en el campo 3
    if (c.size() < 13) return false;
    if (nombre) *nombre = QString::fromUtf8(stat.mid(a + 1, b - a - 1));
    if (ticks) *ticks = c[11].toLongLong() + c[12].toLongLong();
    return true;
}

Vigilante::Vigilante(const QString& carpeta, Reportes* reportes, QObject* padre)
    : QObject(padre), reportes_(reportes), carpeta_(carpeta) {
    activo_ = qEnvironmentVariableIsSet("BETO_RASPBERRY") || qEnvironmentVariableIsSet("BETO_VIGILANTE_UMBRAL");
    if (qEnvironmentVariableIsSet("BETO_VIGILANTE_UMBRAL")) umbral_ = qEnvironmentVariableIntValue("BETO_VIGILANTE_UMBRAL");
    reloj_.start();
    // ¿Venimos de «Despejar memoria»?
    const QString archivo = carpeta_ + QStringLiteral("/despejar.json");
    if (QFile::exists(archivo)) {
        despejeAntes_ = QJsonDocument::fromJson(leer(archivo)).object().value(QStringLiteral("antes")).toInt(-1);
        QFile::remove(archivo);
    }
    connect(&cada_, &QTimer::timeout, this, &Vigilante::revisar);
    cada_.start(10000);
    revisar();
    if (activo_) {
        connect(&latido_, &QTimer::timeout, this, &Vigilante::latido);
        tic_.start();
        latido_.start(500);
    }
}

qint64 Vigilante::rssPropioKb() {
    const QList<QByteArray> p = leer(QStringLiteral("/proc/self/statm")).split(' ');
    return p.size() > 1 ? p[1].toLongLong() * (sysconf(_SC_PAGESIZE) / 1024) : 0;
}

void Vigilante::revisar() {
    // Capturas comparadas (BETO_FIJO): siempre 35 %, para que salgan iguales en cualquier máquina
    const int m = qEnvironmentVariableIsSet("BETO_FIJO") ? 35 : porcentaje(leerMem(leer(QStringLiteral("/proc/meminfo"))));
    if (m != memoria_) { memoria_ = m; emit cambio(); }
    // Cada 30 s, para la historia. Desde el minuto: lo que carga al arrancar no es una pérdida
    if (reloj_.elapsed() >= 60000 && ++vueltas_ % 3 == 1) {
        if (rssInicialKb_ == 0) rssInicialKb_ = rssPropioKb();
        historia_ << m; historiaRss_ << rssPropioKb();
        while (historia_.size() > 60) { historia_.removeFirst(); historiaRss_.removeFirst(); }
    }
    if (!activo_ || m < 0) return;
    if (m >= umbral_) { memoriaAlta_ = true; alarma(QStringLiteral("memoria"), QStringLiteral("Memoria al %1 %").arg(m)); }
    else if (memoriaAlta_ && m < umbral_ - 10) {
        memoriaAlta_ = false;
        if (reportes_) reportes_->anotar(QStringLiteral("vigilante"), QStringLiteral("Memoria: volvió a la normalidad (%1 %)").arg(m));
    }
}

void Vigilante::latido() {
    const qint64 tarde = tic_.restart() - 500;   // lo que llegó tarde el tic
    if (reloj_.elapsed() < 60000) return;        // el arranque siempre tarda: no cuenta
    if (tarde > 1500) {
        alarma(QStringLiteral("congelada"), QStringLiteral("Pantalla congelada %1 s").arg(QString::number(tarde / 1000.0, 'f', 1).replace('.', ',')));
    } else if (tarde > 400 && enCarrera && enCarrera()) {
        const qint64 ahora = reloj_.elapsed();
        trabasCortas_ << ahora;
        while (!trabasCortas_.isEmpty() && ahora - trabasCortas_.first() > 60000) trabasCortas_.removeFirst();
        if (trabasCortas_.size() >= 5)
            alarma(QStringLiteral("tirones"), QStringLiteral("Tirones en carrera: %1 trabas en el último minuto").arg(trabasCortas_.size()));
    }
}

void Vigilante::pantalla(const QString& nombre) {
    if (nombre == pantallaActual_) return;
    pantallaActual_ = nombre;
    pantallas_ << QDateTime::currentDateTime().toString(QStringLiteral("HH:mm")) + QLatin1Char(' ') + nombre;
    while (pantallas_.size() > 10) pantallas_.removeFirst();
}

QHash<int, Vigilante::Proceso> Vigilante::procesos() {
    QHash<int, Proceso> r;
    const qint64 pagina = sysconf(_SC_PAGESIZE) / 1024;
    const QDir d(QStringLiteral("/proc"));
    for (const QString& n : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool ok = false;
        const int pid = n.toInt(&ok);
        if (!ok) continue;
        Proceso p;
        p.pid = pid;
        if (!leerStat(leer(QStringLiteral("/proc/%1/stat").arg(pid)), &p.nombre, &p.ticks)) continue;
        const QList<QByteArray> sm = leer(QStringLiteral("/proc/%1/statm").arg(pid)).split(' ');
        p.rssKb = sm.size() > 1 ? sm[1].toLongLong() * pagina : 0;
        r.insert(pid, p);
    }
    return r;
}

void Vigilante::alarma(const QString& motivo, const QString& titulo) {
    const qint64 ahora = reloj_.elapsed();
    if (ultimaAlarma_.contains(motivo) && ahora - ultimaAlarma_.value(motivo) < kCadaAlarma) return;
    ultimaAlarma_.insert(motivo, ahora);
    if (motivo == QLatin1String("tirones")) trabasCortas_.clear();
    // Dos fotos de los procesos con 1 s de diferencia: así se sabe cuánto procesador usa cada uno
    const QHash<int, Proceso> antes = procesos();
    QTimer::singleShot(1000, this, [this, titulo, antes] { escribirReporte(titulo, antes); });
}

void Vigilante::escribirReporte(const QString& titulo, const QHash<int, Proceso>& antes) {
    if (!reportes_) return;
    const QHash<int, Proceso> ahora = procesos();
    const Mem m = leerMem(leer(QStringLiteral("/proc/meminfo")));
    const long hz = sysconf(_SC_CLK_TCK);
    QStringList l;
    const qint64 prendida = static_cast<qint64>(leer(QStringLiteral("/proc/uptime")).split(' ').value(0).toDouble());
    l << QStringLiteral("⚠ %1 · pantalla: %2 · prendida hace %3").arg(titulo, pantallaActual_.isEmpty() ? QStringLiteral("?") : pantallaActual_, duracion(prendida));
    // Equipo
    const QByteArray t = leer(QStringLiteral("/sys/class/thermal/thermal_zone0/temp")).trimmed();
    const QString tension = this->tension ? this->tension() : QStringLiteral("¿?");
    l << QStringLiteral("Equipo: memoria %1 de %2 (%3 %) · swap %4 · carga %5 · %6 · tensión %7")
             .arg(mb(m.total - m.disponible), mb(m.total)).arg(porcentaje(m))
             .arg(mb(m.swapTotal - m.swapLibre), QString::fromLatin1(leer(QStringLiteral("/proc/loadavg")).split(' ').value(0)),
                  t.isEmpty() ? QStringLiteral("¿? °C") : QString::number(t.toInt() / 1000) + QStringLiteral(" °C"), tension);
    // Los 5 que más memoria usan, con su procesador del último segundo
    QList<Proceso> lista = ahora.values();
    std::sort(lista.begin(), lista.end(), [](const Proceso& a, const Proceso& b) { return a.rssKb > b.rssKb; });
    QStringList ps;
    for (int i = 0; i < std::min<qsizetype>(5, lista.size()); ++i) {
        const Proceso& p = lista[i];
        const int cpu = antes.contains(p.pid) && hz > 0 ? static_cast<int>(std::lround(100.0 * (p.ticks - antes.value(p.pid).ticks) / hz)) : 0;
        ps << QStringLiteral("%1 %2 (procesador %3 %)").arg(p.nombre, mb(p.rssKb)).arg(cpu);
    }
    l << QStringLiteral("Procesos: ") + ps.join(QStringLiteral(" · "));
    // Betómetro: cuánto usa y cuánto crece por minuto (con lo de los últimos 10 minutos)
    const qint64 rss = rssPropioKb();
    QString crece = QStringLiteral("¿? (hace falta 3 min de datos)");
    if (historiaRss_.size() >= 7) {
        const int n = std::min<int>(historiaRss_.size(), 21);   // 21 muestras = 10 minutos
        const qint64 d = historiaRss_.last() - historiaRss_[historiaRss_.size() - n];
        crece = QStringLiteral("%1 MB/min").arg(QString::number(d / 1024.0 / ((n - 1) * 0.5), 'f', 1).replace('.', ','));
    }
    l << QStringLiteral("Betómetro: al minuto de prender usaba %1, ahora %2, crece %3").arg(rssInicialKb_ ? mb(rssInicialKb_) : QStringLiteral("¿?"), mb(rss), crece);
    l << QStringLiteral("Últimas pantallas: ") + pantallas_.join(QStringLiteral(" → "));
    const QStringList a = andando ? andando() : QStringList();
    l << QStringLiteral("Andando: ") + (a.isEmpty() ? QStringLiteral("nada especial") : a.join(QStringLiteral(" · ")));
    QStringList h;
    for (int i = historia_.size() % 2 ? 0 : 1; i < historia_.size(); i += 2) h << QString::number(historia_[i]);   // una por minuto
    l << QStringLiteral("Memoria cada minuto (%): ") + h.join(QStringLiteral(", "));
    for (const QString& x : l) reportes_->anotar(QStringLiteral("vigilante"), x);
}

void Vigilante::despejar() {
    QSaveFile f(carpeta_ + QStringLiteral("/despejar.json"));
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(QJsonObject{{QStringLiteral("antes"), memoria_}}).toJson(QJsonDocument::Compact));
        f.commit();
    }
    if (reportes_) reportes_->anotar(QStringLiteral("vigilante"), QStringLiteral("Despejar memoria (Chequeo previo): estaba al %1 %, Betómetro usaba %2").arg(memoria_).arg(mb(rssPropioKb())));
    // systemd lo vuelve a arrancar solo (Restart=always). Se borra la cuenta de arranques para que unos
    // cuantos despejes seguidos no se tomen como una falla (StartLimitBurst en betometro.service).
    if (qEnvironmentVariableIsSet("BETO_RASPBERRY"))
        QProcess::execute(QStringLiteral("systemctl"), {QStringLiteral("reset-failed"), QStringLiteral("betometro.service")});
    QTimer::singleShot(300, qApp, [] { QCoreApplication::exit(0); });
}
