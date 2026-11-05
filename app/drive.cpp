#include "drive.h"

#include "datos.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrl>

#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {
const QString kScope = QStringLiteral("https://www.googleapis.com/auth/drive.file");
const QString kCarpetaRaiz = QStringLiteral("Betómetro");
const QString kTipoCarpeta = QStringLiteral("application/vnd.google-apps.folder");
const QString kEnlace = QStringLiteral("https://www.google.com/device");
constexpr int kPausaMs = 1500;             // entre pedazo y pedazo: la Pi respira
constexpr int kCadaMs = 10 * 60 * 1000;    // se revisa cada 10 minutos
constexpr qint64 kTrozo = 256 * 1024;      // de a cuánto se le pasa el archivo a curl
const QString kLimite = QStringLiteral("2M");   // curl: 2 MB/s como mucho (no ahoga la wifi)

qint64 ahora() { return QDateTime::currentMSecsSinceEpoch(); }
QJsonObject leerJson(const QByteArray& b) { return QJsonDocument::fromJson(b).object(); }
QByteArray aJson(const QJsonObject& o) { return QJsonDocument(o).toJson(QJsonDocument::Compact); }
QString huella(const QByteArray& b) { return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha1).toHex()); }
// Texto para la búsqueda de Drive (q=…): las comillas y barras van escapadas
QString escapar(QString s) { return s.replace('\\', QLatin1String("\\\\")).replace('\'', QLatin1String("\\'")); }
QByteArray formulario(const QList<QPair<QString, QString>>& campos) {
    QByteArray r;
    for (const auto& [k, v] : campos) r += (r.isEmpty() ? "" : "&") + QUrl::toPercentEncoding(k) + '=' + QUrl::toPercentEncoding(v);
    return r;
}
QString conPreguntas(const QString& base, const QList<QPair<QString, QString>>& campos) {
    return base + (base.contains('?') ? '&' : '?') + QString::fromLatin1(formulario(campos));
}
// Archivo que solo lee el dueño (0600) desde el primer momento: se escribe aparte y se reemplaza
bool escribirPrivado(const QString& ruta, const QByteArray& d) {
    const QByteArray tmp = QFile::encodeName(ruta + QStringLiteral(".nuevo"));
    ::unlink(tmp.constData());
    const int fd = ::open(tmp.constData(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0) return false;
    qint64 hecho = 0;
    while (hecho < d.size()) {
        const ssize_t n = ::write(fd, d.constData() + hecho, static_cast<size_t>(d.size() - hecho));
        if (n <= 0) { ::close(fd); ::unlink(tmp.constData()); return false; }
        hecho += n;
    }
    ::fsync(fd);
    ::close(fd);
    return ::rename(tmp.constData(), QFile::encodeName(ruta).constData()) == 0;
}
}  // namespace

Drive::Drive(Datos* datos, const QString& carpetaDatos, const QString& prueba, QObject* padre)
    : QObject(padre), datos_(datos), carpeta_(carpetaDatos),
      archivoEstado_(carpetaDatos + QStringLiteral("/drive.json")), archivoCliente_(carpetaDatos + QStringLiteral("/drive-cliente.json")) {
    base_ = qEnvironmentVariable("BETO_DRIVE_SERVIDOR");   // solo para probar con un Google de mentira
    real_ = qEnvironmentVariableIsSet("BETO_RASPBERRY") || !base_.isEmpty();
    enlace_ = kEnlace;
    cargar();
    sondeo_.setSingleShot(true);
    connect(&sondeo_, &QTimer::timeout, this, &Drive::esperarCodigo);
    connect(&cada_, &QTimer::timeout, this, &Drive::revisar);
    cada_.start(kCadaMs);
    pronto_.setSingleShot(true);
    connect(&pronto_, &QTimer::timeout, this, &Drive::revisar);
    // Si cambia una carrera (vuelta medida o libre…), el informe se vuelve a subir
    connect(datos_, &Datos::carrerasChanged, this, [this] { huellas_.clear(); if (!trabajando_) pronto_.start(30000); });
    connect(datos_, &Datos::cfgChanged, this, [this] { if (!trabajando_) pronto_.start(3000); });

    // Capturas en la PC: «--prueba drive-codigo», «drive-subiendo», «drive-listo», «drive-esperando», «drive-error»
    QString p = prueba;
    if (p.startsWith(QLatin1String("tablet-"))) p = p.mid(7);
    if (!real_ && p.startsWith(QLatin1String("drive-"))) {
        pruebaFija_ = true;
        const QString q = p.mid(6);
        if (q == QLatin1String("falta")) { estado_ = QStringLiteral("falta"); }
        else if (q == QLatin1String("codigo")) { estado_ = QStringLiteral("codigo"); codigo_ = QStringLiteral("BXRT-KQWM"); }
        else {
            estado_ = QStringLiteral("conectado");
            cuenta_ = QStringLiteral("tvmaty2186@gmail.com");
            if (q == QLatin1String("subiendo")) { subiendo_ = true; progreso_ = 0.45; texto_ = QStringLiteral("Subiendo Santa Clara · video 45 %"); }
            else if (q == QLatin1String("esperando")) texto_ = QStringLiteral("Esperando wifi de casa para los videos (3 videos, 2,1 GB)");
            else if (q == QLatin1String("error")) { texto_ = QStringLiteral("Faltan subir 2 archivos"); error_ = QStringLiteral("Drive lleno"); }
            else { texto_ = QStringLiteral("Todo subido"); listo_ = true; }
        }
    }
}

