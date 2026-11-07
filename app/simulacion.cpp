#include "simulacion.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QStorageInfo>
#include <QTimer>
#include <cmath>

#include "audio.h"
#include "crono.h"
#include "datos.h"
#include "informe.h"
#include "reportes.h"

namespace {
// GPS de mentira: lecturas a 10 Hz sobre una recta (posición y velocidad coherentes)
class GpsDeSimulacion : public FuenteGps {
public:
    double recorrido = 0;
    void dar(double t, double v) {
        recorrido += v * 0.1;
        beto::Muestra m;
        m.t = t; m.velMs = v; m.precisionM = 3;
        m.lat = -34.6 + recorrido / 111320.0; m.lon = -58.4;
        emit muestra(m);
    }
};
}  // namespace

Simulacion::Simulacion(const QString& carpetaDatos, Audio* audio, Reportes* reportes, QObject* padre)
    : QObject(padre), carpeta_(carpetaDatos + QStringLiteral("/simulador")), audio_(audio), reportes_(reportes) {}

int Simulacion::fallas() const {
    int n = 0;
    for (const auto& p : pruebas_) if (!p.toMap().value("ok").toBool()) ++n;
    return n;
}

void Simulacion::anotar(const QString& nombre, bool ok, bool grave, const QString& detalle) {
    pruebas_ << QVariantMap{{"nombre", nombre}, {"ok", ok}, {"grave", grave}, {"detalle", detalle}};
    if (!ok && reportes_) reportes_->anotar(QStringLiteral("simulador"), nombre + QStringLiteral(": ") + detalle);
}

bool Simulacion::mandar() {
    if (!reportes_ || !reportes_->hayLlave()) return false;
    reportes_->enviarAhora();
    return true;
}

void Simulacion::correr() {
    if (estado_ == QLatin1String("corriendo")) return;
    estado_ = QStringLiteral("corriendo");
    pruebas_.clear();
    emit cambio();
    QTimer::singleShot(50, this, &Simulacion::hacer);   // que la pantalla muestre «Probando…» primero
}

