#include "prueba_completa.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QStorageInfo>
#include <unistd.h>

#include "audio.h"
#include "camara.h"
#include "contexto.h"
#include "crono.h"
#include "datos.h"
#include "drive.h"
#include "pulsador_gpio.h"
#include "reportes.h"
#include "salud.h"
#include "simulacion.h"
#include "video_gst.h"
#include "wifi.h"

namespace {
const QString kOk = QStringLiteral("ok"), kLeve = QStringLiteral("leve"), kGrave = QStringLiteral("grave"),
              kSaltado = QStringLiteral("saltado"), kCorriendo = QStringLiteral("corriendo");

QString coma(double v, int dec = 1) { return QString::number(v, 'f', dec).replace('.', ','); }
QByteArray leerArchivo(const QString& ruta, qint64 max = 64 * 1024) {
    QFile f(ruta);
    return f.open(QIODevice::ReadOnly) ? f.read(max) : QByteArray();
}
int temperaturaC() {
    const QByteArray t = leerArchivo(QStringLiteral("/sys/class/thermal/thermal_zone0/temp")).trimmed();
    return t.isEmpty() ? -1 : t.toInt() / 1000;
}
unsigned frenoAhora() {   // get_throttled, el mismo archivo que Salud
    return Salud::leerEstado(leerArchivo(QStringLiteral("/sys/devices/platform/soc/soc:firmware/get_throttled")));
}
QString frenoTexto(unsigned f) {
    if (!f) return QStringLiteral("sin freno");
    QStringList l;
    if (f & 0x1) l << QStringLiteral("tensión baja ahora");
    if (f & 0x8) l << QStringLiteral("frenado por calor ahora");
    if (f & 0x10000) l << QStringLiteral("hubo tensión baja");
    if (f & 0x80000) l << QStringLiteral("hubo freno por calor");
    return QStringLiteral("get_throttled 0x%1 (%2)").arg(f, 0, 16).arg(l.join(QStringLiteral(", ")));
}
// Ticks de procesador (usuario + sistema) de un proceso, de /proc/<pid>/stat. −1 si no se puede.
qint64 ticks(qint64 pid) {
    const QByteArray s = leerArchivo(QStringLiteral("/proc/%1/stat").arg(pid));
    const int fin = s.lastIndexOf(')');
    if (fin < 0) return -1;
    const QList<QByteArray> c = s.mid(fin + 2).split(' ');   // c[0] es el estado (campo 3); utime y stime son los campos 14 y 15
    if (c.size() < 13) return -1;
    return c[11].toLongLong() + c[12].toLongLong();
}
QString mb(qint64 bytes) { return coma(bytes / 1048576.0, bytes >= 100 * 1048576 ? 0 : 1) + QStringLiteral(" MB"); }
QString gb(qint64 bytes) { return coma(bytes / 1073741824.0, 1) + QStringLiteral(" GB"); }
QString tam(qint64 bytes) { return bytes >= 1073741824 ? gb(bytes) : mb(bytes); }
}  // namespace

PruebaCompleta::PruebaCompleta(const Piezas& p, QObject* padre) : QObject(padre), p_(p) {
    cuenta_.setInterval(1000);
    connect(&cuenta_, &QTimer::timeout, this, [this] {
        if (pregunta_.isEmpty()) { cuenta_.stop(); return; }
        if (--segundos_ > 0) { emit cambio(); return; }
        const QString que = pregunta_;
        pregunta_.clear();
        cuenta_.stop();
        terminarPaso(kLeve, que == QLatin1String("voz") ? QStringLiteral("sin respuesta: no se sabe si se escuchó")
                                                        : QStringLiteral("sin probar: no se apretó el pulsador en 20 s"));
    });
    tope_.setSingleShot(true);
    connect(&tope_, &QTimer::timeout, this, [this] {
        pregunta_.clear();
        terminarPaso(kLeve, QStringLiteral("no terminó a tiempo (%1 s): se siguió con el resto").arg(relojPaso_.elapsed() / 1000));
    });
    carga_.setInterval(1000);
    connect(&carga_, &QTimer::timeout, this, &PruebaCompleta::medirCarga);
    if (p_.pulsador)
        connect(p_.pulsador, &PulsadorGpio::marca, this, [this](double edadMs) {
            pulsadorRecibido(QStringLiteral("el pulsador por cable (%1 ms de demora)").arg(qRound(edadMs)));
        });
}

bool PruebaCompleta::rapida() const { return qEnvironmentVariableIsSet("BETO_PRUEBA_RAPIDA"); }
bool PruebaCompleta::raspberry() const { return qEnvironmentVariableIsSet("BETO_RASPBERRY"); }

QVariantList PruebaCompleta::pasos() const {
    QVariantList l;
    for (const Paso& p : pasos_) l << QVariantMap{{"nombre", p.nombre}, {"estado", p.estado}, {"detalle", p.detalle}, {"ms", p.ms}};
    return l;
}

int PruebaCompleta::fallas() const {
    int n = 0;
    for (const Paso& p : pasos_) if (p.estado == kLeve || p.estado == kGrave) ++n;
    return n;
}

QString PruebaCompleta::resumen() const {
    if (estado_.isEmpty()) return QStringLiteral("Todavía no se corrió");
    if (estado_ == kCorriendo) return QStringLiteral("Paso %1 de %2 · %3").arg(paso()).arg(total()).arg(actual_ >= 0 ? pasos_[static_cast<size_t>(actual_)].nombre : QString());
    const int f = fallas();
    if (f == 0) return QStringLiteral("Los %1 pasos, bien").arg(total());
    return QStringLiteral("%1 de %2 pasos con %3").arg(f).arg(total()).arg(estado_ == kGrave ? QStringLiteral("errores") : QStringLiteral("avisos"));
}

QString PruebaCompleta::porQueNo() const {
    if (estado_ == kCorriendo) return QStringLiteral("La prueba ya está corriendo");
    if (p_.crono && p_.crono->enCurso()) return QStringLiteral("Hay una carrera en curso: la prueba no se puede correr");
    return {};
}