Drive::~Drive() {
    if (curl_) { curl_->kill(); curl_->waitForFinished(1000); }
}

QString Drive::url(const QString& camino) const {
    // «oauth2/…» va a oauth2.googleapis.com; lo demás a www.googleapis.com
    if (!base_.isEmpty()) return base_ + '/' + camino;
    if (camino.startsWith(QLatin1String("oauth2/"))) return QStringLiteral("https://oauth2.googleapis.com/") + camino.mid(7);
    return QStringLiteral("https://www.googleapis.com/") + camino;
}

// ---------- Guardado ----------

void Drive::cargar() {
    QFile c(archivoCliente_);
    if (c.open(QIODevice::ReadOnly)) {
        QJsonObject o;
        if (drive::revisarCliente(c.readAll(), &o).isEmpty()) {
            clienteId_ = o.value("client_id").toString();
            clienteSecreto_ = o.value("client_secret").toString();
        }
    }
    QFile e(archivoEstado_);
    if (e.open(QIODevice::ReadOnly)) {
        estadoGuardado_ = leerJson(e.readAll());
        refresh_ = estadoGuardado_.take(QStringLiteral("refreshToken")).toString();
        cuenta_ = estadoGuardado_.take(QStringLiteral("cuenta")).toString();
    }
    estado_ = clienteId_.isEmpty() ? QStringLiteral("falta") : refresh_.isEmpty() ? QStringLiteral("desconectado") : QStringLiteral("conectado");
}

void Drive::guardar() const {
    QJsonObject o = estadoGuardado_;
    if (!refresh_.isEmpty()) o[QStringLiteral("refreshToken")] = refresh_;
    if (!cuenta_.isEmpty()) o[QStringLiteral("cuenta")] = cuenta_;
    if (!escribirPrivado(archivoEstado_, QJsonDocument(o).toJson(QJsonDocument::Indented)))
        qWarning("Drive: no pude guardar %s", qPrintable(archivoEstado_));
}

QString Drive::guardarCliente(const QByteArray& json) {
    QJsonObject limpio;
    const QString problema = drive::revisarCliente(json, &limpio);
    if (!problema.isEmpty()) return problema;
    if (!escribirPrivado(archivoCliente_, QJsonDocument(limpio).toJson(QJsonDocument::Indented))) return QStringLiteral("No pude guardarlo en la tarjeta");
    const bool otro = limpio.value("client_id").toString() != clienteId_;
    clienteId_ = limpio.value("client_id").toString();
    clienteSecreto_ = limpio.value("client_secret").toString();
    if (otro && !refresh_.isEmpty()) {   // con otro cliente, el permiso viejo ya no sirve
        parar();
        refresh_.clear(); access_.clear(); cuenta_.clear();
        guardar();
    }
    if (!pruebaFija_ && (estado_ == QLatin1String("falta") || otro))
        estado_ = refresh_.isEmpty() ? QStringLiteral("desconectado") : QStringLiteral("conectado");
    error_.clear();
    actualizarTexto();
    avisar();
    return {};
}

// ---------- Red ----------

QNetworkRequest Drive::req(const QString& u, bool conToken) const {
    QNetworkRequest r{QUrl(u)};
    if (conToken && !access_.isEmpty()) r.setRawHeader("Authorization", "Bearer " + access_.toUtf8());
    return r;
}

void Drive::pedir(const QString& verbo, const QNetworkRequest& r, const QByteArray& cuerpo, Listo listo, bool deCola) {
    QNetworkRequest q = r;
    q.setTransferTimeout(30000);
    QNetworkReply* rep = red_.sendCustomRequest(q, verbo.toLatin1(), cuerpo);
    if (deCola) enVuelo_ = rep;
    const int g = deCola ? gen_ : -1;
    connect(rep, &QNetworkReply::finished, this, [this, rep, g, listo] {
        rep->deleteLater();
        if (g >= 0 && g != gen_) return;   // se paró la subida
        listo(rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), rep->readAll(), rep);
    });
}

void Drive::conToken(std::function<void()> seguir, std::function<void(const QString&)> mal) {
    if (!access_.isEmpty() && ahora() < venceAccess_ - 60000) return seguir();
    if (refresh_.isEmpty()) return mal(QStringLiteral("Falta conectar: tocá Conectar"));
    QNetworkRequest r = req(url(QStringLiteral("oauth2/token")), false);
    r.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/x-www-form-urlencoded"));
    pedir(QStringLiteral("POST"), r,
          formulario({{"client_id", clienteId_}, {"client_secret", clienteSecreto_}, {"refresh_token", refresh_}, {"grant_type", "refresh_token"}}),
          [this, seguir, mal](int http, const QByteArray& cuerpo, QNetworkReply*) {
              const QJsonObject o = leerJson(cuerpo);
              if (http == 200 && !o.value("access_token").toString().isEmpty()) {
                  access_ = o.value("access_token").toString();
                  venceAccess_ = ahora() + o.value("expires_in").toInt(3600) * 1000LL;
                  return seguir();
              }
              if (cuerpo.contains("invalid_grant")) {   // le sacaron el permiso, o venció (app en «Testing»: 7 días)
                  refresh_.clear(); access_.clear();
                  estado_ = QStringLiteral("desconectado");
                  guardar();
              }
              mal(drive::errorEnPalabras(http, cuerpo));
          });
}

