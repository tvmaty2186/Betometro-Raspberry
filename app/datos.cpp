#include "datos.h"

#if __has_include(<QJSValue>)   // en el programa (las pruebas no tienen QML)
#include <QJSValue>
#define BETO_CON_QJS 1
#endif

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStorageInfo>

#include "informe.h"
#include <QDir>
#include <QLocale>
#include <QRandomGenerator>
#include <algorithm>
#include <sys/resource.h>
#include <unistd.h>
#include <cmath>
#include <map>

using namespace beto;

static const char* const kMeses[] = {"ENE", "FEB", "MAR", "ABR", "MAY", "JUN", "JUL", "AGO", "SEP", "OCT", "NOV", "DIC"};

static QString q(const std::string& s) { return QString::fromStdString(s); }
static std::string s(const QString& t) { return t.toStdString(); }

// Errores como la app (durNum / dur)
static QString durNum(double ms) {
    double a = std::fabs(ms);
    if (a < 59950) return QString::number(fijarDecimalesJs(a / 1000, 1), 'f', 1).replace('.', ',');
    long long t = static_cast<long long>(redondearJs(a / 100)), d = t % 10, sg = t / 10;
    long long h = sg / 3600, m = sg % 3600 / 60, ss = sg % 60;
    auto p2 = [](long long n) { return QString::number(n).rightJustified(2, '0'); };
    return (h ? QString::number(h) + ":" + p2(m) : QString::number(m)) + ":" + p2(ss) + "," + QString::number(d);
}
static QString dur(double ms) { return durNum(ms) + (std::fabs(ms) < 59950 ? " s" : ""); }
static QString nivelErr(double s) { return s <= 0.2 ? "ok" : s <= 0.5 ? "medio" : "mal"; }

// Tiempo ideal y de vuelta: m:ss:d (Matías, 07/11/2026: «:» en todos los tiempos; las diferencias con coma)
static QString reloj(int64_t ms) {   // H:MM:SS
    const int64_t s = ms / 1000;
    return QString::number(s / 3600) + ":" + QString::number(s / 60 % 60).rightJustified(2, '0') + ":" + QString::number(s % 60).rightJustified(2, '0');
}
static QString tiempo(int64_t ms) {
    int64_t dec = ms / 100, d = dec % 10, sg = dec / 10;
    return QString::number(sg / 60) + ":" + QString::number(sg % 60).rightJustified(2, '0') + ":" + QString::number(d);
}
static QString miles(int64_t n) { return QLocale(QLocale::Spanish, QLocale::Argentina).toString(static_cast<qlonglong>(n)); }

Datos::Datos(const QString& carpeta, QObject* padre) : QObject(padre), carpeta_(carpeta), almacen_(s(carpeta)) {
    QDir().mkpath(carpeta);
    auto l = almacen_.cargar();
    base_ = std::move(l.base);
    cfg_ = armarConfig(base_.config);
    if (l.habiaDanado) qWarning("La base estaba dañada: se usó %s", l.origen.c_str());
    hilo_ = std::thread([this] { hiloGuardar(); });
    guardarLuego_.setSingleShot(true);
    guardarLuego_.setInterval(300);
    connect(&guardarLuego_, &QTimer::timeout, this, [this] { guardar(); });
    // Recorrido libre que se cortó (luz, vigilante) antes de Terminar: lo andado queda como su vuelta
    bool recuperado = false;
    for (Carrera& c : base_.carreras) {
        if (!c.extra.tiene("enCurso")) continue;
        if (c.extra["enCurso"].esObjeto()) {
            Vuelta v = Vuelta::desdeJson(c.extra["enCurso"]);
            v.interrumpida = true;
            c.vueltas.push_back(std::move(v));
            qWarning("Recorrido «%s» cortado antes de Terminar: se recuperó lo andado", c.nombre.c_str());
        }
        c.extra.sacar("enCurso");
        recuperado = true;
    }
    if (recuperado) guardar();
}

Datos::~Datos() {
    if (guardarLuego_.isActive()) guardar();   // lo que estaba juntándose
    {
        std::lock_guard<std::mutex> l(m_);
        terminar_ = true;
    }
    cv_.notify_all();
    if (hilo_.joinable()) hilo_.join();   // escribe lo último que quedó pendiente antes de cerrar
}

void Datos::guardarLuego() {
    if (!guardarLuego_.isActive()) guardarLuego_.start();
}

void Datos::guardar() {
    guardarLuego_.stop();
    auto copia = std::make_shared<beto::Base>(base_);   // la copia en memoria es rápida; escribir en la SD no
    {
        std::lock_guard<std::mutex> l(m_);
        pendiente_ = std::move(copia);
    }
    cv_.notify_all();
}

