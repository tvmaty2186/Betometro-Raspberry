#include "clima.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QSaveFile>
#include <QUrlQuery>
#include <QtMath>

int Clima::kDias = 16;

namespace {
const char* kAgente = "Betometro/" BETO_VERSION " (equipo de regularidad)";
constexpr qint64 kViejoMs = 30 * 60 * 1000;   // un pronóstico de hace más de 30 min se vuelve a pedir

QNetworkRequest pedido(const QUrl& u) {
    QNetworkRequest q(u);
    q.setRawHeader("User-Agent", kAgente);
    q.setTransferTimeout(15000);
    return q;
}
double km(double la1, double lo1, double la2, double lo2) {
    const double dy = (la2 - la1) * 111.32, dx = (lo2 - lo1) * 111.32 * qCos(qDegreesToRadians(la1));
    return qSqrt(dx * dx + dy * dy);
}
}  // namespace

Clima::Clima(const QString& carpetaDatos, QObject* padre)
    : QObject(padre), real_(qEnvironmentVariableIsSet("BETO_RASPBERRY")), archivo_(carpetaDatos + QStringLiteral("/clima.json")) {
    cargarCache();
    if (!real_) {   // PC: una posición de GPS de prueba
        hayGps_ = true; gpsLat_ = -37.836; gpsLon_ = -57.506; nombreGps_ = QStringLiteral("Santa Clara del Mar");
    }
}

QString Clima::clave(double lat, double lon) { return QString::number(lat, 'f', 2) + ',' + QString::number(lon, 'f', 2); }

void Clima::cargarCache() {
    QFile f(archivo_);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = o.begin(); it != o.end(); ++it) {
        const QJsonObject e = it.value().toObject();
        Entrada x;
        x.datos = e.value(QStringLiteral("datos")).toObject();
        x.cuando = static_cast<qint64>(e.value(QStringLiteral("cuando")).toDouble());
        if (!x.datos.isEmpty()) cache_.insert(it.key(), x);
    }
}

void Clima::guardarCache() const {
    QJsonObject o;
    const qint64 ahora = QDateTime::currentMSecsSinceEpoch();
    for (auto it = cache_.begin(); it != cache_.end(); ++it)   // solo lo de la última semana
        if (!it->datos.isEmpty() && ahora - it->cuando < 7LL * 24 * 3600 * 1000)
            o.insert(it.key(), QJsonObject{{"datos", it->datos}, {"cuando", static_cast<double>(it->cuando)}});
    QSaveFile f(archivo_);
    if (f.open(QIODevice::WriteOnly)) { f.write(QJsonDocument(o).toJson(QJsonDocument::Compact)); f.commit(); }
}

// ---------------------------------------------------------------- buscar localidad

