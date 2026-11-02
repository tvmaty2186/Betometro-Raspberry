#include "modelo.h"

#include <algorithm>
#include <cmath>

namespace beto {

// ---------- Traza ----------

void Traza::agregar(const Muestra& m, double distanciaM, double restanteM) {
    if (!m.lat || !m.lon) return;
    double d = distanciaM - restanteM;
    filas_.push_back({fijarDecimalesJs((m.t - inicio_) / 1000, 1),
                      redondearJs(std::max(0.0, d) * 10) / 10,
                      fijarDecimalesJs(m.velMs * 3.6, 1),
                      fijarDecimalesJs(*m.lat, 6),
                      fijarDecimalesJs(*m.lon, 6),
                      redondearJs(m.precisionM)});
    if (filas_.size() > kTope) filas_.erase(filas_.begin());
}

// ---------- Vuelta ----------

Vuelta Vuelta::desdeMotor(const VueltaMotor& v, int largada, const Traza* traza) {
    Vuelta r;
    r.largada = largada;
    r.n = v.n;
    r.medida = v.medida;
    r.idealMs = static_cast<int64_t>(std::llround(v.idealMs));
    r.realMs = static_cast<int64_t>(v.realMs);
    if (v.difMs) r.difMs = static_cast<int64_t>(*v.difMs);
    r.fuente = v.fuente;
    r.fin = v.fin;
    r.distM = v.distM;
    r.velPromKmh = v.velPromKmh;
    r.velMaxKmh = v.velMaxKmh;
    r.factorVuelta = v.factorVuelta;
    r.interrumpida = v.interrumpida;
    if (traza && traza->filas().size() >= Traza::kMinimo) r.traza = traza->filas();
    return r;
}

static void ponerOpt(Json& j, const char* k, const std::optional<double>& v) {
    if (v) j[k] = *v;
}

Json Vuelta::aJson() const {
    Json j = Json::objeto();
    j["largada"] = largada;
    j["n"] = n;
    j["medida"] = medida;
    j["idealMs"] = idealMs;
    j["realMs"] = realMs;
    if (difMs) j["difMs"] = *difMs;
    j["fuente"] = fuente;
    j["fin"] = fin;
    ponerOpt(j, "distM", distM);
    ponerOpt(j, "velPromKmh", velPromKmh);
    ponerOpt(j, "velMaxKmh", velMaxKmh);
    ponerOpt(j, "factorVuelta", factorVuelta);
    if (interrumpida) j["interrumpida"] = true;
    if (!traza.empty()) {
        Json t = Json::lista();
        for (const auto& f : traza) {
            Json fila = Json::lista();
            for (double x : f) fila.agregar(x);
            t.agregar(std::move(fila));
        }
        j["traza"] = std::move(t);
    }
    return j;
}

static std::optional<double> leerOpt(const Json& j, const char* k) {
    if (j[k].esNumero()) return j[k].aNumero();
    return std::nullopt;
}

Vuelta Vuelta::desdeJson(const Json& j) {
    Vuelta v;
    v.largada = static_cast<int>(j["largada"].aEntero(1));
    v.n = static_cast<int>(j["n"].aEntero(1));
    v.medida = j["medida"].aBool(true);
    v.idealMs = j["idealMs"].aEntero();
    v.realMs = j["realMs"].aEntero();
    if (j["difMs"].esNumero()) v.difMs = j["difMs"].aEntero();
    v.fuente = j["fuente"].aTexto("pantalla");
    v.fin = j["fin"].aEntero();
    v.distM = leerOpt(j, "distM");
    v.velPromKmh = leerOpt(j, "velPromKmh");
    v.velMaxKmh = leerOpt(j, "velMaxKmh");
    v.factorVuelta = leerOpt(j, "factorVuelta");
    v.interrumpida = j["interrumpida"].aBool(false);
    for (const auto& f : j["traza"].elementos()) {
        FilaTraza fila{};
        const auto& e = f.elementos();
        for (size_t i = 0; i < fila.size() && i < e.size(); ++i) fila[i] = e[i].aNumero();
        v.traza.push_back(fila);
    }
    return v;
}

// ---------- Carrera ----------

// Recorta a n caracteres (no bytes) sin partir una letra con acento
static std::string recortar(const std::string& s, size_t n) {
    size_t i = 0, cuenta = 0;
    while (i < s.size() && cuenta < n) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        i += c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
        ++cuenta;
    }
    return s.substr(0, std::min(i, s.size()));
}