bool PruebaCompleta::correr() {
    if (!porQueNo().isEmpty()) return false;
    pasos_.clear();
    auto agregar = [this](const QString& nombre, void (PruebaCompleta::*f)()) { Paso p; p.nombre = nombre; p.hacer = [this, f] { (this->*f)(); }; pasos_.push_back(std::move(p)); };
    agregar(QStringLiteral("Sistema"), &PruebaCompleta::pasoSistema);
    agregar(QStringLiteral("Simulador de carrera"), &PruebaCompleta::pasoSimulador);
    agregar(QStringLiteral("Voz"), &PruebaCompleta::pasoVoz);
    agregar(QStringLiteral("Pulsador"), &PruebaCompleta::pasoPulsador);
    agregar(QStringLiteral("GPS"), &PruebaCompleta::pasoGps);
    agregar(QStringLiteral("Cámara"), &PruebaCompleta::pasoCamara);
    agregar(QStringLiteral("Rendimiento grabando"), &PruebaCompleta::pasoRendimiento);
    agregar(QStringLiteral("Reproducción del video"), &PruebaCompleta::pasoReproduccion);
    agregar(QStringLiteral("Tarjeta"), &PruebaCompleta::pasoTarjeta);
    agregar(QStringLiteral("Wifi e internet"), &PruebaCompleta::pasoWifi);
    agregar(QStringLiteral("Google Drive"), &PruebaCompleta::pasoDrive);
    agregar(QStringLiteral("Pantallas"), &PruebaCompleta::pasoPantallas);
    estado_ = kCorriendo;
    actual_ = -1;
    cuando_.clear();
    haciendo_.clear();
    pregunta_.clear();
    videoPrueba_.clear();
    camaraMedida_.clear();
    carga_m_ = Carga{};
    relojTotal_.start();
    empezoTotal_ = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    emit cambio();
    emit empezo();
    QTimer::singleShot(50, this, &PruebaCompleta::siguiente);   // que la pantalla se muestre primero
    return true;
}

void PruebaCompleta::siguiente() {
    tope_.stop();
    cuenta_.stop();
    carga_.stop();
    disconnect(espera_);
    if (vivo_) { vivo_->deleteLater(); vivo_ = nullptr; }
    pregunta_.clear();
    haciendo_.clear();
    ++vuelta_;
    ++actual_;
    if (actual_ >= static_cast<int>(pasos_.size())) {
        bool grave = false, leve = false;
        for (const Paso& p : pasos_) { if (p.estado == kGrave) grave = true; if (p.estado == kLeve) leve = true; }
        estado_ = grave ? kGrave : leve ? kLeve : kOk;
        cuando_ = QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy HH:mm"));
        duracionS_ = relojTotal_.elapsed() / 1000;
        guardar();
        if (p_.reportes) {
            // Con la hora de inicio y fin de la prueba y de cada paso (Matías, 07/11/2026)
            p_.reportes->anotar(QStringLiteral("prueba"), QStringLiteral("Prueba completa: %1 (de %2 a %3, duró %4 s)")
                .arg(resumen(), empezoTotal_, QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))).arg(duracionS_));
            for (const Paso& p : pasos_)
                p_.reportes->anotar(QStringLiteral("prueba"), QStringLiteral("%1 [%2] · %3 · %4 s: %5")
                    .arg(p.nombre, p.estado, p.empezo.isEmpty() ? QStringLiteral("--:--:--") : p.empezo, QString::number(p.ms / 1000.0, 'f', 1).replace('.', ','), p.detalle));
            if (p_.reportes->hayLlave()) p_.reportes->enviarAhora();
        }
        emit cambio();
        emit termino();
        return;
    }
    Paso& p = pasos_[static_cast<size_t>(actual_)];
    p.estado = kCorriendo;
    p.empezo = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    relojPaso_.start();
    tope_.start(rapida() ? 20000 : 120000);
    vivo_ = new QObject(this);
    emit cambio();
    QTimer::singleShot(0, this, [this, v = vuelta_] { if (v == vuelta_) pasos_[static_cast<size_t>(actual_)].hacer(); });
}

void PruebaCompleta::terminarPaso(const QString& estado, const QString& detalle) {
    if (estado_ != kCorriendo || actual_ < 0 || actual_ >= static_cast<int>(pasos_.size())) return;
    Paso& p = pasos_[static_cast<size_t>(actual_)];
    if (p.estado != kCorriendo) return;
    p.estado = estado;
    p.detalle = detalle;
    p.ms = relojPaso_.elapsed();
    QTimer::singleShot(0, this, &PruebaCompleta::siguiente);
}

void PruebaCompleta::preguntar(const QString& que, int segundos) {
    pregunta_ = que;
    segundos_ = rapida() ? 2 : segundos;
    cuenta_.start();
    emit cambio();
}

void PruebaCompleta::responder(bool si) {
    if (pregunta_.isEmpty()) return;
    const QString que = pregunta_;
    pregunta_.clear();
    cuenta_.stop();
    if (que == QLatin1String("voz")) {
        const QStringList faltan = sistema_.value("vozFaltan").toStringList();
        const QString det = QStringLiteral("%1 · volumen %2 % · %3")
                                .arg(si ? QStringLiteral("se escuchó la prueba") : QStringLiteral("NO se escuchó"))
                                .arg(p_.datos ? qRound(p_.datos->cfg(QStringLiteral("volumen")).toDouble()) : 0)
                                .arg(faltan.isEmpty() ? QStringLiteral("están los %1 audios").arg(sistema_.value("vozEsperados").toInt())
                                                      : QStringLiteral("faltan %1 audios: %2").arg(faltan.size()).arg(faltan.mid(0, 6).join(QStringLiteral(", "))));
        terminarPaso(!si ? kGrave : faltan.isEmpty() ? kOk : kLeve, det);
    } else {
        terminarPaso(kLeve, QStringLiteral("sin probar: se salteó"));
    }
}

void PruebaCompleta::pulsadorRecibido(const QString& por) {
    if (pregunta_ != QLatin1String("pulsador")) return;
    pregunta_.clear();
    cuenta_.stop();
    terminarPaso(kOk, QStringLiteral("llegó por %1, a los %2 s de pedirlo").arg(por).arg(relojPaso_.elapsed() / 1000));
}

bool PruebaCompleta::mandar() {
    if (!p_.reportes || !p_.reportes->hayLlave()) return false;
    p_.reportes->enviarAhora();
    return true;
}

