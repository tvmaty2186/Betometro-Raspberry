// Carrera de punta a punta con el simulador del NEO-M8N (auditoría 09, P3): simular_neo arma un puerto serie
// virtual y manda NAV-PVT como el módulo; el GpsNeo de verdad lo lee; el Crono corre una carrera de 3 vueltas
// a velocidad conocida, con el reloj de verdad. Se chequean las vueltas guardadas, los tiempos (±0,15 s), las
// distancias (±3 %), la traza y el informe. Después, con --corte, que el módulo pierda la señal sin callarse
// (11A-1.2). Tarda unos 40 s.
// Uso: prueba_carrera <ruta de simular_neo>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <functional>
#include <iostream>

#include "crono.h"

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c) do { ++chequeos; if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

// Deja correr el programa hasta que `listo` dé true o pasen `ms`
static bool esperar(const std::function<bool()>& listo, int ms) {
    QElapsedTimer t;
    t.start();
    while (!listo() && t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return listo();
}

// Arranca simular_neo y espera a que el puerto virtual esté (lo anuncia por la salida)
static bool arrancarSimulador(QProcess& sim, const QString& programa, const QStringList& args) {
    sim.start(programa, args);
    if (!sim.waitForStarted(5000)) { std::cerr << "no arranca " << programa.toStdString() << "\n"; return false; }
    QByteArray salida;
    QElapsedTimer t;
    t.start();
    while (!salida.contains("GPS simulado") && t.elapsed() < 5000) {
        if (sim.waitForReadyRead(500)) salida += sim.readAllStandardOutput();
    }
    return salida.contains("GPS simulado");
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { std::cerr << "Uso: prueba_carrera <simular_neo>\n"; return 2; }
    const QString simularNeo = QString::fromLocal8Bit(argv[1]);
    QTemporaryDir dir, dirCarrera, dirCorte;

    // ---------- 1) Carrera de 3 vueltas a 48 km/h fijos (13,333 m/s), 10 lecturas por segundo ----------
    {
        const QString enlace = dir.path() + "/gps";
        QProcess sim;
        CHEQUEAR(arrancarSimulador(sim, simularNeo, {"--enlace", enlace, "--hz", "10", "--kmh", "48", "--fija", "--largo", "100000"}));
        Datos datos(dirCarrera.path());
        datos.ponerCfg("calibrar", false);   // la distancia cruda, sin ajustar al largo del circuito
        GpsNeo gps(enlace);
        CHEQUEAR(gps.abierto());
        Crono crono(&datos, &gps);
        int muestras = 0, datosGps = 0, satelites = 0;
        double primeraT = 0, ultimaT = 0, maxHueco = 0;
        QObject::connect(&gps, &FuenteGps::muestra, [&](const beto::Muestra& m) {
            if (muestras == 0) primeraT = m.t; else maxHueco = std::max(maxHueco, m.t - ultimaT);
            ultimaT = m.t;
            ++muestras;
        });
        QObject::connect(&gps, &FuenteGps::datos, [&](int s, const QDateTime& utc) { ++datosGps; satelites = s; CHEQUEAR(utc.isValid()); });
        QStringList frases, sonidos;
        QObject::connect(&crono, &Crono::frase, [&](const QString& f) { frases << f; });
        QObject::connect(&crono, &Crono::sonido, [&](const QString& s) { sonidos << s; });
        QString motivo;
        int vueltasAvisadas = -1;
        QObject::connect(&crono, &Crono::terminada, [&](const QString& m, int n, const QString&) { motivo = m; vueltasAvisadas = n; });

        // Las lecturas llegan enseguida (antes de que el GpsNeo termine de configurar el módulo)
        CHEQUEAR(esperar([&] { return muestras >= 5; }, 8000));
        CHEQUEAR(crono.gps()["ok"].toBool());
        CHEQUEAR(std::abs(crono.gps()["kmh"].toDouble() - 48) < 0.5);

        // Carrera: ideal 8 s y 107 m, todas medidas. Vueltas de 6, 8 y 10 s → −2 s, 0 s y +2 s
        const auto r = datos.guardarCarrera({{"nombre", "Simulada"}, {"fecha", QDate::currentDate().toString(Qt::ISODate)},
                                             {"distanciaM", 107}, {"tiempoIdealMs", 8000}, {"patron", "todas"}});
        const QString id = r["id"].toString();
        CHEQUEAR(!id.isEmpty());
        crono.preparar(id);
        CHEQUEAR(crono.listo());
        const double vueltasMs[] = {6000, 8000, 10000};
        std::vector<double> marcasT;   // cuándo se marcó de verdad (reloj de la carrera)
        auto marcar = [&] { marcasT.push_back(ahoraMs()); return crono.marcar("pulsador"); };
        CHEQUEAR(marcar() == "largada");
        CHEQUEAR(crono.enCurso());
        for (double v : vueltasMs) {
            const double hasta = marcasT.back() + v;
            esperar([&] { return ahoraMs() >= hasta; }, static_cast<int>(v) + 2000);
            CHEQUEAR(marcar() == "marca");
        }
        esperar([] { return false; }, 1000);   // la vuelta 4 recién empieza: no se guarda
        crono.terminar();
        CHEQUEAR(!crono.enCurso());
        CHEQUEAR(motivo == "Carrera terminada" && vueltasAvisadas == 3);
        datos.guardarYa();

        // Lecturas: 10 por segundo, sin huecos, con satélites y hora
        CHEQUEAR(muestras >= 250);
        CHEQUEAR(maxHueco < 400);
        CHEQUEAR(datosGps == muestras);
        CHEQUEAR(satelites == 11);
        // El módulo contestó la versión y aceptó la configuración (MON-VER, ACK-ACK)
        CHEQUEAR(crono.gps()["modulo"].toString().contains("NEO-M8N"));
        CHEQUEAR(crono.gps()["original"].toBool());

        // Las vueltas guardadas: tiempos contra el reloj (±150 ms) y distancias contra velocidad × tiempo (±3 %)
        const beto::Carrera* c = datos.buscar(id);
        CHEQUEAR(c && c->vueltas.size() == 3);
        if (c && c->vueltas.size() == 3) {
            for (size_t i = 0; i < 3; ++i) {
                const beto::Vuelta& v = c->vueltas[i];
                const double realMs = marcasT[i + 1] - marcasT[i];
                CHEQUEAR(v.n == static_cast<int>(i) + 1);
                CHEQUEAR(v.medida && !v.interrumpida);
                CHEQUEAR(std::abs(v.realMs - realMs) < 20);
                CHEQUEAR(std::abs(v.realMs - vueltasMs[i]) < 150);
                CHEQUEAR(v.difMs && std::abs(*v.difMs - (vueltasMs[i] - 8000)) < 150);
                const double esperadaM = 48 / 3.6 * realMs / 1000;
                CHEQUEAR(v.distM && std::abs(*v.distM - esperadaM) < esperadaM * 0.03);
                CHEQUEAR(v.velPromKmh && std::abs(*v.velPromKmh - 48) < 1.5);
                CHEQUEAR(v.velMaxKmh && std::abs(*v.velMaxKmh - 48) < 1);
                CHEQUEAR(v.fuente == "pulsador");
                CHEQUEAR(v.traza.size() >= vueltasMs[i] / 100 * 0.8 && v.traza.size() <= vueltasMs[i] / 100 * 1.2);   // 10 por segundo
            }
            CHEQUEAR(c->vueltas[0].traza.size() > 2 && c->vueltas[0].traza.back()[2] > 47 && c->vueltas[0].traza.back()[2] < 49);   // km/h
        }
        // Voz: anuncio de cada vuelta, clic en cada marca
        CHEQUEAR(frases.contains("Vuelta 1, medida") && frases.contains("Vuelta 3, medida"));
        CHEQUEAR(sonidos.count("clic") == 4);
        // Lo que ve Matías después: la ficha y el informe
        const QVariantMap ficha = datos.carrera(id);
        CHEQUEAR(ficha["vueltas"].toInt() == 3 && ficha["medidas"].toInt() == 3 && !ficha["errorTotal"].toString().isEmpty());
        const QVariantMap inf = datos.informe(id);
        CHEQUEAR(inf["vueltas"].toList().size() == 3);
        CHEQUEAR(!inf["pista"].toList().isEmpty());   // hay mapa: la traza tiene posiciones
        const QByteArray html = datos.informeHtml(id);
        CHEQUEAR(html.contains("<html") && html.contains("Simulada") && !html.contains("__DATOS__"));
        CHEQUEAR(datos.nombreInforme(id).contains("Simulada"));
        // La carrera quedó en la tarjeta (un archivo por carrera) y se vuelve a leer igual
        {
            Datos otra(dirCarrera.path());
            const beto::Carrera* c2 = otra.buscar(id);
            CHEQUEAR(c2 && c2->vueltas.size() == 3 && c2->vueltas[2].realMs == c->vueltas[2].realMs);
        }
        crono.salir();
        sim.kill();
        sim.waitForFinished(2000);
    }

    // ---------- 2) El módulo pierde la señal (--corte) sin callarse: fixType 0, pocos satélites ----------
    {
        const QString enlace = dir.path() + "/gps2";
        QProcess sim;
        // vuelta de 100 m a 48 km/h (7,5 s): los primeros 4 s de cada vuelta sin fix
        CHEQUEAR(arrancarSimulador(sim, simularNeo, {"--enlace", enlace, "--hz", "10", "--kmh", "48", "--fija", "--largo", "100", "--corte", "4"}));
        Datos datos(dirCorte.path());
        GpsNeo gps(enlace);
        Crono crono(&datos, &gps);
        int muestras = 0, datosGps = 0, pocos = 0;
        double ultimaT = 0, maxHueco = 0, ultimoDato = 0, maxHuecoDatos = 0;
        QObject::connect(&gps, &FuenteGps::muestra, [&](const beto::Muestra& m) {
            if (muestras > 0) maxHueco = std::max(maxHueco, m.t - ultimaT);
            ultimaT = m.t;
            ++muestras;
        });
        QObject::connect(&gps, &FuenteGps::datos, [&](int s, const QDateTime&) {
            const double t = ahoraMs();
            if (datosGps > 0) maxHuecoDatos = std::max(maxHuecoDatos, t - ultimoDato);
            ultimoDato = t;
            ++datosGps;
            if (s < 4) ++pocos;
        });
        CHEQUEAR(esperar([&] { return muestras >= 3; }, 8000));
        crono.preparar("");   // práctica
        CHEQUEAR(crono.marcar() == "largada");
        bool conGps = false, sinGps = false;
        esperar([&] { if (crono.estado()["sinGps"].toBool()) sinGps = true; else conGps = true; return false; }, 9000);
        crono.terminar();
        CHEQUEAR(datosGps >= 80);                  // nunca se calló: 10 lecturas por segundo todo el tiempo
        CHEQUEAR(maxHuecoDatos < 600);
        CHEQUEAR(pocos >= 30);                     // 4 s de cada 7,5 con 3 satélites
        CHEQUEAR(maxHueco > 3500 && maxHueco < 4600);   // y 4 s seguidos sin posición buena
        CHEQUEAR(conGps && sinGps);                // la carrera lo vio: «sin GPS» (a los 3 s sin lectura) y después volvió
        const beto::Carrera* c = datos.buscar(crono.carreraId());
        CHEQUEAR(c && c->vueltas.size() == 1 && c->vueltas[0].interrumpida);
        crono.salir();
        sim.kill();
        sim.waitForFinished(2000);
    }

    std::cout << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
