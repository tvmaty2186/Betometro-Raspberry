#include "marca_gps.h"

#include <cmath>
#include <optional>

namespace beto {

namespace {
constexpr double kDistRumbo = 15;       // m: para sacar el rumbo de salida
constexpr double kAnchoLinea = 30;      // m a cada lado: más lejos no es la línea (otra parte de la pista)
constexpr int64_t kVentanaMs = 20000;   // se busca el cruce hasta 20 s antes o después de la marca del pulsador
constexpr int64_t kHuecoMs = 2000;      // más de 2 s entre dos lecturas: no se puede repartir
constexpr int64_t kHuecoDudosoMs = 1500;
constexpr double kPrecDudosa = 10;      // m

struct Punto { int64_t t; double x, y, prec; };
struct Cruce { int64_t t; bool dudosa; };
}  // namespace

std::vector<MarcaGps> marcasGps(const Carrera& c) {
    std::vector<MarcaGps> r(c.vueltas.size());
    size_t i = 0;
    while (i < c.vueltas.size()) {
        // Las vueltas de una misma largada (seguidas)
        size_t fin = i;
        while (fin < c.vueltas.size() && c.vueltas[fin].largada == c.vueltas[i].largada) ++fin;
        const auto& v1 = c.vueltas[i];
        if (v1.traza.empty()) { i = fin; continue; }
        const double lat0 = v1.traza.front()[3], lon0 = v1.traza.front()[4];
        const double kx = 111320 * std::cos(lat0 * M_PI / 180), ky = 110540;
        auto xy = [&](const FilaTraza& f) { return std::pair<double, double>{(f[4] - lon0) * kx, (f[3] - lat0) * ky}; };
        // Rumbo de salida
        std::optional<std::pair<double, double>> u;
        for (const auto& f : v1.traza) {
            if (f[0] > 30) break;
            const auto [x, y] = xy(f);
            const double n = std::hypot(x, y);
            if (n >= kDistRumbo) { u = {x / n, y / n}; break; }
        }
        if (!u) { i = fin; continue; }
        // Todas las lecturas de la largada en un solo reloj (ms desde la largada del pulsador)
        std::vector<Punto> P;
        std::vector<int64_t> finVuelta;   // marca del pulsador al terminar cada vuelta
        int64_t inicio = 0;
        for (size_t k = i; k < fin; ++k) {
            for (const auto& f : c.vueltas[k].traza) {
                const auto [x, y] = xy(f);
                P.push_back({inicio + static_cast<int64_t>(std::llround(f[0] * 1000)), x, y, f[5]});
            }
            inicio += c.vueltas[k].realMs;
            finVuelta.push_back(inicio);
        }
        // Todos los cruces de la línea (yendo para adelante) entre dos momentos
        auto cruces = [&](int64_t desde, int64_t hasta) {
            std::vector<Cruce> r;
            for (size_t k = 1; k < P.size(); ++k) {
                const Punto& a = P[k - 1];
                const Punto& b = P[k];
                if (b.t < desde || a.t > hasta || b.t < a.t || b.t - a.t > kHuecoMs) continue;
                const double sa = a.x * u->first + a.y * u->second, sb = b.x * u->first + b.y * u->second;
                if (!(sa < 0 && sb >= 0)) continue;   // cruza la línea yendo para adelante
                const double f = -sa / (sb - sa);
                const double lx = a.x + (b.x - a.x) * f, ly = a.y + (b.y - a.y) * f;
                if (std::fabs(-lx * u->second + ly * u->first) > kAnchoLinea) continue;
                const int64_t t = a.t + static_cast<int64_t>(std::llround((b.t - a.t) * f));
                const bool dudosa = std::max(a.prec, b.prec) > kPrecDudosa || b.t - a.t > kHuecoDudosoMs;
                if (t >= desde && t <= hasta) r.push_back(Cruce{t, dudosa});
            }
            return r;
        };
        // El cruce más cerca de la marca del pulsador
        auto cruce = [&](int64_t cerca) -> std::optional<Cruce> {
            std::optional<Cruce> mejor;
            for (const Cruce& x : cruces(cerca - kVentanaMs, cerca + kVentanaMs))
                if (!mejor || std::llabs(x.t - cerca) < std::llabs(mejor->t - cerca)) mejor = x;
            return mejor;
        };
        std::optional<Cruce> anterior = Cruce{0, false};   // la largada: la misma del pulsador
        for (size_t k = i; k < fin; ++k) {
            const Vuelta& v = c.vueltas[k];
            const int64_t empezo = k == i ? 0 : finVuelta[k - i - 1];
            // ¿Pasó por la línea en el medio de la vuelta? (lejos de la largada y de la marca)
            if (!v.interrumpida)
                for (const Cruce& x : cruces(empezo + kVentanaMs, finVuelta[k - i] - kVentanaMs))
                    if (!x.dudosa) { r[k].cruceSinMarcaMs = x.t - empezo; break; }
            const std::optional<Cruce> este = v.interrumpida ? std::nullopt : cruce(finVuelta[k - i]);
            if (anterior && este) {
                MarcaGps& m = r[k];
                m.hay = true;
                m.dudosa = anterior->dudosa || este->dudosa;
                m.realMs = este->t - anterior->t;
                m.difMs = m.realMs - v.idealMs;
                m.pulsadorMenosGpsMs = finVuelta[k - i] - este->t;
            }
            anterior = este;
        }
        i = fin;
    }
    return r;
}

}  // namespace beto