// ---------- Conectar ----------

void Drive::conectar() {
    if (clienteId_.isEmpty()) { estado_ = QStringLiteral("falta"); avisar(); return; }
    error_.clear();
    if (!real_) {   // PC: un código de mentira y a los 10 s «conectado»
        pruebaFija_ = false;
        estado_ = QStringLiteral("codigo"); codigo_ = QStringLiteral("BXRT-KQWM");
        avisar();
        QTimer::singleShot(10000, this, [this] {
            if (estado_ != QLatin1String("codigo")) return;
            estado_ = QStringLiteral("conectado"); cuenta_ = QStringLiteral("tvmaty2186@gmail.com"); codigo_.clear();
            actualizarTexto(); avisar();
        });
        return;
    }
    estado_ = QStringLiteral("pidiendo");
    avisar();
    QNetworkRequest r = req(url(QStringLiteral("oauth2/device/code")), false);
    r.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/x-www-form-urlencoded"));
    pedir(QStringLiteral("POST"), r, formulario({{"client_id", clienteId_}, {"scope", kScope}}),
          [this](int http, const QByteArray& cuerpo, QNetworkReply*) {
              if (estado_ != QLatin1String("pidiendo")) return;   // canceló
              const QJsonObject o = leerJson(cuerpo);
              if (http != 200 || o.value("user_code").toString().isEmpty()) {
                  estado_ = QStringLiteral("desconectado");
                  error_ = drive::errorEnPalabras(http, cuerpo);
                  avisar();
                  return;
              }
              deviceCode_ = o.value("device_code").toString();
              codigo_ = o.value("user_code").toString();
              enlace_ = o.value("verification_url").toString(o.value("verification_uri").toString(kEnlace));
              intervalo_ = qMax(1, o.value("interval").toInt(5));
              venceCodigo_ = ahora() + o.value("expires_in").toInt(1800) * 1000LL;
              estado_ = QStringLiteral("codigo");
              sondeo_.start(intervalo_ * 1000);
              avisar();
          }, false);
}

void Drive::esperarCodigo() {
    if (estado_ != QLatin1String("codigo")) return;
    if (ahora() > venceCodigo_) {
        estado_ = QStringLiteral("desconectado"); codigo_.clear();
        error_ = QStringLiteral("Se venció el código: tocá Conectar de nuevo");
        avisar();
        return;
    }
    QNetworkRequest r = req(url(QStringLiteral("oauth2/token")), false);
    r.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/x-www-form-urlencoded"));
    pedir(QStringLiteral("POST"), r,
          formulario({{"client_id", clienteId_}, {"client_secret", clienteSecreto_}, {"device_code", deviceCode_},
                      {"grant_type", "urn:ietf:params:oauth:grant-type:device_code"}}),
          [this](int http, const QByteArray& cuerpo, QNetworkReply*) {
              if (estado_ != QLatin1String("codigo")) return;
              const QJsonObject o = leerJson(cuerpo);
              const QString err = o.value("error").toString();
              if (http == 200 && !o.value("refresh_token").toString().isEmpty()) {
                  refresh_ = o.value("refresh_token").toString();
                  access_ = o.value("access_token").toString();
                  venceAccess_ = ahora() + o.value("expires_in").toInt(3600) * 1000LL;
                  estado_ = QStringLiteral("conectado"); codigo_.clear(); deviceCode_.clear(); error_.clear();
                  guardar();
                  buscarCuenta();
                  actualizarTexto();
                  avisar();
                  pronto_.start(3000);
                  return;
              }
              if (err == QLatin1String("slow_down")) intervalo_ += 5;   // Google pide ir más despacio
              if (err == QLatin1String("access_denied")) error_ = QStringLiteral("No diste permiso en el celu: tocá Conectar de nuevo");
              else if (err == QLatin1String("expired_token")) error_ = QStringLiteral("Se venció el código: tocá Conectar de nuevo");
              else if (!err.isEmpty() && err != QLatin1String("authorization_pending") && err != QLatin1String("slow_down"))
                  error_ = drive::errorEnPalabras(http, cuerpo);
              else { sondeo_.start(intervalo_ * 1000); return; }   // todavía no (o sin red un momento): se vuelve a preguntar
              estado_ = QStringLiteral("desconectado"); codigo_.clear();
              avisar();
          }, false);
}

