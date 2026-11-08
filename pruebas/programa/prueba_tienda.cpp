// Pruebas del catálogo de la Tienda (app/tienda.cpp): formato, ids raros, íconos con «..», versión mínima.
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>

#include "tienda.h"

static int fallas = 0;
#define CHEQUEAR(c) do { if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    Tienda t(dir.path());
    CHEQUEAR(t.herramientas().isEmpty());
    CHEQUEAR(!t.leer("{\"formato\":2,\"herramientas\":[]}"));   // otro formato: no se toca
    CHEQUEAR(!t.leer("no es json"));
    CHEQUEAR(t.leer("{\"formato\":1,\"herramientas\":[]}"));
    CHEQUEAR(t.herramientas().isEmpty());
    CHEQUEAR(t.leer(R"({"formato":1,"herramientas":[
        {"id":"brujula","nombre":"Brújula","version":"1.0.0","icono":"brujula/icono.png"},
        {"id":"../malo","nombre":"Mala"},
        {"id":"rara","nombre":"Rara","icono":"../../etc/passwd"},
        {"id":"futura","nombre":"Futura","betometroMinimo":"99.0.0"}]})"));
    const QVariantList l = t.herramientas();
    CHEQUEAR(l.size() == 2);   // sin el id malo ni la que pide un Betómetro más nuevo
    CHEQUEAR(l.value(0).toMap().value("nombre").toString() == QStringLiteral("Brújula"));
    CHEQUEAR(l.value(0).toMap().value("icono").toString().endsWith("/brujula/icono.png"));
    CHEQUEAR(l.value(1).toMap().value("icono").toString().isEmpty());   // ícono con «..»: nada
    if (fallas) { std::cerr << fallas << " fallas\n"; return 1; }
    std::cout << "tienda: todo bien\n";
    return 0;
}
