# Plan de la 1.0.28 (aprobado por Matías, 07/11/2026)

(La 1.0.27 salió antes, solo con ± segundos y la pantalla de carrera de fábrica nueva.)

Todo lo que entra en la 1.0.28, con las decisiones de Matías. **Matías (07/11): entra todo lo de las
auditorías**, también lo que antes quedaba «para después». Solo queda afuera lo que él dijo que no (al final). El detalle técnico de cada arreglo está en el
informe de su auditoría (`docs/auditorias/NN-…md`, punto entre paréntesis). Programar en este orden, probar con
capturas en Docker (`debian:trixie`, ver `raspios/compilar.sh`), commits `[skip ci]` y publicar al final con su OK.

## Reparto (Matías, 07/11/2026): dos versiones

**1.0.28 «sin sorpresas»** (arreglos por dentro, casi sin cambiar pantallas; se lanza apenas esté):
- Sección 1 entera (seguridad y datos), salvo «un archivo por carrera» y «copia de la base a Drive».
- Sección 2: no publicar si falla una prueba, compilar solo desde `main`, arreglar el job roto, `probar.sh`,
  recorrido en `prueba_crono`, cámara de prueba en GitHub.
- Sección 8 (sistema): **hora del GPS**, `config.txt` en `[all]`, PulseAudio sin HDMI y servicios que sobran
  (arranque más rápido), ssh cerrado en la red propia, vigilante 15 s, borrar paquetes viejos.
- El arreglo de «Salir» desde Práctica (abría «nueva carrera»).

**1.0.29 «pantallas y orden»** (la 29 y la 30 juntas): todo lo demás. Lo visual con muestra aprobada antes:
cargar el tiempo por casilleros, engranaje, botones apagados y la pantalla del simulador.

## 0. Antes de programar: medir en la Pi

- [ ] Pasarle a Matías los comandos para pegar por ssh y anotar los números: peso de la base y procesador de la
      cámara (04, sección 4, pasos A y B); arranque, servicios y lugar ocupado (10, «Para mirar en la Pi»).

## 1. Seguridad y datos (no cambia pantallas)

- [x] Recorrido libre: guardar la traza en curso cada tanto, para no perderla si se corta la luz; en el informe,
      «se cortó» si no llegó a Terminar (05-1.1).
- [x] Informe: no caerse con distancia negativa en la traza, más prueba de bases dañadas en `ctest` (02-1.1).
- [x] Guardado: números a texto con `std::to_chars` (mismo archivo, 10 a 40 veces más rápido) (05-2.1, 04-2.1).
- [ ] Guardado: no copiar la base en el hilo principal; no guardar la base entera cuando no hace falta (04-2.2, 2.3).
      Hecho: ajustes y marcas se juntan 0,3 s antes de guardar. Falta la copia: sale con «un archivo por carrera».
- [x] Avisar si no se puede guardar y «Poco lugar» en el Chequeo previo (05-1.2, UX 2.4). **Sin carteles grandes en
      carrera** (Matías: no).
- [x] Tarjeta llena en carrera: **parar el video y que la carrera siga** (Matías: sí) (06-1.3).
- [x] Cámara colgada o desenchufada: matarla si no termina, poder volver a grabar (02-1.2, 06-1.4).
- [x] `nmcli` / `iw` con tiempo límite y menos chequeos de wifi en carrera (02-1.3).
- [ ] Instalador: no tocar `cmdline.txt` si no cambia, `sync`, `trap` si falla a mitad, `Type=notify` (10-1.2 a 1.4).
- [ ] **Vuelta atrás automática** si la versión nueva no arranca, y avisar en pantalla (Matías: sí) (10-1.1).
- [ ] Borrar los paquetes de actualización viejos (10).
- [ ] Borrar una carrera también borra su video (si ningún otro la usa) (pedido de Matías).
- [ ] **Copia de la base a Drive junto con el video y el informe**, cuando se conecta a la wifi favorita (Matías).
- [ ] **Un archivo por carrera** en vez de `betometro.json` único, sin romper el formato de la app (05, sección 4).
      Si se hace, el punto 05-2.2 (escribir sin el árbol `Json`) no hace falta.
- [x] Números del JSON sin depender del idioma del sistema (`std::to_chars` / `from_chars`) (02-2).
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
- [ ] Probar compilar en ARM de verdad (`ubuntu-24.04-arm`) para bajar de 20 a ~5 min (10-2.1). ARM **solo al
      publicar**; en cada subida, solo x86 (Matías).
- [ ] Arreglar el job «Programa Qt» en `debian:trixie` x86 y correr `ctest` entero (datos, crono con video,
      videos, drive) (09-2.b).
- [ ] `probar.sh` falla si una captura no sale (09-2.c).
- [ ] Recorrido libre en `prueba_crono` (09-2.d).
- [ ] Capturas comparadas contra referencias: primero Avispón h/v, después los 4 temas (09-1.2, P2).
- [ ] Dobles de wifi y cámara para que corran en GitHub (09-1.5, P4). La cámara de prueba necesita
      `gstreamer1.0-plugins-ugly` (x264enc): con eso `prueba_crono` y `prueba_videos` pasan en Docker.
