// Las carreras de ejemplo (--muestra): para las capturas y las pruebas. Sigue en el programa: lo usan las capturas.
#include "datos.h"
#include "datos_comun.h"
#include "motor.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>

#include <cmath>
#include <vector>

using namespace beto;

void Datos::cargarMuestra() {
    const Json config = base_.config;   // las carreras de ejemplo no pisan la configuración (armados de prueba)
    base_ = Base{};
    base_.config = config;
    const QDate h = hoy();
    auto agregar = [&](const char* nombre, QDate f, int vueltas, int64_t T, int64_t D, const char* patron,
                       std::vector<int> errores, bool practica = false) {
        Carrera c;
        c.id = nuevaId(1730000000000 + static_cast<int64_t>(base_.carreras.size()), 7);
        c.nombre = nombre;
        c.fecha = s(f.toString(Qt::ISODate));
        (void)vueltas;
        c.tiempoIdealMs = T;
        c.distanciaM = D;
        c.patron = patron;
        c.practica = practica;
        c.creada = s(QDateTime(f, QTime(10, 0)).toString(Qt::ISODate));
        int n = 1;
        for (int e : errores) {
            Vuelta v;
            v.n = n; v.medida = esMedida(c.patron, n); v.idealMs = T; v.realMs = T + e; v.difMs = e;
            // recorrido de ejemplo: un circuito con curvas, a 1 lectura por segundo
            const double L = static_cast<double>(D), seg = (T + e) / 1000.0;
            double d = 0, vmax = 0;
            for (int t = 0; t <= static_cast<int>(seg); ++t) {
                const double fr = d / L, a = 2 * M_PI * fr;
                const double kmh = L / seg * 3.6 * (1 + 0.28 * std::sin(a * 3 + n) - 0.1 * std::cos(a * 5));
                const double R = L / (2 * M_PI), x = R * 1.5 * std::cos(a) + R * 0.25 * std::cos(3 * a), y = R * 0.8 * std::sin(a) + R * 0.15 * std::sin(2 * a);
                FilaTraza p{static_cast<double>(t), redondearJs(d * 10) / 10, fijarDecimalesJs(kmh, 1),
                            fijarDecimalesJs(-32.97 + y / 110540, 6), fijarDecimalesJs(-60.65 + x / (111320 * std::cos(32.97 * M_PI / 180)), 6),
                            static_cast<double>(2 + (t * 7 + n) % 4)};
                v.traza.push_back(p);
                vmax = std::max(vmax, kmh);
                d += kmh / 3.6;
            }
            v.distM = redondearJs(d);
            v.velPromKmh = fijarDecimalesJs(d / seg * 3.6, 1);
            v.velMaxKmh = fijarDecimalesJs(vmax, 1);
            c.vueltas.push_back(v);
            ++n;
        }
        base_.carreras.push_back(c);
    };
    agregar("Gualeguaychú", h.addDays(14), 10, 240000, 3200, "alt1", {});
    agregar("Concepción del Uruguay", h.addDays(28), 8, 210000, 2800, "todas", {});
    // Localidades de muestra (para el clima)
    auto lugar = [&](size_t i, const char* l, double la, double lo) {
        Carrera& c = base_.carreras[base_.carreras.size() - 2 + i];
        c.lugar = l; c.extra["lugarLat"] = Json(la); c.extra["lugarLon"] = Json(lo);
    };
    lugar(0, "Gualeguaychú, Entre Ríos", -33.009, -58.517);
    lugar(1, "Concepción del Uruguay, Entre Ríos", -32.484, -58.232);
    agregar("Santa Clara", h.addDays(-1), 6, 180000, 2500, "todas", {1500, -2100, 800, 1900, -1700, 1600});
    // Para probar el reproductor del informe: si hay un videos/muestra.mkv en la carpeta, va con Santa Clara
    if (QFile::exists(carpeta_ + QStringLiteral("/videos/muestra.mkv"))) {
        Json v = Json::objeto();
        v["archivo"] = std::string("muestra.mkv");
        v["largada"] = static_cast<int64_t>(1);
        v["desdeLargadaMs"] = static_cast<int64_t>(5000);
        v["girar"] = false;
        base_.carreras.back().extra["videos"] = Json::lista();
        base_.carreras.back().extra["videos"].agregar(std::move(v));
    }
    agregar("Paraná", h.addDays(-14), 8, 200000, 2600, "todas", {1200, -2500, 1800, -900, 2200, -1100, 1300, 1100});
    // Video de ejemplo para Santa Clara, solo si está el archivo (para probar el reproductor del informe)
    if (QFileInfo::exists(carpetaVideos() + QStringLiteral("/muestra-santa-clara.mkv"))) {
        Json v = Json::objeto();
        v["archivo"] = "muestra-santa-clara.mkv"; v["largada"] = 1; v["desdeLargadaMs"] = 5000; v["girar"] = false;
        Json l = Json::lista();
        l.agregar(std::move(v));
        base_.carreras[base_.carreras.size() - 2].extra["videos"] = l;
    }
    // Solo para probar pantallas con muchas vueltas (BETO_MUESTRA_LARGA=1): una carrera de 60 vueltas
    // y una práctica de 40, cortitas para que el video de muestra alcance a todas
    if (qEnvironmentVariableIntValue("BETO_MUESTRA_LARGA") == 1) {
        std::vector<int> e60, e40;
        for (int i = 0; i < 60; ++i) e60.push_back(((i * 37) % 23 - 11) * 130);
        for (int i = 0; i < 40; ++i) e40.push_back(((i * 53) % 19 - 9) * 210);
        const bool hayVideo = QFile::exists(carpeta_ + QStringLiteral("/videos/muestra.mkv"));
        auto conVideo = [&](int64_t desde) {
            if (!hayVideo) return;
            Json v = Json::objeto();
            v["archivo"] = std::string("muestra.mkv");
            v["largada"] = static_cast<int64_t>(1);
            v["desdeLargadaMs"] = desde;
            v["girar"] = false;
            base_.carreras.back().extra["videos"] = Json::lista();
            base_.carreras.back().extra["videos"].agregar(std::move(v));
        };
        agregar("Rosario Nocturno", h.addDays(-3), 60, 17000, 400, "alt1", e60);
        conVideo(3000);
        agregar("Práctica larga", h.addDays(-2), 40, 25000, 600, "todas", e40, true);
        conVideo(3590000);   // el video empezó una hora antes: horas en «▸ 1:00:10»
    }
    cfg_ = armarConfig(base_.config);
    emit carrerasChanged();
}