void PruebaCompleta::correrComando(const QString& programa, const QStringList& args, int topeMs,
                                   const std::function<void(int, const QString&)>& listo) {
    auto* proc = new QProcess(vivo_ ? vivo_ : this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::finished, this, [proc, listo](int codigo, QProcess::ExitStatus) {
        listo(codigo, QString::fromUtf8(proc->readAll()));
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [proc, listo](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart) return;
        listo(-1, QStringLiteral("no se pudo correr"));
        proc->deleteLater();
    });
    QTimer::singleShot(topeMs, proc, [proc] { if (proc->state() != QProcess::NotRunning) proc->kill(); });
    proc->start(programa, args);
}

// 1) Sistema: versión, versiones.txt, arranque, temperatura, freno, lugar y memoria
void PruebaCompleta::pasoSistema() {
    sistema_.clear();
    haciendo_ = QStringLiteral("Mirando el sistema…");
    emit cambio();
    const QString versiones = QString::fromUtf8(leerArchivo(p_.carpetaDatos + QStringLiteral("/versiones.txt"), 2000)).trimmed()
                                  .split('\n', Qt::SkipEmptyParts).join(QStringLiteral(" · "));
    const int temp = temperaturaC();
    const unsigned freno = frenoAhora();
    const QStorageInfo lugar(p_.carpetaDatos);
    qint64 memTotal = 0, memLibre = 0;
    for (const QByteArray& l : leerArchivo(QStringLiteral("/proc/meminfo")).split('\n')) {
        if (l.startsWith("MemTotal:")) memTotal = l.mid(9).trimmed().split(' ').first().toLongLong() * 1024;
        if (l.startsWith("MemAvailable:")) memLibre = l.mid(13).trimmed().split(' ').first().toLongLong() * 1024;
    }
    sistema_[QStringLiteral("version")] = QStringLiteral(BETO_VERSION);
    sistema_[QStringLiteral("versiones")] = versiones;
    sistema_[QStringLiteral("temperatura")] = temp;
    sistema_[QStringLiteral("freno")] = freno;
    sistema_[QStringLiteral("libre")] = lugar.isValid() ? lugar.bytesAvailable() : -1;
    sistema_[QStringLiteral("memLibre")] = memLibre;
    auto cerrar = [this, versiones, temp, freno, lugar, memTotal, memLibre](const QString& arranque) {
        QStringList d;
        d << QStringLiteral("v") + QStringLiteral(BETO_VERSION);
        d << arranque;
        d << (temp >= 0 ? QStringLiteral("%1 °C").arg(temp) : QStringLiteral("temperatura: no se puede en la PC"));
        d << (raspberry() ? frenoTexto(freno) : QStringLiteral("freno: no se puede en la PC"));
        if (lugar.isValid()) d << QStringLiteral("libre %1 de %2").arg(gb(lugar.bytesAvailable()), gb(lugar.bytesTotal()));
        if (memTotal > 0) d << QStringLiteral("memoria libre %1 de %2").arg(tam(memLibre), tam(memTotal));
        d << (versiones.isEmpty() ? QStringLiteral("sin versiones.txt") : QStringLiteral("sistema: ") + versiones);
        QString e = kOk;
        if ((freno & 0x9) || (lugar.isValid() && lugar.bytesAvailable() < 1024LL * 1024 * 1024)) e = kGrave;
        else if ((freno & 0x90000) || temp >= 70 || (memTotal > 0 && memLibre < 150LL * 1024 * 1024) || !raspberry()) e = kLeve;
        terminarPaso(e, d.join(QStringLiteral(" · ")));
    };
    if (!raspberry() || !QFile::exists(QStringLiteral("/usr/bin/systemd-analyze"))) { cerrar(QStringLiteral("arranque: no se puede en la PC")); return; }
    correrComando(QStringLiteral("systemd-analyze"), {}, 10000, [this, cerrar](int, const QString& salida) {
        QString arranque = salida.section('\n', 0, 0).trimmed();
        if (arranque.contains(QLatin1String("= "))) arranque = QStringLiteral("arranque %1").arg(arranque.section(QStringLiteral("= "), 1).trimmed());
        else if (arranque.isEmpty()) arranque = QStringLiteral("arranque: systemd-analyze no contestó");
        correrComando(QStringLiteral("systemd-analyze"), {QStringLiteral("blame")}, 10000, [arranque, cerrar](int, const QString& blame) {
            QStringList top;
            for (const QString& l : blame.split('\n', Qt::SkipEmptyParts)) { top << l.simplified(); if (top.size() == 5) break; }
            cerrar(arranque + (top.isEmpty() ? QString() : QStringLiteral(" (más lentos: %1)").arg(top.join(QStringLiteral(", ")))));
        });
    });
}

// 2) Simulador de carrera: el del Chequeo previo (motor, cronómetro, guardado e informe)
void PruebaCompleta::pasoSimulador() {
    if (!p_.simulacion) { terminarPaso(kLeve, QStringLiteral("no hay simulador")); return; }
    haciendo_ = QStringLiteral("Carrera simulada de 3 vueltas…");
    emit cambio();
    auto listo = [this] {
        const QString e = p_.simulacion->estado();
        if (e == kCorriendo || e.isEmpty()) return;
        disconnect(espera_);
        QStringList malas;
        for (const QVariant& v : p_.simulacion->pruebas()) {
            const QVariantMap m = v.toMap();
            if (!m.value("ok").toBool()) malas << m.value("nombre").toString() + QStringLiteral(" (") + m.value("detalle").toString() + QLatin1Char(')');
        }
        terminarPaso(e == kOk ? kOk : e == kLeve ? kLeve : kGrave,
                     QStringLiteral("%1 pruebas en %2 s%3").arg(p_.simulacion->total()).arg(coma(relojPaso_.elapsed() / 1000.0))
                         .arg(malas.isEmpty() ? QStringLiteral(", todas bien") : QStringLiteral(", %1 fallaron: %2").arg(malas.size()).arg(malas.join(QStringLiteral("; ")))));
    };
    espera_ = connect(p_.simulacion, &Simulacion::cambio, this, listo);
    if (p_.simulacion->estado() != kCorriendo) p_.simulacion->correr();
}