- [ ] Una biblioteca `betometro_base` para que las pruebas no recompilen el programa (01-2.2).
- [ ] `-Wall -Wextra` también en el programa y la cámara (02-2).
- [ ] **Borrar Buildroot** (Matías, 07/11): la carpeta `sistema/` (4,9 MB) e `imagen.yml`. Queda en el historial de git
      por si algún día se quiere volver (10-3).
- [ ] Borrar las fotos de referencia viejas de `docs/capturas/` (02/11); las nuevas salen de las capturas comparadas (09-1.2).

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
- [x] Wifi en carrera cada 30 s (04-3.3).
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
- [x] Chequeo previo: cámara prendida sin cámara en rojo; «Poco lugar» (07-2.4).
- [ ] Textos: ~~«undefined satélites»~~ (hecho), banderita «?» en el nombre del informe, línea de práctica de pie cortada,
      «MARCA 1 · solo deja una marca» en el recorrido (07-2.5); hora «--:--» sin hora válida (07-1.4).
- [ ] Compartir con la red propia: mostrar la clave de la wifi (07).
- [ ] Borrar las 10 pantallas viejas que no se usan (Matías: sí) (01-1.1).
- [ ] **Engranaje de verdad** en el inicio, `TIcono.qml`, `ajustes.svg` y el video (`Engranaje.qml`). **Mostrar 2 o 3
      dibujos antes** (08-1).
- [ ] Modo sol, Cámara: el aviso «No se encontró la cámara» se ve (08-2).
- [ ] Carrera ya corrida: el botón grande dice «Ver informe» y abre el informe (Matías: sí) (08-4).
- [ ] Informe en Deportivo y Cabina: el título no se pisa con Vel./Acel./Adel. (08-5).
- [ ] Deportivo: «Apagar» rojo; «Actualizar» con borde rojo y fondo oscuro (Matías) (08-6).
- [ ] Botones apagados con estilo propio, no 45 % de opacidad (08-7). Muestra antes.
- [ ] **Colores fijos pasados a `Tema`**: rojo de peligro, azul de lluvia (con su versión sol), verde de Drive,
      etiquetas de la cámara (08-8).
- [ ] Íconos nuevos: termómetro, rayo (tensión), «HD» (calidad); GPS y Localidad distintos (08-9).
- [ ] Cabina: hora de la barra con B612 normal (08-10); textos que no entran (08-11).
- [ ] Letras de menos de 21 px y cursiva fija (08-12); letras blancas en «Apagar» en modo sol (08-14).
- [ ] Informe: columna de velocidad alineada y flecha de volver de 64 px; ficha de pie sin hueco (08-15, 07-3.5).
- [ ] Título cromado en la barra, solo Avispón (Matías: sí, sin muestra) (08-13).
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

## 6. Respuestas de Matías (07/11/2026)

1. Fecha mal porque la Pi no tenía hora: **tomar siempre la hora de algún lado, del GPS o de internet** (wifi).
   - [ ] Hora del sistema desde el GPS en cuanto haya lectura válida, y por internet si hay wifi (07-1.4).
   - **Si no encuentra la hora, no pasa nada malo: LARGAR nunca se bloquea** y la carrera se usa normal.
2. Si la próxima carrera es hoy: **el botón Carreras queda como ahora** (abre la lista).
3. Fotos de las pantallas en las pruebas: **Matías no quiere verlas**. Si un cambio es a propósito, Claude acepta
   la foto nueva solo.
4. Compilación en ARM: **solo al publicar**; en cada subida, solo la de x86.
5. Carrera simulada en GitHub: **sin circuito real**, uno de ejemplo. Además, **simulador en el Chequeo previo**
   (ver abajo).
6. ssh: **con clave escrita, como ahora**.
7. Sistema de la Raspberry (`apt full-upgrade`): **no tocarlo nunca**.
8. Carrera ya corrida: el botón grande de la ficha dice **«Ver informe»** y lo abre (08-4).
9. Deportivo: **«Apagar» rojo y «Actualizar» con borde rojo** (fondo oscuro) (08-6).
10. Título cromado en la barra, solo Avispón: **sí, sin muestra** (08-13).

## 7. Simulador de carrera en el Chequeo previo (pedido de Matías, 07/11/2026)

- [ ] En el Chequeo previo, una fila **«Simulador»**: corre una carrera simulada corta por detrás (GPS simulado,
      largar, vueltas, MARCAR, Terminar, guardado e informe), sin tocar las carreras de verdad.
- [ ] Revisa todos los errores y muestra el resultado:
  - **Verde «Simulador OK»**: pasó todo.
  - **Naranja, falla leve**: «Revisar». Muestra el detalle y el **QR para mandar el reporte**.
  - **Rojo, necesita atención**: «Hay errores en la simulación que hay que atender. Comunicate con el
    desarrollador». Muestra el detalle y el **QR para mandar el reporte**.
