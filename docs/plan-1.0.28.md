# Plan de la 1.0.28 (aprobado por Matías, 07/11/2026)

(La 1.0.27 salió antes, solo con ± segundos y la pantalla de carrera de fábrica nueva.)

Todo lo que entra en la 1.0.28, con las decisiones de Matías. **Matías (07/11): entra todo lo de las
auditorías**, también lo que antes quedaba «para después». Solo queda afuera lo que él dijo que no (al final). El detalle técnico de cada arreglo está en el
informe de su auditoría (`docs/auditorias/NN-…md`, punto entre paréntesis). Programar en este orden, probar con
capturas en Docker (`debian:trixie`, ver `raspios/compilar.sh`), commits `[skip ci]` y publicar al final con su OK.

## 0. Antes de programar: medir en la Pi

- [ ] Pasarle a Matías los comandos para pegar por ssh y anotar los números: peso de la base y procesador de la
      cámara (04, sección 4, pasos A y B); arranque, servicios y lugar ocupado (10, «Para mirar en la Pi»).

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
- [ ] **Copia de la base a Drive junto con el video y el informe**, cuando se conecta a la wifi favorita (Matías).
- [ ] **Un archivo por carrera** en vez de `betometro.json` único, sin romper el formato de la app (05, sección 4).
      Si se hace, el punto 05-2.2 (escribir sin el árbol `Json`) no hace falta.
- [ ] Números del JSON sin depender del idioma del sistema (`std::to_chars` / `from_chars`) (02-2).
- [ ] Al prender: no avisar «base dañada» si lo roto es `.nuevo`, y borrar el `.danado` viejo (05-3).
- [ ] Base leída sin carreras y `.anterior` con carreras: avisar y usar `.anterior` (05-3).
- [ ] Pendrive (`mount`/`umount`), espera de la cámara y `journalctl` al prender: con tiempo límite y sin trabar
      la pantalla (02-1.4, 10-3).
- [ ] Descarga de la actualización con tiempo límite (02-3).
- [ ] Servidor web: tiempo límite para pedidos a medias y no juntar de más (02-3).
- [ ] Nombre del archivo del pendrive sin pasar por el shell (10-3).
- [ ] Instalador: no pisar la copia buena si se instala la misma versión (10-3).
- [ ] Blindar el servicio sin dejar de ser root (`ProtectSystem=full`, `ProtectHome=yes`, `PrivateTmp=yes`),
      probándolo en la Pi (10-1.5 b).
- [ ] Apagar los servicios que sobran en el arranque (`cloud-init`, `ModemManager`…), según lo medido (10-2.5).
- [ ] Firmar los paquetes de actualización (opcional, medio) (10-3).

## 2. GitHub y pruebas

- [ ] **No publicar si falla una prueba** (Matías: sí). Compilar con las pruebas y arreglar el job roto (09-1.1, 2.a–c).
- [ ] **Compilar solo desde `main`** (Matías: sí). Que un push no cancele una compilación con `[publicar]` (10-2.2).
- [ ] Carrera y recorrido libre de punta a punta con `simular_neo` en las pruebas (09 P3).
- [ ] Probar compilar en ARM de verdad (`ubuntu-24.04-arm`) para bajar de 20 a ~5 min (10-2.1).
- [ ] Arreglar el job «Programa Qt» en `debian:trixie` x86 y correr `ctest` entero (datos, crono con video,
      videos, drive) (09-2.b).
- [ ] `probar.sh` falla si una captura no sale (09-2.c).
- [ ] Recorrido libre en `prueba_crono` (09-2.d).
- [ ] Capturas comparadas contra referencias: primero Avispón h/v, después los 4 temas (09-1.2, P2).
- [ ] Dobles de wifi y cámara para que corran en GitHub (09-1.5, P4).
- [ ] Una biblioteca `betometro_base` para que las pruebas no recompilen el programa (01-2.2).
- [ ] `-Wall -Wextra` también en el programa y la cámara (02-2).
- [ ] Sacarle el disparo a `imagen.yml` (Buildroot, sin usar) (10-3).

## 3. Rendimiento

- [ ] Sacar la animación de la barrita (60 → 20 cuadros por segundo en carrera), la aguja del inicio y el borde que
      late debajo del control de la tele (03-1.1).
- [ ] Listas del video que se rearman 10 veces por segundo → elementos fijos; aliviar el modo tele (03-1.2, 1.3).
- [ ] Crear solo el tablero que se usa (vueltas o recorrido) (03-2.1); un solo fondo (03-2.2); foto del inicio con
      `sourceSize` (03-2.3); juntar capas de cada caja (03-2.4).
- [ ] 480p en **800×600** con la cámara de Matías (Matías: sí), arreglando `--escala` (06-1.2, 2.1).
- [ ] Sonido del video en sincronía (06-1.1).
- [ ] Video: colas de 10 s antes del archivo (06-2.2), MP4 sin `faststart` (06-2.3), un cuadro clave por segundo
      (06-2.4), hacer lugar al abrir la carrera y no en la largada (06-2.5).