void Datos::guardarYa() {
    if (guardarLuego_.isActive()) guardar();
    std::unique_lock<std::mutex> l(m_);
    listo_.wait(l, [this] { return !pendiente_ && !escribiendo_; });
}

void Datos::hiloGuardar() {
    // Este hilo, con menos prioridad: primero GPS, cronómetro, pulsador y pantalla
    setpriority(PRIO_PROCESS, static_cast<id_t>(::gettid()), 10);
    std::unique_lock<std::mutex> l(m_);
    for (;;) {
        cv_.wait(l, [this] { return pendiente_ || terminar_; });
        if (!pendiente_) break;   // terminar y no hay nada pendiente
        auto b = std::move(pendiente_);
        escribiendo_ = true;
        l.unlock();
        std::string err;
        const bool ok = almacen_.guardar(*b, &err);
        b.reset();
        l.lock();
        escribiendo_ = false;
        // Si no se puede guardar (tarjeta llena o dañada) queda en el reporte y la pantalla lo avisa (05-1.2)
        QMetaObject::invokeMethod(this, [this, ok, err] {
            if (!ok) { qWarning("No se pudo guardar la base: %s", err.c_str()); emit error(q("No se pudo guardar: " + err)); }
            if (ok != guardaBien_) { guardaBien_ = ok; emit guardaBienChanged(); }
        }, Qt::QueuedConnection);
        listo_.notify_all();
    }
    listo_.notify_all();
}

// Sin cantidad de vueltas fija (en la carrera pueden agregar vueltas): queda terminada
// cuando tiene vueltas corridas y su fecha ya pasó.
bool Datos::finalizada(const Carrera& c) const {
    if (c.practica || c.vueltas.empty()) return false;
    const QDate f = QDate::fromString(q(c.fecha), Qt::ISODate);
    return f.isValid() && hoy().daysTo(f) < 0;
}

