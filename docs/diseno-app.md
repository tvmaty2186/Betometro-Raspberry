# Diseño de Betómetro

## Principios

- **Se usa en carrera, con el auto moviéndose:** números enormes, alto contraste y botones grandes. MARCAR siempre es el blanco más fácil de tocar.
- **Instrumentos realistas:** aro metálico, cara con profundidad y reflejo arriba (desde la 1.3).
- **Todo proporcional a la pantalla:** nada fijo en píxeles en el inicio ni en la pantalla de carrera.
- **Cambios visuales:** antes de programar se hace una imagen de muestra (Playwright) y Matías la confirma.

## Temas

Se eligen en Ajustes → Tema y pantalla. Cada tema es un bloque `:root[data-tema="…"]` de variables CSS.

| Tema | Valor | Estilo |
|---|---|---|
| **Avispón Verde** (fábrica desde 2.0) | `avispon` | Verde Peugeot 404, cromo, amarillo, león de Matías |
| Rally clásico | `rally` | Tablero antiguo, números ámbar, menús tipo «carnet» de papel (`esCarnet()`) |
| Deportivo moderno | `sport` | Carbono, blanco y rojo, Chakra Petch |
| Cabina de avión | `cabina` | Negro, verde y celeste funcional, B612 |

- **Modo sol (`:root.sol`):** fondo claro y tinta negra gruesa. Se combina con cualquier tema y le gana. Las reglas del Avispón usan `:not(.sol)` para no pisarlo.

### Variables de cada tema

| Grupo | Variables |
|---|---|
| Base | `--idle --panel --panel-2 --ink --ink-dim --lbl --line --warn --acento` |
| Números | `--font-num --peso-num --k` (cuánto entra por carácter) |
| Interfaz | `--font-ui --radio` |
| MARCAR | `--marcar-bg --marcar-ink --marcar-borde` |
| Velocímetro | `--g-dial --g-tick --g-req --g-zona --g-aguja` |
| Instrumentos | `--aro1 --aro2 --glow --cara1 --cara2 --dig --fondo-crono` |
| Menús | `--acc --acc-ink --card-bg --card-borde --sub --hueco --ok --medio --mal` |

## Avispón Verde (2.0)

Está inspirado en el auto de Matías: **Peugeot 404 verde, #101, «el Avispón Verde»**.

- **Elementos:**
  - el león corriendo que dibujó él (melena azul, cuerpo amarillo);
  - el redondel cromado del número sobre una banda blanca y amarilla, como en la puerta;
  - rayas diagonales blanca y amarilla;
  - moldura cromada;
  - un *swoosh* blanco y amarillo abajo.
- **Prohibido:** la marca PEUGEOT, logos de concesionarias y el león de Peugeot. **El león es dibujo propio de Matías.**
- **Muestras aprobadas:** `docs/diseno/avispon/muestras-1.png` (Inicio, Carreras, Ficha, Práctica) y `muestras-2.png` (Carrera vertical y horizontal, Ajustes, Informe). Los estilos de esas muestras están en `kit.css`.

### Paleta

| Uso | Color |
|---|---|
| Verde claro / brillo | `#3d8c57`, `#2f7a49` |
| Verde medio | `#1d5233` |
| Verde oscuro (fondo) | `#0d2a18`, caras de widgets `#1f4a2f → #08190e` |
| Amarillo | `#f4c531` (degradé `#ffe07a → #f4c531 → #d9a20f`) |
| Crema (números, cara del velocímetro) | `#f3ecd8`, cara `#fffbef → #efe6cf → #d6caa9` |
| Blanco de las rayas | `#f7f7f2` |
| Cromo | `linear-gradient(180deg,#fff 0%,#c9ced3 30%,#6f777f 52%,#e8ebee 60%,#9aa1a8 100%)` (`--cromo`) |
| Bordes cromo | `#cfd4d8` / `#4b5258` (doble anillo) |
| Texto secundario | `#d6e6d2` |

### Tipografía

- **Títulos:** Barlow **itálica 800**, rellena con el degradé cromo (`background-clip: text`) y sombra.
- **Números:** Barlow Condensed 700 en itálica.
- **Etiquetas:** Barlow itálica 600, en mayúsculas y espaciadas.

### Componentes

- **Fondo:** degradé radial verde con un damero que se desvanece arriba a la izquierda (`#p-inicio::before`).
- **Botones del inicio:** discos verdes con triple aro cromado.
- **Velocímetro redondo:** cara crema con bisel cromado, aguja negra y números verdes.
- **Widgets:** verde oscuro con doble anillo cromo y números crema.
- **MARCAR / LARGAR:** amarillo degradé con letras **negras** (`!important`, porque si no salían blancas).
- **Tarjetas y fieldsets:** verde translúcido con borde cromo.
- **Fechas de la lista:** redondel cromado con número verde.
- **Secciones:** «PRÓXIMAS» y «CORRIDAS» llevan una banda blanca y amarilla.
- **Número del auto:** se cambia en Ajustes → «Número del auto» (`cfg.numeroAuto`, fábrica «101»).
- **Inicio con foto (2.1):** cabecera verde oscura (león | título cromado | rayas) con filete amarillo; debajo, la foto fija `www/img/inicio.jpg` (los dos pilotos con el 404, recortada desde arriba y fundida en verde) hasta la banda del 101. **La foto es fija: Matías no quiere que se pueda cambiar.**
- **Elementos del inicio:** solo existen en este tema (clases `.av-*`; `#avNum` y `#avBanda` se ubican en `layoutInicio()`).

## Foto del inicio (desde 2.1)

- `www/img/inicio.jpg` (1200 px): el piloto y el copiloto apoyados en el 404. Es **la foto de la app**, fija e igual en todos los temas.
- Va entre la cabecera (alto `--cab` = barra de estado + 98k) y la línea de los botones (`--ny` = `cy − 300k`, que fija `layoutInicio()`).
- Se recorta desde arriba (`center top`) para no cortar las cabezas y abajo se funde con el fondo del tema.
- En los otros temas lleva arriba un filete del color de acento; en Avispón, la cabecera verde con león, rayita, título cromado y rayas.

## Ícono

Es un cronómetro/velocímetro con aro rojo sobre fondo oscuro (`docs/diseno/icono/`).

**Idea pendiente, no pedida todavía:** un ícono con la estética Avispón.

## Caja del equipo dedicado (cancelada)

La caja con el A37 se canceló el 01/11/2026. El modelo queda como referencia.

El modelo 3D interactivo y explotable está en `docs/diseno/caja/caja-betometro.html` (Three.js r128, un solo archivo). Se puede ver en línea en https://claude.ai/artifact/7CS6LmiuA6sTKc4nm5AFmt.

Detalles en `docs/hardware.md`.