// 3) Voz: que estén todos los audios y que se escuche uno
void PruebaCompleta::pasoVoz() {
    QStringList esperados{QStringLiteral("ultima_m"), QStringLiteral("ultima_p"), QStringLiteral("completa"), QStringLiteral("medida"),
                          QStringLiteral("practica"), QStringLiteral("prueba"), QStringLiteral("pulsador"), QStringLiteral("clic"),
                          QStringLiteral("corto"), QStringLiteral("largo"), QStringLiteral("tic")};
    for (int n = 0; n <= 60; ++n) esperados << QStringLiteral("n%1").arg(n, 2, 10, QLatin1Char('0'));
    for (int n = 1; n <= 60; ++n) esperados << QStringLiteral("vuelta_%1_m").arg(n, 2, 10, QLatin1Char('0')) << QStringLiteral("vuelta_%1_p").arg(n, 2, 10, QLatin1Char('0'));
    QStringList faltan;
    if (p_.audio) for (const QString& n : esperados) if (!p_.audio->tiene(n)) faltan << n;
    sistema_[QStringLiteral("vozFaltan")] = faltan;
    sistema_[QStringLiteral("vozEsperados")] = esperados.size();
    if (!p_.audio || !p_.audio->lista()) {
        terminarPaso(raspberry() ? kGrave : kLeve, raspberry() ? QStringLiteral("faltan todos los audios: la carrera va sin cuenta regresiva")
                                                                : QStringLiteral("no se puede en la PC (sin los audios)"));
        return;
    }
    haciendo_ = QStringLiteral("Sonando la prueba de la voz…");
    p_.audio->probar();
    preguntar(QStringLiteral("voz"), 20);
}

// 4) Pulsador: 20 s para apretarlo
void PruebaCompleta::pasoPulsador() {
    if (!p_.pulsador) { terminarPaso(kLeve, QStringLiteral("no se puede en la PC (sin GPIO)")); return; }
    if (!p_.pulsador->activo()) { terminarPaso(kGrave, QStringLiteral("no se pudo abrir el pin: %1").arg(p_.pulsador->error())); return; }
    haciendo_ = QStringLiteral("Esperando el pulsador…");
    preguntar(QStringLiteral("pulsador"), 20);
}

// 5) GPS: módulo, lecturas por segundo, satélites, precisión y de dónde salió la hora
void PruebaCompleta::pasoGps() {
    haciendo_ = QStringLiteral("Escuchando el GPS…");
    emit cambio();
    QTimer::singleShot(rapida() ? 500 : 3000, vivo_, [this] {
        const QVariantMap g = p_.crono ? p_.crono->gps() : QVariantMap();
        const bool simulado = !raspberry() && g.value("modulo").toString().isEmpty();
        const qint64 ultimo = g.value("ultimoDato").toLongLong();
        const bool habla = ultimo > 0 && QDateTime::currentMSecsSinceEpoch() - ultimo < 5000;
        const bool ok = g.value("ok").toBool() && g.value("hz").toInt() > 0;
        const double precision = g.value("precision").toDouble();
        const double umbral = p_.datos && p_.datos->cfg(QStringLiteral("umbralPrecisionM")).toDouble() > 0 ? p_.datos->cfg(QStringLiteral("umbralPrecisionM")).toDouble() : 20;
        const QString fuente = p_.contexto ? p_.contexto->horaFuente() : QString();
        QStringList d;
        d << (simulado ? QStringLiteral("GPS simulado de la PC") : g.value("modulo").toString().isEmpty() ? QStringLiteral("módulo: no se encontró")
             : QStringLiteral("%1%2 (fw %3)").arg(g.value("original").toBool() || !g.contains("original") ? QString() : QStringLiteral("réplica "), g.value("modulo").toString(), g.value("firmware").toString()));
        d << QStringLiteral("%1 lecturas por s%2").arg(g.value("hz").toInt()).arg(g.contains("hzConfig") ? QStringLiteral(" (configurado %1)").arg(g.value("hzConfig").toInt()) : QString());
        d << QStringLiteral("%1 satélites").arg(g.value("satelites").toInt());
        d << (ok ? QStringLiteral("precisión %1 m").arg(coma(precision)) : QStringLiteral("sin posición"));
        d << (fuente == QLatin1String("internet") ? QStringLiteral("hora de internet") : fuente == QLatin1String("gps") ? QStringLiteral("hora del GPS") : QStringLiteral("sin hora confiable"));
        QString e = kOk;
        if (!simulado && !habla) e = raspberry() ? kGrave : kLeve;
        else if (!ok || precision > umbral) e = kLeve;
        terminarPaso(e, d.join(QStringLiteral(" · ")));
    });
}

// 6) Cámara: graba 30 s como «Probar», revisa que el archivo tenga video y sonido, los cuadros y el pase a MP4
void PruebaCompleta::pasoCamara() {
    camaraMedida_.clear();
    const bool prendida = p_.datos && p_.datos->cfg(QStringLiteral("camara")).toBool();
    if (!prendida) { terminarPaso(kSaltado, QStringLiteral("apagada en Ajustes: no se probó")); return; }
    if (!p_.camara || !p_.camara->hay()) { terminarPaso(kGrave, QStringLiteral("prendida en Ajustes pero no se encontró: %1").arg(p_.camara ? p_.camara->problema() : QString())); return; }
    // Antes de grabar se espera a que la cámara se suelte (la vista previa, una grabación que termina) y 3 s más:
    // grabar enseguida daba «failed during initialization» en una de cada dos pruebas (1.0.31). Un pase a MP4
    // no molesta: sigue aparte.
    p_.camara->pararVista();
    p_.camara->pararNivel();
    haciendo_ = QStringLiteral("Esperando que se suelte la cámara…");
    emit cambio();
    auto* suelta = new QTimer(vivo_);
    auto* libreDesde = new QElapsedTimer;
    connect(suelta, &QObject::destroyed, this, [libreDesde] { delete libreDesde; });
    QElapsedTimer* esperando = new QElapsedTimer;
    connect(suelta, &QObject::destroyed, this, [esperando] { delete esperando; });
    esperando->start();
    const int pausa = rapida() ? 300 : 3000;
    connect(suelta, &QTimer::timeout, this, [this, suelta, libreDesde, esperando, pausa] {
        const bool ocupada = p_.camara->grabando() || p_.camara->reintentando() || p_.camara->viendo();
        if (ocupada) {
            libreDesde->invalidate();
            if (esperando->elapsed() < 60000) return;
            suelta->stop();
            terminarPaso(kLeve, QStringLiteral("la cámara estaba ocupada (grabando) más de 1 minuto: no se probó"));
            return;
        }
        if (!libreDesde->isValid()) libreDesde->start();
        if (libreDesde->elapsed() < pausa) return;
        suelta->stop();
        grabarCamara();
    });
    suelta->start(100);
}