QVariantMap Datos::resumen(const Carrera& c) const {
    QVariantMap r;
    const QDate f = QDate::fromString(q(c.fecha), Qt::ISODate);
    const int dias = f.isValid() ? static_cast<int>(hoy().daysTo(f)) : -1;
    double suma = 0, mejor = -1;
    int medidas = 0;
    for (const auto& v : c.vueltas) {
        if (!v.medida || v.interrumpida || !v.difMs) continue;
        double a = std::fabs(static_cast<double>(*v.difMs));
        suma += a;
        mejor = mejor < 0 ? a : std::min(mejor, a);
        ++medidas;
    }
    const bool fin = finalizada(c);
    r["id"] = q(c.id);
    r["nombre"] = q(c.nombre);
    r["fecha"] = q(c.fecha);
    // Localidad (para el clima): nombre y coordenadas elegidas de la lista del buscador
    const Json& ex = c.extra;
    r["lugar"] = q(c.lugar);
    r["lugarLat"] = ex["lugarLat"].esNumero() ? QVariant(ex["lugarLat"].aNumero()) : QVariant();
    r["lugarLon"] = ex["lugarLon"].esNumero() ? QVariant(ex["lugarLon"].aNumero()) : QVariant();
    r["dia"] = f.isValid() ? QString::number(f.day()).rightJustified(2, '0') : QString("--");
    r["mes"] = f.isValid() ? QString(kMeses[f.month() - 1]) : QString();
    r["fechaCorta"] = f.isValid() ? f.toString("dd/MM") : QString();
    r["fechaLarga"] = f.isValid() ? QLocale(QLocale::Spanish, QLocale::Argentina).toString(f, "dddd, d 'de' MMMM 'de' yyyy") : QString();
    r["notas"] = q(c.notas);
    r["practica"] = c.practica;
    const bool rec = esRecorrido(c);
    r["recorrido"] = rec;
    r["distanciaM"] = static_cast<qlonglong>(c.distanciaM);
    r["tiempoIdealMs"] = static_cast<qlonglong>(c.tiempoIdealMs);
    r["patron"] = q(c.patron);
    r["tieneVueltas"] = !c.vueltas.empty();
    r["vueltas"] = static_cast<int>(c.vueltas.size());
    r["finalizada"] = fin;
    r["modificada"] = c.modificada();   // hay una carrera original para restablecer
    r["medidas"] = medidas;
    r["errorTotal"] = medidas ? dur(suma) : QString();
    r["promedio"] = medidas ? dur(suma / medidas) : QString();
    r["mejor"] = medidas ? dur(mejor) : QString();
    r["ideal"] = tiempo(c.tiempoIdealMs);
    r["distancia"] = miles(c.distanciaM);
    r["velocidad"] = c.distanciaM > 0 && c.tiempoIdealMs > 0
                         ? QString::number(fijarDecimalesJs(c.distanciaM / (c.tiempoIdealMs / 1000.0) * 3.6, 1), 'f', 1).replace('.', ',')
                         : QString();

    // Línea de la tarjeta
    QStringList partes;
    if (rec) {   // recorrido libre: distancia y tiempo, sin vueltas ni error
        partes << QStringLiteral("RECORRIDO");
        if (!c.vueltas.empty()) {
            const Vuelta& v = c.vueltas.front();
            if (v.distM) partes << QString::number(*v.distM / 1000, 'f', 1).replace('.', ',') + " km";
            partes << reloj(v.realMs);
            if (v.velPromKmh) partes << "promedio " + QString::number(*v.velPromKmh, 'f', 1).replace('.', ',') + " km/h";
        } else partes << QStringLiteral("sin largar");
    } else if (!c.vueltas.empty()) partes << QString::number(c.vueltas.size()) + (c.vueltas.size() == 1 ? " vuelta" : " vueltas");
    if (rec) {
    } else if (medidas) {
        partes << "error total " + dur(suma);
    } else {
        if (c.distanciaM > 0) partes << miles(c.distanciaM) + " m";
        partes << "ideal " + tiempo(c.tiempoIdealMs);
        partes << (c.patron == "alt1" ? "impares medidas" : c.patron == "alt2" ? "pares medidas" : "todas medidas");
    }
    r["linea"] = partes.join(" · ");

    // Chip de estado (estadoCarrera)
    QString txt, nivel = "neutro";
    if (medidas) { txt = "ERROR " + dur(suma); nivel = nivelErr(suma / medidas / 1000); }
    else if (rec) txt = "RECORRIDO";
    else if (c.practica) txt = "PRÁCTICA";
    else if (fin) txt = "FINALIZADA";
    else if (dias == 0) { txt = "HOY"; nivel = "hoy"; }
    else if (dias == 1) { txt = "MAÑANA"; nivel = "hoy"; }
    else if (dias > 1) txt = "EN " + QString::number(dias) + " DÍAS";
    else txt = "SIN CORRER";
    r["estado"] = txt;
    r["nivel"] = nivel;

    // LARGAR: práctica o fecha de hoy. Sin hora del GPS no se sabe qué día es: se deja largar.
    QString motivo;
    bool puede = !fin;
    if (!fin && !c.practica && horaValida_ && dias != 0) {
        puede = false;
        motivo = dias > 0 ? "Se larga el " + QLocale(QLocale::Spanish, QLocale::Argentina).toString(f, f.year() == hoy().year() ? "d MMM" : "d MMM yyyy").remove('.')
                          : QString("La fecha de la carrera ya pasó");
    }
    if (rec && !c.vueltas.empty()) { puede = false; motivo = QStringLiteral("Otro: Práctica → Recorrido libre"); }
    r["puedeLargar"] = puede;
    r["motivoNoLargar"] = motivo;
    r["proxima"] = !c.practica && !fin && dias >= 0;
    r["creada"] = q(c.creada);

    // Vueltas: tipo de las primeras (para las etiquetas V1 MEDIDA…)
    QVariantList tipos;
    for (int n = 1; n <= 4; ++n) tipos << esMedida(c.patron, n);
    r["tipos"] = tipos;
    return r;
}

QVariantList Datos::proximas() const {
    std::vector<const Carrera*> v;
    for (const auto& c : base_.carreras) if (resumen(c)["proxima"].toBool()) v.push_back(&c);
    std::stable_sort(v.begin(), v.end(), [](auto a, auto b) { return a->fecha < b->fecha; });
    QVariantList r;
    for (auto c : v) r << resumen(*c);
    return r;
}

QVariantList Datos::corridas() const {
    std::vector<const Carrera*> v;
    for (const auto& c : base_.carreras) if (!resumen(c)["proxima"].toBool()) v.push_back(&c);
    std::stable_sort(v.begin(), v.end(), [](auto a, auto b) {
        return a->fecha != b->fecha ? a->fecha > b->fecha : a->creada > b->creada;
    });
    QVariantList r;
    for (auto c : v) r << resumen(*c);
    return r;
}

