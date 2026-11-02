# Informe de la etapa E1 — Motor y modelo de datos

Fecha: 01/11/2026. Resultado: **aceptada**. El motor en C++ da **exactamente lo mismo** que el `Motor` de la app 2.1.1.

## Cómo se probó

- El `Motor` de la app **no se copió a mano**: `motor-app.js` lo saca de `referencia/app-2.1.1/index.html` y lo corre con Node.
- `casos.js` arma 65 recorridos (25 armados a propósito y 40 al azar, siempre los mismos).
- Cada recorrido pasa por los dos motores: `correr-app.js` (app) y `correr_motor` (C++).
- `comparar.js` compara línea por línea: cada evento, cada marca y `estado(t)` cada 100 ms (y en 200 momentos al azar por caso).

Casos armados: 10 Hz y 1 Hz, patrones `todas`/`alt1`/`alt2`, huecos de GPS (4 s y 12 s), mala precisión y precisión justo en el umbral, marcas dobles y justo en el antirrebote, sin antirrebote, autocalibración (GPS de más y de menos, vuelta cortada, fuera de rango, apagada), sin GPS, sin distancia, congelado de 3 s y de 60 s, vuelta pasada, llegar antes, total de vueltas, muestras desordenadas, cambios de Ajustes en plena carrera, factor inicial, terminar a mano, auto detenido.

## Resultado

- Casos: **65**
- Líneas comparadas: **773041** (estados: 418135, vueltas cerradas: 254)
- Vueltas interrumpidas (solo Raspberry, no se comparan): 1
- Resultado: **IGUALES**

| Campo | Mayor diferencia |
|---|---|
| congelada | 0 |
| deltaS | 0 |
| difMs | 0 |
| distM | 0 |
| enCurso | 0 |
| factor | 0 |
| factorVuelta | 0 |
| fuente | 0 |
| idealMs | 0 |
| inicio | 0 |
| medida | 0 |
| n | 0 |
| proxMedida | 0 |
| realMs | 0 |
| restanteM | 0 |
| restanteMs | 0 |
| sinGps | 0 |
| t | 0 |
| transMs | 0 |
| vActKmh | 0 |
| vBaseKmh | 0 |
| vNecKmh | 0 |
| velMaxKmh | 0 |
| velPromKmh | 0 |

**Todas las diferencias son cero.** Lo pedido era menos de 1 ms y menos de 1 cm.

Para que dé idéntico:
- Los redondeos copian a JavaScript: `Math.round` (`redondearJs`) y `toFixed` (`fijarDecimalesJs`), incluidos los empates.
- Se compila con `-ffp-contract=off` y sin `-ffast-math`: así el procesador ARM de la Raspberry hace las mismas cuentas que la PC.
- GitHub Actions corre las pruebas en PC y en ARM 64 bits en cada cambio.

## Lo propio de la Raspberry (no está en la app)

Probado en `pruebas/nucleo/prueba_nucleo.cpp` (118 chequeos):

- **Vuelta interrumpida:** al terminar a mano, la vuelta en curso se guarda con `interrumpida: true`, sin `difMs` (sin error), sin calibrar y con `fuente: "terminar"`. Tiene distancia y velocidades si hubo GPS.
  - No se guarda si dura menos que el antirrebote (por ejemplo, marcar y terminar enseguida).
  - No se guarda cuando la carrera ya llegó al total de vueltas.
- **Solo cronómetro:** sin GPS la carrera anda igual: cuenta, marcas, antirrebote y tiempos. Sin sugerida, sin adelanto y sin metros.
- **Modelo de datos:** carrera, vuelta, traza y base con el mismo JSON que la app. Se conservan los campos viejos y los que no conoce.
- **Configuración:** los valores de fábrica de la app, menos lo que es solo de Android, más `orientacion` (horizontal), `brillo` (100 %, mínimo 10 %) y `volumen` (100 %). Los valores fuera de rango vuelven al límite.
- **Almacén:** escritura segura (`.nuevo` + `fsync` + reemplazo, con copia `.anterior`). Probado: archivo roto, corte justo después de escribir y corte mientras se escribía.

## Cómo correrlo

```
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```
