// Lee una carrera en JSON (entrada) y escribe datosInforme (salida). Uso: correr_informe <umbral> < carrera.json
#include <iostream>
#include <iterator>
#include <string>

#include "informe.h"

int main(int argc, char** argv) {
    std::string texto((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
    const double umbral = argc > 1 ? std::stod(argv[1]) : 20;
    const beto::Carrera c = beto::Carrera::desdeJson(beto::Json::leer(texto));
    std::cout << beto::datosInforme(c, umbral).escribir();
    return 0;
}
