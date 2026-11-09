// Pruebas del clima sin internet (app/clima.*): leer un pronóstico con el formato exacto de la respuesta
// de api.open-meteo.com/v1/forecast (open-meteo.json, al lado de esta prueba: en la PC el programa inventa
// uno y nunca leía el real, 11A-3), la clave redondeada a 0,05° y el tope de 20 lugares guardados (11A-2.1).
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <iostream>

#include "clima.h"

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c) do { ++chequeos; if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    qunsetenv("BETO_RASPBERRY");   // en la PC: no sale a internet

    // ---- Clave: 0,05° (≈5 km), así viajando no se pide un pronóstico por kilómetro
    CHEQUEAR(Clima::clave(-37.836, -57.506) == "-37.85,-57.50");
    CHEQUEAR(Clima::clave(-37.84, -57.51) == Clima::clave(-37.836, -57.506));   // a 1 km: el mismo lugar
    CHEQUEAR(Clima::clave(-37.80, -57.506) != Clima::clave(-37.836, -57.506));  // a 4 km: otro (redondea distinto)
    CHEQUEAR(Clima::clave(-33.009, -58.517) == "-33.00,-58.50");

    // ---- Un JSON real de Open-Meteo guardado en la caché se lee bien
    QFile fj(QStringLiteral(BETO_PRUEBA_DIR "/open-meteo.json"));
    CHEQUEAR(fj.open(QIODevice::ReadOnly));
    const QJsonObject real = QJsonDocument::fromJson(fj.readAll()).object();
    CHEQUEAR(real.contains("daily") && real.contains("hourly") && real.contains("current"));
    const qint64 ahora = QDateTime::currentMSecsSinceEpoch();
    {
        QTemporaryDir dir;
        QJsonObject cache{{Clima::clave(-37.836, -57.506), QJsonObject{{"datos", real}, {"cuando", static_cast<double>(ahora)}}}};
        QFile f(dir.path() + "/clima.json");
        CHEQUEAR(f.open(QIODevice::WriteOnly) && f.write(QJsonDocument(cache).toJson()) > 0);
        f.close();
        Clima c(dir.path());
        CHEQUEAR(c.lugaresEnCache() == 1);
        const QVariantMap p = c.pronostico(-37.836, -57.506);   // está fresco: no lo inventa
        CHEQUEAR(p["ok"].toBool() && !p["cargando"].toBool() && p["error"].toString().isEmpty() && !p["viejo"].toBool());
        const QVariantMap a = p["actual"].toMap();
        CHEQUEAR(a["temp"].toInt() == 18 && a["codigo"].toInt() == 3 && a["viento"].toInt() == 22 && a["rafagas"].toInt() == 39);
        CHEQUEAR(a["lluvia"].toInt() == 35);   // la probabilidad de la hora actual (14 h) sale de «hourly»
        const QVariantList horas = p["horas"].toList();
        CHEQUEAR(horas.size() == 12 && horas[0].toMap()["hora"].toInt() == 14 && horas[1].toMap()["hora"].toInt() == 16);
        CHEQUEAR(horas[0].toMap()["temp"].toInt() == 20);   // 19,8 °C a las 14
        CHEQUEAR(horas[11].toMap()["hora"].toInt() == 12);  // 12 lecturas cada 2 h: un día entero (pasa al día siguiente)
        CHEQUEAR(p["dias"].toList().size() == 2);
        const QVariantList h2 = c.horasDe(-37.836, -57.506, "2026-11-08");
        CHEQUEAR(h2.size() == 12 && h2[0].toMap()["hora"].toInt() == 0 && h2[9].toMap()["hora"].toInt() == 18);
        CHEQUEAR(h2[9].toMap()["codigo"].toInt() == 61 && h2[9].toMap()["lluvia"].toInt() == (18 * 3 + 7) % 100);
        CHEQUEAR(c.horasDe(-37.836, -57.506, "2026-11-09").isEmpty());
        // Hora por hora (doble toque en un día): las 24, con lluvia en mm y viento
        const QVariantList h24 = c.horasDelDia(-37.836, -57.506, "2026-11-08");
        CHEQUEAR(h24.size() == 24 && h24[18].toMap()["hora"].toInt() == 18 && h24[18].toMap()["codigo"].toInt() == 61);
        CHEQUEAR(h24[18].toMap().contains("mm") && h24[18].toMap().contains("rafagas"));
    }

    // ---- Tope: con 25 lugares guardados quedan los 20 más nuevos
    {
        QTemporaryDir dir;
        QJsonObject cache;
        for (int i = 0; i < 25; ++i)
            cache.insert(Clima::clave(-30 - i, -60), QJsonObject{{"datos", real}, {"cuando", static_cast<double>(ahora - i * 60000)}});
        QFile f(dir.path() + "/clima.json");
        CHEQUEAR(f.open(QIODevice::WriteOnly) && f.write(QJsonDocument(cache).toJson()) > 0);
        f.close();
        Clima c(dir.path());
        CHEQUEAR(c.lugaresEnCache() == Clima::kLugares);
        CHEQUEAR(c.pronostico(-30, -60)["ok"].toBool());        // el más nuevo quedó
        CHEQUEAR(c.pronostico(-49, -60)["ok"].toBool());        // el número 20 también
        // Un lugar nuevo entra y sale el más viejo (en la PC se inventa, pero cuenta igual)
        c.pedir(-10, -60);
        CHEQUEAR(c.lugaresEnCache() == Clima::kLugares);
    }

    std::cerr << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
