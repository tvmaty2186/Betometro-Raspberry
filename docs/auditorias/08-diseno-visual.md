# Auditoría 08 — Diseño visual (07/11/2026)

Saqué capturas reales de todas las pantallas en uso, en los 4 temas, horizontal, vertical y modo sol
(Docker «beto», `--muestra`). Comparé contra `docs/app/diseno.md`, `docs/modo-tablet.md`, `docs/app/muestras-*.png`
y `docs/muestras/tablet/`. Lo aprobado (botones grandes, dato en color de acento, velocímetro cortado de pie,
números B612 Mono en Cabina, Informe sin barra grande) no lo discuto.

En general **está muy bien y muy parejo**: barra, botones y filas son las mismas piezas en todas las pantallas.
Lo que sigue son detalles.

## 1. Lo más visible y feo

| # | Qué encontré | Qué propongo | Costo |
|---|---|---|---|
| 1 | **El engranaje de Ajustes es un sol.** Un círculo con 8 rayitas. Está igual en 3 lugares: `app/qml/tablet/TIcono.qml:34` («ajustes»), `app/qml/iconos/ajustes.svg` (inicio con discos) y `app/qml/video/Engranaje.qml:12`. En el inicio queda al lado de Clima, que tiene un sol de verdad: se confunden. | Dibujar un engranaje de verdad (rueda con 8 dientes cuadrados y agujero al medio), uno solo, y usarlo en los 3 lugares. Mostrarle la imagen a Matías antes. | chico |
| 2 | **Modo sol, Cámara: el aviso no se ve.** El cuadro del video tiene color fijo verde oscuro `#05110a` (`TCamara.qml:32`) y en sol las letras son negras: «No se encontró la cámara» desaparece. Además ese cuadro sale verde en Rally, Deportivo y Cabina. | Usar `Tema.caja2` (o negro puro) y letras blancas fijas adentro del cuadro del video. | chico |
| 3 | **Chequeo previo dice «undefined satélites · 2,5 m»** (todos los temas). `TChequeo.qml:32` arma el texto aunque no venga la cantidad de satélites. | Mostrar «GPS listo · 2,5 m» si no hay número, como hace la barra (`TBarra.qml:58`). | chico |
| 4 | **«FINALIZADA» no entra en su botón** (Ficha de una carrera corrida): letra 70 + espaciado 4 toca los dos bordes (`TFicha.qml:80`, `TBoton.qml:43`). Encima es el botón más grande de la pantalla y está apagado (opacidad 0,45 sobre verde = verde oliva sucio). | Achicar a ~52 o sacar el espaciado. Y mejor: que en una carrera corrida el botón grande sea «Ver informe». | chico |
| 5 | **Informe en Deportivo y Cabina: el título se pisa con los botones Vel./Acel./Adel.** «RECORRIDO DE LA VUELTA · COLOR = VELOCIDAD» pasa por debajo de «Vel.» (`Informe.qml:194`, el texto no tiene ancho máximo). | Darle ancho hasta los botones y cortar con «…», o el texto corto («Recorrido · color = velocidad») siempre. | chico |
| 6 | **Deportivo: «Actualizar» y «Apagar» son dos botones rojos pegados** (Ajustes). El acento del tema es rojo y el peligro también (`TBoton.qml:20-24`, rojo fijo). Un toque mal y se apaga en vez de actualizar. | En Deportivo, peligro con borde rojo y fondo oscuro, o el acento de «Actualizar» en blanco (en la app el `--acento` de Deportivo es blanco). | chico |

## 2. Lo que más mejora con poco trabajo