QVariantMap Datos::carrera(const QString& id) const {
    for (const auto& c : base_.carreras) if (q(c.id) == id) return resumen(c);
    Carrera n;   // nueva: valores de la última práctica
    n.fecha = s(hoy().toString(Qt::ISODate));
    n.tiempoIdealMs = cfg_["tiempoIdealMs"].aEntero(240000);
    n.distanciaM = cfg_["distanciaM"].aEntero(3200);
    n.patron = cfg_["patron"].aTexto("alt1");
    QVariantMap r = resumen(n);
    r["id"] = QString();
    r["nueva"] = true;
    return r;
}

QVariantMap Datos::guardarCarrera(const QVariantMap& m) {
    QVariantMap r;
    auto falla = [&](const QString& e) { r["ok"] = false; r["error"] = e; return r; };
    const QString nombre = m.value("nombre").toString().trimmed();
    const qlonglong T = m.value("tiempoIdealMs").toLongLong();
    if (nombre.isEmpty()) return falla("Poné un nombre a la carrera.");
    if (!QDate::fromString(m.value("fecha").toString(), Qt::ISODate).isValid()) return falla("Elegí la fecha de la carrera.");
    if (T < 5000) return falla("El tiempo ideal tiene que ser de al menos 5 segundos.");

    const QString id = m.value("id").toString();
    Carrera* c = id.isEmpty() ? nullptr : base_.buscar(s(id));
    if (!c) {
        Carrera n;
        n.id = nuevaId(QDateTime::currentMSecsSinceEpoch(), QRandomGenerator::global()->generate());
        n.creada = s(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        base_.carreras.push_back(n);
        c = &base_.carreras.back();
    }
    c->nombre = s(nombre.left(Carrera::kMaxNombre));
    c->fecha = s(m.value("fecha").toString());
    c->vueltasTotal = 0;   // ya no se usa: no hay límite de vueltas
    if (m.contains("lugar")) {
        c->lugar = s(m.value("lugar").toString().left(120));
        const QVariant la = m.value("lugarLat"), lo = m.value("lugarLon");
        const bool hay = !c->lugar.empty() && la.isValid() && !la.isNull() && lo.isValid() && !lo.isNull();
        c->extra["lugarLat"] = hay ? Json(la.toDouble()) : Json();
        c->extra["lugarLon"] = hay ? Json(lo.toDouble()) : Json();
    }
    if (m.contains("notas")) c->notas = s(m.value("notas").toString().left(Carrera::kMaxNotas));
    if (!c->parametrosFijos()) {   // con vueltas corridas no se cambian
        c->tiempoIdealMs = T;
        c->distanciaM = std::max<qlonglong>(0, std::min<qlonglong>(1000000, m.value("distanciaM").toLongLong()));
        const QString p = m.value("patron").toString();
        c->patron = s(p == "todas" || p == "alt1" || p == "alt2" ? p : "alt1");
    }
    guardar();
    emit carrerasChanged();
    r["ok"] = true;
    r["id"] = q(c->id);
    return r;
}

QVariantMap Datos::informe(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c) return {};
    const std::string j = datosInforme(*c, cfg_["umbralPrecisionM"].aNumero(20)).escribir();
    return QJsonDocument::fromJson(QByteArray::fromStdString(j)).toVariant().toMap();
}

QString Datos::nombreInforme(const QString& id) const {
    const Carrera* c = buscar(id);
    return c ? q(beto::nombreInforme(*c)) : QString();
}

QByteArray Datos::informeHtml(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c) return {};
    QFile p(QStringLiteral(":/recursos/informe-plantilla.html"));
    if (!p.open(QIODevice::ReadOnly)) return {};
    return QByteArray::fromStdString(beto::informeHtml(*c, cfg_["umbralPrecisionM"].aNumero(20), p.readAll().toStdString()));
}

QString Datos::guardarInformeHtml(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c) return {};
    const QByteArray html = informeHtml(id);
    if (html.isEmpty()) return {};
    QDir().mkpath(carpeta_ + "/informes");
    const QString ruta = carpeta_ + "/informes/" + q(beto::nombreInforme(*c));
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    f.write(html);
    return ruta;
}

QString Datos::lugarLibre() const {
    const QStorageInfo info(carpeta_);
    const double gb = static_cast<double>(info.bytesAvailable()) / (1024.0 * 1024 * 1024);
    if (gb >= 1) return QString::number(gb, 'f', 1).replace('.', ',') + " GB";
    return QString::number(static_cast<int>(gb * 1024)) + " MB";
}

bool Datos::pocoLugar() const {
    const QStorageInfo info(carpeta_);
    return info.isValid() && info.bytesAvailable() < 1024LL * 1024 * 1024;
}

const Carrera* Datos::buscar(const QString& id) const {
    for (const auto& c : base_.carreras) if (q(c.id) == id) return &c;
    return nullptr;
}

