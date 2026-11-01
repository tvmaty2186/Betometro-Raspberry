# Betómetro Raspberry — guía para Claude

Leé esto al empezar cualquier sesión en este repositorio.

| Archivo | Qué tiene |
|---|---|
| [docs/diseno-sistema.md](docs/diseno-sistema.md) | **Lo principal:** sistema, cableado, pantallas aprobadas, decisiones |
| [docs/hardware.md](docs/hardware.md) | Piezas compradas y lo que falta comprar |
| [docs/diseno-app.md](docs/diseno-app.md) | Copia de la guía estética de la app (temas, Avispón Verde, colores, tipografías) |
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

## Recursos

- `recursos/fuentes/`: tipografías (copiadas de la app).
- `recursos/img/leon.png`: el león de Matías. `recursos/img/inicio.jpg`: la foto fija del inicio.

## Muestras

`docs/muestras/pantallas-horizontal.html` se abre con Playwright (`require('playwright')`) a 1280×720; cada pantalla es `#s1`…`#s12`.
