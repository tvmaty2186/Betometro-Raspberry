# Video de la carrera (para más adelante)

Estado: **aprobada por Matías (02/11/2026). Cambio del mismo día: la cámara entra en la imagen nueva (con el actualizador por wifi), no después de la 1.0.0.** Muestra visual: [muestras-video-v1.png](muestras-video-v1.png) (informe con video) y [muestras-video-v2.png](muestras-video-v2.png) (en carrera, solo «REC»). Fuente: [muestras/video.html](muestras/video.html) (`#v1`, `#v2`).

## Qué hace

- Una cámara en el auto graba **toda la carrera**: arranca con la largada y para con «Terminar».
- En el informe, botón **Informe / Video**. Con Video: mitad de la pantalla el video, mitad el informe.
  - Mientras corre el video: el auto se mueve en el mapa, se marca la vuelta en la tabla y se ven velocidad, sugerida y adelanto de ese momento.
  - La línea del video tiene las marcas de cada vuelta (rojo medida, verde libre). Tocar una vuelta de la tabla salta el video ahí.
- **En carrera no se ve el video**: solo un punto rojo «REC» arriba, al lado del GPS. Si la cámara falla, sigue todo igual y el punto se apaga.

## Cómo se hace (técnico)

- **Cámara:** USB que entregue **H.264** ya comprimido (UVC con H.264), 720p a 30 cuadros. Alternativa: Camera Module 3 por la ficha CSI (comprime el chip de la Raspberry; cable plano de hasta 1–2 m). Las webcams que solo dan MJPEG o YUY2 obligan a comprimir en la Pi: **no**.
- **Grabación:** programa aparte (GStreamer: `v4l2src` → `h264parse` → `matroskamux` → archivo), con prioridad baja (`nice 19`, `ionice idle`). Betómetro lo arranca y lo para; nunca espera por él.
- **Sincronía:** al arrancar se anota la hora monotónica del primer cuadro; cada cuadro lleva su hora. Con eso cada marca, muestra del GPS y vuelta cae en un momento exacto del video. Se guarda junto a la carrera: `datos/video/<id carrera>-L<largada>.mkv` y en la carrera `video: {archivo, inicioMs}`.
- **Lugar:** 720p H.264 ≈ 2 GB por hora. En la partición de datos (tarjeta de 32 GB: unas 12 horas). Aviso si queda poco lugar; se pueden borrar videos viejos desde Ajustes.
- **Reproducción:** Qt Multimedia con decodificación por hardware de la Pi. 720p anda; 1080p puede andar a los tirones.
- **Sistema:** volver a prender `CONFIG_MEDIA_SUPPORT` (se apagó en E3 para que el kernel compile más rápido) y agregar UVC (`CONFIG_USB_VIDEO_CLASS`) y a Buildroot GStreamer con `gst1-plugins-good` (v4l2, matroska). Corriente: la cámara toma unos 0,3–0,5 A del USB; con la fuente de 2,5 A alcanza.

## Cámaras (02/11/2026)

| | Modelo | Por qué | Ojo |
|---|---|---|---|
| 1 (la mejor) | **Raspberry Pi Camera Module 3 Wide** (120°, HDR, autofoco) | Oficial. Comprime el chip de la Pi, casi sin procesador. HDR: aguanta sol y sombra. Gran angular para el parabrisas | Cable plano (1 m como mucho, comprar uno largo). En el sistema hace falta `libcamera` |
| 2 | **ELP 1080p con H.264** (familia USBFHD06H / USBFHD04H, lente 2,1 a 2,8 mm, «H.264» en la ficha) | USB: cable largo, va en cualquier lado. Entrega H.264 hecho | Que la ficha diga **H.264** (no solo MJPEG). Calidad de imagen justa |
| ✗ | Logitech C920 / C922 nuevas, webcams genéricas | — | Desde fines de 2018 las Logitech no dan H.264; las genéricas solo MJPEG: la Pi tendría que comprimir |

