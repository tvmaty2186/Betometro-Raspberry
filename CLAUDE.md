# Betómetro Raspberry — guía para Claude

Leé esto al empezar cualquier sesión en este repositorio.

| Archivo | Qué tiene |
|---|---|
| [docs/plan-ingenieria.md](docs/plan-ingenieria.md) | **El plan:** etapas E1 a E6, especificación técnica y criterio de aceptación de cada una. Seguilo en orden |
| [docs/diseno-sistema.md](docs/diseno-sistema.md) | **Lo principal:** sistema, cableado, pantallas aprobadas, decisiones |
| [docs/hardware.md](docs/hardware.md) | Piezas compradas y lo que falta comprar |
| [docs/gps-neo-m8n.md](docs/gps-neo-m8n.md) | El módulo GPS (NEO-M8N-0-10): datos oficiales, configuración y pruebas al recibirlo |
| [docs/app/lo-aprendido.md](docs/app/lo-aprendido.md) | **Todo lo que hace la app y por qué**: datos, motor, voz, ajustes, pantallas, widgets, informe. Es la especificación a copiar |
| [docs/app/diseno.md](docs/app/diseno.md) | Guía estética de la app (temas, Avispón Verde, colores, tipografías) |
| [docs/app/arquitectura.md](docs/app/arquitectura.md) | Cómo está hecha la app |
| [docs/app/historial.md](docs/app/historial.md) | Versiones de la app y decisiones de Matías |
| [pruebas/motor/INFORME.md](pruebas/motor/INFORME.md) | E1: el motor en C++ (`app/nucleo/`) y cómo se compara con la app |
| [PENDIENTES.md](PENDIENTES.md) | Lo que falta hacer |

## Qué es

Betómetro para Raspberry Pi: un equipo **autónomo** para el navegante de carreras de regularidad. Hace lo mismo que la app Android Betómetro (cuenta regresiva en voz, velocidad sugerida con GPS, adelantado/atrasado, historial e informes), pero como **sistema propio**: se prende y arranca directo en Betómetro.

- **Sistema:** Linux a medida con Buildroot, solo lectura, dos copias A/B para actualizar seguro.
- **Programa:** nativo en Qt 6 / QML, hecho de cero. El motor se pasa a C++ con la misma lógica que el `Motor` de la app.
- **App Android:** está en otro repo, [tvmaty2186/Betometro](https://github.com/tvmaty2186/Betometro). **Prohibido tocarla desde acá**; funciona bien. Se puede leer para copiar la lógica (por ejemplo `www/index.html`, sección MOTOR).

El dueño es **Matías Russo** (tvmaty2186@gmail.com). Corre con un **Peugeot 404 verde, número 101, «el Avispón Verde»**.

## Cómo trabajar con Matías

- **Idioma:** español rioplatense (vos), corto y claro. Tiene dislexia: frases simples, sin paredes de texto, lo importante primero.
- **Links:** cuando lo mandás a hacer algo, pasale el link directo para tocar.
- **Antes de construir:** para cosas grandes, charlá primero. Si es visual, mostrale una imagen de muestra antes de programar; él confirma.
- **Tokens:** quiere ahorrarlos. No releas archivos enteros sin necesidad.
- **Fotos de referencia:** prestá mucha atención a posiciones y medidas.
- **El león es suyo:** el león corriendo (melena azul, cuerpo amarillo) lo dibujó él. **No es el logo de Peugeot.** No uses la marca PEUGEOT ni logos de concesionarias.
- **Estética:** igual a la app en todo lo posible.

## Commits

- Autor `Matías Russo <tvmaty2186@gmail.com>`, mensaje en español.
- Versiones propias de este repo (empieza en 1.0.0), separadas de las de la app.

## Lo que ya se decidió en la app vale acá

**No hay que volver a analizar ni preguntar lo que ya se decidió en la app.** Está en `docs/app/`. La Raspberry copia el comportamiento de la app (valores de fábrica, voz, widgets, pantallas de fábrica, informe) salvo lo que dice «Qué cambia en la Raspberry» en `lo-aprendido.md` y lo que dice `diseno-sistema.md`.

- `referencia/app-2.1.1/`: copia del código de la app (versión 2.1.1) para buscar con grep. No se edita.
- `docs/app/muestras-1.png` y `muestras-2.png`: muestras aprobadas del tema Avispón en la app (vertical y horizontal).

## Recursos

- `recursos/fuentes/`: tipografías (copiadas de la app).
- `recursos/img/leon.png`: el león de Matías. `recursos/img/inicio.jpg`: la foto fija del inicio. `recursos/img/betometro-icono-*.png`: ícono de la app.

## Compilar y probar

`cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure` (necesita Node para comparar con la app).

## Muestras

`docs/muestras/pantallas-horizontal.html` se abre con Playwright (`require('playwright')`) a 1280×720; cada pantalla es `#s1`…`#s12`.
