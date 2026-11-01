#include "datos.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QStorageInfo>

#include "informe.h"
#include <QDir>
#include <QLocale>
#include <QRandomGenerator>
#include <algorithm>
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

// Tiempo ideal: m:ss,d
static QString tiempo(int64_t ms) {
    int64_t dec = ms / 100, d = dec % 10, sg = dec / 10;
    return QString::number(sg / 60) + ":" + QString::number(sg % 60).rightJustified(2, '0') + "," + QString::number(d);
}
static QString miles(int64_t n) { return QLocale(QLocale::Spanish, QLocale::Argentina).toString(static_cast<qlonglong>(n)); }

Datos::Datos(const QString& carpeta, QObject* padre) : QObject(padre), carpeta_(carpeta), almacen_(s(carpeta)) {
    QDir().mkpath(carpeta);
    auto l = almacen_.cargar();
    base_ = std::move(l.base);
    cfg_ = armarConfig(base_.config);
    if (l.habiaDanado) qWarning("La base estaba dañada: se usó %s", l.origen.c_str());
}

void Datos::guardar() {
    std::string err;
    if (!almacen_.guardar(base_, &err)) emit error(q("No se pudo guardar: " + err));
}

bool Datos::finalizada(const Carrera& c) const {
    if (c.vueltasTotal <= 0) return false;
    std::map<int, int> n;
    for (const auto& v : c.vueltas) if (!v.interrumpida) ++n[v.largada];
    for (const auto& p : n) if (p.second >= c.vueltasTotal) return true;
    return false;
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
    r["dia"] = f.isValid() ? QString::number(f.day()).rightJustified(2, '0') : QString("--");
    r["mes"] = f.isValid() ? QString(kMeses[f.month() - 1]) : QString();
    r["fechaCorta"] = f.isValid() ? f.toString("dd/MM") : QString();
    r["fechaLarga"] = f.isValid() ? QLocale(QLocale::Spanish, QLocale::Argentina).toString(f, "dddd, d 'de' MMMM 'de' yyyy") : QString();
    r["notas"] = q(c.notas);
    r["practica"] = c.practica;
    r["vueltasTotal"] = c.vueltasTotal;
    r["distanciaM"] = static_cast<qlonglong>(c.distanciaM);
    r["tiempoIdealMs"] = static_cast<qlonglong>(c.tiempoIdealMs);
    r["patron"] = q(c.patron);
    r["tieneVueltas"] = !c.vueltas.empty();
    r["finalizada"] = fin;
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
    if (c.vueltasTotal > 0) partes << QString::number(c.vueltasTotal) + " vueltas";
    if (medidas) {
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
        motivo = dias > 0 ? "Se larga el " + QLocale(QLocale::Spanish, QLocale::Argentina).toString(f, "d MMM yyyy")
                          : QString("La fecha de la carrera ya pasó");
    }
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
    n.vueltasTotal = 10;
    QVariantMap r = resumen(n);
    r["id"] = QString();
    r["nueva"] = true;
    return r;
}

QVariantMap Datos::guardarCarrera(const QVariantMap& m) {
    QVariantMap r;
    auto falla = [&](const QString& e) { r["ok"] = false; r["error"] = e; return r; };
    const QString nombre = m.value("nombre").toString().trimmed();
    const int vueltas = m.value("vueltasTotal").toInt();
    const qlonglong T = m.value("tiempoIdealMs").toLongLong();
    if (nombre.isEmpty()) return falla("Poné un nombre a la carrera.");
    if (!QDate::fromString(m.value("fecha").toString(), Qt::ISODate).isValid()) return falla("Elegí la fecha de la carrera.");
    if (vueltas < 1) return falla("Poné la cantidad de vueltas de la carrera.");
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
    c->vueltasTotal = std::min(300, vueltas);
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

QString Datos::guardarInformeHtml(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c) return {};
    QFile p(QStringLiteral(":/recursos/informe-plantilla.html"));
    if (!p.open(QIODevice::ReadOnly)) return {};
    const std::string html = informeHtml(*c, cfg_["umbralPrecisionM"].aNumero(20), p.readAll().toStdString());
    QDir().mkpath(carpeta_ + "/informes");
    const QString ruta = carpeta_ + "/informes/" + q(beto::nombreInforme(*c));
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    f.write(QByteArray::fromStdString(html));
    return ruta;
}

QString Datos::lugarLibre() const {
    const QStorageInfo info(carpeta_);
    const double gb = static_cast<double>(info.bytesAvailable()) / (1024.0 * 1024 * 1024);
    if (gb >= 1) return QString::number(gb, 'f', 1).replace('.', ',') + " GB";
    return QString::number(static_cast<int>(gb * 1024)) + " MB";
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

bool Datos::agregarVuelta(const QString& id, const Vuelta& v) {
    Carrera* c = base_.buscar(s(id));
    if (!c) return false;
    c->vueltas.push_back(v);
    guardar();   // al instante: un corte de luz no pierde la vuelta
    emit carrerasChanged();
    return true;
}

bool Datos::borrarCarrera(const QString& id) {
    auto& v = base_.carreras;
    auto it = std::find_if(v.begin(), v.end(), [&](const Carrera& c) { return q(c.id) == id; });
    if (it == v.end()) return false;
    v.erase(it);
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

void Datos::ponerCfg(const QString& clave, const QVariant& valor) {
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
    guardar();
    emit cfgChanged();
}

void Datos::cargarMuestra() {
    base_ = Base{};
    const QDate h = hoy();
    auto agregar = [&](const char* nombre, QDate f, int vueltas, int64_t T, int64_t D, const char* patron,
                       std::vector<int> errores, bool practica = false) {
        Carrera c;
        c.id = nuevaId(1730000000000 + static_cast<int64_t>(base_.carreras.size()), 7);
        c.nombre = nombre;
        c.fecha = s(f.toString(Qt::ISODate));
        c.vueltasTotal = vueltas;
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
    agregar("Santa Clara", h.addDays(-1), 6, 180000, 2500, "todas", {1500, -2100, 800, 1900, -1700, 1600});
    agregar("Paraná", h.addDays(-14), 8, 200000, 2600, "todas", {1200, -2500, 1800, -900, 2200, -1100, 1300, 1100});
    cfg_ = armarConfig(base_.config);
    emit carrerasChanged();
}