QString Datos::crearPractica() {
    Carrera c;
    const QDateTime ahora = QDateTime::currentDateTime();
    c.id = nuevaId(ahora.toMSecsSinceEpoch(), QRandomGenerator::global()->generate());
    c.nombre = s("Práctica " + ahora.toString("dd/MM/yy HH:mm"));
    c.fecha = s(ahora.date().toString(Qt::ISODate));
    c.practica = true;
    c.creada = s(ahora.toUTC().toString(Qt::ISODateWithMs));
    c.tiempoIdealMs = cfg_["tiempoIdealMs"].aEntero(240000);
    c.distanciaM = cfg_["distanciaM"].aEntero(3200);
    c.patron = cfg_["patron"].aTexto("alt1");
    base_.carreras.push_back(c);
    guardar();
    emit carrerasChanged();
    return q(c.id);
}

// Recorrido libre (Matías, 07/11/2026): una práctica de una sola «vuelta» abierta, sin tiempo ideal ni
// distancia. Se graba todo el recorrido; MARCAR deja marcas (extra «marcas») y no corta nada.
QString Datos::crearRecorrido() {
    Carrera c;
    const QDateTime ahora = QDateTime::currentDateTime();
    c.id = nuevaId(ahora.toMSecsSinceEpoch(), QRandomGenerator::global()->generate());
    c.nombre = s("Recorrido " + ahora.toString("dd/MM/yy HH:mm"));
    c.fecha = s(ahora.date().toString(Qt::ISODate));
    c.practica = true;
    c.creada = s(ahora.toUTC().toString(Qt::ISODateWithMs));
    c.tiempoIdealMs = kRecorridoIdealMs;
    c.distanciaM = 0;
    c.patron = "todas";
    c.extra["recorrido"] = true;
    c.extra["marcas"] = Json::lista();
    base_.carreras.push_back(c);
    guardar();
    emit carrerasChanged();
    return q(c.id);
}

bool Datos::ponerExtra(const QString& id, const std::string& clave, const Json& valor) {
    Carrera* c = base_.buscar(s(id));
    if (!c) return false;
    c->extra[clave] = valor;
    guardarLuego();
    emit carrerasChanged();
    return true;
}

bool Datos::quitarExtra(const QString& id, const std::string& clave) {
    Carrera* c = base_.buscar(s(id));
    if (!c || !c->extra.tiene(clave)) return false;
    c->extra.sacar(clave);
    guardar();
    emit carrerasChanged();
    return true;
}

bool Datos::agregarVuelta(const QString& id, const Vuelta& v) {
    Carrera* c = base_.buscar(s(id));
    if (!c) return false;
    c->vueltas.push_back(v);
    guardar();   // al instante: un corte de luz no pierde la vuelta
    emit carrerasChanged();
    return true;
}

bool Datos::agregarVideo(const QString& id, const QVariantMap& v) {
    Carrera* c = base_.buscar(s(id));
    if (!c) return false;
    Json j = Json::objeto();
    j["archivo"] = s(v.value("archivo").toString());
    j["largada"] = static_cast<int64_t>(v.value("largada").toInt());
    j["desdeLargadaMs"] = static_cast<int64_t>(v.value("desdeLargadaMs").toLongLong());
    j["girar"] = v.value("girar").toBool();
    j["espejo"] = v.value("espejo").toBool();
    if (!c->extra["videos"].esLista()) c->extra["videos"] = Json::lista();
    c->extra["videos"].agregar(std::move(j));
    guardar();
    emit carrerasChanged();
    return true;
}

QVariantList Datos::videos(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c || !c->extra["videos"].esLista()) return {};
    return QJsonDocument::fromJson(QByteArray::fromStdString(c->extra["videos"].escribir())).toVariant().toList();
}

bool Datos::nombreVideoValido(const QString& a) {
    if (a.isEmpty() || a.startsWith('.') || a.contains('/') || a.contains('\\') || a.contains(QLatin1String(".."))) return false;
    return a.endsWith(QLatin1String(".mkv"), Qt::CaseInsensitive) || a.endsWith(QLatin1String(".mp4"), Qt::CaseInsensitive);
}

static QString tamTexto(qint64 b) {
    if (b >= 1000LL * 1048576) return QString::number(b / 1073741824.0, 'f', 1).replace('.', ',') + QStringLiteral(" GB");
    return QString::number(std::max<qint64>(1, qRound64(b / 1048576.0))) + QStringLiteral(" MB");
}

