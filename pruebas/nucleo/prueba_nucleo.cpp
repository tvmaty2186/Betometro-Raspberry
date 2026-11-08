// Pruebas del núcleo: lo que no se compara con la app (lo propio de la Raspberry, el JSON,
// el modelo y el almacén) y los redondeos iguales a JavaScript.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

#include "almacen.h"
#include "informe.h"
#include "json.h"
#include "marca_gps.h"
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
    IGUAL(d.vueltasTotal, 0);                 // sin límite de vueltas
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
        CHEQUEAR(!l.migrado && !l.habiaDanado);
    }
    // Un archivo por carrera y config.json aparte
    Carrera c;
    c.id = "c1";
    c.nombre = "Primera";
    c.creada = "2026-01-01T10:00:00";
    CHEQUEAR(a.guardarCarrera(c));
    c.nombre = "Segunda";
    CHEQUEAR(a.guardarCarrera(c));
    Json cfg = Json::objeto();
    cfg["volumen"] = 7;
    CHEQUEAR(a.guardarConfig(cfg));
    CHEQUEAR(std::ifstream(dir + "/config.json").good());
    CHEQUEAR(std::ifstream(a.archivoCarrera("c1")).good());
    IGUAL(a.archivoCarrera("c1"), dir + "/carreras/c1.json");
    IGUAL(Almacen::nombreArchivo("a/b..c"), std::string("a_b__c.json"));   // sin caracteres raros
    {
        auto l = a.cargar();
        IGUAL(l.origen, a.carpetaCarreras());
        IGUAL(l.base.carreras.size(), size_t(1));
        IGUAL(l.base.carreras[0].nombre, std::string("Segunda"));
        IGUAL(l.base.config["volumen"].aEntero(), int64_t(7));
    }
    // Copia anterior de cada archivo
    CHEQUEAR(std::ifstream(a.archivoCarrera("c1") + ".anterior").good());
    // Archivo de una carrera roto: se usa su copia anterior, el roto se guarda aparte y las demás se leen igual
    Carrera c2;
    c2.id = "c2";
    c2.nombre = "Otra";
    c2.creada = "2026-01-02T10:00:00";
    CHEQUEAR(a.guardarCarrera(c2));
    {
        std::ofstream f(a.archivoCarrera("c1"), std::ios::trunc);
        f << "{\"id\":\"c1\",\"nom";
    }
    {
        auto l = a.cargar();
        CHEQUEAR(l.habiaDanado);
        CHEQUEAR(!l.avisos.empty());
        IGUAL(l.base.carreras.size(), size_t(2));
        IGUAL(l.base.carreras[0].nombre, std::string("Primera"));   // la copia anterior
        IGUAL(l.base.carreras[1].nombre, std::string("Otra"));      // por fecha de creación
        CHEQUEAR(std::ifstream(a.archivoCarrera("c1") + ".danado").good());
    }
    // Corte de luz justo después de escribir «.nuevo»: se usa lo nuevo
    CHEQUEAR(a.guardarCarrera(c));
    {
        c.nombre = "Tercera";
        std::ofstream f(a.archivoCarrera("c1") + ".nuevo", std::ios::trunc);
        f << c.aJson().escribir();
    }
    {
        auto l = a.cargar();
        IGUAL(l.base.carreras[0].nombre, std::string("Tercera"));
    }
    // Corte de luz mientras se escribía «.nuevo»: está incompleto y se ignora, sin avisar
    CHEQUEAR(a.guardarCarrera(c));
    {
        std::ofstream f(a.archivoCarrera("c1") + ".nuevo", std::ios::trunc);
        f << "{\"id\":\"c1\",\"nom";
    }
    {
        auto l = a.cargar();
        IGUAL(l.base.carreras[0].nombre, std::string("Tercera"));
        CHEQUEAR(!l.habiaDanado);                                              // un guardado cortado no es un daño
        CHEQUEAR(!std::ifstream(a.archivoCarrera("c1") + ".nuevo").good());   // se borró
    }
    // Borrar: el archivo va a la papelera; a los 30 días se vacía
    CHEQUEAR(a.borrarCarrera("c2"));
    CHEQUEAR(!std::ifstream(a.archivoCarrera("c2")).good());
    CHEQUEAR(std::ifstream(a.carpetaPapelera() + "/c2.json").good());
    CHEQUEAR(a.borrarCarrera("noexiste"));   // nunca se guardó: no es error
    a.vaciarPapelera(30);
    CHEQUEAR(std::ifstream(a.carpetaPapelera() + "/c2.json").good());   // recién borrada
    a.vaciarPapelera(0);
    CHEQUEAR(!std::ifstream(a.carpetaPapelera() + "/c2.json").good());
    {
        auto l = a.cargar();
        IGUAL(l.base.carreras.size(), size_t(1));
    }
    // Migración desde betometro.json (un solo archivo, hasta la 1.0.28)
    std::string dir2 = carpetaTemporal();
    {
        Base b;
        b.config["brillo"] = 55;
        Carrera x;
        x.id = "cviejo1"; x.nombre = "Vieja 1"; x.creada = "2025-05-01T10:00:00";
        Vuelta v;
        v.n = 1; v.realMs = 180500; v.difMs = 500;
        v.traza.push_back({0, 0, 12.5, -34.6, -58.38, 4});
        x.vueltas.push_back(v);
        x.extra["recorrido"] = true;
        b.carreras.push_back(x);
        Carrera y;
        y.id = "cviejo2"; y.nombre = "Vieja 2"; y.creada = "2025-04-01T10:00:00";
        b.carreras.push_back(y);
        { std::ofstream f(dir2 + "/betometro.json", std::ios::trunc); f << b.aJson().escribir(); }
        Almacen m(dir2);
        auto l = m.cargar();
        CHEQUEAR(l.migrado && !l.habiaDanado);
        IGUAL(l.base.carreras.size(), size_t(2));
        IGUAL(l.base.carreras[0].id, std::string("cviejo2"));   // por fecha de creación
        CHEQUEAR(l.base.carreras[1].aJson() == x.aJson());
        CHEQUEAR(l.base.config == b.config);
        CHEQUEAR(std::ifstream(dir2 + "/betometro.json.migrado").good());
        CHEQUEAR(!std::ifstream(dir2 + "/betometro.json").good());
        CHEQUEAR(std::ifstream(dir2 + "/carreras/cviejo1.json").good());
        CHEQUEAR(!std::ifstream(dir2 + "/carreras.nuevo").good());
        // La segunda vez no hay nada que migrar
        auto l2 = m.cargar();
        CHEQUEAR(!l2.migrado);
        IGUAL(l2.base.carreras.size(), size_t(2));
    }
    // Migración cortada a mitad (quedó carreras.nuevo/ y betometro.json): se repite y no duplica
    std::string dir3 = carpetaTemporal();
    {
        Base b;
        Carrera x;
        x.id = "cx"; x.nombre = "Equis";
        b.carreras.push_back(x);
        { std::ofstream f(dir3 + "/betometro.json", std::ios::trunc); f << b.aJson().escribir(); }
        std::string mk = "mkdir -p '" + dir3 + "/carreras.nuevo' && echo '{\"id\":\"cx\",\"nombre\":\"a medias\"}' > '" + dir3 + "/carreras.nuevo/cx.json'";
        CHEQUEAR(std::system(mk.c_str()) == 0);
        Almacen m(dir3);
        auto l = m.cargar();
        CHEQUEAR(l.migrado);
        IGUAL(l.base.carreras.size(), size_t(1));
        IGUAL(l.base.carreras[0].nombre, std::string("Equis"));
    }
    for (const std::string& d : {dir, dir2, dir3}) {
        std::string limpiar = "rm -rf '" + d + "'";
        if (std::system(limpiar.c_str()) != 0) std::cerr << "no pude borrar " << d << "\n";
    }
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
    IGUAL(*anuncioVuelta(10, false, 10), std::string("Última vuelta, libre"));
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
    {   // Apretar y soltar llegan juntos (el programa estaba ocupado 60 ms): la marca no se pierde (auditoría 12)
        FiltroPulsador f;
        f.flanco(1000, true);
        f.flanco(1060, false);
        CHEQUEAR(f.proximoVencimiento().has_value());
        auto m = f.revisar(1061);
        CHEQUEAR(m.has_value() && *m == 1000.0);
        CHEQUEAR(!f.revisar(1062).has_value());   // una sola
    }
    {   // Dos toques en el mismo paquete: llegan los dos, en orden
        FiltroPulsador f;
        f.flanco(1000, true);
        f.flanco(1060, false);
        f.flanco(1200, true);
        f.flanco(1260, false);
        auto a = f.revisar(1261), b = f.revisar(1262);
        CHEQUEAR(a.has_value() && *a == 1000.0);
        CHEQUEAR(b.has_value() && *b == 1200.0);
    }
}

