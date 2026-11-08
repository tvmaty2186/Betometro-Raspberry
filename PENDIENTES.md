# Betómetro Raspberry — Pendientes

Actualizado el 08/11/2026 (última versión: 1.0.34). Lo que falta para entregar: [docs/auditorias/12-final.md](docs/auditorias/12-final.md). Plan: [docs/plan-ingenieria.md](docs/plan-ingenieria.md).

## Decisión del 03/11/2026: Raspberry Pi OS Lite en vez de Buildroot
- Betómetro se instala como **programa** en Raspberry Pi OS Lite 64 bits (Debian trixie), no como sistema propio.
  Motivo: cada imagen de Buildroot tardaba 2–3 h y fallaba por piezas del sistema. Ahora se compila solo el programa (~15 min con caché).
- Se pierde: copias A/B (queda la versión anterior guardada para volver a mano) y unos segundos de arranque.
- Buildroot (`sistema/`, `imagen.yml`) se borró el 07/11/2026 (queda en el historial de git).
- Cómo se arma e instala: `raspios/` y `.github/workflows/programa.yml`.

## Hecho
- E1 (motor igual a la app), E2 (programa completo, temas aprobados).
- Cambios de Matías: sin cantidad de vueltas, MEDIDA/LIBRE y «Unir vueltas» en el informe, libres sin error, cronómetro progresivo/regresivo, ideal a la vista, «libre» en todos lados.
- 04/11 (1.0.10): el brillo cambia la pantalla de verdad (antes solo se guardaba); botón BUSCAR por internet en Actualizar.
- 04/11 (1.0.9): GPS por USB (VK-172, VK-162, NEO-M8N por micro USB) o por los pines, lo busca solo.
- 03/11 (1.0.8): foto de arranque en toda la pantalla (la salida de TV de la Pi 3 la achicaba).
- 03/11 (1.0.7): foto de arranque: escala 1 y posición real.
- 03/11 (1.0.6): aviso «Actualizar» en el inicio; informes con el tema Avispón.
- 03/11 (1.0.5): pantalla de arranque con barra de carga y sin letras; pulsador por cable (1.0.4); letras chicas
  más grandes para 5"; últimos detalles de la app (pellizco, cartel de 5 s, barra ±, MARCAR, marco en sol, brillo
  de los temas); reporte de errores automático; wifi propia «Betometro-101»; menos escrituras en la tarjeta.
- 03/11 (1.0.4): el programa lee el pulsador (GPIO17, pin 11 a GND pin 9) con la pull-up interna y la hora
  exacta del kernel; marca en la carrera y responde en el Chequeo previo. Falta probarlo con el pulsador real.
- 03/11 noche (1.0.3): dígitos con la fórmula de la app y «Números − / +», editor con los chicos al frente
  y 4 asas, cara de instrumento con reflejo, colores de pasado/atrasado, estilos Anillo y Barra, redondo en
  más widgets, velocímetro liviano (aguja sin GPS, «máx», inicio animado), ficha para temas de letra ancha,
  botón Terminar más grande. **Arreglo:** el armado guardado en el editor nunca se cargaba.
- 03/11: actualización por wifi probada (1.0.1 → 1.0.2). Wifi real y «Betómetro en la red».
- 03/11: **Betómetro anda en la Raspberry** (Pi OS Lite): arranca solo, pantalla, touch y voz por auriculares.
  - Pantalla: es la Touch Display 2 de **5"** (720×1280), no de 7".
  - Touch: se despierta con `despertar-touch.service` (el sistema lo busca antes de que la pantalla lo encienda).
  - Sonido: el Qt de Debian solo sale por PulseAudio → `pulseaudio-sistema.service`.
  - GitHub prueba cada versión en un Debian arm64 limpio antes de entregarla.