void PruebaCompleta::grabarCamara() {
    const int seg = rapida() ? 4 : 30;
    p_.camara->probarGrabacion(seg);
    if (!p_.camara->grabando()) { terminarPaso(kGrave, QStringLiteral("la grabación no arrancó")); return; }
    tope_.start(rapida() ? 60000 : 300000);
    carga_m_ = Carga{};
    carga_.start();
    medirCarga();
    auto* reloj = new QTimer(vivo_);   // «Grabando… 12 de 30 s»
    connect(reloj, &QTimer::timeout, this, [this, seg] { haciendo_ = QStringLiteral("Grabando… %1 de %2 s").arg(qMin<qint64>(seg, relojPaso_.elapsed() / 1000)).arg(seg); emit cambio(); });
    reloj->start(1000);
    haciendo_ = QStringLiteral("Grabando… 0 de %1 s").arg(seg);
    emit cambio();
    auto* reintentos = new int(0);
    connect(reloj, &QObject::destroyed, this, [reintentos] { delete reintentos; });
    connect(p_.camara, &Camara::reintento, reloj, [reintentos] { ++*reintentos; });
    espera_ = connect(p_.camara, &Camara::grabandoChanged, this, [this, seg, reloj, reintentos] {
        if (p_.camara->grabando() || p_.camara->reintentando()) return;   // si no arrancó, vuelve a probar sola
        disconnect(espera_);
        const QString mkv = p_.camara->archivo();   // el último (si se reintentó, es otro)
        const int veces = *reintentos;
        carga_.stop();
        reloj->stop();
        const QByteArray cabeza = leerArchivo(mkv, 512 * 1024);
        const bool video = cabeza.contains("V_MPEG4"), sonido = cabeza.contains("A_OPUS");
        const qint64 tam = QFileInfo(mkv).size();
        const QVariantMap c = p_.camara->ultimosCuadros();
        const qlonglong n = c.value("cuadros").toLongLong(), cortados = c.value("cortados").toLongLong(), danados = c.value("avisosJpeg").toLongLong();
        const QString falla = p_.camara->falla();
        const bool quiereSonido = p_.camara->dispositivoAudio() != QLatin1String("no");
        camaraMedida_ = {{"segundos", seg}, {"bytes", tam}, {"video", video}, {"sonido", sonido}, {"cuadros", n}, {"cortados", cortados}, {"danados", danados}, {"falla", falla}, {"reintentos", veces}};
        haciendo_ = QStringLiteral("Pasando el video a MP4…");
        emit cambio();
        // El pase a MP4 empieza solo 3 s después de grabar: se espera a que empiece y termine (o a que ya esté)
        const QString mp4 = mkv.left(mkv.size() - 4) + QStringLiteral(".mp4");
        auto* vigila = new QTimer(vivo_);
        auto* vio = new bool(false);
        connect(vigila, &QObject::destroyed, this, [vio] { delete vio; });
        QElapsedTimer* desde = new QElapsedTimer;
        connect(vigila, &QObject::destroyed, this, [desde] { delete desde; });
        desde->start();
        connect(vigila, &QTimer::timeout, this, [this, vigila, vio, desde, mkv, mp4, seg, tam, video, sonido, n, cortados, danados, falla, quiereSonido, veces] {
            // Si está pasando otro video (uno viejo), se espera: el nuestro va después. Antes eso salía como «falló».
            const QString pasando = p_.camara->pasandoAMp4();
            if (!pasando.isEmpty()) {
                if (pasando == mkv) *vio = true;
                else if (!*vio) desde->start();
                return;
            }
            const bool hayMp4 = QFileInfo::exists(mp4), hayMkv = QFileInfo::exists(mkv);
            if (!*vio && !hayMp4 && hayMkv && desde->elapsed() < 20000) return;   // todavía no empezó
            vigila->stop();
            videoPrueba_ = hayMp4 ? mp4 : hayMkv ? mkv : QString();
            camaraMedida_[QStringLiteral("mp4")] = hayMp4;
            camaraMedida_[QStringLiteral("mp4Ms")] = desde->elapsed();
            const double fps = seg > 0 ? double(n) / seg : 0;
            QStringList d;
            d << QStringLiteral("%1 s grabados, archivo de %2").arg(seg).arg(mb(tam));
            // Los cuadros los cuenta el ayudante solo con una webcam MJPEG (los revisa uno por uno); con H.264 no
            d << (n > 0 ? QStringLiteral("%1 cuadros (%2 por s), %3 cortados, %4 dañados").arg(n).arg(coma(fps)).arg(cortados).arg(danados)
                        : QStringLiteral("cuadros: no se cuentan (cámara H.264); los mide la reproducción"));
            d << QStringLiteral("video: %1").arg(video ? QStringLiteral("sí") : QStringLiteral("NO"));
            d << QStringLiteral("sonido: %1").arg(sonido ? QStringLiteral("sí") : quiereSonido ? QStringLiteral("NO") : QStringLiteral("apagado en Ajustes"));
            d << (hayMp4 ? QStringLiteral("MP4 listo en %1 s (%2)").arg(desde->elapsed() / 1000).arg(mb(QFileInfo(mp4).size()))
                         : *vio ? QStringLiteral("el pase a MP4 falló (queda el .mkv)") : QStringLiteral("el pase a MP4 no empezó"));
            if (veces > 0) d << QStringLiteral("no arrancó de entrada: se volvió a probar sola %1 %2").arg(veces).arg(veces == 1 ? QStringLiteral("vez") : QStringLiteral("veces"));
            if (!falla.isEmpty()) d << QStringLiteral("la cámara avisó: ") + falla;
            QString e = kOk;
            if (!video || tam < 10 * 1024 || !falla.isEmpty() || (!hayMp4 && !hayMkv)) e = kGrave;
            else if ((quiereSonido && !sonido) || !hayMp4 || veces > 0 || (n > 0 && (cortados * 50 > n || fps < 15))) e = kLeve;
            terminarPaso(e, d.join(QStringLiteral(" · ")));
        });
        vigila->start(1000);
    });
}

