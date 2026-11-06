#include "reportes.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QNetworkReply>
#include <QProcess>
#include <QSysInfo>

namespace {
Reportes* gReportes = nullptr;
QtMessageHandler gAnterior = nullptr;
QMutex gMutex;
constexpr qint64 kMaxBytes = 200 * 1024;
const char* kRepo = "https://api.github.com/repos/tvmaty2186/Betometro-reportes/issues";

void manejador(QtMsgType tipo, const QMessageLogContext& ctx, const QString& msg) {
    if (gAnterior) gAnterior(tipo, ctx, msg);
    if (!gReportes || tipo == QtDebugMsg || tipo == QtInfoMsg || Reportes::esRuido(msg)) return;
    const char* t = tipo == QtWarningMsg ? "aviso" : tipo == QtCriticalMsg ? "error" : "fatal";
    QString donde;
    if (ctx.file) donde = QStringLiteral(" (%1:%2)").arg(QString::fromUtf8(ctx.file).section('/', -1)).arg(ctx.line);
    gReportes->anotar(QString::fromLatin1(t), msg + donde);
}
}  // namespace

bool Reportes::esRuido(const QString& m) {
    static const QStringList ruido = {
        QStringLiteral("pipewire"), QStringLiteral("PulseAudioService: pa_context_connect"), QStringLiteral("Failed to update audio output"),
        QStringLiteral("UTF-8 locale"), QStringLiteral("No QtMultimedia backends"), QStringLiteral("qt.multimedia.ffmpeg"),
        // Normales, no son fallas (06/11/2026): quedan en el registro del sistema, no en el reporte
        QStringLiteral("Video: descompresor"), QStringLiteral("Video: colores por"), QStringLiteral("Video: sonido por"),
        QStringLiteral("Corrupt JPEG data"),   // la webcam manda algún cuadro cortado: se saltea
        QStringLiteral("QSGPlainTexture: Mipmap"),
        // Del sistema en cada arranque (sonido y wifi de la Pi)
        QStringLiteral("alsa-restore.rules"), QStringLiteral("bgscan simple"), QStringLiteral("brcmf_p2p_send_action_frame"),
        QStringLiteral("Registration to specific type not supported")};
    for (const auto& r : ruido) if (m.contains(r, Qt::CaseInsensitive)) return true;
    return false;
}

Reportes::Reportes(const QString& carpetaDatos, QObject* padre)
    : QObject(padre), carpeta_(carpetaDatos + QStringLiteral("/reportes")) {
    QDir().mkpath(carpeta_);
    archivo_ = carpeta_ + QStringLiteral("/errores.log");
    enviadoHasta_ = carpeta_ + QStringLiteral("/enviado");
    marcaVivo_ = carpeta_ + QStringLiteral("/corriendo");
    // Si quedó la marca de la vez anterior, no se cerró bien: se cayó o se cortó el contacto.
    // La marca guarda el número del arranque del sistema: si es otro, se apagó toda la Raspberry
    // (corte o cuelgue) y se suma lo último que anotó el sistema antes de caerse.
    const QByteArray arranque = leerArranque();
    QFile previa(marcaVivo_);
    if (previa.open(QIODevice::ReadOnly)) {
        const QByteArray m = previa.readAll();
        previa.close();
        const QByteArray arranquePrevio = m.split('\n').value(1).trimmed();
        const bool mismo = !arranque.isEmpty() && arranquePrevio == arranque;
        anotar(QStringLiteral("cierre"), mismo ? QStringLiteral("La vez anterior Betómetro se cerró solo (se cayó o se trabó) y se volvió a abrir")
                                               : QStringLiteral("La vez anterior no se apagó bien (se cortó el contacto o se colgó la Raspberry)"));
        if (qEnvironmentVariableIsSet("BETO_RASPBERRY")) juntarSistema(!mismo);
    }
    QFile v(marcaVivo_);
    if (v.open(QIODevice::WriteOnly)) v.write(QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8() + '\n' + arranque + '\n');
    connect(&cada_, &QTimer::timeout, this, &Reportes::enviar);
    cada_.start(10 * 60 * 1000);
    QTimer::singleShot(60 * 1000, this, &Reportes::enviar);
}

Reportes::~Reportes() {
    if (gReportes == this) { qInstallMessageHandler(gAnterior); gReportes = nullptr; }
    QFile::remove(marcaVivo_);   // se cerró bien
}

QByteArray Reportes::leerArranque() {
    QFile f(QStringLiteral("/proc/sys/kernel/random/boot_id"));
    return f.open(QIODevice::ReadOnly) ? f.readAll().trimmed() : QByteArray();
}

