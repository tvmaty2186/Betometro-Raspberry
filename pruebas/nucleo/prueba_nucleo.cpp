// Pruebas del núcleo: lo que no se compara con la app (lo propio de la Raspberry, el JSON,
// el modelo y el almacén) y los redondeos iguales a JavaScript.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

#include "almacen.h"
#include "json.h"
#include "modelo.h"
#include "motor.h"
#include "programador.h"
#include "neo.h"
#include "pulsador.h"

using namespace beto;

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c)                                                                  \
    do {                                                                             \
        ++chequeos;                                                                  \
        if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } \
    } while (0)
#define IGUAL(a, b)                                                                  \
    do {                                                                             \
        ++chequeos;                                                                  \
        auto va_ = (a); auto vb_ = (b);                                              \
        if (!(va_ == vb_)) {                                                         \
            ++fallas;                                                                \
            std::cerr << __FILE__ << ":" << __LINE__ << ": " #a " = " << va_ << ", se esperaba " << vb_ << "\n"; \
        }                                                                            \
    } while (0)

static void redondeos() {
    IGUAL(redondearJs(0.5), 1.0);
    IGUAL(redondearJs(2.5), 3.0);
    IGUAL(redondearJs(-2.5), -2.0);
    IGUAL(redondearJs(-0.4), 0.0);
    IGUAL(redondearJs(0.49999999999999994), 0.0);
    IGUAL(redondearJs(1234.5), 1235.0);
    // +x.toFixed(n) de JavaScript
    IGUAL(fijarDecimalesJs(0.25, 1), 0.3);     // empate exacto: hacia arriba
    IGUAL(fijarDecimalesJs(-0.25, 1), -0.3);
    IGUAL(fijarDecimalesJs(0.35, 1), 0.3);     // 0.35 es en realidad 0.34999…
    IGUAL(fijarDecimalesJs(1.45, 1), 1.4);
    IGUAL(fijarDecimalesJs(1.005, 2), 1.0);
    IGUAL(fijarDecimalesJs(2.5, 0), 3.0);
    IGUAL(fijarDecimalesJs(0.03125, 4), 0.0313);
    IGUAL(fijarDecimalesJs(57.64999, 1), 57.6);
    IGUAL(fijarDecimalesJs(-34.6037221, 6), -34.603722);
}

static void json() {
    IGUAL(numeroComoJs(100), std::string("100"));
    IGUAL(numeroComoJs(-0.0), std::string("0"));
    IGUAL(numeroComoJs(0.1), std::string("0.1"));
    IGUAL(numeroComoJs(123.456), std::string("123.456"));
    IGUAL(numeroComoJs(1e21), std::string("1e+21"));
    IGUAL(numeroComoJs(1.5e-7), std::string("1.5e-7"));
    IGUAL(numeroComoJs(0.000001), std::string("0.000001"));
    IGUAL(numeroComoJs(1.0 / 3), std::string("0.3333333333333333"));
    IGUAL(numeroComoJs(-34.603722), std::string("-34.603722"));

    std::string texto = R"({"a":1,"b":[true,false,null],"c":"Ñandú \"101\"\n","d":{"e":-2.5e-3},"f":"á🏁"})";
    Json j = Json::leer(texto);
    IGUAL(j["a"].aEntero(), 1);
    IGUAL(j["b"].tamano(), size_t(3));
    IGUAL(j["c"].aTexto(), std::string("Ñandú \"101\"\n"));
    IGUAL(j["d"]["e"].aNumero(), -0.0025);
    IGUAL(j["f"].aTexto(), std::string("á🏁"));
    Json k = Json::leer(j.escribir());
    CHEQUEAR(j == k);
    CHEQUEAR(Json::leer(j.escribir(1)) == j);
    // el orden de las claves se conserva
    IGUAL(j.escribir().substr(0, 10), std::string("{\"a\":1,\"b\""));
    // copiar es copia de verdad
    Json copia = j;
    copia["a"] = 2;
    IGUAL(j["a"].aEntero(), 1);

    bool tiro = false;
    try { Json::leer("{\"a\":1"); } catch (const ErrorJson&) { tiro = true; }
    CHEQUEAR(tiro);
    tiro = false;
    try { Json::leer("{\"a\":1} x"); } catch (const ErrorJson&) { tiro = true; }
    CHEQUEAR(tiro);
}