void PruebaCompleta::medirCarga() {
    Carga& c = carga_m_;
    const qint64 ahora = QDateTime::currentMSecsSinceEpoch();
    const qint64 beto = ticks(::getpid());
    const qint64 pidAyu = p_.camara ? p_.camara->pidGrabacion() : 0;
    const qint64 ayu = pidAyu > 0 ? ticks(pidAyu) : -1;
    const double clk = static_cast<double>(::sysconf(_SC_CLK_TCK));
    if (c.ticksBeto >= 0 && ahora > c.ultimaMs && clk > 0) {
        const double dt = (ahora - c.ultimaMs) / 1000.0;
        const double pb = (beto - c.ticksBeto) / clk / dt * 100;
        c.cpuBeto += pb;
        c.maxBeto = qMax(c.maxBeto, pb);
        if (ayu >= 0 && c.ticksAyu >= 0) { const double pa = (ayu - c.ticksAyu) / clk / dt * 100; c.cpuAyudante += pa; c.maxAyu = qMax(c.maxAyu, pa); }
        ++c.lecturas;
    }
    c.ticksBeto = beto;
    c.ticksAyu = ayu;
    c.ultimaMs = ahora;
    c.tempMax = qMax(c.tempMax, temperaturaC());
    c.freno |= frenoAhora() & 0xF;   // solo lo que pasa ahora: si durante la grabación se frenó
}

// 7) Rendimiento bajo carga: lo medido mientras grababa la cámara
void PruebaCompleta::pasoRendimiento() {
    const Carga& c = carga_m_;
    if (c.lecturas == 0) { terminarPaso(kSaltado, QStringLiteral("no se midió: la cámara no grabó")); return; }
    const double beto = c.cpuBeto / c.lecturas, ayu = c.cpuAyudante / c.lecturas;
    QStringList d;
    d << QStringLiteral("en %1 s grabando").arg(c.lecturas);
    d << QStringLiteral("Betómetro %1 % de un núcleo (máx %2 %)").arg(coma(beto, 0), coma(c.maxBeto, 0));
    d << (c.maxAyu > 0 ? QStringLiteral("ayudante de la cámara %1 % (máx %2 %)").arg(coma(ayu, 0), coma(c.maxAyu, 0)) : QStringLiteral("ayudante: sin medir"));
    d << (c.tempMax >= 0 ? QStringLiteral("temperatura máxima %1 °C").arg(c.tempMax) : QStringLiteral("temperatura: no se puede en la PC"));
    d << (raspberry() ? (c.freno ? QStringLiteral("SE FRENÓ: ") + frenoTexto(c.freno) : QStringLiteral("sin freno")) : QStringLiteral("freno: no se puede en la PC"));
    QString e = kOk;
    if (c.freno) e = kGrave;
    else if (c.tempMax >= 75 || beto > 150 || !raspberry()) e = kLeve;
    terminarPaso(e, d.join(QStringLiteral(" · ")));
}

// 8) Reproducción: el video de prueba con el reproductor de GStreamer, sin pantalla, contando los cuadros que descomprime
void PruebaCompleta::pasoReproduccion() {
    if (videoPrueba_.isEmpty() || !QFileInfo::exists(videoPrueba_)) { terminarPaso(kSaltado, QStringLiteral("sin video de prueba: no se probó")); return; }
    haciendo_ = QStringLiteral("Reproduciendo el video de prueba…");
    emit cambio();
    auto* video = new VideoGst(vivo_);
    auto* error = new QString;
    auto* primero = new qint64(0);    // cuándo llegó el primer cuadro y cuántos había
    auto* base = new qint64(0);
    connect(video, &QObject::destroyed, this, [error, primero, base] { delete error; delete primero; delete base; });
    connect(video, &VideoGst::errorOccurred, this, [error](int, const QString& texto) { if (error->isEmpty()) *error = texto; });
    auto* mira = new QTimer(vivo_);
    connect(mira, &QTimer::timeout, this, [video, primero, base] {
        if (*primero == 0 && video->cuadros() > 0) { *primero = QDateTime::currentMSecsSinceEpoch(); *base = video->cuadros(); }
    });
    mira->start(50);
    const QString archivo = videoPrueba_;
    const int ms = rapida() ? 2500 : 6000;
    video->setSource(QUrl::fromLocalFile(archivo));
    video->play();
    QTimer::singleShot(ms + 1500, vivo_, [this, video, mira, primero, base, error, archivo] {
        mira->stop();
        const qint64 n = video->cuadros() - *base;
        const double seg = *primero > 0 ? (QDateTime::currentMSecsSinceEpoch() - *primero) / 1000.0 : 0;
        const double fps = seg > 0 ? n / seg : 0;
        const QString desc = video->descompresor();
        const qint64 pos = video->position();
        video->stop();
        // Se compara con lo que grabó la cámara: si grabó a 19,8 por s, no se puede ver a 20 (salía naranja «20,0»)
        const qlonglong cg = camaraMedida_.value("cuadros").toLongLong();
        const int sg = camaraMedida_.value("segundos").toInt();
        const double grabado = cg > 0 && sg > 0 ? double(cg) / sg : 0;
        const double pide = grabado > 0 ? qMin(20.0, grabado * 0.9) : 20.0;
        QStringList d;
        d << QStringLiteral("%1 cuadros por s (%2 cuadros en %3 s)").arg(coma(fps)).arg(video->cuadros()).arg(coma(seg));
        if (grabado > 0) d << QStringLiteral("se grabó a %1 por s").arg(coma(grabado));
        d << QStringLiteral("descompresor %1%2").arg(desc.isEmpty() ? QStringLiteral("desconocido") : desc, desc == QLatin1String("v4l2h264dec") ? QStringLiteral(" (el chip de la Pi)") : QString());
        d << QStringLiteral("llegó al segundo %1").arg(coma(pos / 1000.0));
        if (!error->isEmpty()) d << QStringLiteral("el reproductor avisó: ") + *error;
        QString e = kOk;
        if (video->cuadros() == 0) e = raspberry() ? kGrave : kLeve;   // en la PC puede faltar el descompresor
        else if (fps < pide) e = kLeve;
        // El video de prueba ya no hace falta: no ocupa la tarjeta (como «Borrar pruebas» de la cámara)
        if (p_.camara) QTimer::singleShot(500, p_.camara, [this, archivo] { p_.camara->borrarVideo(QFileInfo(archivo).fileName()); });
        terminarPaso(e, d.join(QStringLiteral(" · ")));
    });
}

