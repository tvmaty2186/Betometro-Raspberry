// La base de carreras: cargar, crear, cambiar y borrar carreras (ver datos.h para el reparto en archivos).
#include "datos.h"
#include "datos_comun.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QStorageInfo>

#include <algorithm>

using namespace beto;

Datos::Datos(const QString& carpeta, QObject* padre) : QObject(padre), carpeta_(carpeta), almacen_(s(carpeta)) {
    QDir().mkpath(carpeta);
    auto l = almacen_.cargar();
    base_ = std::move(l.base);
    cfg_ = armarConfig(base_.config);
    for (const std::string& a : l.avisos) qWarning("Base: %s", a.c_str());
    if (l.habiaDanado) qWarning("La base estaba dañada: se usó %s", l.origen.c_str());
    if (l.migrado) qWarning("Base migrada a un archivo por carrera (%zu carreras); betometro.json quedó como .migrado", base_.carreras.size());
    almacen_.vaciarPapelera();
    hilo_ = std::thread([this] { hiloGuardar(); });
    guardarLuego_.setSingleShot(true);
    guardarLuego_.setInterval(300);
    connect(&guardarLuego_, &QTimer::timeout, this, [this] { guardarPendientes(); });
    // Carrera o recorrido libre que se cortó (luz, vigilante) a mitad de una vuelta: lo andado queda como vuelta interrumpida
    for (Carrera& c : base_.carreras) {
        if (!c.extra.tiene("enCurso")) continue;
        if (c.extra["enCurso"].esObjeto()) {
            Vuelta v = Vuelta::desdeJson(c.extra["enCurso"]);
            v.interrumpida = true;
            c.vueltas.push_back(std::move(v));
            qWarning("«%s» se cortó a mitad de la vuelta %d: se recuperó lo andado (%.0f s)", c.nombre.c_str(), v.n, v.realMs / 1000.0);
        }
        c.extra.sacar("enCurso");
        guardar(q(c.id));
    }
}

Datos::~Datos() {
    guardarPendientes();   // lo que estaba juntándose
    {
        std::lock_guard<std::mutex> l(m_);
        terminar_ = true;
    }
    cv_.notify_all();
    if (hilo_.joinable()) hilo_.join();   // escribe lo último que quedó pendiente antes de cerrar
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
    guardar(q(c->id));
    emit carrerasChanged();
    r["ok"] = true;
    r["id"] = q(c->id);
    return r;
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
    guardar(q(c.id));
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
    guardar(q(c.id));
    emit carrerasChanged();
    return q(c.id);
}

bool Datos::ponerExtra(const QString& id, const std::string& clave, const Json& valor) {
    Carrera* c = base_.buscar(s(id));
    if (!c) return false;
    c->extra[clave] = valor;
    guardarLuego(id);
    emit carrerasChanged();
    return true;
}

bool Datos::quitarExtra(const QString& id, const std::string& clave) {
    Carrera* c = base_.buscar(s(id));
    if (!c || !c->extra.tiene(clave)) return false;
    c->extra.sacar(clave);
    guardar(id);
    emit carrerasChanged();
    return true;
}

bool Datos::agregarVuelta(const QString& id, const Vuelta& v) {
    Carrera* c = base_.buscar(s(id));
    if (!c) return false;
    c->vueltas.push_back(v);
    guardar(id);   // al instante: un corte de luz no pierde la vuelta
    emit carrerasChanged();
    return true;
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
    borrar(id);   // su archivo va a carreras/papelera/ (se vacía a los 30 días)
    emit carrerasChanged();
    return true;
}