struct Registro {
    std::vector<VueltaMotor> cerradas, interrumpidas;
    int terminadas = 0;
    Motor::Oyente oyente() {
        return [this](Evento e, const DatosEvento& d) {
            if (e == Evento::Cerrada) cerradas.push_back(*d.vuelta);
            if (e == Evento::Interrumpida) interrumpidas.push_back(*d.vuelta);
            if (e == Evento::Terminada) ++terminadas;
        };
    }
};

static DatosLargada largada(double T, double D, int total = 0) {
    DatosLargada c;
    c.tiempoIdealMs = T;
    c.distanciaM = D;
    c.patron = "alt1";
    c.vueltasTotal = total;
    return c;
}

static void vueltaInterrumpida() {
    {   // Terminar a mano en medio de la vuelta: se guarda marcada, sin error y sin calibrar
        Registro r;
        Motor m(largada(60000, 1000), r.oyente(), [] { return int64_t(123); });
        double t = 0;
        auto gpsHasta = [&](double fin) { for (; t <= fin; t += 100) m.muestra({t, 1000.0 / 60, 3, {}, {}}); };
        gpsHasta(1000);
        m.marcar(1000, "pulsador");
        gpsHasta(61000);
        m.marcar(61000, "pulsador");
        gpsHasta(91000);
        m.terminar(91000, true);
        IGUAL(r.cerradas.size(), size_t(1));
        IGUAL(r.interrumpidas.size(), size_t(1));
        IGUAL(r.terminadas, 1);
        const VueltaMotor& v = r.interrumpidas[0];
        CHEQUEAR(v.interrumpida);
        IGUAL(v.n, 2);
        CHEQUEAR(!v.medida);                     // alt1: la 2 es de práctica
        IGUAL(v.realMs, 30000.0);
        CHEQUEAR(!v.difMs.has_value());
        CHEQUEAR(!v.factorVuelta.has_value());
        CHEQUEAR(v.distM.has_value());
        IGUAL(*v.distM, 500.0);
        IGUAL(v.fuente, std::string("terminar"));
        IGUAL(v.fin, int64_t(123));
        IGUAL(m.vueltas().size(), size_t(2));
        CHEQUEAR(!m.enCurso());
    }
    {   // Si terminás enseguida después de marcar (menos que el antirrebote), no queda una vuelta de nada
        Registro r;
        Motor m(largada(60000, 1000), r.oyente());
        m.marcar(0, "pantalla");
        m.marcar(60000, "pantalla");
        m.terminar(61000, true);
        IGUAL(r.interrumpidas.size(), size_t(0));
        IGUAL(r.terminadas, 1);
    }
    {   // Carrera completa: la vuelta siguiente al total no se guarda, ni a mano ni sola
        Registro r;
        Motor m(largada(60000, 1000, 2), r.oyente());
        m.marcar(0, "pantalla");
        m.marcar(60000, "pantalla");
        m.marcar(120000, "pantalla");
        m.terminar(150000, true);
        IGUAL(r.cerradas.size(), size_t(2));
        IGUAL(r.interrumpidas.size(), size_t(0));
    }
    {   // Terminar solo (no a mano): no guarda interrumpida
        Registro r;
        Motor m(largada(60000, 1000), r.oyente());
        m.marcar(0, "pantalla");
        m.terminar(30000, false);
        IGUAL(r.interrumpidas.size(), size_t(0));
    }
}

static void soloCronometro() {
    // Sin GPS la carrera anda igual: cuenta, marcas y tiempos; sin sugerida ni metros
    Registro r;
    Motor m(largada(60000, 1000), r.oyente());
    Estado e0 = m.estado(0);
    CHEQUEAR(e0.sinGps);
    CHEQUEAR(!e0.vActKmh.has_value());
    IGUAL(m.marcar(1000, "pulsador") == ResultadoMarca::Largada, true);
    Estado e = m.estado(31000);
    CHEQUEAR(e.enCurso);
    CHEQUEAR(e.sinGps);
    IGUAL(e.restanteMs, 30000.0);
    CHEQUEAR(!e.vNecKmh.has_value());
    CHEQUEAR(!e.deltaS.has_value());
    IGUAL(m.marcar(62500, "pulsador") == ResultadoMarca::Marca, true);
    IGUAL(r.cerradas.size(), size_t(1));
    IGUAL(r.cerradas[0].realMs, 61500.0);
    IGUAL(*r.cerradas[0].difMs, 1500.0);
    CHEQUEAR(!r.cerradas[0].distM.has_value());
    CHEQUEAR(!r.cerradas[0].velPromKmh.has_value());
    // Antirrebote igual que con GPS
    IGUAL(m.marcar(63000, "pantalla") == ResultadoMarca::Ignorada, true);
}