QVariantList Datos::videosGuardados() const {
    // De qué carrera es cada archivo (lo dice «videos» de cada carrera)
    std::map<QString, const Carrera*> deCarrera;
    for (const auto& c : base_.carreras)
        if (c.extra["videos"].esLista())
            for (const auto& v : c.extra["videos"].elementos()) deCarrera[q(v["archivo"].aTexto())] = &c;
    QVariantList r;
    const QFileInfoList l = QDir(carpetaVideos()).entryInfoList({QStringLiteral("*.mkv"), QStringLiteral("*.mp4")}, QDir::Files, QDir::Time);
    for (const QFileInfo& f : l) {
        QVariantMap m{{"archivo", f.fileName()}, {"bytes", f.size()}, {"tam", tamTexto(f.size())},
                      {"cuando", f.lastModified().toMSecsSinceEpoch()},
                      {"hora", f.lastModified().toString(QStringLiteral("dd/MM/yy HH:mm"))},
                      {"prueba", f.fileName().startsWith(QLatin1String("prueba-"))}};
        auto it = deCarrera.find(f.fileName());
        if (it != deCarrera.end()) {
            const QDate fc = QDate::fromString(q(it->second->fecha), Qt::ISODate);
            m["carreraId"] = q(it->second->id);
            m["carrera"] = q(it->second->nombre);
            m["fecha"] = fc.isValid() ? fc.toString(QStringLiteral("dd/MM/yy")) : QString();
        }
        r << m;
    }
    return r;
}

bool Datos::olvidarVideo(const QString& archivo) {
    bool cambio = false;
    for (auto& c : base_.carreras) {
        if (!c.extra["videos"].esLista()) continue;
        auto& l = c.extra["videos"].elementos();
        const auto n = l.size();
        l.erase(std::remove_if(l.begin(), l.end(), [&](const Json& v) { return q(v["archivo"].aTexto()) == archivo; }), l.end());
        if (l.size() == n) continue;
        if (l.empty()) c.extra.sacar("videos");   // sin videos: el informe no muestra «Video»
        cambio = true;
    }
    if (cambio) { guardar(); emit carrerasChanged(); }
    return cambio;
}

bool Datos::renombrarVideo(const QString& viejo, const QString& nuevo) {
    bool cambio = false;
    for (auto& c : base_.carreras) {
        if (!c.extra["videos"].esLista()) continue;
        for (auto& v : c.extra["videos"].elementos())
            if (q(v["archivo"].aTexto()) == viejo) { v["archivo"] = s(nuevo); cambio = true; }
    }
    if (cambio) { guardar(); emit carrerasChanged(); }
    return cambio;
}

bool Datos::borrarVideo(const QString& archivo) {
    if (!nombreVideoValido(archivo)) return false;
    const QString ruta = carpetaVideos() + '/' + archivo;
    const bool habia = QFileInfo::exists(ruta);
    if (habia && !QFile::remove(ruta)) return false;
    const bool enCarrera = olvidarVideo(archivo);
    return habia || enCarrera;
}

bool Datos::cambiarMedida(const QString& id, int indice) {
    Carrera* c = base_.buscar(s(id));
    if (!c || indice < 0 || indice >= static_cast<int>(c->vueltas.size())) return false;
    Vuelta& v = c->vueltas[static_cast<size_t>(indice)];
    if (v.interrumpida) return false;
    c->guardarOriginal();
    v.medida = !v.medida;
    guardar();
    emit carrerasChanged();
    return true;
}

bool Datos::unirConSiguiente(const QString& id, int indice) {
    Carrera* c = base_.buscar(s(id));
    if (!c || indice < 0) return false;
    Carrera antes = *c;
    if (!c->unirConSiguiente(static_cast<size_t>(indice))) return false;
    if (!antes.modificada()) { antes.guardarOriginal(); c->extra = antes.extra; }
    guardar();
    emit carrerasChanged();
    return true;
}

bool Datos::restablecer(const QString& id) {
    Carrera* c = base_.buscar(s(id));
    if (!c || !c->restablecer()) return false;
    guardar();
    emit carrerasChanged();
    return true;
}

bool Datos::modificada(const QString& id) const {
    for (const auto& c : base_.carreras)
        if (q(c.id) == id) return c.modificada();
    return false;
}