void Drive::buscarCuenta() {
    conToken([this] {
        pedir(QStringLiteral("GET"), req(url(QStringLiteral("drive/v3/about?fields=user"))), {},
              [this](int http, const QByteArray& cuerpo, QNetworkReply*) {
                  if (http != 200) return;   // no importa: se vuelve a buscar en la próxima revisada
                  cuenta_ = leerJson(cuerpo).value("user").toObject().value("emailAddress").toString();
                  guardar();
                  avisar();
              }, false);
    }, [](const QString&) {});
}

void Drive::cancelar() {
    sondeo_.stop();
    if (pruebaFija_ || !real_) pruebaFija_ = false;
    estado_ = clienteId_.isEmpty() ? QStringLiteral("falta") : QStringLiteral("desconectado");
    codigo_.clear();
    avisar();
}

void Drive::desconectar() {
    parar();
    sondeo_.stop();
    if (real_ && !refresh_.isEmpty()) {   // le saca el permiso a Betómetro en la cuenta de Google
        QNetworkRequest r = req(url(QStringLiteral("oauth2/revoke")), false);
        r.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/x-www-form-urlencoded"));
        pedir(QStringLiteral("POST"), r, formulario({{"token", refresh_}}), [](int, const QByteArray&, QNetworkReply*) {}, false);
    }
    pruebaFija_ = false;
    refresh_.clear(); access_.clear(); cuenta_.clear(); codigo_.clear(); error_.clear(); texto_.clear();
    estadoGuardado_ = QJsonObject();   // otra cuenta tendría otras carpetas (si vuelve la misma, se buscan por nombre)
    listo_ = false; subiendo_ = false;
    estado_ = clienteId_.isEmpty() ? QStringLiteral("falta") : QStringLiteral("desconectado");
    guardar();
    QFile::remove(carpeta_ + QStringLiteral("/.drive-informe.html"));
    avisar();
}

// ---------- Cuándo sube ----------

bool Drive::puede() const {
    return real_ && estado_ == QLatin1String("conectado") && !ocupado_ && !red_ssid_.isEmpty() && !propia_;
}

drive::Opciones Drive::opciones() const {
    drive::Opciones op;
    op.informes = datos_->cfg(QStringLiteral("driveInformes")).toBool();
    op.videos = datos_->cfg(QStringLiteral("driveVideos")).toBool();
    op.videosPermitidos = drive::wifiParaVideos(red_ssid_, propia_, datos_->cfg(QStringLiteral("driveWifis")).toStringList());
    return op;
}

void Drive::ponerCondiciones(const QString& red, bool propia, bool ocupado) {
    if (red == red_ssid_ && propia == propia_ && ocupado == ocupado_) return;
    const bool antes = puede(), estabaOcupado = ocupado_;
    red_ssid_ = red; propia_ = propia; ocupado_ = ocupado;
    if (antes && !puede()) parar();   // empezó una carrera, graba la cámara o se fue la wifi: se para ya
    else if (trabajando_ && tarea_.tipo == drive::Tarea::Video && !opciones().videosPermitidos) { parar(); pronto_.start(5000); }
    else if (!antes && puede()) pronto_.start(estabaOcupado && !ocupado ? 20000 : 5000);   // terminó la carrera o conectó la wifi
    actualizarTexto();
    avisar();
}

void Drive::revisar() {
    if (pruebaFija_) return;
    if (puede() && cuenta_.isEmpty()) buscarCuenta();
    actualizarTexto();
    avisar();
    if (puede() && !trabajando_) seguirCola();
}

QList<drive::CarreraLocal> Drive::carrerasLocales() {
    QList<drive::CarreraLocal> r;
    for (const QVariant& v : datos_->corridas() + datos_->proximas()) {
        const QVariantMap m = v.toMap();
        if (!m.value("tieneVueltas").toBool()) continue;
        drive::CarreraLocal c;
        c.id = m.value("id").toString();
        c.nombre = m.value("nombre").toString();
        c.carpeta = drive::nombreCarpeta(m.value("fecha").toString(), c.nombre);
        if (!huellas_.contains(c.id)) huellas_.insert(c.id, huella(datos_->informeHtml(c.id)));
        c.versionInforme = huellas_.value(c.id);
        for (const QVariant& x : datos_->videos(c.id)) {
            const QString a = x.toMap().value("archivo").toString();
            const QFileInfo f(datos_->carpetaVideos() + '/' + a);
            if (Datos::nombreVideoValido(a) && f.isFile()) c.videos << drive::VideoLocal{a, f.size()};
        }
        r << c;
    }
    return r;
}

void Drive::actualizarTexto() {
    if (pruebaFija_) return;
    listo_ = false;
    if (estado_ != QLatin1String("conectado")) { texto_.clear(); return; }
    if (subiendo_) {
        texto_ = QStringLiteral("Subiendo %1 · %2").arg(tarea_.nombre, tarea_.tipo == drive::Tarea::Informe
                     ? QStringLiteral("informe") : QStringLiteral("video %1 %").arg(qRound(progreso_ * 100)));
        return;
    }
    if (ocupado_) { texto_ = QStringLiteral("Se sube después de la carrera"); return; }   // en carrera no se calcula nada
    const drive::Pendientes p = drive::pendientes(estadoGuardado_.value("carreras").toObject(), carrerasLocales(), opciones());
    const QString esperando = p.videosEsperando
        ? QStringLiteral("Esperando wifi de casa para los videos (%1 %2, %3)").arg(p.videosEsperando)
              .arg(p.videosEsperando == 1 ? QStringLiteral("video") : QStringLiteral("videos"), drive::textoBytes(p.bytesEsperando))
        : QString();
    if (p.tareas.isEmpty()) { texto_ = esperando.isEmpty() ? QStringLiteral("Todo subido") : esperando; listo_ = esperando.isEmpty(); return; }
    const QString falta = p.tareas.size() == 1 ? QStringLiteral("Falta subir 1 archivo") : QStringLiteral("Faltan subir %1 archivos").arg(p.tareas.size());
    texto_ = red_ssid_.isEmpty() || propia_ ? falta + QStringLiteral(": se sube cuando haya wifi") : falta;
}

