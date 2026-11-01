// Pruebas de las reglas de Carreras (Datos): orden, cuándo se larga, validaciones, borrar y guardado.
#include <QCoreApplication>
#include <QDate>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

#include "datos.h"

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c) do { ++chequeos; if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

static QVariantMap carreraNueva(const QString& nombre, QDate f, int vueltas = 5) {
    return {{"nombre", nombre}, {"fecha", f.toString(Qt::ISODate)}, {"vueltasTotal", vueltas},
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
        m = carreraNueva("X", hoy, 0);
        CHEQUEAR(d.guardarCarrera(m)["error"].toString() == "Poné la cantidad de vueltas de la carrera.");
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
            const QVariantMap inf = d.informe(id);
            CHEQUEAR(!inf["pista"].toList().isEmpty());
            CHEQUEAR(inf["vueltas"].toList().size() == 6);
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