bool Datos::borrarCarrera(const QString& id) {
    auto& v = base_.carreras;
    auto it = std::find_if(v.begin(), v.end(), [&](const Carrera& c) { return q(c.id) == id; });
    if (it == v.end()) return false;
    // Sus videos también (Matías, 07/11): si no, quedaban en la tarjeta ocupando lugar. Solo los que no usa otra carrera.
    QStringList videos;
    if (it->extra["videos"].esLista())
        for (const auto& x : it->extra["videos"].elementos()) videos << q(x["archivo"].aTexto());
    v.erase(it);
    for (const QString& a : videos) {
        bool otra = false;
        for (const auto& c : v)
            if (c.extra["videos"].esLista())
                for (const auto& x : c.extra["videos"].elementos()) otra = otra || q(x["archivo"].aTexto()) == a;
        if (!otra && nombreVideoValido(a)) QFile::remove(carpetaVideos() + '/' + a);
    }
    guardar();
    emit carrerasChanged();
    return true;
}

static QVariant aVariant(const Json& j) {
    switch (j.tipo()) {
        case Json::Tipo::Bool: return j.aBool();
        case Json::Tipo::Numero: return j.aNumero();
        case Json::Tipo::Texto: return q(j.aTexto());
        case Json::Tipo::Lista:
        case Json::Tipo::Objeto: return QJsonDocument::fromJson(QByteArray::fromStdString(j.escribir())).toVariant();
        default: return {};
    }
}

QVariant Datos::cfg(const QString& clave) const { return aVariant(cfg_[s(clave)]); }

QVariantMap Datos::ajustes() const {
    QVariantMap m;
    for (const auto& p : cfg_.pares()) m[q(p.first)] = aVariant(p.second);
    return m;
}

void Datos::ponerCfg(const QString& clave, const QVariant& valorQml) {
    // Un objeto de JavaScript puede llegar como QJSValue: se pasa a mapa/lista (si no, se guardaba como
    // el texto «[object Object]» y no andaban las llaves de «Datos sobre el video»)
#ifdef BETO_CON_QJS
    const QVariant valor = valorQml.typeId() == qMetaTypeId<QJSValue>() ? valorQml.value<QJSValue>().toVariant() : valorQml;
#else
    const QVariant& valor = valorQml;
#endif
    Json v;
    switch (valor.typeId()) {
        case QMetaType::Bool: v = valor.toBool(); break;
        case QMetaType::Int: case QMetaType::LongLong: case QMetaType::Double: v = valor.toDouble(); break;
        case QMetaType::QVariantList: case QMetaType::QVariantMap: case QMetaType::QJsonArray: case QMetaType::QJsonValue:
            v = Json::leer(QJsonDocument::fromVariant(valor).toJson(QJsonDocument::Compact).toStdString());
            break;
        default:
            if (valor.canConvert<QVariantList>() && !valor.canConvert<QString>()) {
                v = Json::leer(QJsonDocument::fromVariant(valor.toList()).toJson(QJsonDocument::Compact).toStdString());
            } else {
                v = s(valor.toString());
            }
    }
    base_.config[s(clave)] = v;
    cfg_ = armarConfig(base_.config);
    guardarLuego();
    emit cfgChanged();
}

