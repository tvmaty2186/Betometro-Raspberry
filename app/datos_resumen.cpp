// El resumen de cada carrera para la pantalla (tarjetas, chip de estado, LARGAR) y las listas próximas/corridas.
#include "datos.h"
#include "datos_comun.h"
#include "motor.h"

#include <QLocale>

#include <algorithm>
#include <cmath>

using namespace beto;

static const char* const kMeses[] = {"ENE", "FEB", "MAR", "ABR", "MAY", "JUN", "JUL", "AGO", "SEP", "OCT", "NOV", "DIC"};

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