- 04/11 (1.0.12): en carrera sin cartel de calor/tensión y procesador al máximo salvo con calor o tensión baja; Actualizar busca en la página de GitHub si la API no contesta.
- 04/11 (1.0.11): aviso de tensión baja y calor (inicio, carrera, Chequeo previo y reporte con la vuelta);
  vigilante: si Betómetro se congela 30 s, se reinicia solo; procesador al máximo en carrera; prioridad a GPS,
  cronómetro y pulsador (wifi y cámara después); el reporte suma lo último del sistema si se colgó o se cortó
  la corriente (registros de avisos en la tarjeta y memoria de cuelgue «ramoops»); memoria de reserva en la RAM (zram).

- 05/11 (1.0.13): sugerida que pasa los 160 km/h → queda en 160 y en rojo, en el velocímetro y en el
  cuadrado «Sugerida» (sin el ■). **Al marcar ya no se congela**: la base se guarda por detrás en otro hilo
  (antes guardar varios MB en la SD frenaba la pantalla medio segundo; empeoraba con cada carrera a 5 Hz).
  Chequeo previo: «Módulo» en verde si está conectado y hablando («Réplica NEO-M8N…» si no es original); rojo solo
  «Sin comunicación».
  Inicio: carteles GPS y wifi más grandes (con punto verde/rojo; en horizontal al lado del nombre del auto) y
  ícono de wifi de abanico a la izquierda de las franjas en diagonal (tocarlo abre Conectividad). Arreglo de las
  rayitas de la pantalla de wifi (se encimaban).
  Aceleración del informe con ventana de ~1 s (a 5 Hz saltaba ±4 m/s² por ruido; a 1 Hz igual que la app).

- 05/11 (1.0.14): sin el cartel de wifi en el inicio (no entraba; queda el abanico); el QR de
  Conectividad se toca y se ve en grande. **Clima** (Open-Meteo, gratis y sin clave): botón en el inicio (Ajustes
  arriba, Clima donde estaba Ajustes); pantalla con ahora, hora por hora y 7 días; lugar del GPS o localidad
  buscada (se elige de una lista real: Open-Meteo y, si no encuentra, OpenStreetMap); campo «Localidad» en la
  carrera; en Próximas, botón con el clima del día de la carrera («desde dd/MM» si falta: hay 16 días).

- 05/11 (1.0.15): cámaras comunes **MJPEG** (la webcam genérica): la Pi las pasa a H.264 con su chip.
  Falta probar con la cámara real.

## Ahora (Matías)
1. Tenía la 1.0.12 (04/11); actualizar a la 1.0.17. Probado bien: brillo, foto de arranque, Actualizar. Falta mirar el Chequeo previo
   (tarjeta «Alimentación y temperatura»), el pulsador (pin 11 y 9) y la wifi propia (lejos de casa).
2. GPS: el micro USB de su NEO-M8N solo da corriente (probado 04/11). Va por los pines (VCC pin 17, GND, TXD → pin 10,
   RXD → pin 8) o, si prefiere USB, con un VK-162/VK-172.
3. Reporte de errores por internet (opcional): crear el repo privado `Betometro-reportes` y una llave que solo
   pueda crear issues ahí; se copia a la Raspberry en `/var/lib/betometro/reportes.token`. Sin eso, los
   errores igual quedan en la página «Betómetro en la red» → «Reporte de errores».

## 1.0.18 (publicada 06/11/2026)
- Video en el informe (Raspberry y página): Informe/Video, datos encima, pantalla completa, datos que se mueven en pausa.
- Cámara: Espejo. Lista de videos guardados con borrar. El borrado automático también saca el video de la carrera.
- Modo tele por HDMI (`raspios/kms.json`; DSI-1 y HDMI-A-1 confirmados en la Pi). Sin enchufe en caliente: botón Reiniciar.
- Pulsador = play/pausa mientras se ve un video.
- Video en MP4 automático al terminar de grabar (lo abre el iPhone); nunca en carrera.
- Calidad de grabación: 480p, 720p o 1080p. En 480p, si la cámara no lo da, se toma en 720p y se achica (solo webcams MJPEG).
- Google Drive: informes y videos (estos solo con la wifi elegida). Ver [docs/google-drive.md](docs/google-drive.md).
- Falta probar en la Pi: video en la pantalla (velocidad de la Pi 3), la tele, Drive con Google de verdad, celular.