static const char* const kViejos[] = {"piloto", "navegante", "auto", "numero", "categoria"};
static const char* const kConocidos[] = {"id", "nombre", "fecha", "lugar", "notas", "piloto", "navegante", "auto",
                                         "numero", "categoria", "tiempoIdealMs", "distanciaM", "patron", "practica",
                                         "creada", "vueltasTotal", "vueltas"};

int Carrera::proximaLargada() const {
    int m = 0;
    for (const auto& v : vueltas) m = std::max(m, v.largada);
    return m + 1;
}

bool Carrera::unirConSiguiente(size_t i) {
    if (i + 1 >= vueltas.size()) return false;
    Vuelta& a = vueltas[i];
    const Vuelta& b = vueltas[i + 1];
    if (a.largada != b.largada || a.interrumpida || b.interrumpida) return false;
    const double segA = static_cast<double>(a.realMs) / 1000.0;
    const double dA = a.distM ? *a.distM : (a.traza.empty() ? 0.0 : a.traza.back()[1]);
    for (FilaTraza f : b.traza) {   // la traza de la segunda sigue a la de la primera
        f[0] = fijarDecimalesJs(f[0] + segA, 1);
        f[1] = fijarDecimalesJs(f[1] + dA, 1);
        a.traza.push_back(f);
    }
    a.realMs += b.realMs;
    a.difMs = a.realMs - a.idealMs;
    a.fuente = b.fuente;
    a.fin = b.fin;
    if (a.distM && b.distM) a.distM = fijarDecimalesJs(*a.distM + *b.distM, 1);
    else a.distM.reset();
    if (a.distM && a.realMs > 0) a.velPromKmh = fijarDecimalesJs(*a.distM / (a.realMs / 1000.0) * 3.6, 1);
    else a.velPromKmh.reset();
    if (a.velMaxKmh || b.velMaxKmh) a.velMaxKmh = std::max(a.velMaxKmh.value_or(0), b.velMaxKmh.value_or(0));
    a.factorVuelta.reset();   // era de cada pedazo: ya no vale
    const int largada = a.largada;
    vueltas.erase(vueltas.begin() + static_cast<long>(i) + 1);
    for (size_t k = i + 1; k < vueltas.size(); ++k)
        if (vueltas[k].largada == largada) --vueltas[k].n;
    return true;
}

std::vector<const Vuelta*> Carrera::vueltasDeLargada(int largada) const {
    std::vector<const Vuelta*> r;
    for (const auto& v : vueltas) if (v.largada == largada) r.push_back(&v);
    return r;
}

Json Carrera::aJson() const {
    Json j = Json::objeto();
    j["id"] = id;
    j["nombre"] = recortar(nombre, kMaxNombre);
    j["fecha"] = fecha;
    j["lugar"] = lugar;
    j["notas"] = recortar(notas, kMaxNotas);
    for (const char* k : kViejos) j[k] = extra[k].esTexto() ? extra[k] : Json("");
    j["tiempoIdealMs"] = tiempoIdealMs;
    j["distanciaM"] = distanciaM;
    j["patron"] = patron;
    j["practica"] = practica;
    j["creada"] = creada;
    if (vueltasTotal > 0) j["vueltasTotal"] = vueltasTotal;   // solo de versiones viejas
    for (const auto& p : extra.pares()) {
        bool conocido = false;
        for (const char* k : kConocidos) if (p.first == k) conocido = true;
        if (!conocido) j[p.first] = p.second;
    }
    Json vs = Json::lista();
    for (const auto& v : vueltas) vs.agregar(v.aJson());
    j["vueltas"] = std::move(vs);
    return j;
}