| # | Qué encontré | Qué propongo | Costo |
|---|---|---|---|
| 7 | **Botones apagados = 45 % de opacidad** (`TBoton.qml:18`). El amarillo apagado sobre verde queda oliva: LARGAR de las carreras próximas (Carreras), FINALIZADA, «Probar 10 s». Parece sucio, no apagado. | Un estilo «apagado» propio: fondo del tema, borde punteado o gris, letras `Tema.tintaSuave`. | chico |
| 8 | **Colores fijos fuera de `Tema`:** rojo de peligro `#e0473a/#b8231a/#8f130b/#ffb3aa` (`TBoton.qml:20-24`), azul de lluvia `#5fb4ff` (`TClima.qml:66,128`, `TCarreras.qml:61`), verde `#0a2414` en la barra de Drive (`TDrive.qml:97`, sale verde en todos los temas), cromo `#c9ced3` y `#fff` en las etiquetas de la cámara (`TCamara.qml:61-70`). En sol, el azul de lluvia casi no se lee sobre blanco. | Pasarlos a `Tema` (`Tema.mal` ya existe; sumar `Tema.lluvia` con su versión sol). | chico |
| 9 | **Íconos repetidos o que no dicen nada:** Cámara → «Calidad» usa el ícono de *Pantalla de carrera* (`TCamara.qml:92`); Chequeo → «Temperatura» usa la *paleta de tema* (`TChequeo.qml:46`), «Tensión» usa *apagar* (`:43`); GPS y Localidad son el mismo dibujo («gps» y «lugar» son idénticos, `TIcono.qml:41` y `:62`). | Tres íconos nuevos chicos: termómetro, rayo (tensión) y «HD» (calidad). | chico |
| 10 | **Hora y números en Cabina con huecos:** «03: 39», «4: 00, 0», «3. 200 m». B612 Mono le da a «:» y «.» el ancho de un número. Es igual que la app en los números grandes, pero en la **hora de la barra** (`TBarra.qml:77`) y en Práctica queda raro. | Solo en la barra: hora con `Tema.fuente` (B612 normal). Lo demás, como la app. | chico |
| 11 | **Textos que no entran en Cabina** (B612 es más ancha): «Avispón · brillo 10…» (Ajustes), «betometro.local:80…» y «Betometro-1…» (Conectividad), «Próxima: Gualeguaychú 2…» (inicio tablet). «Pantalla y tema» y «Velocidad y GPS» bajan a 2 renglones en Deportivo y Cabina y no en Avispón. | Dato más corto en Ajustes («Avispón · 100 %»); en Cabina bajar el nombre de 29 a 27. | chico |

## 3. El resto

| # | Qué encontré | Qué propongo | Costo |
|---|---|---|---|
| 12 | Letras de menos de 21 px en pantallas grandes (la regla dice que no): «CARRERA» 15 px y en cursiva fija `italic: true` en todos los temas (`TClima.qml:122`), «desde 20/11» 18 px (`TCarreras.qml:64`), versión 20 px (`TBarra.qml:47`). | Pasar a 21 y `italic: Tema.cursiva`. | chico |
| 13 | Títulos de la barra en blanco liso (`TBarra.qml:38`). La guía del Avispón dice título **cromado** (como el Informe y la app). | Cromo solo en Avispón. Mostrar muestra antes. | chico |
| 14 | Modo sol, «Apagar»: letras negras sobre rojo (`TBoton.qml:16`, usa `Tema.tinta`). Se lee mal. | Letras blancas en peligro, siempre. | chico |
| 15 | Informe: la columna de velocidad (44,3) queda más alta que el resto de la fila; flecha de volver de 40 px (en las demás, 64). Ficha de pie: hueco grande entre «Medidas» y el botón. | Alinear al centro; volver más grande; en la ficha vertical, botón pegado a las filas. | chico |
| 16 | Ajustes horizontal: 9 botones en grilla de 5 × 2, queda un hueco abajo a la derecha. | Aceptable. Si molesta, «Apagar» más ancho ocupando el hueco. | chico |

**Nota de herramientas:** el Docker no tenía `qt6-svg-plugins` y los discos del inicio salían sin íconos. Lo
instalé en el contenedor (no toca el repo). En la Raspberry está en `raspios/paquetes.txt:9`, así que allá anda.

## Las 3 que haría primero

1. **El engranaje** (punto 1): es lo que Matías ya vio, y está en 3 lugares.
2. **Los 4 arreglos de texto** (puntos 2, 3, 4 y 5): son errores que se ven, cada uno de una línea.
3. **Botón apagado y colores fijos** (puntos 7 y 8): de una vez sale parejo en los 4 temas y en sol.

## Preguntas para Matías

(Lo visual se lo mostramos con imagen antes de programar.)

1. ¿Te muestro 2 o 3 dibujos de **engranaje** para que elijas uno?
2. En una carrera **ya corrida**, ¿querés que el botón grande de la ficha sea **«Ver informe»** en vez de «FINALIZADA»?
3. En **Deportivo**, «Actualizar» y «Apagar» salen los dos rojos. ¿Preferís «Actualizar» en blanco (como el acento de la app) o «Apagar» con borde rojo y fondo oscuro?
4. ¿Querés el **título cromado** en la barra de arriba, solo en el Avispón (como en la app)?