// 9) Tarjeta: guardar una carrera de prueba (carpeta aparte) y escribir 50 MB con fsync
void PruebaCompleta::pasoTarjeta() {
    haciendo_ = QStringLiteral("Probando la tarjeta…");
    emit cambio();
    const QString carpeta = p_.carpetaDatos + QStringLiteral("/prueba-completa-datos");
    QDir(carpeta).removeRecursively();
    QDir().mkpath(carpeta);
    QElapsedTimer t;
    t.start();
    bool guardo = false;
    {
        Datos d(carpeta);
        const QVariantMap r = d.guardarCarrera({{"nombre", "Prueba completa"}, {"fecha", QDate::currentDate().toString(Qt::ISODate)},
                                                {"tiempoIdealMs", 60000}, {"distanciaM", 1000}, {"patron", "todas"}});
        d.guardarYa();
        guardo = r.value("ok").toBool() && d.guardaBien();
    }
    const qint64 msGuardar = t.elapsed();
    QDir(carpeta).removeRecursively();
    const QString archivo = p_.carpetaDatos + QStringLiteral("/prueba-completa.bin");
    const int mbs = rapida() ? 10 : 50;
    t.restart();
    correrComando(QStringLiteral("dd"), {QStringLiteral("if=/dev/zero"), QStringLiteral("of=") + archivo, QStringLiteral("bs=1M"),
                                         QStringLiteral("count=%1").arg(mbs), QStringLiteral("conv=fsync")}, 90000,
                  [this, t, msGuardar, guardo, archivo, mbs](int codigo, const QString&) {
                      const double seg = t.elapsed() / 1000.0;
                      const bool escribio = codigo == 0 && QFileInfo(archivo).size() == qint64(mbs) * 1048576;
                      QFile::remove(archivo);
                      const double vel = escribio && seg > 0 ? mbs / seg : 0;
                      const QStorageInfo lugar(p_.carpetaDatos);
                      QStringList d;
                      d << QStringLiteral("guardar una carrera: %1 ms%2").arg(msGuardar).arg(guardo ? QString() : QStringLiteral(" (FALLÓ)"));
                      d << (escribio ? QStringLiteral("escribir %1 MB: %2 s (%3 MB/s)").arg(mbs).arg(coma(seg)).arg(coma(vel)) : QStringLiteral("no se pudieron escribir %1 MB").arg(mbs));
                      if (lugar.isValid()) d << QStringLiteral("libre %1").arg(gb(lugar.bytesAvailable()));
                      QString e = kOk;
                      if (!guardo || !escribio) e = kGrave;
                      else if (vel < 5 || msGuardar > 3000) e = kLeve;
                      terminarPaso(e, d.join(QStringLiteral(" · ")));
                  });
}

// 10) Wifi e internet: red conectada y un HEAD a github.com con tiempo límite
void PruebaCompleta::pasoWifi() {
    // Lo que se prueba es la ANTENA, no que haya wifi: en la ruta o en boxes puede no haber ninguna red y está bien
    // (Matías, 08/11/2026). Falla solo si no aparece la placa de wifi o no se ve ninguna red cerca.
    const QString red = p_.wifi ? p_.wifi->conectada() : QString();
    const bool propia = p_.wifi && p_.wifi->propia();
    const QString ip = p_.wifi ? p_.wifi->direccion() : QString();
    if (raspberry() && !QFileInfo::exists(QStringLiteral("/sys/class/net/wlan0"))) {
        terminarPaso(kGrave, QStringLiteral("no aparece la placa de wifi (wlan0): revisar la antena o el sistema"));
        return;
    }
    QStringList d;
    if (propia) {   // la red propia andando ya prueba la antena
        d << QStringLiteral("antena bien: red propia «%1» armada (%2 celus conectados)").arg(p_.wifi->nombrePropia()).arg(p_.wifi->conectadosPropia());
        if (!ip.isEmpty()) d << QStringLiteral("IP %1").arg(ip);
        d << QStringLiteral("sin internet (no es una falla)");
        terminarPaso(kOk, d.join(QStringLiteral(" · ")));
        return;
    }
    if (red.isEmpty()) {   // sin conexión: se mira si la antena ve redes
        const auto cuantas = [this] {
            int n = p_.wifi ? static_cast<int>(p_.wifi->cerca().size()) : 0;
            if (p_.wifi) for (const QVariant& g : p_.wifi->guardadas()) if (g.toMap().value("cerca").toBool()) ++n;
            return n;
        };
        auto cerrar = [this, cuantas] {
            const int n = cuantas();
            if (n > 0) terminarPaso(kOk, QStringLiteral("antena bien: %1 %2 a la vista · sin conexión (no es una falla)").arg(n).arg(n == 1 ? QStringLiteral("red") : QStringLiteral("redes")));
            else if (p_.wifi && !p_.wifi->redes()) terminarPaso(kOk, QStringLiteral("placa de wifi presente · «Conectarse a redes wifi» está apagado: no se buscó"));
            else terminarPaso(kLeve, QStringLiteral("no se ve ninguna red cerca: puede ser el lugar (sin wifi alrededor) o la antena"));
        };
        if (cuantas() > 0 || !p_.wifi || !p_.wifi->redes()) { cerrar(); return; }
        haciendo_ = QStringLiteral("Buscando redes con la antena…");
        emit cambio();
        p_.wifi->buscar();
        auto* espera = new QTimer(vivo_);
        espera->setSingleShot(true);
        connect(espera, &QTimer::timeout, this, cerrar);
        espera->start(rapida() ? 500 : 12000);
        return;
    }
    d << QStringLiteral("antena bien: wifi «%1», %2 de 4 rayitas").arg(red).arg(p_.wifi->senal());
    if (!ip.isEmpty()) d << QStringLiteral("IP %1").arg(ip);
    haciendo_ = QStringLiteral("Probando internet…");
    emit cambio();
    auto* red_ = new QNetworkAccessManager(vivo_);
    QNetworkRequest pedido(QUrl(QStringLiteral("https://github.com/")));
    pedido.setTransferTimeout(rapida() ? 3000 : 8000);
    QElapsedTimer t;
    t.start();
    QNetworkReply* r = red_->head(pedido);
    connect(r, &QNetworkReply::finished, this, [this, r, t, d]() mutable {
        r->deleteLater();
        const bool internet = r->error() == QNetworkReply::NoError || r->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid();
        d << (internet ? QStringLiteral("internet: sí, github.com contestó en %1 ms").arg(t.elapsed())
                       : QStringLiteral("esa wifi no tiene internet ahora (no es una falla del equipo)"));
        terminarPaso(kOk, d.join(QStringLiteral(" · ")));
    });
}