static void traza() {
    Traza t;
    t.empezar(1000);
    Muestra sinPos{1100, 10, 3, {}, {}};
    t.agregar(sinPos, 1000, 900);
    IGUAL(t.filas().size(), size_t(0));
    Muestra m{3456.7, 13.8889, 4.6, -34.60372219, -58.38159111};
    t.agregar(m, 1000, 1000.04);                 // d negativo → 0
    const FilaTraza& f = t.filas()[0];
    IGUAL(f[0], 2.5);
    IGUAL(f[1], 0.0);
    IGUAL(f[2], 50.0);
    IGUAL(f[3], -34.603722);
    IGUAL(f[4], -58.381591);
    IGUAL(f[5], 5.0);
    for (int i = 0; i < 20005; ++i) t.agregar(m, 1000, 500);
    IGUAL(t.filas().size(), Traza::kTope);
}

static void modelo() {
    Json cfg = armarConfig(Json::objeto());
    IGUAL(cfg["volumen"].aNumero(), 100.0);
    IGUAL(cfg["tema"].aTexto(), std::string("avispon"));
    IGUAL(cfg["anticipoMs"].aNumero(), 40.0);
    IGUAL(cfg["cuentaN"].aNumero(), 20.0);

    Json g = Json::objeto();
    g["volumen"] = 35;
    g["umbralPrecisionM"] = 500;
    g["tiempoIdealMs"] = 3000;
    g["modoSol"] = "auto";
    g["tema"] = "inventado";
    g["cuentaN"] = 0;
    g["widgetsH"] = Json::lista();            // las claves que no conoce se conservan
    Json c = armarConfig(g);
    IGUAL(c["volumen"].aNumero(), 35.0);      // se usa el último que eligió
    IGUAL(c["umbralPrecisionM"].aNumero(), 200.0);
    IGUAL(c["tiempoIdealMs"].aNumero(), 240000.0);
    IGUAL(c["modoSol"].aTexto(), std::string("no"));
    IGUAL(c["tema"].aTexto(), std::string("avispon"));
    CHEQUEAR(c["widgetsH"].esLista());

    Carrera k;
    k.id = nuevaId(1730419200000, 12345);
    CHEQUEAR(k.id.size() > 5 && k.id[0] == 'c');
    k.nombre = "Regularidad Ñandú";
    k.fecha = "2026-11-01";
    k.tiempoIdealMs = 180000;
    k.distanciaM = 2500;
    k.patron = "alt2";
    k.vueltasTotal = 6;
    k.extra["piloto"] = "viejo";
    k.extra["algoNuevo"] = 7;
    DatosLargada d = datosLargada(k, c);
    IGUAL(d.congelarMs, 3000.0);              // max(cuentaN = 0, 3) s
    IGUAL(d.factorGps, 1.0);
    IGUAL(d.vueltasTotal, 6);
    IGUAL(k.proximaLargada(), 1);
    CHEQUEAR(!k.parametrosFijos());

    VueltaMotor vm;
    vm.n = 3; vm.medida = true; vm.idealMs = 180000; vm.realMs = 90000; vm.interrumpida = true;
    vm.fuente = "terminar"; vm.fin = 99; vm.distM = 1200;
    k.vueltas.push_back(Vuelta::desdeMotor(vm, 2));
    CHEQUEAR(k.parametrosFijos());
    IGUAL(k.proximaLargada(), 3);

    Base b;
    b.config = g;
    b.carreras.push_back(k);
    Json j = b.aJson();
    IGUAL(j["schemaVersion"].aEntero(), int64_t(2));
    const Json& vj = j["carreras"].elementos()[0]["vueltas"].elementos()[0];
    CHEQUEAR(!vj.tiene("difMs"));
    CHEQUEAR(vj["interrumpida"].aBool());
    IGUAL(j["carreras"].elementos()[0]["piloto"].aTexto(), std::string("viejo"));
    IGUAL(j["carreras"].elementos()[0]["algoNuevo"].aEntero(), int64_t(7));
    Base b2 = Base::desdeJson(Json::leer(j.escribir()));
    CHEQUEAR(b2.aJson() == j);

    // Nombre largo: se recorta a 80 letras sin partir acentos
    Carrera largo;
    largo.nombre = std::string(79, 'a') + "ñññ";
    std::string n = largo.aJson()["nombre"].aTexto();
    IGUAL(n, std::string(79, 'a') + "ñ");
}

