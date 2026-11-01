// Informe de una carrera: armado del recorrido con todas las vueltas y los datos para dibujar.
// Misma lógica que `armarRecorrido` y `datosInforme` de la app 2.1.1 (docs/app/lo-aprendido.md, sección 9).
// Lo propio de la Raspberry: la vuelta interrumpida se incluye, marcada (`interrumpida: true`).
#pragma once

#include <string>

#include "json.h"
#include "modelo.h"

namespace beto {

// Lo que la plantilla del informe recibe como `C` (y lo que dibuja la pantalla del informe).
// `umbralPrecisionM`: el de la configuración (se usa max(10, umbral)).
Json datosInforme(const Carrera& c, double umbralPrecisionM);

// El HTML de una sola página, con la plantilla de la app (`informe-plantilla.html`).
std::string informeHtml(const Carrera& c, double umbralPrecisionM, const std::string& plantilla);

// Nombre del archivo: «🏁 nombre dd-mm-aaaa.html» (sin \ / : * ? " < > |, hasta 40 letras)
std::string nombreInforme(const Carrera& c);

}  // namespace beto