void Simulacion::hacer() {
    QElapsedTimer reloj;
    reloj.start();
    QDir(carpeta_).removeRecursively();   // en la misma tarjeta que las carreras: prueba también el guardado
    QDir().mkpath(carpeta_);
    {
        double ahora = 1000000;
        Datos datos(carpeta_);
        GpsDeSimulacion gps;
        Crono crono(&datos, &gps);
        crono.ponerReloj([&] { return ahora; });
        auto andar = [&](double ms, double v) { const double fin = ahora + ms; while (ahora < fin) { ahora += 100; gps.dar(ahora, v); } };
        datos.ponerCfg(QStringLiteral("tiempoIdealMs"), 60000);
        datos.ponerCfg(QStringLiteral("distanciaM"), 1000);
        datos.ponerCfg(QStringLiteral("patron"), QStringLiteral("todas"));
        const double v = 1000.0 / 60;   // 60 km/h justo: cada vuelta de 1000 m en 60 s

        // 1) Largada y 3 vueltas
        crono.preparar(QString());
        const QString r0 = crono.marcar(QStringLiteral("simulador"));
        anotar(QStringLiteral("Largada"), r0 == QLatin1String("largada"), true, QStringLiteral("al marcar dio «%1»").arg(r0));
        andar(1500, v);
        const QString rIgn = crono.marcar(QStringLiteral("simulador"));
        anotar(QStringLiteral("Marca doble ignorada"), rIgn == QLatin1String("ignorada"), true, QStringLiteral("la segunda marca dio «%1»").arg(rIgn));
        andar(30000 - 1500, v);
        crono.refrescarAhora();
        const QVariantMap e = crono.estado();
        const double sug = e.value("vNecKmh").toDouble(), delta = e.value("deltaS").toDouble();
        anotar(QStringLiteral("Velocidad sugerida"), e.value("vNecKmh").isValid() && std::fabs(sug - 60) < 3, true,
               QStringLiteral("a mitad de vuelta y en tiempo dio %1 km/h (tenía que dar 60)").arg(sug, 0, 'f', 1));
        anotar(QStringLiteral("Adelantado / atrasado"), e.value("deltaS").isValid() && std::fabs(delta) < 0.6, true,
               QStringLiteral("yendo justo dio %1 s").arg(delta, 0, 'f', 1));
        andar(30000, v);
        crono.marcar(QStringLiteral("simulador"));                  // V1: justo
        andar(62000, v * 60 / 62);                                    // V2: 2 s tarde
        crono.marcar(QStringLiteral("simulador"));
        andar(59000, v * 60 / 59);                                    // V3: 1 s antes
        crono.marcar(QStringLiteral("simulador"));
        const QString id = crono.carreraId();
        const beto::Carrera* c = datos.buscar(id);
        const bool tres = c && c->vueltas.size() == 3;
        anotar(QStringLiteral("Vueltas guardadas"), tres, true, QStringLiteral("quedaron %1 de 3").arg(c ? static_cast<int>(c->vueltas.size()) : 0));
        if (tres) {
            const auto d = [&](int i) { return c->vueltas[static_cast<size_t>(i)].difMs.value_or(99999); };
            const bool tiempos = std::llabs(d(0)) <= 150 && std::llabs(d(1) - 2000) <= 150 && std::llabs(d(2) + 1000) <= 150;
            anotar(QStringLiteral("Tiempos de las vueltas"), tiempos, true,
                   QStringLiteral("dieron %1 / %2 / %3 ms (tenían que ser 0 / +2000 / −1000)").arg(d(0)).arg(d(1)).arg(d(2)));
            const double dist = c->vueltas[0].distM.value_or(0);
            anotar(QStringLiteral("Distancia por GPS"), std::fabs(dist - 1000) < 30, true, QStringLiteral("la V1 midió %1 m (1000)").arg(dist, 0, 'f', 0));
            anotar(QStringLiteral("Recorrido grabado"), c->vueltas[0].traza.size() >= 50, true,
                   QStringLiteral("la V1 tiene %1 puntos").arg(c->vueltas[0].traza.size()));
            // Informe con el mismo cálculo que el de la carrera
            const beto::Json inf = beto::datosInforme(*c, 20);
            anotar(QStringLiteral("Informe"), inf.esObjeto() && inf["vueltas"].tamano() == 3, true, QStringLiteral("el informe no salió completo"));
        }
        crono.terminar();
        // 2) Guardado en la tarjeta: se cierra y se vuelve a abrir
        datos.guardarYa();
        anotar(QStringLiteral("Guardar en la tarjeta"), datos.guardaBien(), true, QStringLiteral("no se pudo escribir la base"));
        {
            Datos otra(carpeta_);
            const beto::Carrera* g = otra.buscar(id);
            anotar(QStringLiteral("Leer lo guardado"), g && g->vueltas.size() >= 3, true, QStringLiteral("al abrir de nuevo faltaban vueltas"));
        }
        // 3) Recorrido libre: las marcas no cortan nada
        crono.salir();
        crono.preparar(QStringLiteral("recorrido"));
        crono.marcar(QStringLiteral("simulador"));
        andar(10000, v);
        const QString m1 = crono.marcar(QStringLiteral("simulador"));
        andar(5000, v);
        anotar(QStringLiteral("Recorrido libre"), m1 == QLatin1String("marca") && crono.enCurso(), true, QStringLiteral("la marca dio «%1»").arg(m1));
        crono.terminar();
    }
    QDir(carpeta_).removeRecursively();
    // Leves: avisan, pero se puede correr
    anotar(QStringLiteral("Voz"), audio_ && audio_->lista(), false, QStringLiteral("faltan los audios de la voz: la carrera va sin cuenta regresiva"));
    const QStorageInfo lugar(QFileInfo(carpeta_).absolutePath());
    anotar(QStringLiteral("Lugar en la tarjeta"), !lugar.isValid() || lugar.bytesAvailable() > 1024LL * 1024 * 1024, false,
           QStringLiteral("quedan %1 MB").arg(lugar.bytesAvailable() / (1024 * 1024)));
    const qint64 ms = reloj.elapsed();
    anotar(QStringLiteral("Velocidad del equipo"), ms < 8000, false, QStringLiteral("la simulación tardó %1 s (lo normal es menos de 8)").arg(ms / 1000.0, 0, 'f', 1));

    bool grave = false, leve = false;
    for (const auto& p : pruebas_) {
        const QVariantMap m = p.toMap();
        if (!m.value("ok").toBool()) (m.value("grave").toBool() ? grave : leve) = true;
    }
    estado_ = grave ? QStringLiteral("grave") : leve ? QStringLiteral("leve") : QStringLiteral("ok");
    cuando_ = QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy HH:mm"));
    if (estado_ != QLatin1String("ok") && reportes_)
        reportes_->anotar(QStringLiteral("simulador"), QStringLiteral("Simulador: %1 de %2 pruebas fallaron").arg(fallas()).arg(total()));
    emit cambio();
}