// ---------- La cola ----------

void Drive::borrarSubidos() {
    if (!borrador_ || !datos_->cfg(QStringLiteral("driveBorrarSubidos")).toBool()) return;
    const QJsonObject carreras = estadoGuardado_.value("carreras").toObject();
    for (auto c = carreras.begin(); c != carreras.end(); ++c) {
        const QJsonObject videos = c.value().toObject().value("videos").toObject();
        for (auto v = videos.begin(); v != videos.end(); ++v) {
            const QFileInfo f(datos_->carpetaVideos() + '/' + v.key());
            // Solo si en Drive quedó entero (mismo tamaño)
            if (f.isFile() && f.size() == v.value().toObject().value("bytes").toInteger(-1)) borrador_(v.key());
        }
    }
}

void Drive::seguirCola() {
    if (!puede() || trabajando_) return;
    borrarSubidos();
    const drive::Pendientes p = drive::pendientes(estadoGuardado_.value("carreras").toObject(), carrerasLocales(), opciones());
    if (p.tareas.isEmpty()) { actualizarTexto(); avisar(); return; }
    trabajando_ = true;
    tarea_ = p.tareas.first();
    reintento401_ = false;
    conToken([this] { conCarpeta(tarea_, [this](const QString& carpeta) { subir(tarea_, carpeta); }); },
             [this](const QString& e) { fallar(e); });
}

void Drive::conCarpeta(const drive::Tarea& t, std::function<void(const QString&)> seguir) {
    const QString yaEsta = estadoGuardado_.value("carreras").toObject().value(t.carrera).toObject().value("carpeta").toString();
    if (!yaEsta.isEmpty()) return seguir(yaEsta);   // aunque se haya cambiado el nombre: misma carpeta
    const QString carrera = t.carrera, nombre = t.carpeta;
    auto enRaiz = [this, carrera, nombre, seguir](const QString& raiz) {
        buscarOCrearCarpeta(nombre, raiz, [this, carrera, seguir](const QString& id) {
            QJsonObject cs = estadoGuardado_.value("carreras").toObject();
            QJsonObject e = cs.value(carrera).toObject();
            e[QStringLiteral("carpeta")] = id;
            cs[carrera] = e;
            estadoGuardado_[QStringLiteral("carreras")] = cs;
            guardar();
            seguir(id);
        });
    };
    const QString raiz = estadoGuardado_.value("raiz").toString();
    if (!raiz.isEmpty()) return enRaiz(raiz);
    buscarOCrearCarpeta(kCarpetaRaiz, QStringLiteral("root"), [this, enRaiz](const QString& id) {
        estadoGuardado_[QStringLiteral("raiz")] = id;
        guardar();
        enRaiz(id);
    });
}

void Drive::buscarOCrearCarpeta(const QString& nombre, const QString& padre, std::function<void(const QString&)> seguir) {
    // Primero se busca (si ya la había creado Betómetro), así no queda repetida
    const QString q = QStringLiteral("name = '%1' and mimeType = '%2' and '%3' in parents and trashed = false")
                          .arg(escapar(nombre), kTipoCarpeta, escapar(padre));
    pedir(QStringLiteral("GET"), req(conPreguntas(url(QStringLiteral("drive/v3/files")), {{"q", q}, {"fields", "files(id)"}, {"spaces", "drive"}})), {},
          [this, nombre, padre, seguir](int http, const QByteArray& cuerpo, QNetworkReply*) {
              if (http != 200) return fallar(drive::errorEnPalabras(http, cuerpo));
              const QJsonArray fs = leerJson(cuerpo).value("files").toArray();
              if (!fs.isEmpty()) return seguir(fs.first().toObject().value("id").toString());
              QNetworkRequest r = req(url(QStringLiteral("drive/v3/files?fields=id")));
              r.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json; charset=UTF-8"));
              pedir(QStringLiteral("POST"), r, aJson({{"name", nombre}, {"mimeType", kTipoCarpeta}, {"parents", QJsonArray{padre}}}),
                    [this, seguir](int http2, const QByteArray& c2, QNetworkReply*) {
                        const QString id = leerJson(c2).value("id").toString();
                        if (http2 != 200 || id.isEmpty()) return fallar(drive::errorEnPalabras(http2, c2));
                        seguir(id);
                    });
          });
}