void Datos::cargarMuestra() {
    const Json config = base_.config;   // las carreras de ejemplo no pisan la configuración (armados de prueba)
    base_ = Base{};
    base_.config = config;
    const QDate h = hoy();
    auto agregar = [&](const char* nombre, QDate f, int vueltas, int64_t T, int64_t D, const char* patron,
                       std::vector<int> errores, bool practica = false) {
        Carrera c;
        c.id = nuevaId(1730000000000 + static_cast<int64_t>(base_.carreras.size()), 7);
        c.nombre = nombre;
        c.fecha = s(f.toString(Qt::ISODate));
        (void)vueltas;
        c.tiempoIdealMs = T;
        c.distanciaM = D;
        c.patron = patron;
        c.practica = practica;
        c.creada = s(QDateTime(f, QTime(10, 0)).toString(Qt::ISODate));
        int n = 1;
        for (int e : errores) {
            Vuelta v;
            v.n = n; v.medida = esMedida(c.patron, n); v.idealMs = T; v.realMs = T + e; v.difMs = e;
            // recorrido de ejemplo: un circuito con curvas, a 1 lectura por segundo
            const double L = static_cast<double>(D), seg = (T + e) / 1000.0;
            double d = 0, vmax = 0;
            for (int t = 0; t <= static_cast<int>(seg); ++t) {
                const double fr = d / L, a = 2 * M_PI * fr;
                const double kmh = L / seg * 3.6 * (1 + 0.28 * std::sin(a * 3 + n) - 0.1 * std::cos(a * 5));
                const double R = L / (2 * M_PI), x = R * 1.5 * std::cos(a) + R * 0.25 * std::cos(3 * a), y = R * 0.8 * std::sin(a) + R * 0.15 * std::sin(2 * a);
                FilaTraza p{static_cast<double>(t), redondearJs(d * 10) / 10, fijarDecimalesJs(kmh, 1),
                            fijarDecimalesJs(-32.97 + y / 110540, 6), fijarDecimalesJs(-60.65 + x / (111320 * std::cos(32.97 * M_PI / 180)), 6),
                            static_cast<double>(2 + (t * 7 + n) % 4)};
                v.traza.push_back(p);
                vmax = std::max(vmax, kmh);
                d += kmh / 3.6;
            }
            v.distM = redondearJs(d);
            v.velPromKmh = fijarDecimalesJs(d / seg * 3.6, 1);
            v.velMaxKmh = fijarDecimalesJs(vmax, 1);
            c.vueltas.push_back(v);
            ++n;
        }
        base_.carreras.push_back(c);
    };
    agregar("Gualeguaychú", h.addDays(14), 10, 240000, 3200, "alt1", {});
    agregar("Concepción del Uruguay", h.addDays(28), 8, 210000, 2800, "todas", {});
    // Localidades de muestra (para el clima)
    auto lugar = [&](size_t i, const char* l, double la, double lo) {
        Carrera& c = base_.carreras[base_.carreras.size() - 2 + i];
        c.lugar = l; c.extra["lugarLat"] = Json(la); c.extra["lugarLon"] = Json(lo);
    };
    lugar(0, "Gualeguaychú, Entre Ríos", -33.009, -58.517);
    lugar(1, "Concepción del Uruguay, Entre Ríos", -32.484, -58.232);
    agregar("Santa Clara", h.addDays(-1), 6, 180000, 2500, "todas", {1500, -2100, 800, 1900, -1700, 1600});
    // Para probar el reproductor del informe: si hay un videos/muestra.mkv en la carpeta, va con Santa Clara
    if (QFile::exists(carpeta_ + QStringLiteral("/videos/muestra.mkv"))) {
        Json v = Json::objeto();
        v["archivo"] = std::string("muestra.mkv");
        v["largada"] = static_cast<int64_t>(1);
        v["desdeLargadaMs"] = static_cast<int64_t>(5000);
        v["girar"] = false;
        base_.carreras.back().extra["videos"] = Json::lista();
        base_.carreras.back().extra["videos"].agregar(std::move(v));
    }
    agregar("Paraná", h.addDays(-14), 8, 200000, 2600, "todas", {1200, -2500, 1800, -900, 2200, -1100, 1300, 1100});
    // Video de ejemplo para Santa Clara, solo si está el archivo (para probar el reproductor del informe)
    if (QFileInfo::exists(carpetaVideos() + QStringLiteral("/muestra-santa-clara.mkv"))) {
        Json v = Json::objeto();
        v["archivo"] = "muestra-santa-clara.mkv"; v["largada"] = 1; v["desdeLargadaMs"] = 5000; v["girar"] = false;
        Json l = Json::lista();
        l.agregar(std::move(v));
        base_.carreras[base_.carreras.size() - 2].extra["videos"] = l;
    }
    // Solo para probar pantallas con muchas vueltas (BETO_MUESTRA_LARGA=1): una carrera de 60 vueltas
    // y una práctica de 40, cortitas para que el video de muestra alcance a todas
    if (qEnvironmentVariableIntValue("BETO_MUESTRA_LARGA") == 1) {
        std::vector<int> e60, e40;
        for (int i = 0; i < 60; ++i) e60.push_back(((i * 37) % 23 - 11) * 130);
        for (int i = 0; i < 40; ++i) e40.push_back(((i * 53) % 19 - 9) * 210);
        const bool hayVideo = QFile::exists(carpeta_ + QStringLiteral("/videos/muestra.mkv"));
        auto conVideo = [&](int64_t desde) {
            if (!hayVideo) return;
            Json v = Json::objeto();
            v["archivo"] = std::string("muestra.mkv");
            v["largada"] = static_cast<int64_t>(1);
            v["desdeLargadaMs"] = desde;
            v["girar"] = false;
            base_.carreras.back().extra["videos"] = Json::lista();
            base_.carreras.back().extra["videos"].agregar(std::move(v));
        };
        agregar("Rosario Nocturno", h.addDays(-3), 60, 17000, 400, "alt1", e60);
        conVideo(3000);
        agregar("Práctica larga", h.addDays(-2), 40, 25000, 600, "todas", e40, true);
        conVideo(3590000);   // el video empezó una hora antes: horas en «▸ 1:00:10»
    }
    cfg_ = armarConfig(base_.config);
    emit carrerasChanged();
}
