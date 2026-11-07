# Auditoría 06 — Video (07/11/2026)

Hecha sin la Raspberry: leí el código y medí en la PC (Docker, GStreamer 1.26). Lo que depende del chip de la Pi
(`v4l2h264enc`, `v4l2h264dec`) es revisión de código, no medición. Marco **[medido]** (en la PC) y **[estimado]** (Pi).
No repito lo que ya está en las auditorías 02 (la grabación que puede quedar trabada al cerrar) y 04 (medir el
procesador de la cámara en la Pi).

**En corto:** la cadena de grabación y el reproductor están bien pensados. Encontré **dos cosas rotas**: el video
queda **desincronizado medio segundo** si graba con sonido, y **480p falla** con una webcam que no da 480p (la de
Matías). Después, cosas chicas que mejoran mucho con poco trabajo.

## 1. Lo más grave

### 1.1 Con sonido, el video queda corrido respecto de los datos (medio segundo o más)
- **Qué encontré:** `betometro_camara.cpp:115-135` avisa `primerCuadro` = hora del **primer cuadro de la cámara**, y
  `crono.cpp:60` guarda `desdeLargadaMs = largada − primerCuadro`. El reproductor (`Reproductor.qml:29`, la página del
  celu y `informe.cpp:247`) asume que el **segundo 0 del archivo es ese primer cuadro**. No es así cuando hay sonido:
  el micrófono empieza enseguida y la webcam tarda (0,3 a 1 s en largar el primer cuadro). El archivo empieza con el
  **primer sonido**, y el primer cuadro queda más adelante.
- **[medido]** Armé un .mkv como lo hace el programa (video a 0,70 s, sonido a 0,05 s): en el .mkv el primer cuadro cae
  en la posición **0,643 s**, no en 0; después de pasarlo a MP4, igual (**0,643 s**). Sin sonido, los dos formatos
  empiezan en el primer cuadro (0) y todo cierra. `matroskamux offset-to-zero=true` no lo arregla (corre al sonido, no
  al video).
- **Riesgo:** alto. Cada marca, vuelta y lectura del GPS se ve **adelantada** tanto como tardó la cámara en arrancar
  (medio segundo típico, hasta 1 s). E7 pide sincronía al décimo.
- **Propongo** (en `betometro_camara.cpp`, modo `grabar`): tirar el sonido que llega **antes del primer cuadro** (un
  probe en la rama de audio que descarta buffers con PTS menor al del primer cuadro) y poner `offset-to-zero=true` en
  `matroskamux`. Así el archivo empieza en el primer cuadro en .mkv y en .mp4, con error ≤ 20 ms (un buffer de audio).
  Además, avisar `primerCuadro` recién con el primer cuadro **que se graba**: hoy el probe de la hora (`:324`) corre
  antes que el que tira los JPEG cortados (`:325`), y si el primero viene roto se anota una hora que no está en el video.
- **Prueba en la Pi** (con sonido prendido), después de grabar 5 s desde Ajustes → Cámara → Probar:
  ```bash
  f=$(ls -t /var/lib/betometro/videos/prueba-*.m* | head -1); gst-launch-1.0 -v filesrc location=$f ! decodebin name=d d. ! identity silent=false ! fakesink 2>&1 | grep -m1 -o "pts: [0-9:.]*"
  ```
  Si el primer `pts` no es `0:00:00.0…`, ese número es cuánto está corrido el video.
- **Cuesta:** chico.

### 1.2 480p está roto con una cámara que no da 480p (la de Matías da 1080p, 720p y 800×600)
- **Qué encontré:** con `--escala 848x480`, `compresorMjpeg` (`betometro_camara.cpp:220-223`) arma
  `videoscale ! video/x-raw,width=848,height=480 ! video/x-raw,format=I420 ! v4l2h264enc`: **dos filtros de caps
  seguidos**, que GStreamer no acepta. **[medido]**: `gst_parse_launch` falla con «no element "video"». La grabación
  no arranca (`{"error":…}` → el REC se apaga y queda «Cámara» gris). En la PC no se ve porque `--prueba` usa otra
  cadena (`:261-263`). Si Matías vio 480p andando (PENDIENTES), su cámara debe dar algún 640×480; igual está mal.