Carrera Carrera::desdeJson(const Json& j) {
    Carrera c;
    c.id = j["id"].aTexto();
    c.nombre = j["nombre"].aTexto();
    c.fecha = j["fecha"].aTexto();
    c.lugar = j["lugar"].aTexto();
    c.notas = j["notas"].aTexto();
    c.tiempoIdealMs = j["tiempoIdealMs"].aEntero(240000);
    c.distanciaM = j["distanciaM"].aEntero(0);
    c.patron = j["patron"].aTexto("todas");
    c.practica = j["practica"].aBool(false);
    c.creada = j["creada"].aTexto();
    c.vueltasTotal = static_cast<int>(j["vueltasTotal"].aEntero(0));
    for (const auto& v : j["vueltas"].elementos()) c.vueltas.push_back(Vuelta::desdeJson(v));
    for (const auto& p : j.pares()) {
        bool conocido = false;
        for (const char* k : kConocidos) if (p.first == k) conocido = true;
        bool viejo = false;
        for (const char* k : kViejos) if (p.first == k) viejo = true;
        if (!conocido || viejo) c.extra[p.first] = p.second;
    }
    return c;
}

std::string nuevaId(int64_t ahoraMs, uint32_t azar) {
    static const char dig[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::string b36;
    uint64_t x = static_cast<uint64_t>(std::max<int64_t>(0, ahoraMs));
    do { b36.insert(b36.begin(), dig[x % 36]); x /= 36; } while (x);
    std::string extra;
    for (int i = 0; i < 4; ++i) { extra += dig[azar % 36]; azar /= 36; }
    return "c" + b36 + extra;
}

// ---------- Configuración ----------

Json configFabrica() {
    Json c = Json::objeto();
    c["tiempoIdealMs"] = 240000;      // 4:00
    c["distanciaM"] = 3200;
    c["patron"] = "alt1";
    c["cuentaN"] = 20;
    c["modoCuenta"] = "voz";
    c["pitidoCero"] = false;
    c["clicMarca"] = true;
    c["anticipoMs"] = 40;             // como la app nativa, hasta medirlo en E4
    c["avisoInicio"] = true;
    c["antirreboteMs"] = 3000;
    c["colMedida"] = "#b3132b";
    c["colPractica"] = "#0f6e3a";
    c["velActiva"] = true;
    c["pitidoCadaS"] = 0;
    c["umbralPrecisionM"] = 20;
    c["calibrar"] = true;
    c["tema"] = "avispon";
    c["numeroAuto"] = "101";
    c["nombreAuto"] = "Avispón Verde";   // nuevo de la Raspberry: se ve en el inicio
    c["modoSol"] = "no";              // sin sensor de luz: «auto» no aplica
    c["indicador"] = "fondo";
    c["widgets"] = nullptr;           // armado vertical; null = de fábrica
    // Nuevos de la Raspberry
    c["orientacion"] = "horizontal";
    c["brillo"] = 100;                // %
    c["volumen"] = 100;
    c["gpsSoloGps"] = false;
    c["gpsModulo"] = "auto";          // auto (se detecta), m8 (NEO-M8N), 7 (NEO-7M), 6 (NEO-6M)
    // Cámara y micrófono (video de la carrera)
    c["camara"] = false;              // de fábrica apagada: se prende al instalarla
    c["camaraInicio"] = "abrir";      // abrir (al abrir la pantalla de carrera), largada, mano (botón REC)
    c["camaraFinS"] = 10;             // segundos de más después de «Terminar»
    c["camaraCalidad"] = "720p";      // 720p, 1080p
    c["camaraSonido"] = "camara";     // camara (micrófono de la cámara), usb (micrófono USB), no
    c["camaraGirar"] = false;         // girar 180° (cámara dada vuelta en el techo)
    c["camaraBorrarViejos"] = true;   // si falta lugar, borra los videos más viejos (nunca informes)
    return c;
}

namespace {

void numeroEn(Json& c, const Json& def, const char* k, double min, double max) {
    if (!c[k].esNumero() || !std::isfinite(c[k].aNumero())) { c[k] = def[k]; return; }
    c[k] = std::min(max, std::max(min, c[k].aNumero()));
}

void opcionEn(Json& c, const Json& def, const char* k, std::initializer_list<const char*> validas) {
    std::string v = c[k].aTexto();
    for (const char* x : validas) if (v == x) return;
    c[k] = def[k];
}

void boolEn(Json& c, const Json& def, const char* k) {
    if (!c[k].esBool()) c[k] = def[k];
}

}  // namespace

Json armarConfig(const Json& guardado) {
    const Json def = configFabrica();
    Json c = def;
    for (const auto& p : guardado.pares()) c[p.first] = p.second;

    // tiempo ideal: menos de 5 s no vale, queda el de fábrica
    if (!c["tiempoIdealMs"].esNumero() || c["tiempoIdealMs"].aNumero() < 5000) c["tiempoIdealMs"] = def["tiempoIdealMs"];
    numeroEn(c, def, "distanciaM", 0, 1000000);
    opcionEn(c, def, "patron", {"todas", "alt1", "alt2"});
    numeroEn(c, def, "cuentaN", 0, 60);
    opcionEn(c, def, "modoCuenta", {"voz", "pitidos"});
    boolEn(c, def, "pitidoCero");
    boolEn(c, def, "clicMarca");
    numeroEn(c, def, "anticipoMs", 0, 1500);
    boolEn(c, def, "avisoInicio");
    numeroEn(c, def, "antirreboteMs", 0, 60000);
    boolEn(c, def, "velActiva");
    numeroEn(c, def, "pitidoCadaS", 0, 600);
    numeroEn(c, def, "umbralPrecisionM", 1, 200);
    boolEn(c, def, "calibrar");
    opcionEn(c, def, "tema", {"avispon", "rally", "sport", "cabina"});
    {
        std::string n = c["numeroAuto"].aTexto();
        if (n.empty()) c["numeroAuto"] = def["numeroAuto"];
    }
    if (c["modoSol"].aTexto() == "auto") c["modoSol"] = "no";
    opcionEn(c, def, "modoSol", {"no", "si"});
    opcionEn(c, def, "indicador", {"marco", "fondo"});
    opcionEn(c, def, "orientacion", {"horizontal", "vertical"});
    opcionEn(c, def, "gpsModulo", {"auto", "m8", "7", "6"});
    numeroEn(c, def, "brillo", 10, 100);     // nunca negro del todo
    numeroEn(c, def, "volumen", 0, 100);
    boolEn(c, def, "camara");
    opcionEn(c, def, "camaraInicio", {"abrir", "largada", "mano"});
    numeroEn(c, def, "camaraFinS", 0, 120);
    opcionEn(c, def, "camaraCalidad", {"720p", "1080p"});
    opcionEn(c, def, "camaraSonido", {"camara", "usb", "no"});
    boolEn(c, def, "camaraGirar");
    boolEn(c, def, "camaraBorrarViejos");
    return c;
}

DatosLargada datosLargada(const Carrera& c, const Json& cfg) {
    DatosLargada d;
    d.tiempoIdealMs = static_cast<double>(c.tiempoIdealMs);
    d.distanciaM = static_cast<double>(c.distanciaM);
    d.patron = c.patron;
    d.vueltasTotal = 0;   // sin límite: en la carrera pueden agregar vueltas
    d.antirreboteMs = cfg["antirreboteMs"].aNumero(3000);
    d.umbralPrecisionM = cfg["umbralPrecisionM"].aNumero(20);
    d.calibrar = cfg["calibrar"].aBool(true);
    d.congelarMs = std::max(cfg["cuentaN"].aNumero(20), 3.0) * 1000;
    d.factorGps = 1;
    return d;
}

// ---------- Base ----------

Carrera* Base::buscar(const std::string& id) {
    for (auto& c : carreras) if (c.id == id) return &c;
    return nullptr;
}

Json Base::aJson() const {
    Json j = Json::objeto();
    j["schemaVersion"] = schemaVersion;
    j["config"] = config;
    Json cs = Json::lista();
    for (const auto& c : carreras) cs.agregar(c.aJson());
    j["carreras"] = std::move(cs);
    return j;
}

Base Base::desdeJson(const Json& j) {
    Base b;
    b.schemaVersion = static_cast<int>(j["schemaVersion"].aEntero(2));
    if (j["config"].esObjeto()) b.config = j["config"];
    for (const auto& c : j["carreras"].elementos()) b.carreras.push_back(Carrera::desdeJson(c));
    return b;
}

}  // namespace beto
