# Detalles de pantalla de la app 2.4.1 que faltan en la Raspberry

**Estado (1.0.3, 03/11/2026):** hechos A1-A6, B7-B11 y B13, C14-C17, D19, E21, E22, E24, E25, F28 y F29.
Quedan: B12 (pellizco), C18 (menores del editor), D20 (glow de otros temas), E23 (colores de la barra ±),
E26 (espaciado de MARCAR) y F27 (marco en sol).

Relevado el 03/11/2026 a pedido de Matías («hay detalles del diseño de la app que acá no están»).
Referencia de la app: `www/index.html` (CSS de widgets: líneas 124-160, 269-312, 314-343, 419-434,
636-660; JS de widgets y editor desde la línea 1526: `crearWidget`, `construirTablero`,
`dibujarEdicion`, `golpe`, `imantar`). Orden: de lo que más se nota en carrera a lo que menos.

## A. Tamaño de los números (lo que más se nota)
1. **Fórmula del tamaño, no «achicar hasta que entre».** App: `font-size = min(A·alto, k·B/100·ancho / n / anchoOpción)`
   con alto/ancho del interior del widget (sin el relleno 8×12 px) y `line-height .9`. A/B: general 58/100; cuenta 82/105
   (66 con barra); velocímetro digital con barra 46/100; delta 70/95; tipo 36/90; MARCAR `min(52cqh, 18cqw)`.
   Raspberry: `widgets/Numero.qml` usa `Text.Fit` → dígitos ~30 % más chicos (cuenta horizontal: 138 px contra ~192 px ×1,2).
2. **`n` = cantidad de caracteres (mínimo 3)**, y el tamaño cambia solo si cambia el largo del texto (`ponerNum`).
   Con `Text.Fit` el tamaño «respira» entre décimas.
3. **Dígitos de ancho fijo** (`tabular-nums`): en QML `font.features: { "tnum": 1 }`. Si no, los números bailan de costado.
4. **«Números − / +» (`ancho`, 0,7 a 1,8; cuenta de fábrica 1,2):** divide el tamaño por `ancho` y estira con
   `scaleX(ancho)`. Es «cómo se agrandan los dígitos». Falta en Armado, Editor y Numero.
5. **`k` por tema:** Avispón/base 190, Rally 235, Deportivo 165, Cabina 150 (sol usa el del tema). Falta `Tema.k`.
6. **Rendimiento Pi 3:** `Crono.estado` cada 50 ms rehace `Text.Fit` en todos los widgets; el velocímetro redondo repinta
   todo el Canvas. Pintar la cara una vez y mover solo la aguja (`rotation`).

## B. Widgets chicos siempre al frente (editor)
7. **El elegido no sube de capa** (app: orden por área, grandes atrás; solo sube el que se arrastra, z 20).
   Raspberry `Tablero.qml`: `z: elegido ? 10 : 0` → tapa a los chicos y se queda con los toques. Cambiar a `z: moviendo ? 20 : 0`.
8. **Borde y asas en una capa aparte, arriba de todo** (app: asas z 25, guías z 24).
9. **El toque cuenta solo dentro de la forma** (redondos: el círculo; `golpe()` del más adelante al más atrás).
10. **Bug:** el redondo se achica al tocar el asa (Raspberry usa `hypot·2/1.414`; app `d = max(70, 2·distancia)`).
11. **Cuatro asas** en los cuadrados (nw, ne, sw, se), mínimo 60×44.
12. **Pellizco con dos dedos** sobre el elegido (si la pantalla es multitáctil).
13. **Borde del elegido punteado** amarillo `#ffd23f`, 3 px, a 7 px.

## C. Opciones del editor que faltan
14. **Estilo:** cuenta en Números o **Anillo**; delta en Número o **Barra** (±5 s, «Adelantado 1,2 s»).
15. **Redondo** en más widgets (todos menos tipo y MARCAR), con relleno interior del 16 %.
16. **Bug del dato extra:** se muestra solo si `extra` es verdadero (en la Raspberry, si no es falso).
17. **Velocímetro redondo:** dibujar el dato extra «máx N».
18. Menores: MARCAR nuevo en su lugar de fábrica; widgets nuevos en fracciones de la pantalla; cartel de ayuda que se va a los 5 s.

## D. Cara de los widgets (aspecto de instrumento)
19. **Avispón:** fondo radial `#1f4a2f → #08190e`, sombra interior abajo, sombra difuminada (blur 18, desplazamiento 10)
    y **reflejo** arriba (lente blanca .12 → 0, 36 % de alto). Sin reflejo ni sombra en sol.
20. **Glow** en Rally (ámbar), Deportivo (rojo), Cabina (celeste) y aro triple.

## E. Colores y textos
21. **Pasado:** cuenta regresiva «Pasado» en amarillo (`--warn`); progresiva «Pasado el ideal» en rojo (`--mal` #ff6b5a).
22. **ATRASADO** en amarillo (`--warn`), no en alerta.
23. **Barra ±:** colores #d7263d / #f5c518 / #2ecc71, ancho 62 %, cursor con borde #111 (salvo que las muestras aprobadas digan otra cosa).
24. **Datos extra en minúscula** («base 48», «de 3.200 m»), no con `Sub` en mayúsculas.
25. **Cursiva solo en Avispón** (no en sol); en sol, etiquetas en negrita.
26. **MARCAR:** letra espaciada .14em.

## F. Pantalla de carrera y otros
27. Marco del indicador en sol: 11 px (7 normal).
28. Velocímetro sin GPS: aguja en 0 con opacidad .25 (no escondida).
29. Velocímetro del inicio: se mueve solo (`47 + 2,5·sin(0,9t) + 1,2·sin(2,3t)`) con sugerida en 48 (después del punto 6).

## Archivos de la Raspberry a tocar
`app/qml/widgets/` (Numero, Tablero, Caja, WCuenta, WDelta, WNumero, WVelocidad, WVelocimetro, Barra, WMarcar),
`app/qml/componentes/Velocimetro.qml`, `app/qml/pantallas/` (Editor, Carrera, Inicio), `app/qml/Tema.qml`, `app/qml/Armado.qml`.