namespace {
QString claveDe(const drive::Tarea& t) {
    return t.tipo == drive::Tarea::Informe ? QStringLiteral("informe:") + t.carrera : QStringLiteral("video:") + t.archivo;
}
}  // namespace

void Drive::subir(const drive::Tarea& t, const QString& carpeta) {
    carpetaTarea_ = carpeta;
    if (t.tipo == drive::Tarea::Informe) {
        const QByteArray html = datos_->informeHtml(t.carrera);
        rutaTarea_ = carpeta_ + QStringLiteral("/.drive-informe.html");
        QFile f(rutaTarea_);
        if (html.isEmpty() || !f.open(QIODevice::WriteOnly | QIODevice::Truncate) || f.write(html) != html.size())
            return fallar(QStringLiteral("No pude armar el informe"));
        f.close();
        versionTarea_ = huella(html);
        total_ = html.size();
        mime_ = QStringLiteral("text/html");
        nombreArchivo_ = datos_->nombreInforme(t.carrera);
    } else {
        rutaTarea_ = datos_->carpetaVideos() + '/' + t.archivo;
        total_ = QFileInfo(rutaTarea_).size();
        versionTarea_.clear();
        mime_ = t.archivo.endsWith(QLatin1String(".mkv"), Qt::CaseInsensitive) ? QStringLiteral("video/x-matroska") : QStringLiteral("video/mp4");
        nombreArchivo_ = t.archivo;
        if (total_ <= 0) { huellas_.clear(); return fallar(QStringLiteral("No está el video ") + t.archivo); }
    }
    subiendo_ = true;
    progreso_ = 0;
    actualizarTexto();
    avisar();
    // ¿Quedó cortada una subida de este mismo archivo? Se pregunta dónde quedó y se sigue de ahí.
    const QJsonObject s = estadoGuardado_.value("sesion").toObject();
    if (s.value("clave").toString() == claveDe(t) && s.value("bytes").toInteger() == total_ && !s.value("url").toString().isEmpty() &&
        s.value("version").toString() == versionTarea_)
        return preguntarSesion();
    if (t.tipo == drive::Tarea::Informe && !t.idDrive.isEmpty()) return empezarSesion();   // se reemplaza el que está
    // ¿Ya está en Drive? (si se borró drive.json o se volvió a conectar la misma cuenta)
    const QString q = QStringLiteral("name = '%1' and '%2' in parents and trashed = false").arg(escapar(nombreArchivo_), escapar(carpeta));
    pedir(QStringLiteral("GET"), req(conPreguntas(url(QStringLiteral("drive/v3/files")), {{"q", q}, {"fields", "files(id,size)"}, {"spaces", "drive"}})), {},
          [this](int http, const QByteArray& cuerpo, QNetworkReply*) {
              if (http == 404) { estadoGuardado_.remove(QStringLiteral("raiz")); olvidarCarpeta(); return fallar(QStringLiteral("La carpeta de Drive no está: se vuelve a crear")); }
              if (http != 200) return fallar(drive::errorEnPalabras(http, cuerpo));
              const QJsonArray fs = leerJson(cuerpo).value("files").toArray();
              const QJsonObject f = fs.isEmpty() ? QJsonObject() : fs.first().toObject();
              if (!f.isEmpty() && tarea_.tipo == drive::Tarea::Video && f.value("size").toString().toLongLong() == total_)
                  return terminoArchivo(aJson({{"id", f.value("id")}}));   // ya estaba entero
              if (!f.isEmpty() && tarea_.tipo == drive::Tarea::Informe) tarea_.idDrive = f.value("id").toString();
              empezarSesion();
          });
}

void Drive::olvidarCarpeta() {
    QJsonObject cs = estadoGuardado_.value("carreras").toObject();
    QJsonObject e = cs.value(tarea_.carrera).toObject();
    e.remove(QStringLiteral("carpeta"));
    cs[tarea_.carrera] = e;
    estadoGuardado_[QStringLiteral("carreras")] = cs;
    guardar();
}

void Drive::empezarSesion() {
    // Subida «resumable»: Google da una dirección para esta subida; después se manda de a pedazos
    const bool cambiar = !tarea_.idDrive.isEmpty();   // informe que cambió: mismo archivo de Drive (PATCH)
    const QString u = cambiar ? url(QStringLiteral("upload/drive/v3/files/") + tarea_.idDrive + QStringLiteral("?uploadType=resumable&fields=id"))
                              : url(QStringLiteral("upload/drive/v3/files?uploadType=resumable&fields=id"));
    QJsonObject meta{{"name", nombreArchivo_}};
    if (!cambiar) meta[QStringLiteral("parents")] = QJsonArray{carpetaTarea_};
    QNetworkRequest r = req(u);
    r.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json; charset=UTF-8"));
    r.setRawHeader("X-Upload-Content-Type", mime_.toLatin1());
    r.setRawHeader("X-Upload-Content-Length", QByteArray::number(total_));
    pedir(cambiar ? QStringLiteral("PATCH") : QStringLiteral("POST"), r, aJson(meta),
          [this, cambiar](int http, const QByteArray& cuerpo, QNetworkReply* rep) {
              const QString lugar = QString::fromUtf8(rep->rawHeader("Location"));
              if (http == 200 && !lugar.isEmpty()) {
                  estadoGuardado_[QStringLiteral("sesion")] = QJsonObject{{"clave", claveDe(tarea_)}, {"url", lugar}, {"bytes", total_},
                                                                         {"version", versionTarea_}};
                  guardar();
                  cortes_ = 0;
                  return mandarPedazo(0);
              }
              if (http == 401 && !reintento401_) {   // el permiso de una hora venció antes: se pide otro
                  reintento401_ = true; access_.clear();
                  return conToken([this] { empezarSesion(); }, [this](const QString& e) { fallar(e); });
              }
              if (http == 404 && cambiar) {   // borraron el informe en Drive: se sube uno nuevo
                  tarea_.idDrive.clear();
                  return empezarSesion();
              }
              if (http == 404) {   // borraron la carpeta: se vuelve a crear en la próxima
                  estadoGuardado_.remove(QStringLiteral("raiz"));
                  olvidarCarpeta();
                  fallar(QStringLiteral("La carpeta de Drive no está: se vuelve a crear"));
                  pronto_.start(5000);
                  return;
              }
              fallar(drive::errorEnPalabras(http, cuerpo));
          });
}