void Clima::buscarLugar(const QString& texto) {
    const QString t = texto.trimmed();
    const int yo = ++pedidoLugar_;
    if (t.size() < 3) { sugerencias_.clear(); buscandoLugar_ = false; emit sugerenciasChanged(); return; }
    buscandoLugar_ = true;
    emit sugerenciasChanged();
    if (!real_) {
        static const QVariantList lugares = {
            QVariantMap{{"nombre", "Santa Clara del Mar"}, {"detalle", "Buenos Aires, Argentina"}, {"lat", -37.836}, {"lon", -57.506}},
            QVariantMap{{"nombre", "Gualeguaychú"}, {"detalle", "Entre Ríos, Argentina"}, {"lat", -33.009}, {"lon", -58.517}},
            QVariantMap{{"nombre", "Concepción del Uruguay"}, {"detalle", "Entre Ríos, Argentina"}, {"lat", -32.484}, {"lon", -58.232}},
            QVariantMap{{"nombre", "Quilmes"}, {"detalle", "Buenos Aires, Argentina"}, {"lat", -34.720}, {"lon", -58.254}},
            QVariantMap{{"nombre", "Paraná"}, {"detalle", "Entre Ríos, Argentina"}, {"lat", -31.732}, {"lon", -60.529}}};
        QTimer::singleShot(300, this, [this, yo, t] {
            if (yo != pedidoLugar_) return;
            sugerencias_.clear();
            for (const auto& l : lugares)
                if (l.toMap().value("nombre").toString().contains(t, Qt::CaseInsensitive)) sugerencias_ << l;
            buscandoLugar_ = false;
            emit sugerenciasChanged();
        });
        return;
    }
    QUrl u(QStringLiteral("https://geocoding-api.open-meteo.com/v1/search"));
    u.setQuery(QUrlQuery{{"name", t}, {"count", "8"}, {"language", "es"}, {"format", "json"}});
    QNetworkReply* r = red_.get(pedido(u));
    connect(r, &QNetworkReply::finished, this, [this, r, yo, t] {
        r->deleteLater();
        if (yo != pedidoLugar_) return;   // ya escribió otra cosa
        QVariantList l;
        for (const auto& v : QJsonDocument::fromJson(r->readAll()).object().value(QStringLiteral("results")).toArray()) {
            const QJsonObject o = v.toObject();
            QStringList det;
            for (const char* k : {"admin1", "country"}) { const QString x = o.value(QLatin1String(k)).toString(); if (!x.isEmpty() && !det.contains(x)) det << x; }
            l << QVariantMap{{"nombre", o.value(QStringLiteral("name")).toString()}, {"detalle", det.join(QStringLiteral(", "))},
                             {"lat", o.value(QStringLiteral("latitude")).toDouble()}, {"lon", o.value(QStringLiteral("longitude")).toDouble()}};
        }
        if (l.isEmpty()) { buscarEnOsm(t, yo); return; }   // nada (¿error de ortografía?): OpenStreetMap
        sugerencias_ = l;
        buscandoLugar_ = false;
        emit sugerenciasChanged();
    });
}

void Clima::buscarEnOsm(const QString& texto, int yo) {
    QUrl u(QStringLiteral("https://nominatim.openstreetmap.org/search"));
    u.setQuery(QUrlQuery{{"q", texto}, {"format", "jsonv2"}, {"limit", "8"}, {"accept-language", "es"}, {"addressdetails", "1"}});
    QNetworkReply* r = red_.get(pedido(u));
    connect(r, &QNetworkReply::finished, this, [this, r, yo] {
        r->deleteLater();
        if (yo != pedidoLugar_) return;
        QVariantList l;
        for (const auto& v : QJsonDocument::fromJson(r->readAll()).array()) {
            const QJsonObject o = v.toObject(), a = o.value(QStringLiteral("address")).toObject();
            QString nombre = o.value(QStringLiteral("name")).toString();
            if (nombre.isEmpty()) nombre = o.value(QStringLiteral("display_name")).toString().section(',', 0, 0);
            QStringList det;
            for (const char* k : {"state", "country"}) { const QString x = a.value(QLatin1String(k)).toString(); if (!x.isEmpty()) det << x; }
            l << QVariantMap{{"nombre", nombre}, {"detalle", det.join(QStringLiteral(", "))},
                             {"lat", o.value(QStringLiteral("lat")).toString().toDouble()}, {"lon", o.value(QStringLiteral("lon")).toString().toDouble()}};
        }
        sugerencias_ = l;
        buscandoLugar_ = false;
        emit sugerenciasChanged();
    });
}

// ---------------------------------------------------------------- GPS

void Clima::ponerPosicion(double lat, double lon) {
    if (!real_ || (lat == 0 && lon == 0)) return;
    const bool nuevo = !hayGps_;
    gpsLat_ = lat; gpsLon_ = lon; hayGps_ = true;
    if (nuevo) emit gpsChanged();
    // El nombre del lugar se pregunta de nuevo solo si se movió más de 5 km
    if (km(lat, lon, nombradoLat_, nombradoLon_) > 5) nombrarGps();
}