**Opción barata (02/11/2026):** cualquier webcam USB común que dé **MJPEG** (casi todas: Logitech C270, C920 nuevas, genéricas 1080p). La Raspberry la descomprime y la vuelve a comprimir en H.264 **con su chip de video** (bcm2835-codec: `v4l2jpegdec` → `v4l2h264enc`), sin cargar el procesador. El kernel de la imagen ya trae el driver. **Programado el 05/11/2026** (`betometro-camara grabar --formato mjpeg`): `image/jpeg` → `jpegdec` (procesador) → `videoconvert` → `v4l2h264enc` (chip, 4 Mbit/s) → `h264parse` → mkv. Probado en la Pi el 05/11/2026: con `v4l2jpegdec` (con o sin `v4l2convert`) se traba después del primer cuadro; con `jpegdec` anda (2,5 MB en 5 s a 1280×720). Hace falta `gstreamer1.0-plugins-bad` (h264parse). Betómetro elige primero una cámara H.264 y si no una MJPEG («· MJPEG · lista»). Webcam de Matías: genérica, MJPEG 1920×1080, 1280×720 y 800×600 a 30 cuadros, con micrófono. Falta medir cuánto procesador usa grabando en carrera.
Otras: cámara CSI barata OV5647 con lente de 160° (necesita libcamera y `camera_auto_detect` en config.txt: el gancho de la actualización tendría que actualizar config.txt). Cámara de acción aparte (GoPro, dashcam): graba sola; se sincroniza después con el pitido de la largada (manual).

Prueba al recibirla (USB): `v4l2-ctl --list-formats` tiene que mostrar **H264**.
Fuentes: https://medium.com/@cmeister2/logitech-c920-c922-webcams-no-longer-provide-native-h-264-1e0e4b6a9b74 · https://www.megatone.net/producto/camara-de-seguridad-usb-elp-2mp-1080p-co_MKT0HGGNOC/

## Video en el informe (aprobado por Matías, 05/11/2026)
Muestras: [muestras/video/completo-andando.png](muestras/video/completo-andando.png), [completo-pausa.png](muestras/video/completo-pausa.png),
[completo-editar.png](muestras/video/completo-editar.png) y la vista a medias [muestras-video-v1.png](muestras-video-v1.png).
- **Vista a medias:** en el informe, botón Informe / Video; mitad video, mitad informe. Tocar el mapa o una vuelta lleva el video ahí.
- **Pantalla completa:** doble toque sobre el video (otro doble toque vuelve).
- **Datos encima, en tiras finitas de una línea:** arriba a la izquierda «V3 · MEDIDA» (chip del color de acento); arriba a la
  derecha el adelanto («+0,4 s ATRASADO», rojo atrasado / verde en tiempo); abajo a la izquierda, apiladas: «VUELTA 2:14,6»,
  «DIST. 1.842 / 3.200 m», «VEL. 48 km/h» con barrita contra la sugerida y «SUG. 47,5»; abajo a la derecha la **pista** con un
  punto amarillo donde va el auto (esta caja es más grande, está bien así). Barra de abajo: play/pausa, tiempo, línea con una
  rayita por vuelta (tocarla salta a esa vuelta).
- **Configuración solo en pausa:** en pausa aparece el **engranaje** en la barra. Adentro: prender/apagar cada dato y **mover**
  los datos (mantener apretado → se pone translúcido con borde punteado → arrastrar → imán con los otros datos y los bordes).
  Lo acomodado queda guardado para todos los videos.
- **También en la página del celular** (mismo reproductor y datos), para verlo en el celu o en la tele con Smart View /
  Chromecast, y en el navegador de una Smart TV. Y **en la tele por HDMI** desde la Raspberry.
- **Sincronía:** en la carrera, `videos: [{archivo, largada, desdeLargadaMs, girar}]` (crono.cpp `asociarVideo`). El momento del
  video de un punto de la traza = `desdeLargadaMs + (suma de realMs de las vueltas anteriores de esa largada) + t·1000`.

## Modo tele (HDMI, aprobado por Matías el 05/11/2026)
Muestra: [muestras/video/tele.png](muestras/video/tele.png) (fuente `muestras/video/tele.html`).
- **Botón «Ver en la tele»** en la cabecera del informe: solo si la carrera tiene video y hay tele enchufada.
  Sin tele, todo sigue como siempre.
- **La tele:** el video entero, con los mismos datos y la barra que la pantalla completa. Los datos van en el mismo lugar
  que en la Pi (mismas fracciones guardadas, `videoDatos`); la tele se arma en 1280×720.
- **La Pi es el control:** «EN LA TELE · carrera» y **Salir**; «Ir a la vuelta» (V1…Vn con su tiempo, la actual en amarillo);
  −10 s / play-pausa grande / +10 s; la línea de tiempo con una rayita por vuelta (se arrastra) y «6:28 / 15:20»;
  **«Datos en la tele»**: prender y apagar cada dato. Moverlos se hace en la pantalla completa de la Pi.
- **Tele enchufada sin modo tele:** fondo verde oscuro, el león y «BETÓMETRO» (nunca la marca del auto).
- **Pulsador por cable:** con el video a la vista (a medias, pantalla completa o en la tele) anda / pausa, y se ve un ratito el
  botón grande. Dos apretones a menos de 300 ms cuentan como uno. Sin video a la vista (o en otra pantalla) no hace nada.