// Marca doble (caso real del 31/10/2026, ideal 1:50,0): la V5 duró 4,1 s y la V6 106,3 s.
// Unidas dan una vuelta de 110,5 s; las siguientes se renumeran.
static void unirVueltas() {
    Carrera c;
    c.tiempoIdealMs = 110000;
    auto vuelta = [&](int n, bool medida, int64_t real, double dist, double vmax) {
        Vuelta v;
        v.n = n; v.medida = medida; v.idealMs = 110000; v.realMs = real; v.difMs = real - 110000;
        v.distM = dist; v.velMaxKmh = vmax; v.velPromKmh = dist / (real / 1000.0) * 3.6;
        v.traza = {FilaTraza{0, 0, 90, 0, 0, 3}, FilaTraza{real / 1000.0, dist, 90, 0, 0, 3}};
        c.vueltas.push_back(v);
    };
    vuelta(4, false, 110071, 2710, 115.3);
    vuelta(5, true, 4125, 112, 100);
    vuelta(6, false, 106336, 2597, 112);
    vuelta(7, true, 101164, 2747, 129.7);
    CHEQUEAR(!c.unirConSiguiente(3));            // la última no tiene siguiente
    CHEQUEAR(c.unirConSiguiente(1));
    IGUAL(c.vueltas.size(), size_t(3));
    const Vuelta& u = c.vueltas[1];
    IGUAL(u.n, 5);
    CHEQUEAR(u.medida);                          // queda el tipo de la primera
    IGUAL(u.realMs, int64_t(110461));
    IGUAL(*u.difMs, int64_t(461));
    IGUAL(*u.distM, 2709.0);
    IGUAL(*u.velPromKmh, 88.3);
    IGUAL(*u.velMaxKmh, 112.0);
    IGUAL(u.traza.size(), size_t(4));
    IGUAL(u.traza[2][0], 4.1);                   // la traza de la segunda sigue a la primera
    IGUAL(u.traza[3][1], 2709.0);
    IGUAL(c.vueltas[2].n, 6);                    // la 7 pasa a ser la 6
    // Otra largada o interrumpida: no se une
    c.vueltas[2].largada = 2;
    CHEQUEAR(!c.unirConSiguiente(1));
    c.vueltas[2].largada = 1;
    c.vueltas[2].interrumpida = true;
    CHEQUEAR(!c.unirConSiguiente(1));
}