## 1.0.17 (05/11/2026)
- Cámara: paquete gstreamer1.0-plugins-bad (h264parse) y v4l-utils; vista previa con cualquier tamaño chico; antes
  de grabar espera a que la vista previa y el micrófono suelten los dispositivos. **Falta el error exacto de la
  grabación**: resuelto, la cadena con `v4l2jpegdec` se trababa; ahora `jpegdec` + compresor del chip (probado en la Pi).
- Inicio: el velocímetro muestra la velocidad real si hay GPS (lectura de menos de 3 s); si no, la animación.

## Modo Tablet (aprobado 05/11/2026)
Botones grandes como tablet, interruptor dentro de cada tema. Plan y muestras: [docs/modo-tablet.md](docs/modo-tablet.md).
**Programado completo (1.0.16):** se prende en Ajustes → Pantalla y tema → «Modo Tablet».

## Para la próxima (pedidos de Matías)
- **Borrar carrera = borrar su video** (07/11/2026). Hoy `Datos::borrarCarrera` saca la carrera pero el video
  (`extra.videos`, archivo en `<datos>/videos/`, y su .mp4) queda en la tarjeta ocupando lugar. Al borrar: borrar
  también esos archivos (solo si ninguna otra carrera usa el mismo archivo). Avisarlo en el «Tocá otra vez».
- **Engranaje de verdad** (07/11/2026): el ícono de Ajustes parece un sol (círculo con rayitas). Dibujar un
  engranaje (rueda con dientes y agujero en el medio) en el **inicio** (`TIcono.qml`, «ajustes») y en el botón del
  **video** (`video/Engranaje.qml`). Mostrarle una muestra antes.

## 1.0.28 «sin sorpresas» (publicada 07/11/2026)
Arreglos por dentro: recorrido que no se pierde, instalador seguro con vuelta atrás, hora del GPS, cámara, wifi,
ssh, arranque más rápido, pruebas antes de publicar. **Falta probar en la Pi:** hora del GPS sin wifi, vuelta atrás
automática y el arranque.

## 1.0.37 (publicada 08/11/2026)
La Tienda lee el catálogo de Betometro-tienda (vacío por ahora; una herramienta publicada ahí aparece sola).
Inicio del velocímetro: orden invertido (Carreras arriba, Ajustes abajo) y más aire entre los botones.

## 1.0.36 «más segura» (publicada 08/11/2026)
Botones del celu solo desde nuestras páginas; Drive pide el código de la pantalla; la Pi comprueba la huella
(sha256) de cada actualización; los avisos se escriben de a tandas; largada sin GPS viejo; tiempo ideal mínimo
5 s; rojo y naranja del tema (sol); guía de SD nueva ([docs/sd-nueva.md](docs/sd-nueva.md)).
**Ojo:** la huella se comprueba desde la 1.0.36 en adelante (la 1.0.35 instala la 1.0.36 sin comprobar).

## 1.0.35 (publicada 08/11/2026)
Manual del navegante (Prueba completa → Manual; celu /ayuda y /manual.pdf), botón Tienda en los dos inicios con
lugares vacíos, volumen y X para cerrar en el video a pantalla completa, «en construcción» con salida.

## Tienda de herramientas (decidido con Matías, 08/11/2026)
- Botón **Tienda** en los dos inicios y pantalla con 8 lugares punteados vacíos (publicado en la 1.0.35). Sin brújula por ahora.
- Cada herramienta es un programa Qt para la Pi, en un paquete `.tar.gz` con su ícono y su ficha (nombre, versión, descripción).
- **Un solo repositorio `Betometro-tienda`**: una carpeta por herramienta + el catálogo `tienda.json`. GitHub arma cada paquete.
- En la Pi: baja el catálogo, Instalar (comprueba el hash), Abrir → Betómetro se cierra y abre la herramienta; al salir o si se
  traba, Betómetro vuelve solo. Nunca con una carrera en curso.
- Falta: que Matías cree el repositorio y elija la primera herramienta (ideas: brújula, registrador de viaje, calculadora de
  regularidad, velocímetro grande, visor de videos, música, un juego).