static std::string carpetaTemporal() {
    char plantilla[] = "/tmp/beto-pruebaXXXXXX";
    const char* r = mkdtemp(plantilla);
    return r ? r : "/tmp";
}

static void almacen() {
    std::string dir = carpetaTemporal();
    Almacen a(dir);
    {
        auto l = a.cargar();
        IGUAL(l.origen, std::string(""));
        IGUAL(l.base.carreras.size(), size_t(0));
    }
    Base b;
    Carrera c;
    c.id = "c1";
    c.nombre = "Primera";
    b.carreras.push_back(c);
    CHEQUEAR(a.guardar(b));
    b.carreras[0].nombre = "Segunda";
    CHEQUEAR(a.guardar(b));
    {
        auto l = a.cargar();
        IGUAL(l.origen, a.archivo());
        IGUAL(l.base.carreras[0].nombre, std::string("Segunda"));
    }
    // Copia anterior
    {
        std::ifstream f(a.archivo() + ".anterior");
        CHEQUEAR(f.good());
    }
    // Archivo principal roto: se usa la copia anterior y el roto se guarda aparte
    {
        std::ofstream f(a.archivo(), std::ios::trunc);
        f << "{\"schemaVersion\":2,\"carre";
    }
    {
        auto l = a.cargar();
        CHEQUEAR(l.habiaDanado);
        IGUAL(l.origen, a.archivo() + ".anterior");
        IGUAL(l.base.carreras[0].nombre, std::string("Primera"));
        std::ifstream f(a.archivo() + ".danado");
        CHEQUEAR(f.good());
    }
    // Corte de luz justo después de escribir «.nuevo»: se usa lo nuevo
    {
        b.carreras[0].nombre = "Tercera";
        std::ofstream f(a.archivo() + ".nuevo", std::ios::trunc);
        f << b.aJson().escribir();
    }
    {
        auto l = a.cargar();
        IGUAL(l.base.carreras[0].nombre, std::string("Tercera"));
    }
    // Corte de luz mientras se escribía «.nuevo»: está incompleto y se ignora
    CHEQUEAR(a.guardar(b));
    {
        std::ofstream f(a.archivo() + ".nuevo", std::ios::trunc);
        f << "{\"schemaVersion\":2,\"carreras\":[{\"id\":\"c1\",\"nom";
    }
    {
        auto l = a.cargar();
        IGUAL(l.origen, a.archivo());
        IGUAL(l.base.carreras[0].nombre, std::string("Tercera"));
    }
    std::string limpiar = "rm -rf '" + dir + "'";
    if (std::system(limpiar.c_str()) != 0) std::cerr << "no pude borrar " << dir << "\n";
}