- **Técnico:** un solo reproductor (el del informe). En modo tele su salida de video pasa a la ventana de la tele: el video se
  decodifica una sola vez (Pi 3). Archivos: `app/qml/Tele.qml`, `video/VistaVideo.qml` (lo que se ve, en la Pi y en la tele),
  `video/ControlTele.qml`, `video/VentanaTele.qml`, `app/hdmi.cpp`. Pantallas: `raspios/kms.json` (la de 5" es la principal y lleva
  el tacto; HDMI a 1280×720), con `QT_QPA_EGLFS_KMS_CONFIG` en `betometro.service`.
- **Enchufar con Betómetro abierto:** Qt 6.8 (eglfs_kms) arma las pantallas solo al abrir. Si el sistema dice que hay tele
  (`/sys/class/drm/card*-HDMI-A-*/status`) pero Qt no la tiene, el botón aparece igual y explica «Hay que reiniciar Betómetro
  para usar la tele» con **Reiniciar** (nunca en carrera): Betómetro se cierra y systemd lo abre de nuevo a los 2 s.
  Si se desenchufa la tele, la ventana de la tele se cierra y el video vuelve a la Pi.
- **Confirmado en la Pi de Matías (05/11/2026):** `/sys/class/drm/` tiene `card0`, `card0-DSI-1`, `card0-HDMI-A-1` y `card0-Writeback-1`; los nombres de `kms.json` están bien.
- **Probar en la PC:** `QT_QPA_PLATFORM=offscreen:configfile=pantallas.json` con dos pantallas (`DSI-1` y `HDMI-A-1`),
  `--prueba tele`, `tele-datos`, `tele-reiniciar`, `video-pulsador` y `--captura-tele archivo.png`.

## Ajustes → Cámara y micrófono (lista aprobada 02/11/2026)

Muestra: [muestras-camara.png](muestras-camara.png) (fuente `muestras/camara.html`, `#k1`). Se entra desde **Ajustes** (no va en el inicio).

1. Cámara sí / no. 2. Estado («ELP 1080p · H.264 · lista»; también en el Chequeo previo). 3. Vista previa para apuntarla (nunca en carrera). 4. Girar 180°. 5. Empieza: al abrir la carrera (de fábrica) / en la largada / a mano (REC). 6. Termina: N s después de «Terminar» (10 s). 7. Calidad 720p / 1080p. 8. Sonido: de la cámara / **micrófono USB** / sin sonido. 9. Probar: grabación de 10 s; micrófono con barra de nivel y «Grabar 5 s y escuchar». 10. Lugar: horas que entran. 11. Borrar lo más viejo si falta lugar (nunca informes). 12. Videos guardados (borrar por carrera).

## Cuándo

Etapa nueva **E7**, después de la 1.0.0. Primero tiene que andar perfecto la carrera. Aceptación de E7: con la cámara grabando, la prueba de demora de voz y pulsador da igual que sin cámara, y el video queda sincronizado al décimo de segundo con las marcas.

## MP4 automático (05/11/2026)
- Se graba en `.mkv` (si se corta la luz, lo grabado se ve igual). Al terminar de grabar, y al prender por si
  quedó alguno, `betometro-camara mp4` lo pasa a `.mp4`: la imagen no se vuelve a comprimir (rápido) y el sonido
  pasa de Opus a AAC (`avenc_aac`, paquete `gstreamer1.0-libav`), así lo abre también el iPhone.
- Nunca en carrera: si empieza una, se corta y se sigue después. Se escribe en `.mp4.parte`; al terminar bien se
  renombra, se cambia el nombre en la carrera (`Datos::renombrarVideo`) y se borra el `.mkv`. Si falla, queda el `.mkv`.
- Drive espera a que termine (no sube el `.mkv`).

## Reproducir con el chip de la Raspberry (06/11/2026, sin publicar todavía)
- El MediaPlayer de Qt (FFmpeg) descomprime con el procesador en la Pi. `app/video_gst.*` (QML `VideoGst`) usa
  GStreamer `playbin`, que elige `v4l2h264dec` (el descompresor de video de la Pi; se le sube la prioridad).
- Mismos nombres y estados que MediaPlayer. La imagen va al `videoSink` de `ImagenVideo` (YUV → RGB en el procesador,
  a la mitad si sobra tamaño). El sonido por PulseAudio (`gstreamer1.0-pulseaudio`).
- Si no está el chip (PC), GStreamer usa otro descompresor (`avdec_h264` o `openh264dec`).