void Drive::preguntarSesion() {
    // «¿Hasta dónde llegó?»: PUT vacío con «Content-Range: bytes */total»
    QNetworkRequest r = req(estadoGuardado_.value("sesion").toObject().value("url").toString());
    r.setRawHeader("Content-Range", "bytes */" + QByteArray::number(total_));
    r.setHeader(QNetworkRequest::ContentLengthHeader, 0);
    pedir(QStringLiteral("PUT"), r, {}, [this](int http, const QByteArray& cuerpo, QNetworkReply* rep) {
        if (http == 200 || http == 201) return terminoArchivo(cuerpo);
        if (http == 308) {
            qint64 sig = 0;
            if (drive::siguienteByte(rep->rawHeader("Range"), sig)) return mandarPedazo(sig);
        }
        if (http == 308 || http == 404 || http == 410) {   // la subida venció (dura una semana): se empieza de nuevo
            estadoGuardado_.remove(QStringLiteral("sesion"));
            guardar();
            return empezarSesion();
        }
        fallar(drive::errorEnPalabras(http, cuerpo));
    });
}

void Drive::mandarPedazo(qint64 desde) {
    if (!puede()) return fallar(QString());
    if (desde >= total_) return preguntarSesion();
    progreso_ = static_cast<double>(desde) / static_cast<double>(total_);
    actualizarTexto();
    avisar();
    const qint64 hasta = qMin(desde + drive::kPedazo, total_) - 1;
    delete leyendo_;
    leyendo_ = new QFile(rutaTarea_, this);
    if (!leyendo_->open(QIODevice::ReadOnly) || !leyendo_->seek(desde)) {
        estadoGuardado_.remove(QStringLiteral("sesion"));
        huellas_.clear();
        return fallar(QStringLiteral("No pude leer ") + nombreArchivo_);
    }
    faltaMandar_ = hasta - desde + 1;
    // La dirección de la subida y el permiso van en un archivo para curl (solo del dueño), no en la línea de
    // comandos, que la puede ver cualquiera
    const QString cfg = carpeta_ + QStringLiteral("/.drive-curl");
    const QByteArray conf = "url = \"" + estadoGuardado_.value("sesion").toObject().value("url").toString().toUtf8() + "\"\n"
                            "header = \"Authorization: Bearer " + access_.toUtf8() + "\"\n";
    if (!escribirPrivado(cfg, conf)) return fallar(QStringLiteral("No pude escribir en la tarjeta"));
    QStringList a{QStringLiteral("-sS"), QStringLiteral("-i"), QStringLiteral("-X"), QStringLiteral("PUT"),
                  QStringLiteral("--config"), cfg, QStringLiteral("--data-binary"), QStringLiteral("@-"),
                  QStringLiteral("--limit-rate"), kLimite, QStringLiteral("--connect-timeout"), QStringLiteral("20"),
                  QStringLiteral("--max-time"), QStringLiteral("600"),
                  QStringLiteral("-H"), QStringLiteral("Expect:"), QStringLiteral("-H"), QStringLiteral("Content-Type: ") + mime_,
                  QStringLiteral("-H"), QStringLiteral("Content-Range: bytes %1-%2/%3").arg(desde).arg(hasta).arg(total_)};
    if (!base_.isEmpty()) a << QStringLiteral("--noproxy") << QStringLiteral("*");   // Google de mentira en la PC
    curl_ = new QProcess(this);
    // Prioridad baja, como nmcli y la cámara: primero GPS, cronómetro y pantalla. Y la tarjeta, cuando nadie la usa.
    curl_->setChildProcessModifier([] {
        [[maybe_unused]] const int r = ::nice(10);
        ::syscall(SYS_ioprio_set, 1 /* IOPRIO_WHO_PROCESS */, 0, 3 << 13 /* IOPRIO_CLASS_IDLE */);
    });
    const int g = gen_;
    connect(curl_, &QProcess::bytesWritten, this, [this, g] { if (g == gen_) alimentar(); });
    connect(curl_, &QProcess::errorOccurred, this, [this, g](QProcess::ProcessError e) {
        if (g == gen_ && e == QProcess::FailedToStart) fallar(QStringLiteral("Falta curl en el equipo"));
    });
    connect(curl_, &QProcess::finished, this, [this, g, desde, cfg](int codigo, QProcess::ExitStatus) {
        if (g != gen_ || !curl_) return;
        QProcess* p = curl_;
        curl_ = nullptr;
        p->deleteLater();
        delete leyendo_;
        leyendo_ = nullptr;
        QFile::remove(cfg);
        int http = 0;
        QByteArray range, cuerpo;
        if (codigo != 0 || !drive::leerRespuesta(p->readAllStandardOutput(), http, range, cuerpo)) {
            // Se cortó (wifi, Google…): se pregunta hasta dónde llegó y se sigue. Muchos cortes seguidos: más tarde.
            if (++cortes_ > 3) return fallar(QStringLiteral("Se cortó la subida: se sigue más tarde"));
            QTimer::singleShot(kPausaMs * 4, this, [this, g] { if (g == gen_) preguntarSesion(); });
            return;
        }
        if (http == 308) {
            qint64 sig = desde;
            if (!drive::siguienteByte(range, sig) || sig <= desde) {
                if (++cortes_ > 3) return fallar(QStringLiteral("Google no recibe el video: se sigue más tarde"));
            } else cortes_ = 0;
            QTimer::singleShot(kPausaMs, this, [this, g, sig] { if (g == gen_) mandarPedazo(sig); });
            return;
        }
        if (http == 200 || http == 201) return terminoArchivo(cuerpo);
        if (http == 404 || http == 410) {
            estadoGuardado_.remove(QStringLiteral("sesion"));
            guardar();
            fallar(QStringLiteral("La subida venció: se empieza de nuevo"));
            pronto_.start(5000);
            return;
        }
        if (http == 401 && !reintento401_) {
            reintento401_ = true; access_.clear();
            return conToken([this, desde] { mandarPedazo(desde); }, [this](const QString& e) { fallar(e); });
        }
        fallar(drive::errorEnPalabras(http, cuerpo));
    });
    curl_->start(QStringLiteral("curl"), a);
    alimentar();
}