static void voz() {
    using T = Sonido::Tipo;
    // Una vuelta de 60 s, cuenta de 20 s en voz, anticipo 40 ms: se recorre cada 15 ms
    auto correr = [](const AjustesVoz& a, double T_ms, double hasta) {
        ProgramadorVoz p;
        p.reiniciarVuelta();
        std::vector<std::pair<double, Sonido>> dichos;
        for (double t = 0; t <= hasta; t += 15) {
            Estado e;
            e.enCurso = true;
            e.transMs = t;
            e.restanteMs = T_ms - t;
            for (const auto& s : p.tic(e, a)) dichos.push_back({t, s});
        }
        return dichos;
    };
    AjustesVoz a;
    auto d = correr(a, 60000, 62000);
    IGUAL(d.size(), size_t(21));                         // 20, 19, …, 1 y «cero»
    IGUAL(d.front().second.numero, 20);
    CHEQUEAR(d.front().first >= 40000 - 40 - 15 && d.front().first <= 40000 - 40 + 15);   // adelantado 40 ms
    IGUAL(d[19].second.numero, 1);
    CHEQUEAR(d.back().second.tipo == T::Cero);
    CHEQUEAR(d.back().first >= 60000 - 40 - 15 && d.back().first <= 60000);
    // Nunca repite ni vuelve atrás
    for (size_t i = 1; i < 20; ++i) IGUAL(d[i].second.numero, d[i - 1].second.numero - 1);
    // Pitido largo en vez de «cero», sin anticipo
    a.pitidoCero = true;
    d = correr(a, 60000, 62000);
    CHEQUEAR(d.back().second.tipo == T::PitidoLargo);
    CHEQUEAR(d.back().first >= 60000 && d.back().first <= 60015);
    // Pitidos en vez de voz: sin anticipo, 20 pitidos cortos + el del cero
    a = AjustesVoz{};
    a.enVoz = false;
    d = correr(a, 60000, 62000);
    IGUAL(d.size(), size_t(21));
    CHEQUEAR(d.front().second.tipo == T::PitidoCorto && d.front().first >= 40000 && d.front().first <= 40000 + 15);
    // Sin cuenta: nada (ni «cero»)
    a = AjustesVoz{};
    a.cuentaN = 0;
    IGUAL(correr(a, 60000, 62000).size(), size_t(0));
    // Tic cada 10 s: en 10, 20, 30 (a los 40 ya empieza la cuenta: se calla)
    a = AjustesVoz{};
    a.pitidoCadaS = 10;
    d = correr(a, 60000, 39000);
    IGUAL(d.size(), size_t(3));
    CHEQUEAR(d[0].second.tipo == T::Tic);
    // Anuncios
    IGUAL(*anuncioVuelta(3, true, 10), std::string("Vuelta 3, medida"));
    IGUAL(*anuncioVuelta(10, false, 10), std::string("Última vuelta, práctica"));
    CHEQUEAR(!anuncioVuelta(11, true, 10).has_value());
    IGUAL(*anuncioVuelta(25, true, 0), std::string("Vuelta 25, medida"));
}

