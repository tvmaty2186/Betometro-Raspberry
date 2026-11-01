// Prueba de una carrera entera en el crono, con reloj y GPS de mentira:
// práctica que se guarda al largar, vueltas guardadas, antirrebote, carrera completa y vuelta interrumpida.
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>

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

    // 2) Carrera de 2 vueltas: al cerrar la segunda termina sola
    const QDate hoy = QDate::currentDate();
    auto r = datos.guardarCarrera({{"nombre", "Prueba"}, {"fecha", hoy.toString(Qt::ISODate)}, {"vueltasTotal", 2},
                                   {"distanciaM", 1000}, {"tiempoIdealMs", 60000}, {"patron", "alt1"}});
    const QString id = r["id"].toString();
    crono.preparar(id);
    CHEQUEAR(crono.marcar("pulsador") == "largada");
    andar(ahora + 59000, 1000.0 / 60);
    CHEQUEAR(crono.marcar("pulsador") == "marca");
    andar(ahora + 60500, 1000.0 / 60);
    CHEQUEAR(crono.marcar("pulsador") == "marca");
    CHEQUEAR(!crono.enCurso());
    CHEQUEAR(motivo == "Carrera completa" && vueltas == 2);
    const beto::Carrera* c = datos.buscar(id);
    CHEQUEAR(c && c->vueltas.size() == 2);
    CHEQUEAR(c && c->vueltas[0].medida && !c->vueltas[1].medida);   // alt1
    CHEQUEAR(c && c->vueltas[0].fuente == "pulsador");
    CHEQUEAR(datos.carrera(id)["finalizada"].toBool());
    // Voz: anuncio de cada vuelta, «Última vuelta» y «Carrera completa»; clic en cada marca aceptada
    CHEQUEAR(frases.contains("Vuelta 1, medida"));
    CHEQUEAR(frases.contains("Última vuelta, práctica"));
    CHEQUEAR(frases.last() == "Carrera completa");
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

    std::cout << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