void Drive::alimentar() {
    if (!curl_ || !leyendo_ || curl_->property("cerrado").toBool()) return;
    while (faltaMandar_ > 0 && curl_->bytesToWrite() < kTrozo) {
        const QByteArray b = leyendo_->read(qMin(kTrozo, faltaMandar_));
        if (b.isEmpty()) { curl_->kill(); return; }   // el archivo se achicó: se corta y se pregunta
        curl_->write(b);
        faltaMandar_ -= b.size();
    }
    if (faltaMandar_ == 0) { curl_->setProperty("cerrado", true); curl_->closeWriteChannel(); }
}

void Drive::terminoArchivo(const QByteArray& cuerpo) {
    const QString id = leerJson(cuerpo).value("id").toString();
    if (id.isEmpty()) return fallar(QStringLiteral("Google no devolvió el archivo"));
    QJsonObject cs = estadoGuardado_.value("carreras").toObject();
    QJsonObject e = cs.value(tarea_.carrera).toObject();
    if (tarea_.tipo == drive::Tarea::Informe) {
        e[QStringLiteral("informe")] = QJsonObject{{"id", id}, {"version", versionTarea_}};
        QFile::remove(rutaTarea_);
    } else {
        QJsonObject vs = e.value("videos").toObject();
        vs[tarea_.archivo] = QJsonObject{{"id", id}, {"bytes", total_}};
        e[QStringLiteral("videos")] = vs;
    }
    cs[tarea_.carrera] = e;
    estadoGuardado_[QStringLiteral("carreras")] = cs;
    estadoGuardado_.remove(QStringLiteral("sesion"));
    guardar();
    trabajando_ = false; subiendo_ = false; progreso_ = 0; cortes_ = 0;
    error_.clear();
    // «Borrar del equipo los videos ya subidos»: por la cámara, que nunca borra el que está grabando
    if (tarea_.tipo == drive::Tarea::Video && borrador_ && datos_->cfg(QStringLiteral("driveBorrarSubidos")).toBool()) borrador_(tarea_.archivo);
    actualizarTexto();
    avisar();
    const int g = gen_;
    QTimer::singleShot(kPausaMs, this, [this, g] { if (g == gen_) seguirCola(); });
}

void Drive::fallar(const QString& texto) {
    if (curl_) { curl_->disconnect(this); curl_->kill(); curl_->deleteLater(); curl_ = nullptr; }
    delete leyendo_;
    leyendo_ = nullptr;
    if (!texto.isEmpty()) error_ = texto;
    if (!texto.isEmpty()) qWarning("Google Drive: %s", qPrintable(texto));
    trabajando_ = false; subiendo_ = false; progreso_ = 0;
    actualizarTexto();
    avisar();
}

void Drive::parar() {
    ++gen_;   // lo que estaba en camino ya no hace nada
    if (enVuelo_) enVuelo_->abort();
    fallar(QString());
}