// El informe real (pruebas/reales/practica-2026-10-31-beto.html, app del celular): se leen sus
// vueltas y se une la V5 (4,1 s, marca de más) con la V6. Tiene que dar 1:50,5 (+0,5 s) y 8 vueltas.
static void informeReal() {
    std::ifstream f(std::string(BETO_REALES) + "/practica-2026-10-31-beto.html");
    std::stringstream ss;
    ss << f.rdbuf();
    const std::string html = ss.str();
    const size_t a = html.find("const C=");
    CHEQUEAR(a != std::string::npos);
    if (a == std::string::npos) return;
    const size_t b = html.find('\n', a);
    std::string datos = html.substr(a + 8, b - a - 8);
    while (!datos.empty() && (datos.back() == ';' || datos.back() == ' ' || datos.back() == '\r')) datos.pop_back();
    const Json C = Json::leer(datos);
    Carrera c;
    c.tiempoIdealMs = C["idealMs"].aEntero();
    for (const Json& j : C["vueltas"].elementos()) {
        Vuelta v;
        v.n = static_cast<int>(j["n"].aEntero());
        v.largada = static_cast<int>(j["largada"].aEntero(1));
        v.medida = j["medida"].aBool(true);
        v.idealMs = c.tiempoIdealMs;
        v.realMs = j["realMs"].aEntero();
        v.difMs = j["difMs"].aEntero();
        v.distM = j["L"].aNumero();
        v.velMaxKmh = j["velMaxKmh"].aNumero();
        c.vueltas.push_back(v);
    }
    IGUAL(c.vueltas.size(), size_t(9));
    IGUAL(c.vueltas[4].realMs, int64_t(4125));
    CHEQUEAR(c.unirConSiguiente(4));
    IGUAL(c.vueltas.size(), size_t(8));
    IGUAL(c.vueltas[4].realMs, int64_t(110461));
    IGUAL(*c.vueltas[4].difMs, int64_t(461));
    IGUAL(*c.vueltas[4].distM, 2709.0);
    IGUAL(c.vueltas[7].n, 8);
}