- **Propongo:** un solo caps: `videoscale ! video/x-raw,format=I420,width=848,height=480`. Y mejor todavía, ver 2.1.
- **Cuesta:** chico (una línea).

### 1.3 Si se llena la tarjeta en carrera, se corta el video y puede fallar el guardado de la carrera
- **Qué encontré:** `camara.cpp:308-319` hace lugar **solo al empezar** (si hay menos de 2 GB libres, borra hasta
  tener 4). Grabando no se mira nada. 720p son ~1,8 GB por hora: con 2,5 GB libres y una carrera de 2 h, la tarjeta se
  llena. `filesink` da error, la grabación se corta, y `betometro.json` (misma partición) **no se puede escribir**:
  se pierden vueltas.
- **Propongo:** usar el aviso `cuadros` que ya llega cada 30 s (`camara.cpp:103`) para mirar el lugar libre y parar
  la grabación (ordenada, con `terminate()`) si quedan menos de 300 MB. Y en `hacerLugar` pedir lugar según las
  horas previstas (2 GB solo alcanzan para una hora).
- **Cuesta:** chico.

### 1.4 Cámara desenchufada o colgada en carrera: no se vuelve a grabar; si se cuelga, nadie se entera
- **Qué encontré:** si la webcam se desenchufa, `v4l2src` da error → `alBus` corta (`betometro_camara.cpp:77-83`),
  el .mkv queda completo (es «streamable») y Betómetro muestra «Cámara» gris (`Carrera.qml:118-120`). Bien. Pero
  **no se intenta de nuevo** si la vuelven a enchufar. Y si la cámara **se cuelga sin error** (pasa con webcams USB
  por el cable o la corriente), `v4l2src` espera para siempre: el REC sigue rojo y no se graba nada.
- **Propongo:** (a) en `Camara`, si entre dos avisos `cuadros` (30 s) el número no sube, matar al ayudante y volver a
  arrancar; (b) en carrera, si la grabación se cortó, reintentar cada 10 s con otro archivo. La carrera ya soporta
  varios videos por largada (`informe.cpp:258-266` elige el que empezó antes de cada vuelta): solo hay que llamar a
  `empezarVideo()` + `videoPendiente_ = true` de nuevo (`crono.cpp:49-62`).
- **Cuesta:** medio.

## 2. Lo que más mejora con poco trabajo

### 2.1 480p: tomar 800×600 de la cámara en vez de achicar 720p con el procesador
- **Qué encontré:** hoy 480p con webcam MJPEG = descomprimir 720p + `videoscale` por procesador
  (`betometro_camara.cpp:220`). **[medido]** en un núcleo, 300 cuadros: 720p→480p con `videoscale` **0,66 s**;
  720p solo **0,44 s**; **800×600 directo 0,26 s** (2,6 veces menos que el 480p de hoy, 40 % menos que 720p).
  Sería 4:3 (como la tele vieja), no 16:9; el reproductor respeta la proporción.
- **Propongo:** agregar 800×600 a la lista de tamaños de 480p (`camara.cpp:286`) y bajar el bitrate a 2 Mbit/s.
  Alternativa a probar en la Pi: achicar con el ISP del chip (`v4l2convert`, cero procesador) después de `jpegdec`
  (la combinación probada el 05/11 era con `v4l2jpegdec`, no esta):
  ```bash
  gst-launch-1.0 -e v4l2src device=/dev/video0 num-buffers=150 ! image/jpeg,width=1280,height=720,framerate=30/1 ! jpegdec ! v4l2convert ! video/x-raw,format=I420,width=848,height=480 ! v4l2h264enc extra-controls=controls,video_bitrate=2000000,repeat_sequence_header=1 ! video/x-h264,level=\(string\)4 ! h264parse ! matroskamux ! filesink location=/tmp/isp.mkv && ls -la /tmp/isp.mkv
  ```
  Si da más de 1 MB y se ve bien (`gst-play-1.0 /tmp/isp.mkv`), vale la pena.
