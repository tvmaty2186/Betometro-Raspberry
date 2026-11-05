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

## Ajustes → Cámara y micrófono (lista aprobada 02/11/2026)

Muestra: [muestras-camara.png](muestras-camara.png) (fuente `muestras/camara.html`, `#k1`). Se entra desde **Ajustes** (no va en el inicio).

1. Cámara sí / no. 2. Estado («ELP 1080p · H.264 · lista»; también en el Chequeo previo). 3. Vista previa para apuntarla (nunca en carrera). 4. Girar 180°. 5. Empieza: al abrir la carrera (de fábrica) / en la largada / a mano (REC). 6. Termina: N s después de «Terminar» (10 s). 7. Calidad 720p / 1080p. 8. Sonido: de la cámara / **micrófono USB** / sin sonido. 9. Probar: grabación de 10 s; micrófono con barra de nivel y «Grabar 5 s y escuchar». 10. Lugar: horas que entran. 11. Borrar lo más viejo si falta lugar (nunca informes). 12. Videos guardados (borrar por carrera).

## Cuándo

Etapa nueva **E7**, después de la 1.0.0. Primero tiene que andar perfecto la carrera. Aceptación de E7: con la cámara grabando, la prueba de demora de voz y pulsador da igual que sin cámara, y el video queda sincronizado al décimo de segundo con las marcas.