- [ ] Video, el resto: un solo `nice` (10) (06-3.1), no borrar el video que se pasa a MP4 (06-3.2), conteo de
      cuadros también si falla (06-3.3), sacar la rama vieja de 1080p (06-3.7).
- [ ] Relojes del cronómetro prendidos solo con una carrera preparada (02-2).
- [ ] GPS con puerto fijo: apagar el aviso si se desenchufa (02-2).
- [ ] Traza larga: `std::deque` en vez de borrar el primero (02-3, 04-3.5).
- [ ] Wifi en carrera cada 30 s (04-3.3).
- [ ] Drive: no armar el informe de todas las carreras en el hilo principal (04-3.6).
- [ ] Modo tele: contar vueltas en C++, sin armar el informe de cada carrera (04-3.7, 01-2.4).
- [ ] Anillo de la cuenta y franja de la Sugerida sin repintar de más (03-3).
- [ ] `Datos.cfg()` fuera de los enlaces de la carrera; león y vista de la cámara con `sourceSize` (03-3).

## 4. Pantallas y uso (decididos por Matías)

- [x] ± segundos grande (`WDelta.qml`) y nueva pantalla de fábrica (`Armado.qml`): acostada ± arriba a la derecha
      y la Sugerida al lado de la velocidad; de pie ± abajo al lado de la última vuelta. **Salió en la 1.0.27.**
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
- [ ] **Engranaje de verdad** en el inicio, `TIcono.qml`, `ajustes.svg` y el video (`Engranaje.qml`). **Mostrar 2 o 3
      dibujos antes** (08-1).
- [ ] Modo sol, Cámara: el aviso «No se encontró la cámara» se ve (08-2).
- [ ] «FINALIZADA» entra en su botón (08-4). Pregunta: ¿«Ver informe» en una carrera ya corrida?
- [ ] Informe en Deportivo y Cabina: el título no se pisa con Vel./Acel./Adel. (08-5).
- [ ] Deportivo: «Actualizar» y «Apagar» no los dos rojos (08-6). Pregunta: ¿cuál cambia?
- [ ] Botones apagados con estilo propio, no 45 % de opacidad (08-7). Muestra antes.
- [ ] **Colores fijos pasados a `Tema`**: rojo de peligro, azul de lluvia (con su versión sol), verde de Drive,
      etiquetas de la cámara (08-8).
- [ ] Íconos nuevos: termómetro, rayo (tensión), «HD» (calidad); GPS y Localidad distintos (08-9).
- [ ] Cabina: hora de la barra con B612 normal (08-10); textos que no entran (08-11).
- [ ] Letras de menos de 21 px y cursiva fija (08-12); letras blancas en «Apagar» en modo sol (08-14).
- [ ] Informe: columna de velocidad alineada y flecha de volver de 64 px; ficha de pie sin hueco (08-15, 07-3.5).
- [ ] Título cromado en la barra, solo Avispón (08-13). Pregunta, con muestra.
- [ ] Borrar: una sola forma de confirmar (lista y ficha) (07-3.5).
- [ ] Informe: avisar si el video quedó cortado porque se desenchufó la cámara (07-3.3).

## 5. Ordenar el código (sin cambiar lo que se ve)

- [ ] Partir `datos.cpp` (hace cinco cosas) (01-1.2).
- [ ] Sacar las 3 páginas web de adentro de `servidor.cpp` a archivos (01-1.3).
- [ ] Partir `Informe.qml` (seis pantallas en una) (01-1.4).
- [ ] Formatos repetidos a `Formato.qml` (01-2.1). Juntarlo con los tiempos «m:ss:d».
- [ ] Plantilla del informe: marcar los bloques propios y el video en `recursos/informe-video.js` (01-2.3).
- [ ] Objetos globales como singletons de Qt 6, después de borrar las pantallas viejas (01-3.1).
- [ ] `plan-ingenieria.md` que diga cómo están las carpetas hoy (01-3.3).
- [ ] `Crono.estado` en datos sueltos, solo si lo medido en la Pi muestra lentitud (01-3.2, 03-3).

## 6. Preguntas que faltan contestar

1. Si la carrera quedó con fecha de otro día (la Pi sin hora): ¿bloquear LARGAR como la app o dejar con un segundo toque? (07)
2. Carreras: si la próxima es hoy, ¿el botón del inicio va directo al crono (2 toques)? (07-3.1)
3. ¿Ver la imagen de antes y después cuando cambia una pantalla a propósito, o alcanza con el aviso? (09)
4. ¿El ARM de GitHub solo cuando se publica? (09)
5. ¿Hay un circuito de verdad para la carrera simulada de las pruebas? (09)
6. ¿Entrás por ssh con clave escrita o con llave? (10)
7. ¿Un `apt full-upgrade` en casa de vez en cuando, o no tocar el sistema? (10)

## No entra (Matías dijo que no)

- Botón Deshacer, carteles grandes en carrera, clave distinta por equipo.