// «Restablecer» (app 2.4): antes del primer cambio se guarda la carrera como se corrió; volver
// atrás deshace uniones y cambios de MEDIDA/LIBRE, y se puede volver a guardar después.
static void restablecerCarrera() {
    Carrera c;
    c.tiempoIdealMs = 100000;
    for (int n = 1; n <= 4; n++) {
        Vuelta v;
        v.n = n; v.medida = n % 2 == 1; v.idealMs = 100000; v.realMs = n == 2 ? 4000 : 99000; v.difMs = v.realMs - 100000;
        c.vueltas.push_back(v);
    }
    const Json antes = c.aJson()["vueltas"];
    CHEQUEAR(!c.modificada());
    CHEQUEAR(!c.restablecer());                  // sin cambios no hay nada que restablecer
    c.guardarOriginal();
    CHEQUEAR(c.unirConSiguiente(0));
    c.guardarOriginal();                         // la segunda vez no pisa la original
    c.vueltas[1].medida = !c.vueltas[1].medida;
    IGUAL(c.vueltas.size(), size_t(3));
    CHEQUEAR(c.modificada());
    const Carrera leida = Carrera::desdeJson(Json::leer(c.aJson().escribir()));   // se guarda y se lee
    CHEQUEAR(leida.modificada());
    CHEQUEAR(c.restablecer());
    IGUAL(c.vueltas.size(), size_t(4));
    CHEQUEAR(!c.modificada());
    CHEQUEAR(c.aJson()["vueltas"] == antes);
}

// Informe con datos raros (base dañada o de afuera): distancia negativa, de más, posición rara.
// Antes se escribía fuera de la memoria y el programa se cerraba (auditoría 02-1.1).
static void informeDanado() {
    for (int caso = 0; caso < 4; ++caso) {
        Carrera c;
        c.tiempoIdealMs = 60000;
        for (int n = 1; n <= 3; ++n) {
            Vuelta v;
            v.n = n; v.idealMs = 60000; v.realMs = 60000; v.difMs = 0; v.distM = 1000;
            for (int i = 0; i < 60; ++i) {
                double d = i * 1000.0 / 59;
                if (caso == 0 && i == 30) d = -500;          // distancia negativa
                if (caso == 1 && i == 10) d = -1e9;
                if (caso == 2 && i == 20) d = 1e12;          // mucho más larga que la vuelta
                double lat = -34.6 + d * 1e-6, lon = -58.38;
                if (caso == 3 && i == 5) lat = 1e6;          // posición imposible
                v.traza.push_back({i * 1.0, d, 50, lat, lon, 4});
            }
            c.vueltas.push_back(v);
        }
        const Json j = datosInforme(c, 20);
        CHEQUEAR(j.esObjeto());
    }
}