## 1.0.34 «auditoría final» (publicada 08/11/2026)
Arreglos de la auditoría 12: el pulsador ya no pierde una marca si el programa estaba ocupado; si una versión
nueva se traba al abrir, vuelve sola a la anterior (con sus servicios); el instalador no queda en negro si
falla adentro de una función; la hora no se corrige con la carrera en curso; documentos al día.
El manual está hecho para la 1.0.35 (Prueba completa → Manual; celu /ayuda y /manual.pdf; si se cambia `recursos/web/ayuda.html`, rearmar el PDF con `node herramientas/manual-pdf.js`).

## 1.0.33 (publicada 08/11/2026)
Los videos MP4 vuelven a tener sonido en el informe de la Pi (el reproductor buscaba el sonido solo al principio).

## 1.0.32 «cámara despierta» (publicada 07/11/2026)
La Pi ya no duerme la cámara (era la causa de «failed during initialization»). La Prueba completa cuenta bien
los cuadros, graba los 30 s aunque reintente y compara la reproducción con lo grabado.

## 1.0.31 «cámara firme» (publicada 07/11/2026)
Si la cámara falla al arrancar, prueba sola de nuevo a los 2 s (también en carrera). La Prueba completa espera a
que la cámara se suelte, marca la hora de cada paso y no dice «falló» si hay otro video pasando a MP4. El pase a
MP4 no se corta al largar. **Falta probar en la Pi:** correr la Prueba completa varias veces seguidas.

## 1.0.30 (publicada 07/11/2026)
Gráficos a pantalla completa (Pi y celu), Prueba completa (Ajustes), arreglo de pantallas en negro, sin MARCAR
con la pantalla diseñada, video «procesando», Drive por HTTP/1.1, arranque sin esperar al sonido, hora en el
Chequeo, pruebas nuevas. Los reportes llegan solos a tvmaty2186/Betometro-reportes.
Lo que queda: [docs/plan-1.0.30.md](docs/plan-1.0.30.md) (casillas sin marcar).

## 1.0.29 «pantallas y orden» (publicada 07/11/2026)
Tiempos m:ss:d, tiempo por casilleros, IR A LARGAR, Terminar → informe, engranaje, MARCAR opcional,
simulador en el Chequeo, video en sincronía y 480p, un archivo por carrera (migra sola), copia de la base.
**Antes de instalarla:** copia de la base (`scp` en docs/conectarse-ssh.md).
Todo lo que entra, con las decisiones de Matías: **[docs/plan-1.0.28.md](docs/plan-1.0.28.md)**. Seguir esa lista.
**Matías (07/11): entra todo lo de las auditorías**, también lo que era «para después» (engranaje, Drive con la base,
un archivo por carrera, ordenar el código). Afuera solo Deshacer, carteles grandes y clave distinta.

## Auditorías (plan aprobado 07/11/2026)
Diez revisiones del proyecto por dentro, de a una, en sesiones nuevas. Lista, modelo de cada una y el prompt
para la sesión: [docs/auditorias/README.md](docs/auditorias/README.md). Los informes van a `docs/auditorias/`.
**Hechas las 10 (07/11/2026).** Resumen y preguntas para Matías: [docs/auditorias/00-resumen.md](docs/auditorias/00-resumen.md).
Respuestas de Matías en el resumen. **OK de Matías (07/11): 1.0.28 con todos los arreglos chicos de A y B más
todas las mejoras de rendimiento** (guardado, barrita, listas del video, tableros, capas, fondo, 480p 800×600).
Salió en la 1.0.27 (publicada 07/11): **± segundos** grande (`WDelta.qml`) en la pantalla de fábrica. Acostada: arriba
a la derecha y la Sugerida abajo, al lado de la velocidad. De pie: abajo, al lado de la última vuelta (`Armado.qml`).
UX: no a los carteles grandes; «Práctica libre» pasa a «Práctica»; Recorrido libre queda adentro de Práctica.
**MARCAR en pantalla opcional** (Matías, 07/11): llave «MARCAR en pantalla» en Ajustes → Pulsador y en el Chequeo previo
(prendida de fábrica). Apagada: sin botón, larga y marca solo el pulsador; **nada en pantalla como plan B**. El lugar
libre muestra más datos, sumando Faltan y Tiempo ideal. Armado de esa pantalla: el diseñador hace 2 propuestas desde cero.

