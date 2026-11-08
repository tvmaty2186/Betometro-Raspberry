// Marca virtual por GPS (Matías, 08/11/2026): solo de referencia, para el informe. Nunca marca ni cambia nada.
// La línea de llegada es el punto donde se largó (la primera lectura de la vuelta 1), de través al rumbo de salida.
// Al final de cada vuelta se busca, en el recorrido grabado, cuándo el auto cruzó esa línea (entre dos lecturas,
// repartiendo el tiempo según la distancia). La largada es la misma que la del pulsador.
#pragma once

#include "modelo.h"

#include <vector>

namespace beto {

struct MarcaGps {
    bool hay = false;               // se pudo calcular
    bool dudosa = false;            // GPS impreciso o con huecos justo en la línea
    int64_t realMs = 0;             // tiempo de la vuelta con la marca virtual
    int64_t difMs = 0;              // realMs − ideal (positivo = tarde)
    int64_t pulsadorMenosGpsMs = 0; // positivo = el pulsador marcó después de cruzar la línea
};

// Una por vuelta, en el mismo orden que c.vueltas
std::vector<MarcaGps> marcasGps(const Carrera& c);

}  // namespace beto