- [ ] El QR abre en el celu el reporte (en «Betómetro en la red» → «Reporte de errores») para mandárselo al
      desarrollador.
- [ ] Qué es leve y qué es grave: definirlo al programar (por ejemplo, grave: no guarda, el informe no abre,
      los tiempos dan mal; leve: tarda más de lo normal).
- [ ] Mostrar muestra de la pantalla antes de programar.

## 8. Auditoría 11: lo que faltó (Matías: sí a todo, 07/11/2026)

Detalle en [auditorias/11a-programa.md](auditorias/11a-programa.md) (A) y [auditorias/11b-sistema.md](auditorias/11b-sistema.md) (B).
Medido en la Pi el 07/11 (tabla al final de 11b).

Sistema operativo:
- [ ] **ssh cerrado en la red propia** y límite de intentos: nftables con pocas reglas + `sshd_config.d`
      (`MaxAuthTries 3`, `AllowUsers beto`). En casa se entra igual, con clave (B-1.1).
- [ ] **Hora: la Pi estaba 11 h atrasada** porque la wifi no tenía internet: arranca con la hora del apagado y nada
      la corrige. **La hora del GPS es lo primero del sistema.** Mostrar en el Chequeo previo de dónde salió la hora
      (GPS, internet o sin hora) (Pi, 07/11).
- [ ] Arranque 12 s más rápido: PulseAudio sin la tarjeta HDMI (hoy tarda 10 s y falla) y apagar `cloud-init`,
      `NetworkManager-wait-online`, `rpi-eeprom-update`, `udisks2`, `sshswitch`. **Dejar `avahi`** (Pi, 07/11).
- [ ] Quitar el núcleo de la Pi 5 (`linux-image-rpi-2712`) (Pi, 07/11).
- [x] `sudo` pide la clave: confirmado en la Pi (07/11).
- [ ] **Hora del sistema desde el GPS** cuando internet no la puso, sin tocar el cronómetro (B-1.2). Corregir
      `docs/diseno-sistema.md`, que dice que ya se hacía.
- [ ] Zona horaria Argentina y nombre `betometro` en el instalador (B-2.4).
- [ ] `config.txt`: escribir las líneas dentro de `[all]` (B-1.3).
- [ ] PulseAudio con `Type=notify`, Betómetro espera al sonido (B-1.4).
- [ ] Con wifi, el instalador actualiza **solo** certificados, zona horaria, `sshd`, `dnsmasq` y `wpa_supplicant`
      (nunca núcleo, Qt, Mesa ni GStreamer) y guarda las versiones para el reporte (Matías: sí) (B-2.1).
- [ ] Red propia: crearla una vez y después solo prenderla (`wifi.cpp:138`) (B-2.2).
- [ ] NetworkManager: sin ahorro de energía del wifi y sin MAC al azar, si la Pi muestra fallas (B-2.3).
- [ ] Vigilante en 15 s (lo máximo del chip) y `bluetooth.service` apagado (B-3).
- [ ] Anotar en `docs/raspios.md` que trixie pasa a archivo hacia 2028; fijar la versión de `edge-tts` (B-3).

Programa:
- [ ] **Clima en carrera sin internet:** no reintentar el nombre del lugar 10 veces por segundo (A-1.1).
- [ ] Simulador de GPS que pierda la señal como el NEO-M8N real (A-1.2). El **simulador del Chequeo previo va
      adentro del programa** (GPS y pulsador simulados) (Matías: sí).
- [ ] Reportes: avisar si la llave de GitHub venció; no perder el medio de un reporte largo; no repetir lo ya
      mandado; `reportes.token` con permiso 0600 (A-1.3, A-3). El repo `Betometro-reportes` es privado (Matías).
- [ ] **Voz guardada en el repo** (los WAV), para no depender de edge-tts ni de la caché de 7 días de GitHub
      (Matías: sí) (A-1.4).
- [ ] Caché del clima con tope (A-2.1); prueba con un JSON real de Open-Meteo (A-3).
- [ ] Si falta un audio, que quede en el reporte (A-2.2).
- [ ] `despertar-touch`: dejar rastro si demora (A-2.3).
- [ ] `/control/accion` solo por POST (A-2.4).
- [ ] Página Drive del celu sin mostrar el mail de Google (A-2.5).
- [ ] QR: anotar la versión de `qrcodegen` y atajar el error (A-2.6).
- [ ] Control de la tele: no fallar si la tele termina mientras se toca (A-3).
- [ ] Drive: borrar `.drive-curl` al arrancar (A-3).
- [ ] Arranque: que «no apagues» se vea mientras se rearma la foto de arranque (A-3).

## No entra (Matías dijo que no)

- Botón Deshacer, carteles grandes en carrera, clave distinta por equipo.