// Lo último que anotó el sistema: errores del arranque anterior (si se apagó toda la Raspberry) o de
// este (si solo se cayó Betómetro), y si el núcleo se colgó, lo que dejó en la memoria que sobrevive al
// reinicio (pstore, con dtoverlay=ramoops). Se suma al reporte de errores.
void Reportes::juntarSistema(bool arranqueAnterior) {
    QProcess j;
    j.start(QStringLiteral("journalctl"), {arranqueAnterior ? QStringLiteral("-b-1") : QStringLiteral("-b"), QStringLiteral("-p"), QStringLiteral("err"),
                                            QStringLiteral("-n"), QStringLiteral("20"), QStringLiteral("--no-pager"), QStringLiteral("-o"), QStringLiteral("short")});
    if (j.waitForFinished(4000)) {
        const QStringList lineas = QString::fromUtf8(j.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
        // Lo que ya se anotó no se repite: si Betómetro se cae varias veces en el mismo arranque, el sistema
        // devuelve las mismas líneas cada vez
        const QString archivoVisto = carpeta_ + QStringLiteral("/sistema-visto");
        QStringList visto;
        QFile fv(archivoVisto);
        if (fv.open(QIODevice::ReadOnly)) { visto = QString::fromUtf8(fv.readAll()).split('\n', Qt::SkipEmptyParts); fv.close(); }
        for (const QString& l : lineas) {
            const QString t = l.trimmed();
            if (t.startsWith(QLatin1String("--")) || esRuido(t) || visto.contains(t)) continue;
            anotar(QStringLiteral("sistema"), t);
            visto << t;
        }
        while (visto.size() > 300) visto.removeFirst();
        if (fv.open(QIODevice::WriteOnly | QIODevice::Truncate)) fv.write(visto.join('\n').toUtf8() + '\n');
    }
    const QDir pstore(QStringLiteral("/sys/fs/pstore"));
    for (const QString& n : pstore.entryList(QDir::Files)) {
        QFile f(pstore.filePath(n));
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QList<QByteArray> lineas = f.readAll().split('\n');
        for (qsizetype i = std::max<qsizetype>(0, lineas.size() - 25); i < lineas.size(); ++i)
            if (!lineas[i].trimmed().isEmpty()) anotar(QStringLiteral("cuelgue"), QString::fromUtf8(lineas[i]).trimmed());
        f.close();
        QFile::remove(pstore.filePath(n));   // ya se anotó: se libera para el próximo
    }
}

void Reportes::instalar() {
    gReportes = this;
    gAnterior = qInstallMessageHandler(manejador);
}

void Reportes::anotar(const QString& tipo, const QString& texto) {
    QMutexLocker l(&gMutex);
    QFile f(archivo_);
    if (f.size() > kMaxBytes) {   // se recorta: queda la mitad más nueva
        if (f.open(QIODevice::ReadOnly)) { const QByteArray d = f.readAll(); f.close(); QFile w(archivo_);
            if (w.open(QIODevice::WriteOnly | QIODevice::Truncate)) w.write(d.mid(d.size() / 2)); }
        QFile::remove(enviadoHasta_);
    }
    if (!f.open(QIODevice::Append)) return;
    const QString linea = QStringLiteral("%1 · v%2 · %3 · %4\n")
                              .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                                   QStringLiteral(BETO_VERSION), tipo, QString(texto).replace('\n', ' '));
    f.write(linea.toUtf8());
}

QByteArray Reportes::texto() const {
    QFile f(archivo_);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

void Reportes::enviar() {
    if (carrera_) return;   // nunca durante una carrera
    QFile t(carpeta_ + QStringLiteral("/../reportes.token"));
    if (!t.open(QIODevice::ReadOnly)) return;   // sin llave: quedan guardados (se ven en /reportes)
    const QByteArray llave = t.readAll().trimmed();
    if (llave.isEmpty()) return;
    const QByteArray todo = texto();
    QFile e(enviadoHasta_);
    qint64 desde = e.open(QIODevice::ReadOnly) ? e.readAll().trimmed().toLongLong() : 0;
    e.close();
    if (desde > todo.size()) desde = 0;
    const QByteArray nuevo = todo.mid(desde);
    if (nuevo.trimmed().isEmpty()) return;
    QNetworkRequest q{QUrl(QString::fromLatin1(kRepo))};
    q.setRawHeader("Authorization", "Bearer " + llave);
    q.setRawHeader("Accept", "application/vnd.github+json");
    q.setRawHeader("User-Agent", "Betometro/" BETO_VERSION);
    q.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    q.setTransferTimeout(20000);
    const QString titulo = QStringLiteral("Reporte v%1 · %2 · %3 líneas")
                               .arg(QStringLiteral(BETO_VERSION), QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")))
                               .arg(nuevo.count('\n'));
    const QJsonObject o{{"title", titulo},
                        {"body", QStringLiteral("Equipo: %1 · %2\n\n```\n%3\n```").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
                                                                                  QString::fromUtf8(nuevo.right(60000)))}};
    QNetworkReply* r = red_.post(q, QJsonDocument(o).toJson(QJsonDocument::Compact));
    const qint64 hasta = todo.size();
    connect(r, &QNetworkReply::finished, this, [this, r, hasta] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) return;   // sin internet: se reintenta más tarde
        QFile e(enviadoHasta_);
        if (e.open(QIODevice::WriteOnly | QIODevice::Truncate)) e.write(QByteArray::number(hasta));
    });
}