- **Cuesta:** chico.

### 2.2 Colas de 3 s antes del archivo: una traba de la tarjeta pierde cuadros
- **Qué encontré:** `betometro_camara.cpp:272` pone `queue max-size-time=3000000000` para el video y una `queue`
  de fábrica (1 s) para el sonido. Las tarjetas SD se traban 1 a 3 s al escribir (más mientras `betometro.json`
  se reescribe con `fsync`, auditoría 04). Si la cola se llena, el compresor y la cámara se frenan: cuadros perdidos
  (no se rompe la sincronía porque la hora va en cada cuadro, pero hay saltos).
- **Propongo:** `queue max-size-time=10000000000 max-size-buffers=0 max-size-bytes=0` en las dos ramas (10 s de
  720p son ~5 MB de memoria). Nada de `leaky`: tirar cuadros comprimidos rompe la imagen hasta el siguiente cuadro
  clave.
- **Cuesta:** chico.

### 2.3 MP4: `faststart=true` escribe el archivo dos veces
- **Qué encontré:** `betometro_camara.cpp:295` usa `mp4mux faststart=true`: arma el MP4 en un archivo temporal y
  lo vuelve a copiar entero (2 GB leídos + 4 GB escritos por hora de video). Sirve para empezar a ver antes de bajar
  todo, pero el servidor ya manda **por pedazos con `Range`** (`servidor.cpp:222-258`) y los navegadores buscan el
  índice al final con eso. En la Pi el reproductor lee el archivo local.
- **Propongo:** `faststart=false`. Mitad de escrituras en la tarjeta y la mitad de tiempo. Probar una vez en el celu
  (Chrome y iPhone) que el video sigue arrancando rápido por la página.
- **Cuesta:** chico.

### 2.4 Cuadros clave: dejar 1 por segundo
- **Qué encontré:** no se fija `h264_i_frame_period`; el chip usa 60 cuadros (2 s). Saltar a una vuelta
  (`video_gst.cpp:277`, búsqueda exacta) descomprime hasta 2 s de video antes de mostrar; en el celu lo mismo.
- **Propongo:** `extra-controls=controls,video_bitrate=…,h264_i_frame_period=30,repeat_sequence_header=1`. Cuesta
  un poco de bitrate (5 a 10 %); con 4 Mbit/s a 720p sobra.
- **Cuesta:** chico.

### 2.5 Hacer lugar justo en la largada
- **Qué encontré:** con «Empieza: en la largada», `Camara::empezar` (`camara.cpp:274`) borra videos viejos
  **en el hilo principal, en el instante de la largada** (borrar un .mp4 de 2 GB en ext4 sobre SD tarda hasta medio
  segundo): la voz y la pantalla se pueden trabar justo ahí.
- **Propongo:** hacer lugar al abrir la carrera (`Crono::preparar`), sea cual sea el ajuste de «Empieza».
- **Cuesta:** chico.

## 3. El resto

