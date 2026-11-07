#include "informe.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <limits>
#include <optional>
#include <regex>

namespace beto {

namespace {

struct Pt {
    const FilaTraza* p;
    double L;
    double f;
    double qx, qy;
    bool ok;
};

struct VueltaR {
    size_t indice;   // en c.vueltas
    double L;
    std::vector<Pt> pts;
};

struct Recorrido {
    std::vector<std::array<double, 2>> pista;
    bool cerrada = false;
    std::vector<VueltaR> vueltas;
};

double hypotJs(double a, double b) { return std::sqrt(a * a + b * b); }

// Mediana ponderada: ordenar por valor (estable, como sort de JS) y devolver el primero donde el peso acumulado llega a la mitad
double mediana(const std::vector<double>& x, const std::vector<double>& w) {
    std::vector<size_t> o(x.size());
    for (size_t i = 0; i < o.size(); ++i) o[i] = i;
    std::stable_sort(o.begin(), o.end(), [&](size_t a, size_t b) { return x[a] < x[b]; });
    double tot = 0;
    for (double k : w) tot += k;
    double s = 0;
    for (size_t i : o) {
        s += w[i];
        if (s >= tot / 2) return x[i];
    }
    return x[o.back()];
}

std::optional<Recorrido> armarRecorrido(const Carrera& c, double umbralCfg) {
    Recorrido R;
    std::vector<size_t> vs;
    for (size_t i = 0; i < c.vueltas.size(); ++i)
        if (c.vueltas[i].traza.size() >= 5) vs.push_back(i);
    if (vs.empty()) return std::nullopt;
    const double umbral = std::max(10.0, umbralCfg > 0 ? umbralCfg : 20.0);
    const auto& T0 = c.vueltas[vs[0]].traza;
    const FilaTraza* p0 = &T0[0];
    for (const auto& p : T0) if (p[5] <= umbral) { p0 = &p; break; }
    const double kx = 111320 * std::cos((*p0)[3] * M_PI / 180), ky = 110540;

    // 1) puntos buenos de cada vuelta: precisión aceptable y sin saltos imposibles
    for (size_t idx : vs) {
        const Vuelta& v = c.vueltas[idx];
        const auto& T = v.traza;
        double base = v.distM && *v.distM != 0 ? *v.distM : (T.back()[1] != 0 ? T.back()[1] : 1);
        VueltaR V{idx, std::max(50.0, base), {}};
        for (const auto& p : T)
            V.pts.push_back({&p, V.L, std::max(0.0, std::min(1.0, p[1] / V.L)), (p[4] - (*p0)[4]) * kx, (p[3] - (*p0)[3]) * ky, p[5] <= umbral});
        const Pt* prev = nullptr;
        for (auto& o : V.pts) {
            if (!o.ok) continue;
            if (prev) {
                const double dt = std::max(0.5, (*o.p)[0] - (*prev->p)[0]), dd = hypotJs(o.qx - prev->qx, o.qy - prev->qy);
                if (dd / dt > std::max((*o.p)[2], (*prev->p)[2]) / 3.6 * 1.6 + 8 + ((*o.p)[5] + (*prev->p)[5]) / dt) { o.ok = false; continue; }
            }
            prev = &o;
        }
        R.vueltas.push_back(std::move(V));
    }
    double sumaL = 0;
    for (const auto& V : R.vueltas) sumaL += V.L;
    const double Lm = sumaL / static_cast<double>(R.vueltas.size());
    const int B = static_cast<int>(std::max(40.0, std::min(1500.0, redondearJs(Lm / 5))));

    auto construir = [&]() -> std::optional<std::vector<std::array<double, 2>>> {
        std::vector<std::vector<double>> bx(B), by(B), bw(B);
        for (const auto& V : R.vueltas)
            for (const auto& o : V.pts)
                if (o.ok) {
                    const int b = std::min(B - 1, static_cast<int>(std::floor(o.f * B)));
                    const double w = 1 / std::pow(std::max(3.0, (*o.p)[5]), 2);
                    bx[b].push_back(o.qx); by[b].push_back(o.qy); bw[b].push_back(w);
                }
        std::vector<std::optional<std::array<double, 2>>> P(B);
        std::vector<bool> lleno(B, false);
        std::vector<int> llenos;
        for (int i = 0; i < B; ++i)
            if (!bx[i].empty()) { P[i] = std::array<double, 2>{mediana(bx[i], bw[i]), mediana(by[i], bw[i])}; lleno[i] = true; llenos.push_back(i); }
        if (llenos.empty()) return std::nullopt;
        const auto ini = *P[llenos.front()], fin = *P[llenos.back()];
        R.cerrada = hypotJs(ini[0] - fin[0], ini[1] - fin[1]) < std::max(40.0, Lm * 0.04);
        // tramos vacíos: se completan uniendo los vecinos con señal (en circuito, dando la vuelta)
        for (int i = 0; i < B; ++i) {
            if (P[i]) continue;
            int a = -1, b = -1;
            for (int k = 1; k < B; ++k) {
                const int j = R.cerrada ? (i - k + B) % B : i - k;
                if (j >= 0 && P[j] && lleno[j]) { a = j; break; }
                if (!R.cerrada && j < 0) break;
            }
            for (int k = 1; k < B; ++k) {
                const int j = R.cerrada ? (i + k) % B : i + k;
                if (j < B && P[j] && lleno[j]) { b = j; break; }
                if (!R.cerrada && j >= B) break;
            }
            if (a < 0 && b < 0) continue;
            if (a < 0) P[i] = P[b];
            else if (b < 0) P[i] = P[a];
            else {
                const double da = (i - a + B) % B, db = (b - i + B) % B, t = da / (da + db);
                P[i] = std::array<double, 2>{(*P[a])[0] + ((*P[b])[0] - (*P[a])[0]) * t, (*P[a])[1] + ((*P[b])[1] - (*P[a])[1]) * t};
            }
        }
        // suavizado: promedio móvil de ±4 tramos, 3 pasadas
        std::vector<std::array<double, 2>> Q(B);
        for (int i = 0; i < B; ++i) Q[i] = *P[i];
        for (int pasada = 0; pasada < 3; ++pasada) {
            std::vector<std::array<double, 2>> N(B);
            for (int i = 0; i < B; ++i) {
                double sx = 0, sy = 0;
                int n = 0;
                for (int k = -4; k <= 4; ++k) {
                    int j = i + k;
                    if (R.cerrada) j = (j + B) % B;
                    else if (j < 0 || j >= B) continue;
                    sx += Q[j][0]; sy += Q[j][1]; ++n;
                }
                N[i] = {sx / n, sy / n};
            }
            Q = std::move(N);
        }
        return Q;
    };

    auto pista = construir();
    if (!pista) return std::nullopt;
    // 2) cada vuelta se encaja sobre el recorrido común y se vuelve a armar
    for (auto& V : R.vueltas)
        for (auto& o : V.pts) {
            if (!o.ok) continue;
            const int c0 = static_cast<int>(std::floor(o.f * B));
            const int r = static_cast<int>(std::max(3.0, redondearJs(B * 0.06)));
            int best = c0;
            double bd = std::numeric_limits<double>::infinity();
            for (int k = -r; k <= r; ++k) {
                int j = c0 + k;
                if (R.cerrada) j = (j + B) % B;
                else if (j < 0 || j >= B) continue;
                const double d = std::pow((*pista)[j][0] - o.qx, 2) + std::pow((*pista)[j][1] - o.qy, 2);
                if (d < bd) { bd = d; best = j; }
            }
            o.f = (best + 0.5) / B;
        }
    auto otra = construir();
    R.pista = otra ? *otra : *pista;
    return R;
}

}  // namespace

// Recorrido libre: resumen (tiempo, distancia, promedio, máxima, parado, aceleración máxima) y las marcas,
// cada una con su lugar en el dibujo (x, y) y en la línea de la vuelta (i: índice de la vuelta)
static void agregarRecorrido(const Carrera& c, Json& out) {
    out["recorrido"] = true;
    Json r = Json::objeto();
    int iv = -1;
    for (size_t i = 0; i < c.vueltas.size(); ++i) if (!c.vueltas[i].traza.empty() || iv < 0) iv = static_cast<int>(i);
    const Json& lista = out["vueltas"];
    const Json* M = nullptr;
    if (iv >= 0) {
        const Vuelta& v = c.vueltas[static_cast<size_t>(iv)];
        r["tiempoMs"] = v.realMs;
        r["distM"] = v.distM ? Json(*v.distM) : Json(0.0);
        r["promKmh"] = v.velPromKmh ? Json(*v.velPromKmh) : Json();
        r["maxKmh"] = v.velMaxKmh ? Json(*v.velMaxKmh) : Json();
        double parado = 0;
        for (size_t k = 1; k < v.traza.size(); ++k)
            if (v.traza[k][2] < 3) parado += std::min(2.0, v.traza[k][0] - v.traza[k - 1][0]);
        r["paradoMs"] = redondearJs(parado * 1000);
        const Json& j = lista.elementos()[static_cast<size_t>(iv)];
        if (j["m"].esLista()) M = &j["m"];
        double amax = 0;
        if (M) for (const auto& p : M->elementos()) if (p.elementos()[7].aNumero() > 0) amax = std::max(amax, p.elementos()[3].aNumero());
        r["acelMax"] = fijarDecimalesJs(amax, 1);
        r["vuelta"] = iv;
    }
    out["resumen"] = r;
    Json marcas = Json::lista();
    if (c.extra["marcas"].esLista())
        for (const auto& mk : c.extra["marcas"].elementos()) {
            Json x = mk;
            const double t = mk["ms"].aNumero() / 1000;
            if (M && !M->elementos().empty()) {   // la lectura más cercana en el tiempo
                const Json* mejor = &M->elementos().front();
                for (const auto& p : M->elementos())
                    if (std::fabs(p.elementos()[0].aNumero() - t) < std::fabs(mejor->elementos()[0].aNumero() - t)) mejor = &p;
                x["x"] = mejor->elementos()[5];
                x["y"] = mejor->elementos()[6];
            }
            x["vuelta"] = iv;
            marcas.agregar(std::move(x));
        }
    out["marcas"] = marcas;
}

Json datosInforme(const Carrera& c, double umbralPrecisionM) {
    const double D = static_cast<double>(c.distanciaM), T = static_cast<double>(c.tiempoIdealMs) / 1000;
    Json out = Json::objeto();
    out["nombre"] = c.nombre;
    out["fecha"] = c.fecha;
    out["idealMs"] = c.tiempoIdealMs;
    out["distanciaM"] = D;
    out["cerrada"] = false;
    out["pista"] = Json::lista();
    std::vector<int> largadas;
    for (const auto& v : c.vueltas) if (std::find(largadas.begin(), largadas.end(), v.largada) == largadas.end()) largadas.push_back(v.largada);
    out["largadas"] = static_cast<int>(largadas.size());
    Json vs = Json::lista();
    for (const auto& v : c.vueltas) {
        Json j = Json::objeto();
        j["n"] = v.n;
        j["largada"] = v.largada;
        j["medida"] = v.medida;
        j["realMs"] = v.realMs;
        if (v.difMs) j["difMs"] = *v.difMs;
        j["velPromKmh"] = v.velPromKmh ? Json(*v.velPromKmh) : Json();
        j["velMaxKmh"] = v.velMaxKmh ? Json(*v.velMaxKmh) : Json();
        j["m"] = Json();
        j["L"] = v.distM && *v.distM != 0 ? *v.distM : (D != 0 ? D : 1.0);
        if (v.interrumpida) j["interrumpida"] = true;   // solo Raspberry
        vs.agregar(std::move(j));
    }
    // Solo Raspberry: los videos de la cámara y, en cada vuelta, en qué momento del video empieza.
    // Un punto de la traza (t en segundos desde que empezó la vuelta) está en el video en videoMs + t·1000.
    // videoMs = desdeLargadaMs + suma de realMs de las vueltas anteriores de la misma largada.
    const Json& videos = c.extra["videos"];
    if (videos.esLista() && videos.tamano() > 0) {
        out["videos"] = videos;
        std::vector<std::pair<int, int64_t>> antes;   // largada → ms corridos antes de esta vuelta
        auto& lv = vs.elementos();
        for (size_t i = 0; i < c.vueltas.size(); ++i) {
            const Vuelta& v = c.vueltas[i];
            auto it = std::find_if(antes.begin(), antes.end(), [&](const auto& a) { return a.first == v.largada; });
            if (it == antes.end()) { antes.emplace_back(v.largada, 0); it = antes.end() - 1; }
            // Si en una largada hay más de un video (REC a mano), va el último que empezó antes de la vuelta;
            // si ninguno empezó antes, el que empezó primero (la vuelta arranca antes que el video).
            int elegido = -1;
            int64_t mejor = 0;
            const auto& lista = videos.elementos();
            for (size_t k = 0; k < lista.size(); ++k) {
                if (lista[k]["largada"].aEntero(1) != v.largada) continue;
                const int64_t ms = lista[k]["desdeLargadaMs"].aEntero(0) + it->second;
                const bool mejora = elegido < 0 || (ms >= 0 && (mejor < 0 || ms < mejor)) || (ms < 0 && mejor < 0 && ms > mejor);
                if (mejora) { elegido = static_cast<int>(k); mejor = ms; }
            }
            if (elegido >= 0) { lv[i]["video"] = elegido; lv[i]["videoMs"] = mejor; }
            it->second += v.realMs;
        }
    }
    out["vueltas"] = vs;

    const auto R = armarRecorrido(c, umbralPrecisionM);
    if (!R) return out;

    // encuadre del dibujo en 364 × 330 (norte arriba)
    double x0 = INFINITY, x1 = -INFINITY, y0 = INFINITY, y1 = -INFINITY;
    for (const auto& p : R->pista) { x0 = std::min(x0, p[0]); x1 = std::max(x1, p[0]); y0 = std::min(y0, p[1]); y1 = std::max(y1, p[1]); }
    const double esc = std::min((364 - 40) / std::max(1.0, x1 - x0), (330 - 40) / std::max(1.0, y1 - y0));
    const double ox = (364 - (x1 - x0) * esc) / 2, oy = (330 - (y1 - y0) * esc) / 2;
    auto pant = [&](double qx, double qy) {
        return std::array<double, 2>{fijarDecimalesJs(ox + (qx - x0) * esc, 1), fijarDecimalesJs(330 - oy - (qy - y0) * esc, 1)};
    };
    Json pista = Json::lista();
    for (const auto& p : R->pista) { auto s = pant(p[0], p[1]); pista.agregar(Json(Json::Lista{s[0], s[1]})); }
    out["pista"] = pista;
    out["cerrada"] = R->cerrada;
    const int B = static_cast<int>(R->pista.size());
    auto enPista = [&](double f) {
        const double x = f * B - 0.5;
        const int i = static_cast<int>(std::floor(x));
        const double t = x - i;
        const auto& a = R->pista[((i % B) + B) % B];
        const auto& b = R->pista[(((i + 1) % B) + B) % B];
        if (R->cerrada || (i >= 0 && i + 1 < B)) return pant(a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t);
        const int k = static_cast<int>(std::max(0.0, std::min(static_cast<double>(B - 1), redondearJs(x))));
        return pant(R->pista[k][0], R->pista[k][1]);
    };
    auto& lista = out["vueltas"].elementos();
    for (const auto& V : R->vueltas) {
        Json m = Json::lista();
        const auto& P = V.pts;
        for (size_t k = 0; k < P.size(); ++k) {
            const auto& p = *P[k].p;
            const FilaTraza* prev = k > 0 ? P[k - 1].p : nullptr;
            // Aceleración con las lecturas de ~1 s antes y ~1 s después (como la app con el GPS del celular a 1 Hz).
            // Con las vecinas inmediatas a 5 Hz (0,4 s) el ruido de la velocidad daba saltos de ±4 m/s² (04/11/2026).
            size_t ia = k, ib = k;
            while (ia > 0 && p[0] - (*P[ia - 1].p)[0] <= 1.0) --ia;
            if (ia > 0 && p[0] - (*P[ia].p)[0] < 1.0) --ia;
            while (ib + 1 < P.size() && (*P[ib + 1].p)[0] - p[0] <= 1.0) ++ib;
            if (ib + 1 < P.size() && (*P[ib].p)[0] - p[0] < 1.0) ++ib;
            const FilaTraza& A = *P[ia].p;
            const FilaTraza& B = *P[ib].p;
            double a = 0;   // la primera lectura, sin anterior: 0 (como la app)
            if (ia < k && B[0] > A[0]) a = ((B[2] - A[2]) / 3.6) / (B[0] - A[0]);
            const bool corte = prev && p[0] - (*prev)[0] > 4;   // más de 4 s sin lecturas
            const double ade = D > 0 && T > 0 ? p[1] / (D / T) - p[0] : 0;
            const auto xy = enPista(P[k].f);
            m.agregar(Json(Json::Lista{p[0], p[1], p[2], fijarDecimalesJs(std::max(-9.0, std::min(9.0, a)), 2),
                                       fijarDecimalesJs(ade, 1), xy[0], xy[1], P[k].ok && !corte ? 1 : 0}));
        }
        lista[V.indice]["m"] = m;
        lista[V.indice]["L"] = V.L;
    }
    if (esRecorrido(c)) agregarRecorrido(c, out);
    return out;
}

static std::string escaparHtml(const std::string& t) {
    std::string r;
    for (char ch : t) {
        switch (ch) {
            case '&': r += "&amp;"; break;
            case '<': r += "&lt;"; break;
            case '>': r += "&gt;"; break;
            case '"': r += "&quot;"; break;
            default: r += ch;
        }
    }
    return r;
}

std::string informeHtml(const Carrera& c, double umbralPrecisionM, const std::string& plantilla) {
    std::string datos = datosInforme(c, umbralPrecisionM).escribir();
    std::string seguro;
    for (char ch : datos) { if (ch == '<') seguro += "\\u003c"; else seguro += ch; }
    std::string html = plantilla;
    auto reemplazar = [&](const std::string& marca, const std::string& valor) {
        const auto pos = html.find(marca);
        if (pos != std::string::npos) html.replace(pos, marca.size(), valor);
    };
    reemplazar("__DATOS__", seguro);
    reemplazar("__NOMBRE__", escaparHtml(c.nombre));
    return html;
}

std::string nombreInforme(const Carrera& c) {
    // nombre sin \ / : * ? " < > | (cada grupo pasa a un espacio), recortado, hasta 40 letras
    std::string n = std::regex_replace(c.nombre, std::regex(R"([\\/:*?"<>|]+)"), " ");
    const auto a = n.find_first_not_of(' '), b = n.find_last_not_of(' ');
    n = a == std::string::npos ? "" : n.substr(a, b - a + 1);
    size_t i = 0, letras = 0;
    while (i < n.size() && letras < 40) {
        const unsigned char ch = static_cast<unsigned char>(n[i]);
        i += ch < 0x80 ? 1 : ch < 0xE0 ? 2 : ch < 0xF0 ? 3 : 4;
        ++letras;
    }
    n = n.substr(0, std::min(i, n.size()));
    std::string fecha = c.fecha;   // AAAA-MM-DD → DD-MM-AAAA
    if (fecha.size() == 10) fecha = fecha.substr(8, 2) + "-" + fecha.substr(5, 2) + "-" + fecha.substr(0, 4);
    return n + " " + fecha + ".html";   // sin la banderita emoji: la fuente de la Pi no la dibuja («?», 07-2.5)
}

}  // namespace beto