## Siguiente
- **Red propia (06/11/2026):** Matías la probó lejos de casa y no se armaba. Arreglado para la 1.0.23 (sin probar
  en la Pi): busca redes de nuevo antes de decidir, botón «Armar la red propia ahora», el motivo si falla
  (en Conectividad), paquetes `dnsmasq-base` e `iw`, chip amarillo en la barra con los celus conectados.
- **Video con la imagen rota (resuelto):** era 1080p a 30 cuadros, la Pi 3 no llegaba. En 720p y 480p salió
  limpio (Matías, 06/11). Desde la 1.0.24 **no hay 1080p** (Matías: rompía muchas imágenes): solo 480p y 720p.
  Si estaba en 1080p pasa solo a 720p.
- **1.0.26 publicada (07/11/2026): Recorrido libre** (Práctica libre → «Recorrido libre»). Sin vueltas, sin tiempo ni
  distancia; graba GPS (1 lectura por segundo, entran 5 h) y video; MARCAR deja marcas numeradas y no corta nada;
  sin voz. Informe (Pi y celu): mapa con largada, llegada y marcas, velocidad con las marcas, resumen (tiempo,
  distancia, promedio, máxima, parado, aceleración máxima) y lista de marcas (tocar una lleva el video ahí).
  Muestras aprobadas: `docs/muestras/recorrido/`. Falta probarlo en el auto.
- **1.0.25 publicada (07/11/2026):** Conectividad → llave «Conectarse a redes wifi». Apagada: no se conecta a
  ninguna red y arma la red propia enseguida (sin internet). Pedido de Matías; falta su OK para publicar.
  Carrera: REC, GPS y Terminar del mismo alto, borde y letra; el punto del REC parpadea cada 500 ms.
- **1.0.24 publicada (06/11/2026).** Falta que Matías la pruebe en la Pi.
- **Menú del engranaje del video (1.0.24, sin probar en la Pi):** las llaves no cambiaban y los datos volvían a
  su lugar al soltarlos. Causa: una versión anterior había guardado «videoDatos» como el texto «[object Object]»
  y quedaba así. Ahora si no es un objeto se arranca de cero (probado acá con ese dato roto: anda).
- Hecho hasta la 1.0.22 (06/11/2026): video en el informe (Pi, celu, tele), modo tele con el celu de control y
  carreras como capítulos, Drive, MP4, 480p, GPS quieto, todas las pantallas grandes.
- Probado por Matías (06/11/2026), todo bien: corriente y calor (convertidor y disipador), pulsador real,
  GPS en el auto, letras en movimiento y voz.
- **Falta solo:** una carrera larga con la cámara grabando (temperatura y rendimiento de la Pi 3). Después,
  mirar el «Reporte de errores» y `vcgencmd get_throttled` (tiene que dar 0x0).
- Solo lectura total (overlay): necesita regrabar la tarjeta con una partición aparte para las carreras.
  Por ahora: casi nada se escribe en la tarjeta, las carreras se guardan de forma segura y se repara sola al prender.

## Más adelante
- Bluetooth (si el GPS va por USB queda libre): música desde el celular por los auriculares con la voz arriba;
  pulsador Bluetooth solo como respaldo (los botones baratos se duermen y llegan tarde). Charlado, no decidido.
- E6: prueba en pista y versión 1.0.0 definitiva.
- App del celular: el prompt está en [docs/prompt-app.md](docs/prompt-app.md) (se hace en otra sesión).

## Compras
- Auriculares con cable (3,5 mm), pulsador (10 kΩ y 100 nF).
- Para el auto: placa de borneras a tornillo para los GPIO, convertidor 12 → 5 V (3 A o más, ajustado a 5,1 V), fusible 3 A, cable de 0,75 mm², fichas GX12, cable espiralado, disipadores.
- Webcam USB MJPEG 1080p gran angular (para el video).
