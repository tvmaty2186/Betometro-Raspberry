// Prueba de una carrera entera en el crono, con reloj y GPS de mentira:
// práctica que se guarda al largar, vueltas guardadas, antirrebote, carrera sin límite de vueltas y vuelta interrumpida.
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>

#include "camara.h"
#include "crono.h"

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c) do { ++chequeos; if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

class GpsDePrueba : public FuenteGps {
public:
    void dar(double t, double v) { beto::Muestra m; m.t = t; m.velMs = v; m.precisionM = 3; m.lat = -34.6; m.lon = -58.4; emit muestra(m); }
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    Datos datos(dir.path());
    GpsDePrueba gps;
    Crono crono(&datos, &gps);
    double ahora = 10000;
    crono.ponerReloj([&] { return ahora; });
    // avanzar el tiempo con GPS a 10 Hz y velocidad constante
    auto andar = [&](double hasta, double v) { while (ahora < hasta) { ahora += 100; gps.dar(ahora, v); } };

    QStringList sonidos, frases;
    QObject::connect(&crono, &Crono::sonido, [&](const QString& s) { sonidos << s; });
    QObject::connect(&crono, &Crono::frase, [&](const QString& f) { frases << f; });

    // 1) Práctica: no existe hasta largar
    datos.ponerCfg("tiempoIdealMs", 60000);
    datos.ponerCfg("distanciaM", 1000);
    crono.preparar("");
    CHEQUEAR(crono.listo() && !crono.enCurso());
    CHEQUEAR(datos.corridas().isEmpty());
    CHEQUEAR(crono.marcar() == "largada");
    CHEQUEAR(crono.enCurso());
    CHEQUEAR(datos.corridas().size() == 1);
    const QString idPractica = crono.carreraId();
    CHEQUEAR(datos.carrera(idPractica)["practica"].toBool());
    andar(ahora + 1000, 1000.0 / 60);
    CHEQUEAR(crono.marcar() == "ignorada");          // antes del antirrebote (3 s)
    andar(10000 + 61000, 1000.0 / 60);
    CHEQUEAR(crono.marcar() == "marca");
    CHEQUEAR(crono.ultima()["n"].toInt() == 1);
    CHEQUEAR(qAbs(crono.ultima()["difMs"].toDouble() - 1000) < 1);
    const beto::Carrera* p = datos.buscar(idPractica);
    CHEQUEAR(p && p->vueltas.size() == 1);
    CHEQUEAR(p && p->vueltas[0].distM.has_value() && qAbs(*p->vueltas[0].distM - 1017) <= 2);
    CHEQUEAR(p && p->vueltas[0].traza.size() >= 5);
    // Terminar a mano a mitad de la vuelta 2: queda interrumpida
    QString motivo;
    int vueltas = -1;
    QObject::connect(&crono, &Crono::terminada, [&](const QString& m, int n, const QString&) { motivo = m; vueltas = n; });
    andar(ahora + 30000, 1000.0 / 60);
    crono.terminar();
    CHEQUEAR(!crono.enCurso());
    CHEQUEAR(motivo == "Carrera terminada" && vueltas == 1);
    p = datos.buscar(idPractica);
    CHEQUEAR(p && p->vueltas.size() == 2 && p->vueltas[1].interrumpida && !p->vueltas[1].difMs);

    // 2) Carrera sin límite de vueltas: después de 2 vueltas sigue en curso; se termina a mano
    const QDate hoy = QDate::currentDate();
    auto r = datos.guardarCarrera({{"nombre", "Prueba"}, {"fecha", hoy.toString(Qt::ISODate)},
                                   {"distanciaM", 1000}, {"tiempoIdealMs", 60000}, {"patron", "alt1"}});
    const QString id = r["id"].toString();
    crono.preparar(id);
    CHEQUEAR(crono.marcar("pulsador") == "largada");
    andar(ahora + 59000, 1000.0 / 60);
    CHEQUEAR(crono.marcar("pulsador") == "marca");
    andar(ahora + 60500, 1000.0 / 60);
    CHEQUEAR(crono.marcar("pulsador") == "marca");
    CHEQUEAR(crono.enCurso());   // pueden agregar vueltas: no termina sola
    crono.terminar();
    CHEQUEAR(!crono.enCurso());
    CHEQUEAR(motivo == "Carrera terminada" && vueltas == 2);
    const beto::Carrera* c = datos.buscar(id);
    CHEQUEAR(c && c->vueltas.size() == 2);   // la 3 recién empezada no se guarda (como la app)
    CHEQUEAR(c && c->vueltas[0].medida && !c->vueltas[1].medida);   // alt1
    CHEQUEAR(c && c->vueltas[0].fuente == "pulsador");
    CHEQUEAR(!datos.carrera(id)["finalizada"].toBool());   // es hoy: se puede volver a largar
    CHEQUEAR(datos.carrera(id)["puedeLargar"].toBool());
    // Voz: anuncio de cada vuelta, sin «Última vuelta»; clic en cada marca aceptada
    CHEQUEAR(frases.contains("Vuelta 1, medida"));
    CHEQUEAR(frases.contains("Vuelta 2, libre"));
    CHEQUEAR(!frases.contains("Última vuelta, libre"));
    CHEQUEAR(sonidos.count("clic") == 5);   // práctica: largada y 1 marca (la ignorada no suena); carrera: largada y 2 marcas

    // 3) Sin GPS: anda igual (solo cronómetro)
    QTemporaryDir dir2;
    Datos datos2(dir2.path());
    FuenteGps nada;
    Crono crono2(&datos2, &nada);
    crono2.ponerReloj([&] { return ahora; });
    crono2.preparar("");
    crono2.marcar();
    ahora += 245000;   // práctica de fábrica: 4:00
    crono2.refrescarAhora();
    CHEQUEAR(crono2.estado()["sinGps"].toBool());
    CHEQUEAR(crono2.estado()["restanteMs"].toDouble() < 0);
    CHEQUEAR(crono2.marcar() == "marca");
    const beto::Carrera* s = datos2.buscar(crono2.carreraId());
    CHEQUEAR(s && s->vueltas.size() == 1 && !s->vueltas[0].distM.has_value());

    // 3b) Recorrido libre (1.0.26): sin vueltas; MARCAR deja marcas numeradas y no corta nada; traza a 1 por
    //     segundo; cada minuto guarda lo andado (por si se corta la luz); Terminar deja una sola vuelta, no
    //     interrumpida, y sin frases de voz (09-2.d, 05-1.1)
    {
        QTemporaryDir dirR;
        Datos datosR(dirR.path());
        GpsDePrueba gpsR;
        Crono cR(&datosR, &gpsR);
        cR.ponerReloj([&] { return ahora; });
        QStringList frasesR;
        QObject::connect(&cR, &Crono::frase, [&](const QString& f) { frasesR << f; });
        auto andarR = [&](double hasta, double v) { while (ahora < hasta) { ahora += 100; gpsR.dar(ahora, v); } };
        cR.preparar("recorrido");
        CHEQUEAR(datosR.corridas().isEmpty());            // no existe hasta largar
        CHEQUEAR(cR.marcar() == "largada");
        const QString idR = cR.carreraId();
        CHEQUEAR(!idR.isEmpty());
        andarR(ahora + 20000, 12.5);                       // 45 km/h
        CHEQUEAR(cR.marcar() == "marca");
        CHEQUEAR(cR.marcar() == "ignorada");              // dos marcas en menos de 1 s
        andarR(ahora + 50000, 12.5);
        CHEQUEAR(cR.marcar() == "marca");
        andarR(ahora + 15000, 12.5);                       // pasa el minuto: se guardó lo andado
        const beto::Carrera* r = datosR.buscar(idR);
        CHEQUEAR(r && r->extra.tiene("enCurso") && r->vueltas.empty());
        CHEQUEAR(r && r->extra["marcas"].tamano() == 2);
        CHEQUEAR(r && std::abs(r->extra["marcas"].elementos()[0]["m"].aNumero() - 250) < 15);
        cR.terminar();
        r = datosR.buscar(idR);
        CHEQUEAR(r && r->vueltas.size() == 1 && !r->vueltas[0].interrumpida && !r->extra.tiene("enCurso"));
        CHEQUEAR(r && r->vueltas[0].traza.size() >= 80 && r->vueltas[0].traza.size() <= 90);   // ~85 s a 1 por segundo
        CHEQUEAR(frasesR.isEmpty());
    }

    // 4) Video (si está el ayudante betometro-camara y GStreamer): cámara de prueba, empieza al abrir
    //    la carrera, se anota en qué momento del video fue la largada y para unos segundos después.
    if (QFile::exists(QCoreApplication::applicationDirPath() + "/betometro-camara")) {
        qputenv("BETO_CAMARA_PRUEBA", "1");
        QTemporaryDir dir3;
        Datos datos3(dir3.path());
        datos3.ponerCfg("camara", true);
        datos3.ponerCfg("camaraFinS", 1);
        datos3.ponerCfg("camaraSonido", "no");
        Camara cam(&datos3, dir3.path());
        FuenteGps nada3;
        Crono crono3(&datos3, &nada3);
        crono3.ponerCamara(&cam);
        auto esperar = [&](const std::function<bool()>& listo, int ms) {
            QElapsedTimer t; t.start();
            while (!listo() && t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            return listo();
        };
        crono3.preparar("");
        CHEQUEAR(cam.grabando());
        CHEQUEAR(esperar([&] { return cam.primerCuadro() > 0; }, 8000));
        esperar([] { return false; }, 700);
        CHEQUEAR(crono3.marcar() == "largada");
        const auto v = datos3.videos(crono3.carreraId());
        CHEQUEAR(v.size() == 1);
        const double desde = v.isEmpty() ? 0 : v[0].toMap()["desdeLargadaMs"].toDouble();
        CHEQUEAR(desde >= 500 && desde < 3000);   // la largada fue unos 0,7 s después del primer cuadro
        crono3.terminar();
        CHEQUEAR(cam.grabando());                 // sigue 1 s más
        CHEQUEAR(esperar([&] { return !cam.grabando(); }, 8000));
        CHEQUEAR(cam.falla().isEmpty());
        const QString archivo = cam.archivo();
        CHEQUEAR(QFile(archivo).size() > 10000);
        CHEQUEAR(!v.isEmpty() && archivo.endsWith(v[0].toMap()["archivo"].toString()));
    } else {
        std::cout << "(sin betometro-camara: no se prueba el video)\n";
    }

    std::cout << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