void Clima::nombrarGps() {
    nombradoLat_ = gpsLat_; nombradoLon_ = gpsLon_;
    QUrl u(QStringLiteral("https://nominatim.openstreetmap.org/reverse"));
    u.setQuery(QUrlQuery{{"lat", QString::number(gpsLat_, 'f', 4)}, {"lon", QString::number(gpsLon_, 'f', 4)},
                         {"format", "jsonv2"}, {"zoom", "10"}, {"accept-language", "es"}});
    QNetworkReply* r = red_.get(pedido(u));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) { nombradoLat_ = 999; return; }   // sin internet: se reintenta
        const QJsonObject a = QJsonDocument::fromJson(r->readAll()).object().value(QStringLiteral("address")).toObject();
        for (const char* k : {"city", "town", "village", "municipality", "county", "state"}) {
            const QString x = a.value(QLatin1String(k)).toString();
            if (!x.isEmpty()) { nombreGps_ = x; emit gpsChanged(); return; }
        }
    });
}

// ---------------------------------------------------------------- pronóstico

// Avisa a la pantalla después (nunca mientras la pantalla está leyendo: daba un lazo de enlaces)
void Clima::avisar() {
    if (avisoPendiente_) return;
    avisoPendiente_ = true;
    QTimer::singleShot(0, this, [this] { avisoPendiente_ = false; ++version_; emit cambio(); });
}

