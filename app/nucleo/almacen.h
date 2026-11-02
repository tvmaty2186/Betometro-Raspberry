// Almacén: la base entera en un archivo JSON en la partición de datos.
//
// Escritura segura (para que un corte de corriente no rompa nada):
//   1. Se escribe todo en «betometro.json.nuevo» y se fuerza a disco (fsync).
//   2. El archivo actual pasa a ser «betometro.json.anterior» (la copia anterior).
//   3. «.nuevo» pasa a ser «betometro.json» y se fuerza la carpeta a disco.
// Al leer se prueba, en orden: .nuevo, betometro.json, .anterior. Se usa el primero que esté sano.
#pragma once

#include <string>

#include "modelo.h"

namespace beto {

class Almacen {
public:
    explicit Almacen(std::string carpeta, std::string nombre = "betometro.json");

    struct Lectura {
        Base base;
        std::string origen;      // archivo del que salió; vacío si no había nada
        bool habiaDanado = false; // algún archivo estaba roto (se guarda aparte como «.danado»)
    };
    Lectura cargar() const;

    // Devuelve false si no pudo escribir (y deja el archivo anterior como estaba)
    bool guardar(const Base& base, std::string* error = nullptr) const;

    std::string archivo() const { return carpeta_ + "/" + nombre_; }

private:
    std::string carpeta_, nombre_;
};

}  // namespace beto