| | Qué encontré | Propongo | Cuesta |
|---|---|---|---|
| 3.1 | Prioridad real de la grabación: `nice(5)` del hijo (`camara.cpp:92`) **más** `nice -n 15` (`:295`) = 19, no 5 como dice `docs/raspios.md`. Con el procesador al máximo en carrera puede quedarse sin tiempo y perder cuadros. | Un solo `nice` (10) y corregir el doc. `ionice` no sirve en la SD (sin planificador BFQ). | chico |
| 3.2 | `hacerLugar` puede borrar el .mkv que se está pasando a MP4; el .mp4 queda huérfano (la carrera ya lo olvidó). | Saltear `mp4Entrada_` o matar la conversión antes. | chico |
| 3.3 | Al cortarse por error, el ayudante no manda el conteo de cuadros (`alBus:77`): el reporte no dice cuántos JPEG vinieron rotos. | Llamar `alContar` también en ERROR. | chico |
| 3.4 | `do-timestamp=true` (`:267`) marca la hora cuando el cuadro **sale del driver**, con el ayudante en prioridad baja: en carrera puede sumar decenas de ms de ruido. `uvcvideo` da la hora de captura en cada cuadro. | Probar sin `do-timestamp` (v4l2src usa la hora del driver si es monotónica). Solo si 1.1 no alcanza para el décimo. | chico |
| 3.5 | La webcam MJPEG manda JPEG 4:2:2; `jpegdec ! videoconvert` lo pasa a I420 por procesador. **[medido]** el `videoconvert` cuesta 5 % del total: no vale la pena tocarlo. El grueso es `jpegdec` (**[estimado]** 35–55 % de un núcleo de la Pi a 720p/30). | Nada. Si en la Pi pasa de un núcleo: 15 cuadros (auditoría 04) o 2.1. | — |
| 3.6 | Reproducción: cada cuadro 720p sube 1,4 MB de texturas por `Mesa vc4` (el procesador las reordena). **[estimado]** 15–25 % de un núcleo mirando video en la Pi; no es en carrera. | Medir con `top` mientras se ve un video. Si molesta, mostrar a 30 cuadros solo en pantalla completa. | — |
| 3.7 | Código muerto: la rama de 15 cuadros para 1080p (`:268`) ya no se usa desde la 1.0.24. | Sacarla o dejarla documentada. | chico |
| 3.8 | `Camara::~Camara` no espera a `mp4_`; queda un `.mp4.parte` que se pisa al reintentar. Inofensivo. | Nada. | — |

**Lo que está bien y no tocaría:** reloj monotónico forzado en el tubo (`:317-319`); MKV «streamable» (se corta la luz
y se ve igual); JPEG cortados salteados; MP4 sin recomprimir la imagen; `v4l2h264dec` con `capssetter` y el shader
YUV en `ImagenVideo` (el tubo nunca espera a la pantalla); `alsasrc` esclavo del reloj del sistema (no deriva en
carreras largas); `Range` en el servidor.

## ¿Otra cámara?
La webcam MJPEG anda, pero obliga a la Pi a descomprimir cada cuadro con el procesador (lo único pesado de toda la
cadena de video). `docs/video.md` ya lo dice: una **ELP USB con H.264** (ficha con «H264» en `v4l2-ctl --list-formats`)
deja el procesador libre y usa la cadena que ya existe (`--formato h264`). La Camera Module 3 por CSI es mejor imagen
pero pide `libcamera` y tocar `config.txt` desde el actualizador: más trabajo. Yo no compraría nada hasta medir en la
Pi una carrera entera con la webcam (auditoría 04): si `betometro-camara` queda abajo de un núcleo y la temperatura
no se dispara, con esta cámara alcanza.

## Las 3 que haría primero
1. **1.1** Sonido corrido: tirar el audio anterior al primer cuadro + `offset-to-zero`, y medir en la Pi con el comando.
2. **1.2** Arreglar los caps de `--escala` (una línea) y, con eso, **2.1** (800×600 para 480p).
3. **1.3 + 1.4** Lugar libre y cuadros que no llegan: mirar los dos en el aviso de cada 30 s.

## Preguntas para Matías
- 480p: ¿te sirve en **800×600** (4:3, un poco cuadrado) a cambio de que la Pi trabaje menos de la mitad? Si preferís
  16:9, probamos el achicado con el ISP del chip (comando en 2.1).
- Si la cámara se corta en carrera y la volvés a enchufar, ¿querés que **siga grabando sola** en otro archivo? (1.4)
- Cuando queda poco lugar en carrera, ¿preferís que **pare el video** (y la carrera sigue) o que borre videos viejos
  mientras graba? Yo pararía el video: nunca arriesgar la carrera.
- ¿Querés que el video de prueba de 10 s avise si el sonido quedó corrido (prueba de 1.1 hecha sola)?