static void gpsNeo() {
    LecturaGps l;
    l.iTOW = 345678900; l.anio = 2026; l.mes = 11; l.dia = 1; l.hora = 14; l.minuto = 32; l.segundo = 7; l.horaValida = true;
    l.fix3d = true; l.satelites = 9; l.lat = -34.6037222; l.lon = -58.3815931; l.precisionM = 2.1; l.velMs = 13.333; l.precisionVelMs = 0.15;
    // UBX: ida y vuelta, en pedacitos (como llega por el cable)
    const auto m = armarNavPvt(l);
    IGUAL(m.size(), size_t(100));
    DecodificadorNeo d;
    std::vector<LecturaGps> r;
    for (size_t i = 0; i < m.size(); i += 7) {
        auto x = d.agregar(m.data() + i, std::min<size_t>(7, m.size() - i));
        r.insert(r.end(), x.begin(), x.end());
    }
    IGUAL(r.size(), size_t(1));
    IGUAL(r[0].iTOW, l.iTOW);
    CHEQUEAR(r[0].fix3d && r[0].horaValida);
    IGUAL(r[0].satelites, 9);
    CHEQUEAR(std::fabs(r[0].lat - l.lat) < 1e-7 && std::fabs(r[0].lon - l.lon) < 1e-7);
    CHEQUEAR(std::fabs(r[0].precisionM - 2.1) < 1e-9 && std::fabs(r[0].velMs - 13.333) < 1e-9);
    CHEQUEAR(std::fabs(r[0].precisionVelMs - 0.15) < 1e-9);
    IGUAL(r[0].hora, 14);
    // Basura antes y un mensaje con la suma mala: se descarta y sigue
    auto malo = m;
    malo[30] ^= 0xFF;
    std::vector<uint8_t> todo = {'x', 0xB5, 0x00};
    todo.insert(todo.end(), malo.begin(), malo.end());
    todo.insert(todo.end(), m.begin(), m.end());
    DecodificadorNeo d2;
    IGUAL(d2.agregar(todo.data(), todo.size()).size(), size_t(1));
    CHEQUEAR(d2.errores() >= 1);
    // Sin posición 3D buena
    l.fix3d = false;
    const auto sinFix = armarNavPvt(l);
    CHEQUEAR(!d.agregar(sinFix.data(), sinFix.size())[0].fix3d);
    l.fix3d = true;
    // NMEA (GGA + GST + RMC)
    const std::string n = armarNmea(l);
    DecodificadorNeo dn;
    auto rn = dn.agregar(reinterpret_cast<const uint8_t*>(n.data()), n.size());
    IGUAL(rn.size(), size_t(1));
    CHEQUEAR(rn[0].deNmea && rn[0].fix3d && rn[0].horaValida);
    CHEQUEAR(std::fabs(rn[0].lat - l.lat) < 1e-6 && std::fabs(rn[0].lon - l.lon) < 1e-6);
    CHEQUEAR(std::fabs(rn[0].velMs - l.velMs) < 0.001);
    CHEQUEAR(std::fabs(rn[0].precisionM - 2.1) < 0.05);
    IGUAL(rn[0].dia, 1);
    // Configuración del módulo: mensajes UBX bien armados (los lee el mismo decodificador sin errores)
    const auto cfg = configuracionNeo(false);
    IGUAL(cfg.size(), size_t(11));   // GNSS, RATE, NAV5, NAV-PVT, 6 NMEA apagados y CFG-CFG
    DecodificadorNeo dc;
    for (const auto& c : cfg) dc.agregar(c.data(), c.size());
    const auto prt = cambioA115200();
    dc.agregar(prt.data(), prt.size());
    IGUAL(dc.errores(), 0);
    // CFG-RATE: 200 ms con GPS + GLONASS (5 Hz, el máximo del NEO-M8N con dos sistemas); 100 ms con GPS solo
    IGUAL(int(cfg[1][6] | (cfg[1][7] << 8)), 200);
    IGUAL(int(configuracionNeo(true)[1][6] | (configuracionNeo(true)[1][7] << 8)), 100);
    // CFG-GNSS: GLONASS (gnssId 6, último bloque) prendido o apagado
    IGUAL(int(cfg[0][6 + 4 + 8 * 5 + 4] & 1), 1);
    IGUAL(int(configuracionNeo(true)[0][6 + 4 + 8 * 5 + 4] & 1), 0);
    IGUAL(int(cfg[0][6 + 4 + 8 * 2 + 4] & 1), 0);   // Galileo apagado
    // MON-VER: original y copia
    VersionNeo orig{"EXT CORE 3.01 (107888)", "00080000", {"ROM BASE 2.01 (75331)", "FWVER=SPG 3.01", "PROTVER=18.00", "MOD=NEO-M8N-0"}};
    const auto mv = armarMonVer(orig);
    DecodificadorNeo dv;
    dv.agregar(mv.data(), mv.size());
    CHEQUEAR(dv.version().has_value() && dv.version()->pareceOriginal());
    IGUAL(dv.version()->firmware(), std::string("SPG 3.01"));
    IGUAL(dv.version()->modelo(), std::string("NEO-M8N-0"));
    VersionNeo copia{"ROM CORE 3.01 (107888)", "00080000", {"FWVER=SPG 3.01", "PROTVER=18.00"}};
    const auto mc = armarMonVer(copia);
    DecodificadorNeo dcp;
    dcp.agregar(mc.data(), mc.size());
    CHEQUEAR(!dcp.version()->pareceOriginal());
    // Otras familias que se consiguen
    CHEQUEAR(familiaDe(orig) == FamiliaNeo::M8);
    CHEQUEAR(familiaDe(VersionNeo{"1.00 (59842)", "00070000", {}}) == FamiliaNeo::U7);
    CHEQUEAR(familiaDe(VersionNeo{"7.03 (45969)", "00040007", {}}) == FamiliaNeo::U6);
    // NEO-7M: NAV-PVT de 84 bytes (mismos campos): se lee
    {
        auto m84 = armarNavPvt(l);
        m84.erase(m84.begin() + 6 + 84, m84.end() - 2);
        m84[4] = 84;
        uint8_t a = 0, b = 0;
        for (size_t k = 2; k < m84.size() - 2; ++k) { a = static_cast<uint8_t>(a + m84[k]); b = static_cast<uint8_t>(b + a); }
        m84[m84.size() - 2] = a; m84[m84.size() - 1] = b;
        DecodificadorNeo d7;
        auto r7 = d7.agregar(m84.data(), m84.size());
        IGUAL(r7.size(), size_t(1));
        CHEQUEAR(std::fabs(r7[0].velMs - 13.333) < 1e-9);
    }
    // NEO-7M: GLONASS apagado (un sistema a la vez) y 10 Hz
    {
        const auto c7 = configuracionNeo(false, FamiliaNeo::U7);
        IGUAL(int(c7[0][6 + 4 + 8 * 5 + 4] & 1), 0);
        IGUAL(int(c7[1][6] | (c7[1][7] << 8)), 100);
    }
    // NEO-6M: sin CFG-GNSS (no lo tiene), 5 Hz y NMEA prendido (GGA, RMC, GST)
    {
        const auto c6 = configuracionNeo(false, FamiliaNeo::U6);
        IGUAL(int(c6[0][3]), 0x08);   // el primero ya es CFG-RATE
        IGUAL(int(c6[0][6] | (c6[0][7] << 8)), 200);
        int prendidas = 0;
        for (const auto& c : c6) if (c[3] == 0x01 && c[6] == 0xF0 && c[8] == 1) ++prendidas;
        IGUAL(prendidas, 3);
    }
}