// 11) Google Drive: cómo está
void PruebaCompleta::pasoDrive() {
    if (!p_.drive) { terminarPaso(kSaltado, QStringLiteral("sin Drive")); return; }
    const QString e = p_.drive->estado();
    if (e != QLatin1String("conectado")) { terminarPaso(kSaltado, QStringLiteral("sin conectar (%1): no se usa").arg(e.isEmpty() ? QStringLiteral("nunca se conectó") : e)); return; }
    QStringList d;
    d << QStringLiteral("conectado como %1").arg(p_.drive->cuenta());
    if (!p_.drive->texto().isEmpty()) d << p_.drive->texto();
    if (!p_.drive->error().isEmpty()) d << QStringLiteral("problema: ") + p_.drive->error();
    terminarPaso(p_.drive->error().isEmpty() ? kOk : kLeve, d.join(QStringLiteral(" · ")));
}

// 12) Pantallas: que cada .qml de tablet/ y pantallas/ se pueda armar (sin mostrarlo): detecta un módulo que falta
void PruebaCompleta::pasoPantallas() {
    if (!motor_) { terminarPaso(kLeve, QStringLiteral("sin motor de QML")); return; }
    auto* lista = new QStringList;
    for (const QString& carpeta : {QStringLiteral(":/qt/qml/Betometro/tablet"), QStringLiteral(":/qt/qml/Betometro/pantallas"),
                                   QStringLiteral(":/Betometro/tablet"), QStringLiteral(":/Betometro/pantallas")}) {
        QDirIterator it(carpeta, {QStringLiteral("*.qml")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) *lista << it.next();
    }
    lista->sort();
    if (lista->isEmpty()) { delete lista; terminarPaso(kGrave, QStringLiteral("no se encontraron las pantallas en los recursos del programa")); return; }
    auto* errores = new QStringList;
    auto* i = new int(0);
    connect(vivo_, &QObject::destroyed, this, [lista, errores, i] { delete lista; delete errores; delete i; });
    // Una por vuelta del bucle de eventos, así la pantalla no se traba
    auto* paso = new QTimer(vivo_);
    connect(paso, &QTimer::timeout, this, [this, lista, errores, i, paso] {
        if (*i >= lista->size()) {
            paso->stop();
            const QString det = errores->isEmpty()
                                    ? QStringLiteral("las %1 pantallas se arman sin error (%2 ms)").arg(lista->size()).arg(relojPaso_.elapsed())
                                    : QStringLiteral("%1 de %2 pantallas con error: %3").arg(errores->size()).arg(lista->size()).arg(errores->join(QStringLiteral("; ")));
            terminarPaso(errores->isEmpty() ? kOk : kGrave, det);
            return;
        }
        const QString ruta = lista->at(*i);
        ++*i;
        haciendo_ = QStringLiteral("Armando %1…").arg(QFileInfo(ruta).fileName());
        emit cambio();
        QQmlComponent c(motor_, QUrl(QStringLiteral("qrc") + ruta));
        if (c.isError()) {
            QString e = QFileInfo(ruta).fileName() + QStringLiteral(": ");
            for (const QQmlError& err : c.errors()) { e += err.description(); break; }
            *errores << e;
        }
    });
    paso->start(0);
}

void PruebaCompleta::guardar() {
    QFile j(p_.carpetaDatos + QStringLiteral("/prueba-completa.json"));
    if (j.open(QIODevice::WriteOnly | QIODevice::Truncate)) j.write(json());
    QFile t(p_.carpetaDatos + QStringLiteral("/prueba-completa.txt"));
    if (t.open(QIODevice::WriteOnly | QIODevice::Truncate)) t.write(texto());
}

QByteArray PruebaCompleta::json() const {
    QJsonArray pasos;
    for (const Paso& p : pasos_) pasos << QJsonObject{{"nombre", p.nombre}, {"estado", p.estado}, {"detalle", p.detalle}, {"ms", p.ms}, {"empezo", p.empezo}};
    QJsonObject o{{"version", QStringLiteral(BETO_VERSION)}, {"estado", estado_}, {"cuando", cuando_}, {"empezo", empezoTotal_}, {"resumen", resumen()},
                  {"paso", paso()}, {"total", total()}, {"haciendo", haciendo_}, {"pregunta", pregunta_}, {"segundos", segundos_},
                  {"duracionS", estado_ == kCorriendo || estado_.isEmpty() ? 0 : duracionS_}, {"pasos", pasos}};
    return QJsonDocument(o).toJson(QJsonDocument::Indented);
}

QByteArray PruebaCompleta::texto() const {
    QString t = QStringLiteral("Prueba completa · Betómetro v%1 · empezó %2 · %3\n").arg(QStringLiteral(BETO_VERSION), empezoTotal_.isEmpty() ? QStringLiteral("--:--:--") : empezoTotal_, cuando_.isEmpty() ? QStringLiteral("(sin terminar)") : cuando_);
    t += QStringLiteral("RESULTADO: %1 · %2%3\n\n").arg(estado_ == kOk ? QStringLiteral("TODO BIEN") : estado_ == kLeve ? QStringLiteral("REVISAR") : estado_ == kGrave ? QStringLiteral("HAY ERRORES") : estado_ == kCorriendo ? QStringLiteral("CORRIENDO") : QStringLiteral("SIN CORRER"), resumen(),
                                                     duracionS_ > 0 && estado_ != kCorriendo ? QStringLiteral(" · duró %1 min %2 s").arg(duracionS_ / 60).arg(duracionS_ % 60) : QString());
    int n = 0;
    for (const Paso& p : pasos_) {
        const QString marca = p.estado == kOk ? QStringLiteral("BIEN") : p.estado == kLeve ? QStringLiteral("AVISO") : p.estado == kGrave ? QStringLiteral("ERROR")
                            : p.estado == kSaltado ? QStringLiteral("sin probar") : p.estado == kCorriendo ? QStringLiteral("corriendo") : QStringLiteral("espera");
        t += QStringLiteral("%1. [%2] %3 · %4 · %5 s: %6\n").arg(++n, 2).arg(marca, p.nombre, p.empezo.isEmpty() ? QStringLiteral("--:--:--") : p.empezo,
                                                                    QString::number(p.ms / 1000.0, 'f', 1).replace('.', ','), p.detalle.isEmpty() ? QStringLiteral("—") : p.detalle);
    }
    return t.toUtf8();
}