// Números al guardar: el camino rápido (to_chars) da el mismo texto que el de siempre (05-2.1)
static void numerosRapidos() {
    uint64_t z = 88172645463325252ull;
    auto azar = [&]() { z ^= z << 13; z ^= z >> 7; z ^= z << 17; return z; };
    int distintos = 0;
    for (int i = 0; i < 300000; ++i) {
        double x;
        switch (i % 5) {
            case 0: x = -34.6 + (azar() % 2000000) / 1e6; break;                         // latitud
            case 1: x = std::round((azar() % 3000000) / 10.0) / 10.0; break;              // un decimal
            case 2: x = static_cast<double>(azar() % 100000000); break;                   // ms
            case 3: { uint64_t b = azar(); std::memcpy(&x, &b, 8); if (!std::isfinite(x)) x = 1; break; }   // cualquiera
            default: x = (azar() % 1000000) / 1e9; break;                                // chiquitos
        }
        if (numeroComoJs(x) != numeroComoJsLento(x)) ++distintos;
    }
    IGUAL(distintos, 0);
    IGUAL(numeroComoJs(1e21), std::string("1e+21"));
    IGUAL(numeroComoJs(-0.5), std::string("-0.5"));
    IGUAL(numeroComoJs(1e-7), std::string("1e-7"));
    IGUAL(Json::leer("[1e999]").elementos()[0].aNumero(), HUGE_VAL);
    IGUAL(Json::leer("-58.3815911").aNumero(), -58.3815911);
}

// Marca virtual por GPS: un círculo de 300 m de radio a 72 km/h, GPS a 5 por segundo. El pulsador marca cada vuelta
// con un desfasaje conocido; la marca virtual tiene que encontrar el cruce real (con el redondeo a 0,1 s de la traza)
static void marcaGps() {
    const double R = 300, vel = 20, L = 2 * M_PI * R, periodo = L / vel * 1000;   // ms
    const double lat0 = -32.97, lon0 = -60.65, kx = 111320 * std::cos(lat0 * M_PI / 180), ky = 110540;
    const std::vector<int64_t> desfase = {120, 80, -50, 200};   // + = el pulsador marcó tarde
    auto armar = [&](double prec) {
        Carrera c;
        c.tiempoIdealMs = static_cast<int64_t>(periodo);
        int64_t inicio = 0;
        for (size_t k = 0; k < desfase.size(); ++k) {
            const int64_t fin = static_cast<int64_t>(std::llround(periodo * (k + 1))) + desfase[k];
            Vuelta v;
            v.n = static_cast<int>(k) + 1; v.idealMs = c.tiempoIdealMs; v.realMs = fin - inicio; v.difMs = v.realMs - v.idealMs;
            for (int64_t t = (inicio / 200 + 1) * 200; t < fin; t += 200) {   // lecturas en la grilla de 200 ms del GPS
                const double th = vel * t / 1000.0 / R, x = R * std::sin(th), y = R * (1 - std::cos(th));
                v.traza.push_back(FilaTraza{fijarDecimalesJs((t - inicio) / 1000.0, 1), 0, 72,
                                            fijarDecimalesJs(lat0 + y / ky, 6), fijarDecimalesJs(lon0 + x / kx, 6), prec});
            }
            if (k == 0) v.traza.insert(v.traza.begin(), FilaTraza{0, 0, 72, lat0, lon0, prec});
            c.vueltas.push_back(v);
            inicio = fin;
        }
        return c;
    };
    const auto m = marcasGps(armar(3));
    IGUAL(m.size(), desfase.size());
    for (size_t k = 0; k + 1 < desfase.size(); ++k) {
        CHEQUEAR(m[k].hay && !m[k].dudosa);
        CHEQUEAR(std::llabs(m[k].pulsadorMenosGpsMs - desfase[k]) <= 60);   // el redondeo de la traza: ±50 ms
        const int64_t real = static_cast<int64_t>(std::llround(periodo));
        CHEQUEAR(std::llabs(m[k].realMs - real) <= 120);
    }
    // La última: el pulsador marcó tarde y la traza termina en la marca, después de cruzar: se encuentra igual
    CHEQUEAR(m[3].hay);
    // Con poca precisión: dudosa
    const auto md = marcasGps(armar(25));
    CHEQUEAR(md[0].hay && md[0].dudosa);
    // Sin traza: no hay
    Carrera vacia; vacia.vueltas.resize(2);
    const auto mv = marcasGps(vacia);
    CHEQUEAR(!mv[0].hay && !mv[1].hay);
}

int main() {
    numerosRapidos();
    informeDanado();
    unirVueltas();
    informeReal();
    restablecerCarrera();
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
    marcaGps();
    std::cout << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