// Recorre el filtro de a 1 ms, como lo haría el temporizador, y junta las marcas
static std::vector<double> marcasDe(FiltroPulsador& f, const std::vector<std::pair<double, bool>>& flancos, double hasta) {
    std::vector<double> r;
    size_t k = 0;
    for (double t = 0; t <= hasta; t += 0.5) {
        while (k < flancos.size() && flancos[k].first <= t) { f.flanco(flancos[k].first, flancos[k].second); ++k; }
        if (auto m = f.revisar(t)) r.push_back(*m);
    }
    return r;
}

static void pulsador() {
    {   // Apretar y soltar limpio: una marca, con la hora de apretar (no de soltar)
        FiltroPulsador f;
        auto m = marcasDe(f, {{1000, true}, {1300, false}}, 2000);
        IGUAL(m.size(), size_t(1));
        IGUAL(m[0], 1000.0);
    }
    {   // Rebotes al apretar: una sola marca, con la hora del primer contacto
        FiltroPulsador f;
        auto m = marcasDe(f, {{1000, true}, {1001, false}, {1002, true}, {1003.5, false}, {1004, true}, {1250, false}}, 2000);
        IGUAL(m.size(), size_t(1));
        IGUAL(m[0], 1000.0);
    }
    {   // Pico de ruido (una chispa): no marca
        FiltroPulsador f;
        auto m = marcasDe(f, {{1000, true}, {1000.2, false}, {1500, true}, {1500.1, false}}, 2000);
        IGUAL(m.size(), size_t(0));
        IGUAL(f.descartados(), 2);
    }
    {   // Mantenerlo apretado 5 s: una sola marca
        FiltroPulsador f;
        auto m = marcasDe(f, {{1000, true}, {6000, false}}, 7000);
        IGUAL(m.size(), size_t(1));
    }
    {   // Rebotes al soltar: no marcan de nuevo; la próxima apretada sí
        FiltroPulsador f;
        auto m = marcasDe(f, {{1000, true}, {1300, false}, {1301, true}, {1302, false}, {1303, true}, {1304, false}, {1500, true}, {1700, false}}, 2000);
        IGUAL(m.size(), size_t(2));
        IGUAL(m[1], 1500.0);
    }
    {   // La marca se confirma a los 10 ms de apoyar el dedo (no hay que esperar a soltar)
        FiltroPulsador f;
        f.flanco(1000, true);
        CHEQUEAR(!f.revisar(1005).has_value());
        auto m = f.revisar(1010);
        CHEQUEAR(m.has_value() && *m == 1000.0);
        CHEQUEAR(!f.proximoVencimiento().has_value());   // apretado: no hay nada pendiente
    }
}

int main() {
    pulsador();
    gpsNeo();
    voz();
    redondeos();
    json();
    vueltaInterrumpida();
    soloCronometro();
    traza();
    modelo();
    almacen();
    std::cout << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