void Clima::pedir(double lat, double lon, bool forzar) {
    const QString k = clave(lat, lon);
    Entrada& e = cache_[k];
    const qint64 ahora = QDateTime::currentMSecsSinceEpoch();
    if (e.pidiendo || (!forzar && !e.datos.isEmpty() && ahora - e.cuando < kViejoMs)) return;
    if (!forzar && ahora - e.intento < 60000) return;   // falló hace menos de un minuto (sin wifi): no insistir
    e.intento = ahora;
    if (!real_) { e.datos = inventado(lat, lon); e.cuando = ahora; avisar(); return; }
    e.pidiendo = true;
    QUrl u(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    u.setQuery(QUrlQuery{
        {"latitude", QString::number(lat, 'f', 4)}, {"longitude", QString::number(lon, 'f', 4)},
        {"current", "temperature_2m,weather_code,precipitation,wind_speed_10m,wind_gusts_10m"},
        {"hourly", "temperature_2m,precipitation_probability,precipitation,weather_code,wind_speed_10m,wind_gusts_10m"},
        {"daily", "weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,precipitation_sum,wind_speed_10m_max,wind_gusts_10m_max"},
        {"timezone", "auto"}, {"forecast_days", QString::number(kDias)}});
    QNetworkReply* r = red_.get(pedido(u));
    connect(r, &QNetworkReply::finished, this, [this, r, k] {
        r->deleteLater();
        Entrada& e = cache_[k];
        e.pidiendo = false;
        const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        if (r->error() != QNetworkReply::NoError || !o.contains(QStringLiteral("daily"))) {
            e.error = QStringLiteral("Sin internet: no se pudo bajar el pronóstico");
            qWarning("Clima: %s", qPrintable(r->errorString()));
        } else {
            e.datos = o;
            e.cuando = QDateTime::currentMSecsSinceEpoch();
            e.error.clear();
            guardarCache();
        }
        avisar();
    });
    avisar();   // «cargando»
}

QJsonObject Clima::inventado(double lat, double lon) const {
    // Pronóstico de prueba para la PC (no sale a internet)
    const QDate hoy = QDate::currentDate();
    const int h0 = QTime::currentTime().hour();
    QJsonArray ht, hp, hm, hc, hv, hr, dt, dmax, dmin, dc, dp, dmm, dv, dr;
    const int codigos[] = {2, 61, 3, 0, 0, 2, 63, 1, 3, 80, 0, 2, 95, 1, 0, 3};
    const double semilla = qAbs(lat * 7 + lon * 3);
    for (int d = 0; d < kDias; ++d) {
        const double base = 18 + 4 * qSin(d * 0.9 + semilla);
        dt.append(hoy.addDays(d).toString(Qt::ISODate));
        dmax.append(qRound(base + 5)); dmin.append(qRound(base - 4));
        dc.append(codigos[d % 16]); dp.append(codigos[d % 16] >= 60 ? 70 + d % 3 * 10 : d % 4 * 10);
        dmm.append(codigos[d % 16] >= 60 ? 4.5 : 0.0); dv.append(15 + d % 5 * 3); dr.append(28 + d % 5 * 5);
        for (int h = 0; h < 24; ++h) {
            ht.append(hoy.addDays(d).toString(Qt::ISODate) + QStringLiteral("T%1:00").arg(h, 2, 10, QChar('0')));
            hp.append(qRound(base + 5 * qSin((h - 9) * M_PI / 12)));
            const int c = h >= 18 && h <= 22 && d < 2 ? 61 : (h < 7 || h > 20 ? 3 : codigos[d % 16]);
            hc.append(c); hm.append(c >= 60 ? 60 : 10); hr.append(c >= 60 ? 1.2 : 0.0); hv.append(12 + h % 6 * 2);
        }
    }
    QJsonObject hourly{{"time", ht}, {"temperature_2m", hp}, {"precipitation_probability", hm}, {"precipitation", hr},
                       {"weather_code", hc}, {"wind_speed_10m", hv}, {"wind_gusts_10m", hv}};
    QJsonObject daily{{"time", dt}, {"weather_code", dc}, {"temperature_2m_max", dmax}, {"temperature_2m_min", dmin},
                      {"precipitation_probability_max", dp}, {"precipitation_sum", dmm}, {"wind_speed_10m_max", dv}, {"wind_gusts_10m_max", dr}};
    QJsonObject current{{"temperature_2m", 21}, {"weather_code", 2}, {"precipitation", 0}, {"wind_speed_10m", 18}, {"wind_gusts_10m", 32},
                        {"time", hoy.toString(Qt::ISODate) + QStringLiteral("T%1:00").arg(h0, 2, 10, QChar('0'))}};
    return QJsonObject{{"current", current}, {"hourly", hourly}, {"daily", daily}};
}

QVariantMap Clima::pronostico(double lat, double lon) {
    pedir(lat, lon);
    const Entrada e = cache_.value(clave(lat, lon));
    QVariantMap r;
    r["cargando"] = e.pidiendo;
    r["error"] = e.error;
    r["ok"] = !e.datos.isEmpty();
    if (e.datos.isEmpty()) return r;
    const qint64 ahora = QDateTime::currentMSecsSinceEpoch();
    r["actualizado"] = QDateTime::fromMSecsSinceEpoch(e.cuando).toString(QStringLiteral("dd/MM HH:mm"));
    r["viejo"] = ahora - e.cuando > 3LL * 3600 * 1000;
    const QJsonObject c = e.datos.value(QStringLiteral("current")).toObject();
    const QJsonObject h = e.datos.value(QStringLiteral("hourly")).toObject();
    const QJsonArray ht = h.value(QStringLiteral("time")).toArray();
    // Hora actual del lugar (la del pronóstico)
    const QString horaActual = c.value(QStringLiteral("time")).toString().left(13);
    int i0 = 0;
    for (int i = 0; i < ht.size(); ++i) if (ht[i].toString().left(13) == horaActual) { i0 = i; break; }
    r["actual"] = QVariantMap{{"temp", qRound(c.value(QStringLiteral("temperature_2m")).toDouble())},
                              {"codigo", c.value(QStringLiteral("weather_code")).toInt()},
                              {"mm", c.value(QStringLiteral("precipitation")).toDouble()},
                              {"lluvia", h.value(QStringLiteral("precipitation_probability")).toArray().at(i0).toInt()},
                              {"viento", qRound(c.value(QStringLiteral("wind_speed_10m")).toDouble())},
                              {"rafagas", qRound(c.value(QStringLiteral("wind_gusts_10m")).toDouble())}};
    // Las próximas horas (cada 2 h, 12 lecturas: un día entero)
    QVariantList horas;
    for (int i = i0; i < ht.size() && horas.size() < 12; i += 2)
        horas << QVariantMap{{"hora", ht[i].toString().mid(11, 2).toInt()},
                             {"temp", qRound(h.value(QStringLiteral("temperature_2m")).toArray().at(i).toDouble())},
                             {"codigo", h.value(QStringLiteral("weather_code")).toArray().at(i).toInt()},
                             {"lluvia", h.value(QStringLiteral("precipitation_probability")).toArray().at(i).toInt()}};
    r["horas"] = horas;
    QVariantList dias;
    const QJsonArray dt = e.datos.value(QStringLiteral("daily")).toObject().value(QStringLiteral("time")).toArray();
    for (int i = 0; i < dt.size() && i < 7; ++i) dias << dia(lat, lon, dt[i].toString());
    r["dias"] = dias;
    return r;
}

QVariantMap Clima::dia(double lat, double lon, const QString& fecha) {
    const QDate f = QDate::fromString(fecha, Qt::ISODate), hoy = QDate::currentDate();
    QVariantMap r{{"fecha", fecha}, {"disponible", false}};
    if (!f.isValid() || f < hoy) return r;
    if (hoy.daysTo(f) >= kDias) { r["desde"] = f.addDays(-(kDias - 1)).toString(QStringLiteral("dd/MM")); return r; }
    pedir(lat, lon);
    const Entrada e = cache_.value(clave(lat, lon));
    const QJsonObject d = e.datos.value(QStringLiteral("daily")).toObject();
    const QJsonArray t = d.value(QStringLiteral("time")).toArray();
    for (int i = 0; i < t.size(); ++i) {
        if (t[i].toString() != fecha) continue;
        auto v = [&](const char* k) { return d.value(QLatin1String(k)).toArray().at(i).toDouble(); };
        r["disponible"] = true;
        r["codigo"] = static_cast<int>(v("weather_code"));
        r["max"] = qRound(v("temperature_2m_max"));
        r["min"] = qRound(v("temperature_2m_min"));
        r["lluvia"] = qRound(v("precipitation_probability_max"));
        r["mm"] = v("precipitation_sum");
        r["viento"] = qRound(v("wind_speed_10m_max"));
        r["rafagas"] = qRound(v("wind_gusts_10m_max"));
        return r;
    }
    r["cargando"] = e.pidiendo || e.datos.isEmpty();
    return r;
}

QVariantList Clima::horasDe(double lat, double lon, const QString& fecha) {
    pedir(lat, lon);
    const Entrada e = cache_.value(clave(lat, lon));
    const QJsonObject h = e.datos.value(QStringLiteral("hourly")).toObject();
    const QJsonArray ht = h.value(QStringLiteral("time")).toArray();
    QVariantList l;
    for (int i = 0; i < ht.size(); ++i) {
        if (!ht[i].toString().startsWith(fecha)) continue;
        const int hora = ht[i].toString().mid(11, 2).toInt();
        if (hora % 2) continue;   // cada 2 horas: 12 en el día
        l << QVariantMap{{"hora", hora},
                         {"temp", qRound(h.value(QStringLiteral("temperature_2m")).toArray().at(i).toDouble())},
                         {"codigo", h.value(QStringLiteral("weather_code")).toArray().at(i).toInt()},
                         {"lluvia", h.value(QStringLiteral("precipitation_probability")).toArray().at(i).toInt()}};
    }
    return l;
}
