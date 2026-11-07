// Prueba del wifi (app/wifi.cpp) con un nmcli de mentira (pruebas/dobles/nmcli) primero en el PATH (09 P4):
// en casa se conecta a la red guardada; lejos, a los 90 s arma la red propia; con «Conectarse a redes wifi»
// apagado la arma enseguida; si falta dnsmasq lo dice; conectar con clave mala y buena; olvidar.
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <functional>
#include <iostream>

#include "wifi.h"

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c) do { ++chequeos; if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

static bool esperar(const std::function<bool()>& listo, int ms) {
    QElapsedTimer t;
    t.start();
    while (!listo() && t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return listo();
}

static QString registro(const QString& archivo) {
    QFile f(archivo);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

// Un escenario nuevo: estado limpio y registro vacío
struct Escenario {
    QTemporaryDir dir;
    QString reg;
    Escenario(const char* nombre) : reg(dir.path() + "/registro") {
        qputenv("NMCLI_FALSO_ESTADO", (dir.path() + "/estado").toUtf8());
        qputenv("NMCLI_FALSO_ESCENARIO", nombre);
        qputenv("NMCLI_FALSO_REGISTRO", reg.toUtf8());
    }
    QString llamadas() const { return registro(reg); }
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    qputenv("BETO_RASPBERRY", "1");   // el wifi de verdad (nmcli), no el simulado de la PC
    qputenv("PATH", QByteArray(BETO_DOBLES) + ":" + qgetenv("PATH"));
    CHEQUEAR(QFile::exists(QStringLiteral(BETO_DOBLES "/nmcli")));

    // campos(): los «\:» de nmcli -t
    CHEQUEAR(Wifi::campos("yes:80:Casa") == QStringList({"yes", "80", "Casa"}));
    CHEQUEAR(Wifi::campos("no:55:Red\\:con\\:puntos") == QStringList({"no", "55", "Red:con:puntos"}));

    // 1) En casa: red guardada y conectada; se ven las redes de los vecinos
    {
        Escenario e("casa");
        Wifi w;
        QStringList conectadas;
        QObject::connect(&w, &Wifi::conectado, [&](const QString& r) { conectadas << r; });
        CHEQUEAR(esperar([&] { return w.conectada() == "Casa" && w.guardadas().size() == 2 && !w.ocupado(); }, 5000));
        CHEQUEAR(conectadas == QStringList{"Casa"});
        CHEQUEAR(w.senal() == 4 && w.horaInternet() && !w.propia());
        CHEQUEAR(w.guardadas()[0].toMap()["nombre"] == "Casa" && w.guardadas()[0].toMap()["conectada"].toBool());
        CHEQUEAR(w.guardadas()[1].toMap()["nombre"] == "Celu de Matías" && !w.guardadas()[1].toMap()["cerca"].toBool());
        // A los 3 s busca redes: las cercanas sin las guardadas, de la más fuerte a la más débil
        CHEQUEAR(esperar([&] { return w.cerca().size() == 2 && !w.buscando(); }, 6000));
        CHEQUEAR(w.cerca()[0].toMap()["nombre"] == "Vecino_2.4G" && w.cerca()[0].toMap()["senal"].toInt() == 3);
        CHEQUEAR(w.cerca()[1].toMap()["nombre"] == "Invitados" && !w.cerca()[1].toMap()["clave"].toBool());
        CHEQUEAR(w.tieneClave("Vecino_2.4G") && !w.tieneClave("Invitados"));
        CHEQUEAR(e.llamadas().contains("-t -f SSID,SIGNAL,SECURITY dev wifi list --rescan yes"));
        // Conectar: clave corta, clave mala, clave buena
        w.conectar("Vecino_2.4G", "corta");
        CHEQUEAR(w.error().contains("8 letras") && !w.conectando());
        w.conectar("Vecino_2.4G", "incorrecta");
        CHEQUEAR(w.conectando());
        CHEQUEAR(esperar([&] { return !w.conectando(); }, 5000));
        CHEQUEAR(w.error() == "La clave no es correcta" && w.conectada() == "Casa");
        w.conectar("Vecino_2.4G", "avispon101");
        CHEQUEAR(esperar([&] { return w.conectada() == "Vecino_2.4G" && !w.ocupado(); }, 5000));
        CHEQUEAR(w.error().isEmpty() && conectadas.last() == "Vecino_2.4G");
        CHEQUEAR(e.llamadas().contains("dev wifi connect Vecino_2.4G password avispon101"));
        CHEQUEAR(esperar([&] { return w.guardadas().size() == 3; }, 5000));   // quedó guardada
        // Guardada: se conecta sin clave (connection up)
        w.olvidar("Vecino_2.4G");
        CHEQUEAR(esperar([&] { return w.guardadas().size() == 2 && w.conectada().isEmpty() && !w.ocupado(); }, 5000));
        CHEQUEAR(e.llamadas().contains("connection delete Vecino_2.4G"));
        w.conectar("Casa", "");
        CHEQUEAR(esperar([&] { return w.conectada() == "Casa" && !w.ocupado(); }, 5000));
        CHEQUEAR(e.llamadas().contains("connection up preconfigured"));
        // «Conectarse a redes wifi» apagado: corta y arma la propia enseguida, con el número del auto
        w.ponerPropia(true, "101");
        w.ponerRedes(false);
        CHEQUEAR(esperar([&] { return w.propia() && !w.armandoPropia() && w.conectadosPropia() == 1; }, 8000));
        CHEQUEAR(w.conectada().isEmpty() && w.nombrePropia() == "Betometro-101" && w.errorPropia().isEmpty());
        const QString ll = e.llamadas();
        CHEQUEAR(ll.contains("dev disconnect wlan0"));
        CHEQUEAR(ll.contains("dev wifi hotspot ifname wlan0 con-name betometro-propia ssid Betometro-101 band bg password betometro"));
        CHEQUEAR(ll.contains("iw dev wlan0 station dump"));
        w.conectar("Casa", "");
        CHEQUEAR(w.error().contains("Conectarse a redes wifi"));   // con las redes apagadas no se conecta
        // Prendido de nuevo: apaga la propia
        w.ponerRedes(true);
        CHEQUEAR(esperar([&] { return !w.propia() && !w.ocupado(); }, 5000));
        CHEQUEAR(e.llamadas().contains("connection down betometro-propia"));
        // Vuelta a apagar: la propia ya está guardada con el mismo nombre y clave, solo se prende (11B-2.2)
        const int antes = e.llamadas().count("dev wifi hotspot");
        w.ponerRedes(false);
        CHEQUEAR(esperar([&] { return w.propia() && !w.ocupado(); }, 8000));
        CHEQUEAR(e.llamadas().count("dev wifi hotspot") == antes);
        CHEQUEAR(e.llamadas().contains("connection up betometro-propia"));
    }

    // 2) Lejos de casa: nada en 90 s → busca de nuevo y arma la propia
    {
        Escenario e("nada");
        Wifi w;
        qint64 ahora = 1000000;
        w.ponerReloj([&] { return ahora; });
        CHEQUEAR(esperar([&] { return !w.ocupado(); }, 5000));
        CHEQUEAR(w.conectada().isEmpty() && !w.propia() && !w.horaInternet());
        w.revisarAhora();   // empieza a contar
        CHEQUEAR(esperar([&] { return !w.ocupado(); }, 5000));
        ahora += 60000;
        w.revisarAhora();   // todavía no
        CHEQUEAR(esperar([&] { return !w.ocupado(); }, 5000));
        CHEQUEAR(!w.propia() && !w.armandoPropia());
        CHEQUEAR(!e.llamadas().contains("hotspot"));
        ahora += 35000;     // 95 s sin red
        w.revisarAhora();
        CHEQUEAR(esperar([&] { return w.propia() && !w.armandoPropia(); }, 8000));
        const QString ll = e.llamadas();
        CHEQUEAR(ll.indexOf("-t -f SSID dev wifi list --rescan yes") >= 0 && ll.indexOf("-t -f SSID dev wifi list --rescan yes") < ll.indexOf("hotspot"));
        // Con la propia armada, «Buscar redes» la apaga antes de buscar
        w.buscar();
        CHEQUEAR(esperar([&] { return !w.propia() && !w.buscando() && !w.ocupado(); }, 8000));
        CHEQUEAR(e.llamadas().contains("connection down betometro-propia"));
        // Apagada la opción de la red propia: no se arma; y con otro número de auto cambia el nombre
        w.ponerPropia(false, "7");
        CHEQUEAR(w.nombrePropia() == "Betometro-7");
        ahora += 200000;
        w.revisarAhora();
        CHEQUEAR(esperar([&] { return !w.ocupado(); }, 5000));
        CHEQUEAR(!w.propia() && !w.armandoPropia());
    }

    // 3) Falta dnsmasq: no se puede armar y el motivo queda en palabras (Conectividad)
    {
        Escenario e("dnsmasq");
        Wifi w;
        CHEQUEAR(esperar([&] { return !w.ocupado(); }, 5000));
        w.ponerRedes(false);
        CHEQUEAR(esperar([&] { return !w.armandoPropia() && !w.errorPropia().isEmpty() && !w.ocupado(); }, 8000));
        CHEQUEAR(!w.propia());
        CHEQUEAR(w.errorPropia().contains("dnsmasq"));
        CHEQUEAR(e.llamadas().contains("dev wifi hotspot"));
    }

    std::cout << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
