// Pruebas de las reglas de Carreras (Datos): orden, cuándo se larga, validaciones, borrar y guardado.
#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

#include "datos.h"

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c) do { ++chequeos; if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

static QVariantMap carreraNueva(const QString& nombre, QDate f) {
    return {{"nombre", nombre}, {"fecha", f.toString(Qt::ISODate)},
            {"distanciaM", 3000}, {"tiempoIdealMs", 180000}, {"patron", "alt1"}};
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    const QDate hoy = QDate::currentDate();
    {
        Datos d(dir.path());
        CHEQUEAR(d.proximas().isEmpty() && d.corridas().isEmpty());

        // Validaciones (mensajes de la app)
        auto m = carreraNueva("", hoy);
        CHEQUEAR(d.guardarCarrera(m)["error"].toString() == "Poné un nombre a la carrera.");
        m = carreraNueva("X", hoy); m["fecha"] = "";
        CHEQUEAR(d.guardarCarrera(m)["error"].toString() == "Elegí la fecha de la carrera.");
        m = carreraNueva("X", hoy); m["tiempoIdealMs"] = 4900;
        CHEQUEAR(d.guardarCarrera(m)["error"].toString() == "El tiempo ideal tiene que ser de al menos 5 segundos.");

        // Próximas por fecha ascendente; hoy se larga, otro día no
        const QString idLejos = d.guardarCarrera(carreraNueva("Lejos", hoy.addDays(20)))["id"].toString();
        const QString idHoy = d.guardarCarrera(carreraNueva("Hoy", hoy))["id"].toString();
        const QString idPasada = d.guardarCarrera(carreraNueva("Pasada", hoy.addDays(-3)))["id"].toString();
        CHEQUEAR(!idLejos.isEmpty() && !idHoy.isEmpty() && !idPasada.isEmpty());
        auto prox = d.proximas();
        CHEQUEAR(prox.size() == 2);
        CHEQUEAR(prox[0].toMap()["nombre"] == "Hoy" && prox[1].toMap()["nombre"] == "Lejos");
        CHEQUEAR(prox[0].toMap()["puedeLargar"].toBool());
        CHEQUEAR(prox[0].toMap()["estado"] == "HOY");
        CHEQUEAR(!prox[1].toMap()["puedeLargar"].toBool());
        CHEQUEAR(prox[1].toMap()["motivoNoLargar"].toString().startsWith("Se larga el"));
        CHEQUEAR(prox[1].toMap()["estado"] == "EN 20 DÍAS");
        auto corr = d.corridas();
        CHEQUEAR(corr.size() == 1 && corr[0].toMap()["estado"] == "SIN CORRER");
        CHEQUEAR(corr[0].toMap()["motivoNoLargar"] == "La fecha de la carrera ya pasó");

        // Sin hora del GPS se puede largar igual (decisión de la Raspberry)
        d.setHoraValida(false);
        CHEQUEAR(d.carrera(idLejos)["puedeLargar"].toBool());
        d.setHoraValida(true);

        // Nueva: toma los valores de la última práctica
        d.ponerCfg("tiempoIdealMs", 150000);
        CHEQUEAR(d.carrera("")["tiempoIdealMs"].toLongLong() == 150000);
        CHEQUEAR(d.carrera("")["nueva"].toBool());

        // Cambiar
        auto c = d.carrera(idHoy);
        c["nombre"] = "Hoy cambiada";
        c["tiempoIdealMs"] = 90000;
        CHEQUEAR(d.guardarCarrera(c)["ok"].toBool());
        CHEQUEAR(d.carrera(idHoy)["ideal"] == "1:30,0");
        CHEQUEAR(d.carrera(idHoy)["velocidad"] == "120,0");

        // Armado de widgets: una lista de objetos se guarda y vuelve igual
        QVariantList armado{QVariantMap{{"t", "marcar"}, {"x", 0.5}, {"y", 0.6}, {"w", 0.4}, {"h", 0.3}},
                            QVariantMap{{"t", "velocimetro"}, {"forma", "redondo"}, {"x", 0.1}, {"y", 0.1}, {"w", 0.3}, {"h", 0.2}}};
        d.ponerCfg("widgetsH", armado);
        const QVariantList vuelta = d.ajustes()["widgetsH"].toList();
        CHEQUEAR(vuelta.size() == 2);
        CHEQUEAR(vuelta[1].toMap()["forma"] == "redondo" && vuelta[0].toMap()["x"].toDouble() == 0.5);

        CHEQUEAR(d.borrarCarrera(idPasada));
        CHEQUEAR(d.corridas().isEmpty());
    }
    {
        // Se guardó en disco: al abrir de nuevo está todo
        Datos d(dir.path());
        CHEQUEAR(d.proximas().size() == 2);
        CHEQUEAR(d.cfg("tiempoIdealMs").toDouble() == 150000);
        CHEQUEAR(d.ajustes()["widgetsH"].toList().size() == 2);
    }
    {
        // Recorrido libre cortado (luz) antes de Terminar: lo guardado cada minuto vuelve como su vuelta
        QTemporaryDir rec;
        QString id;
        {
            Datos d(rec.path());
            id = d.crearRecorrido();
            beto::Vuelta v;
            v.n = 1; v.medida = false; v.realMs = 120000; v.distM = 1500; v.interrumpida = true; v.fuente = "corte";
            for (int i = 0; i < 12; ++i) v.traza.push_back({i * 10.0, i * 125.0, 45, -34.6 + i * 1e-4, -58.38, 4});
            CHEQUEAR(d.ponerExtra(id, "enCurso", v.aJson()));
        }
        {
            Datos d(rec.path());
            const beto::Carrera* c = d.buscar(id);
            CHEQUEAR(c && c->vueltas.size() == 1 && c->vueltas[0].interrumpida && c->vueltas[0].traza.size() == 12);
            CHEQUEAR(c && !c->extra.tiene("enCurso"));
        }
    }
    {
        // Borrar una carrera borra también su video, salvo que lo use otra carrera (Matías, 07/11)
        QTemporaryDir vd;
        Datos d(vd.path());
        QDir().mkpath(vd.path() + "/videos");
        auto archivo = [&](const QString& n) { QFile f(vd.path() + "/videos/" + n); f.open(QIODevice::WriteOnly); f.write("x"); };
        archivo("carrera-1.mp4"); archivo("carrera-2.mp4");
        const QString a = d.guardarCarrera(carreraNueva("A", QDate::currentDate()))["id"].toString();
        const QString b = d.guardarCarrera(carreraNueva("B", QDate::currentDate().addDays(1)))["id"].toString();
        d.agregarVideo(a, {{"archivo", "carrera-1.mp4"}, {"largada", 1}});
        d.agregarVideo(a, {{"archivo", "carrera-2.mp4"}, {"largada", 2}});
        d.agregarVideo(b, {{"archivo", "carrera-2.mp4"}, {"largada", 1}});
        CHEQUEAR(d.borrarCarrera(a));
        CHEQUEAR(!QFile::exists(vd.path() + "/videos/carrera-1.mp4"));   // solo era de A
        CHEQUEAR(QFile::exists(vd.path() + "/videos/carrera-2.mp4"));    // B lo usa todavía
    }
    {
        // Muestra: Santa Clara corrida, con error total 9,6 s y finalizada
        QTemporaryDir otro;
        Datos d(otro.path());
        d.cargarMuestra();
        bool hay = false;
        for (const auto& v : d.corridas()) {
            auto m = v.toMap();
            if (m["nombre"] == "Santa Clara") {
                hay = true;
                CHEQUEAR(m["errorTotal"] == "9,6 s");
                CHEQUEAR(m["finalizada"].toBool());
                CHEQUEAR(!m["puedeLargar"].toBool());
                CHEQUEAR(m["nivel"] == "mal");
            }
        }
        CHEQUEAR(hay);
        // Informe: datos con recorrido y HTML completo con la plantilla
        for (const auto& v : d.corridas()) {
            auto m = v.toMap();
            if (m["nombre"] != "Santa Clara") continue;
            const QString id = m["id"].toString();
            // Vuelta libre a mano: la V1 (+1,5 s) deja de sumar error; vuelve a medida y suma de nuevo
            CHEQUEAR(d.cambiarMedida(id, 0));
            CHEQUEAR(!d.buscar(id)->vueltas[0].medida);
            CHEQUEAR(d.carrera(id)["errorTotal"] == "8,1 s");
            CHEQUEAR(d.informe(id)["vueltas"].toList()[0].toMap()["medida"] == false);
            CHEQUEAR(d.cambiarMedida(id, 0));
            CHEQUEAR(d.carrera(id)["errorTotal"] == "9,6 s");
            CHEQUEAR(!d.cambiarMedida(id, 99));
            // Unir la V4 con la V5 (marca de más): quedan 5 vueltas
            CHEQUEAR(d.unirConSiguiente(id, 3));
            CHEQUEAR(d.buscar(id)->vueltas.size() == 5 && d.buscar(id)->vueltas[4].n == 5);
            CHEQUEAR(!d.unirConSiguiente(id, 4));
            const QVariantMap inf = d.informe(id);
            CHEQUEAR(!inf["pista"].toList().isEmpty());
            CHEQUEAR(inf["vueltas"].toList().size() == 5);
            CHEQUEAR(d.nombreInforme(id).endsWith(".html") && d.nombreInforme(id).contains("Santa Clara"));
            const QString ruta = d.guardarInformeHtml(id);
            QFile f(ruta);
            CHEQUEAR(f.open(QIODevice::ReadOnly));
            const QByteArray html = f.readAll();
            CHEQUEAR(html.contains("const C={\"nombre\":\"Santa Clara\""));
            CHEQUEAR(!html.contains("__DATOS__") && !html.contains("__NOMBRE__"));
            CHEQUEAR(html.contains("interrumpida"));   // la plantilla de la Raspberry
        }
    }
    std::cout << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
