# Plan de la 1.0.27 (aprobado por Matías, 07/11/2026)

Todo lo que entra en la 1.0.27, con las decisiones de Matías. El detalle técnico de cada arreglo está en el
informe de su auditoría (`docs/auditorias/NN-…md`, punto entre paréntesis). Programar en este orden, probar con
capturas en Docker (`debian:trixie`, ver `raspios/compilar.sh`), commits `[skip ci]` y publicar al final con su OK.

## 1. Seguridad y datos (no cambia pantallas)

- [ ] Recorrido libre: guardar la traza en curso cada tanto, para no perderla si se corta la luz; en el informe,
      «se cortó» si no llegó a Terminar (05-1.1).
- [ ] Informe: no caerse con distancia negativa en la traza, más prueba de bases dañadas en `ctest` (02-1.1).
- [ ] Guardado: números a texto con `std::to_chars` (mismo archivo, 10 a 40 veces más rápido) (05-2.1, 04-2.1).
- [ ] Guardado: no copiar la base en el hilo principal; no guardar la base entera cuando no hace falta (04-2.2, 2.3).
- [ ] Avisar si no se puede guardar y «Poco lugar» en el Chequeo previo (05-1.2, UX 2.4). **Sin carteles grandes en
      carrera** (Matías: no).
- [ ] Tarjeta llena en carrera: **parar el video y que la carrera siga** (Matías: sí) (06-1.3).
- [ ] Cámara colgada o desenchufada: matarla si no termina, poder volver a grabar (02-1.2, 06-1.4).
- [ ] `nmcli` / `iw` con tiempo límite y menos chequeos de wifi en carrera (02-1.3).
- [ ] Instalador: no tocar `cmdline.txt` si no cambia, `sync`, `trap` si falla a mitad, `Type=notify` (10-1.2 a 1.4).
- [ ] **Vuelta atrás automática** si la versión nueva no arranca, y avisar en pantalla (Matías: sí) (10-1.1).
- [ ] Borrar los paquetes de actualización viejos (10).
- [ ] Borrar una carrera también borra su video (si ningún otro la usa) (pedido de Matías).

## 2. GitHub y pruebas

- [ ] **No publicar si falla una prueba** (Matías: sí). Compilar con las pruebas y arreglar el job roto (09-1.1, 2.a–c).
- [ ] **Compilar solo desde `main`** (Matías: sí). Que un push no cancele una compilación con `[publicar]` (10-2.2).
- [ ] Carrera y recorrido libre de punta a punta con `simular_neo` en las pruebas (09 P3).
- [ ] Probar compilar en ARM de verdad (`ubuntu-24.04-arm`) para bajar de 20 a ~5 min (10-2.1).

## 3. Rendimiento

- [ ] Sacar la animación de la barrita (60 → 20 cuadros por segundo en carrera), la aguja del inicio y el borde que
      late debajo del control de la tele (03-1.1).
- [ ] Listas del video que se rearman 10 veces por segundo → elementos fijos; aliviar el modo tele (03-1.2, 1.3).
- [ ] Crear solo el tablero que se usa (vueltas o recorrido) (03-2.1); un solo fondo (03-2.2); foto del inicio con
      `sourceSize` (03-2.3); juntar capas de cada caja (03-2.4).
- [ ] 480p en **800×600** con la cámara de Matías (Matías: sí), arreglando `--escala` (06-1.2, 2.1).
- [ ] Sonido del video en sincronía (06-1.1).

## 4. Pantallas y uso (decididos por Matías)

- [x] ± segundos grande (`WDelta.qml`) y nueva pantalla de fábrica (`Armado.qml`): acostada ± arriba a la derecha
      y la Sugerida al lado de la velocidad; de pie ± abajo al lado de la última vuelta. **Hecho.**
- [ ] **MARCAR en pantalla opcional:** llave «MARCAR en pantalla» en Ajustes → Pulsador y en el Chequeo previo
      (prendida de fábrica). Apagada: sin botón; larga y marca solo el pulsador; **nada en pantalla como plan B**.
      Armado sin MARCAR (aprobado): [muestras/sin-marcar/](muestras/sin-marcar/) — acostada `horizontal.png`
      (cronómetro a todo el ancho con datos extra; V1, Ideal y ± segundos encima; abajo Velocidad, Sugerida,
      Faltan y Última vuelta; **no cambiar nada**), de pie `vertical.png` (la propuesta 2, con reloj). Medidas en `armado.json` (px). Guardarlo como
      segundo armado de fábrica (cfg `widgetsH`/`widgets` siguen siendo los del usuario; el «sin MARCAR» se usa
      cuando la llave está apagada y el usuario no armó uno propio).
- [ ] **Tiempos con «:» en todos lados: `m:ss:d`** (Matías, opción A): ideal «3:00:0», cronómetro «0:24:8», tiempos de
      vuelta «3:01:5». **Las diferencias siguen con coma**: error, ± segundos, «+1,5 s».
      Ojo: la página del celu (`recursos/informe-plantilla.html`) y los textos de C++ (`datos.cpp` «tiempo()»)
      también.
- [ ] **Cargar el tiempo por casilleros:** minutos y segundos por separado (cada uno se toca y se escribe de izquierda
      a derecha, con − / +; segundos hasta 59). **Las décimas no se editan** (quedan en 0). En Práctica y en la ficha
      de la carrera. Mostrar muestra antes de programar.
- [ ] Al tocar **Terminar**: ir derecho al **informe**, con el **error total grande arriba** (07-1.1, 2.1).
- [ ] **«IR A LARGAR»** en los botones de Práctica, Carreras y Ficha que solo llevan al crono (07-1.2).
- [ ] «Práctica libre» pasa a **«Práctica»** (Recorrido libre queda adentro de Práctica, con ese nombre).
- [ ] En Carreras, LARGAR gris con el motivo («Se larga el 21 nov») (07-2.2).
- [ ] Sin GPS en carrera: «SIN GPS · seguí por tiempo» en la Sugerida y un pitido al cortarse (07-2.3).
- [ ] Chequeo previo: cámara prendida sin cámara en rojo; «Poco lugar» (07-2.4).
- [ ] Textos: «undefined satélites», banderita «?» en el nombre del informe, línea de práctica de pie cortada,
      «MARCA 1 · solo deja una marca» en el recorrido (07-2.5); hora «--:--» sin hora válida (07-1.4).
- [ ] Compartir con la red propia: mostrar la clave de la wifi (07).
- [ ] Borrar las 10 pantallas viejas que no se usan (Matías: sí) (01-1.1).

## 5. Para después (no entra en la 1.0.27)

- Engranaje de verdad (con muestra antes).
- Copia de la base a Drive **junto con el video y el informe**, cuando se conecta a la wifi favorita (Matías).
- Un archivo por carrera en vez de `betometro.json` único (05, sección 4), en una versión aparte.
- Ordenar `datos.cpp`, `servidor.cpp` e `Informe.qml` (01).
- No: botón Deshacer, carteles grandes, clave distinta por equipo.
