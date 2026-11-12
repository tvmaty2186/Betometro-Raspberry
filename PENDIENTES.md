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

## 1.0.59 (publicada 10/11/2026): repaso de letras y cuadros
Revisor automático de textos (`app/revisar_textos.h`, `BETO_REVISAR_TEXTOS=1` con `--captura`): 33 pantallas × 5
temas × 2 orientaciones. Arreglado:
- **Cabina:** los números en B612 (antes B612 Mono): «0: 01: 5» se salía del cuadro y pisaba ± segundos; «3. 200 m»
  y «49, 6» con huecos. Ahora «0:01:5», «3.200 m».
- **Prueba completa parada:** «Manual del navegante» quedaba encima de «Mandar el resultado» y se salía (todos los temas).
- **Clima parado:** los botones GPS / Buscar / A carrera tapaban lluvia y viento (ahora en una fila abajo).
- **Mapa y gráfico completos:** las flechas ‹ › pegadas a los botones de vuelta; en el gráfico no pisan «Viendo km…».
- **Tiempo ideal con 0 minutos** (de 4:00 a 0:50): se rechazaba por «menos de 5 s»; ahora abre los segundos.
- Cámara: con buena alimentación arranca al primer intento (lo de antes era el USB que se reiniciaba por la tensión).

## 1.0.58 (publicada 10/11/2026)
Del chequeo del 10/11/2026 (recorrido de 39,8 km en el Sentra: GPS impecable, videos ya pasados a MP4):
- **Actualizar sin internet:** un solo aviso y no reintenta cada minuto; busca al conectarse a una wifi o cada 15 min.
- **Tensión baja corta:** el bit «hubo tensión baja» (0x10000) se avisa una vez por arranque (antes un bajón de
  menos de 2 s no quedaba anotado; la Pi tenía throttled=0x50000 sin aviso). Con el cargador de 60 W del Sentra;
  Matías va a poner una fuente de 5 A propia.
- **Cámara que no arranca al primer intento** (en cada carrera; anda en el 2.º): el aviso ahora dice el motivo,
  para arreglarlo con el dato. Puede ser la tensión baja (el USB se reinicia).

## 1.0.57 (publicada 10/11/2026)
- **Paso a MP4 trabado** (09/11/2026): un .mkv cortado dejaba al ayudante trabado para siempre (la Pi lo confirmó:
  el .parte no crecía desde hacía 25 min). «Preparando el video» no se iba, no dejaba apagar y los otros 6 videos
  nunca se pasaban. Ahora un vigía lo corta si no avanza en 2 min (o pasa 60 min); queda anotado en
  `videos/mp4-fallados.txt` y no se reintenta. El .mkv se sigue viendo igual.
- «Preparando el video» **ya no bloquea** Apagar/Reiniciar (cortarlo no pierde nada): solo avisa.
- Apagar/Reiniciar también bloqueados **actualizando** (bajando o instalando), con la **Prueba completa** en marcha y
  con un **comando del buzón** corriendo. Antes de apagar se espera a que la carrera que se está guardando quede en
  la tarjeta (`Sistema::antesDeApagar` → `Datos::guardarYa`).

## 1.0.56 (publicada 09/11/2026)
- Color de la vuelta siempre en **pantalla completa** (se sacó el «marco»). Ajustes → Pantalla y tema: **Vuelta medida**
  y **Vuelta libre** con 5 colores cada una (rojo, verde, azul, amarillo, naranja).
- Salir con el pulsador: a los 3 s, cartel grande **«TERMINANDO CARRERA»**; a los 5 s sale. Soltar antes lo cancela.
- **Apagar/Reiniciar bloqueados** mientras se graba, se pasa a MP4 o se sube a Google Drive (avisa qué está haciendo).

## 1.0.55 (publicada 09/11/2026)
- Carrera: cada marca que cuenta prende un **marco amarillo** alrededor de la pantalla (~0,35 s) y MARCAR se ilumina
  más tiempo. Las marcas ignoradas no muestran nada (a propósito).
- Informe: la tabla muestra los **metros de cada vuelta** (en vez de km/h) y ya no tiene la columna VIDEO.
- Informe con video: «En este momento» en 2×2: Velocidad · Sugerida / Distancia · Adelanto.

## 1.0.54 (publicada 09/11/2026)
- **Salir con el pulsador** (Ajustes → Pulsador, apagado de fábrica): mantenido 5 s termina la carrera y va al informe.
- Mapa y gráfico a pantalla completa: fila **‹ V1 V2 … ›** (y TODAS en el gráfico) para cambiar de vuelta desde adentro.
- Cuadro «Todas las vueltas»: los números entran en su columna (se pisaban con el tema Cabina).
- Video: ya no dice «No está el archivo del video» si el informe estaba abierto cuando se pasó a MP4.

## Lo que salió de las carreras reales del 09/11/2026
Auditoría de 2 carreras (Recorrido 12:02 y Práctica 12:05, 350 m / 50 s, 5 vueltas medidas): GPS impecable
(precisión 1 m, sin huecos ni saltos), cámara bien (5 cuadros de 9031), datos bien aunque se cortó la corriente.
La vuelta real medía ~380 m (no 350): por eso llegaba siempre tarde. La 6.ª fue a boxes (interrumpida).
- **Hecho en 1.0.54: Salir con el pulsador** (Ajustes → Pulsador, apagado de fábrica). Mantenido 2 s: «Seguí
  apretando para salir»; 5 s: «Carrera terminada», sale y va al informe. La vuelta que abrió esa marca no se guarda;
  no sale si todavía no hay vueltas cerradas (en un recorrido, sí, y se guarda). Dos frases nuevas en `voz/grabar.py`.
- **Video «No está el archivo del video»** (12:13 y 12:14): hecho en 1.0.54. Al pasar a MP4 el video cambia de nombre
  y el informe abierto seguía con el viejo. Que «se tilda las primeras veces» puede ser la conversión a MP4 andando
  justo después de la carrera: ver si sigue.
- **Cuadro «Todas las vueltas» amontonado** (tema Cabina) y **cambiar de vuelta desde el mapa y el gráfico**: hechos en 1.0.54.
- **Pulsador virtual (GPS) sin dato en las vueltas 4 y 5** (misma vuelta de siempre). Revisar por qué.

## 1.0.28 «sin sorpresas» (publicada 07/11/2026)
Arreglos por dentro: recorrido que no se pierde, instalador seguro con vuelta atrás, hora del GPS, cámara, wifi,
ssh, arranque más rápido, pruebas antes de publicar. **Falta probar en la Pi:** hora del GPS sin wifi, vuelta atrás
automática y el arranque.

## 1.0.53 (publicada 09/11/2026)
- Buzón de comandos por GitHub («Ayuda a distancia», en Conectividad): Claude revisa y arregla la Pi a distancia,
  sin ssh. Cada 2 min, solo con internet y nunca en carrera. Ver docs/raspios.md.

## 1.0.52 (publicada 09/11/2026)
- Carrera nueva: arranca con todas las vueltas medidas (antes, la última elección de Práctica; de fábrica impares).
  Práctica: de fábrica «Todas»; recuerda lo último que se eligió.
- Al terminar una carrera justo después de marcar, quedaba guardada la vuelta recién empezada y al arrancar de nuevo
  aparecía una vuelta cortada de 0 s (se vio en el reporte del simulador, 08/11/2026).

## 1.0.51 (publicada 09/11/2026)
- Chequeo previo: la pantalla de carrera se acomoda sola según por dónde llegó la marca de prueba del pulsador
  (pantalla → con MARCAR; pulsador o tecla → sin MARCAR). Cambia el ajuste «MARCAR en pantalla».
- Flecha de las barras (tiempo y velocidad sugerida): siempre blanco intenso con borde negro grueso, un poco más grande
  (antes, sin dato, iba transparente y se veía verde, sobre todo en el Deportivo).

## 1.0.50 (publicada 09/11/2026)
- Arranque: Betómetro ya no espera a la red (~3,5 s menos); la Prueba completa dice cuánto tarda cada parte.
- Clima: doble toque en un día → hora por hora.
- Clima adjuntado a una carrera («A carrera»): se actualiza solo con internet (2 veces por día; el día de la carrera,
  cada hora; sin internet no intenta y pide al conectarse). Se ve en Carreras y en el informe (⋮).
- **Falta:** con la 1.0.50 en la Pi, correr la Prueba completa y mirar el arranque («Betómetro empezó a los …»).

## 1.0.49 (publicada 09/11/2026)
- Tema propio: el acento siempre se lee (se aclara u oscurece si no se distingue del fondo); con letras negras, los
  gráficos, mapas y tablas van sobre gris claro.
- La vuelta en curso se guarda cada 20 s: si se corta la luz queda como interrumpida con lo andado.
- Informe: aviso de marca faltante (⚠, el GPS vio que se pasó la línea sin marcar).
- Ejemplo de la PC con el pulsador un poquito tarde; `BETO_MEDIR_ARRANQUE`, `BETO_MUESTRA_FALTA`.
- **Falta:** el arranque de la Pi (25 s) está en el sistema; Matías tiene que pasar `systemd-analyze`.

## 1.0.48 (publicada 08/11/2026)
- El informe abre más rápido: las pantallas completas (gráfico, mapa, Comparar) se arman al abrirlas.
- GStreamer se prepara en segundo plano a los 10 s de prender (el informe se trabó 8,5 s la primera vez).
- Mapa a pantalla completa: zoom y arrastre sin tirones (se redibuja al soltar).
- Tema propio: gráficos, mapas, tablas y tarjetas del video sobre gris fijo; el error total sobre pastilla oscura.
- Sin el aviso de «video cortado».
- Con el video: **Mapa | Gráfico** al lado (el gráfico con los modos, raya donde va el video, toque para saltar).

## 1.0.47 (publicada 08/11/2026)
- **Pulsador virtual (GPS)** en el informe, solo de referencia (`app/nucleo/marca_gps.cpp`): tiempo y error al lado
  del manual, «dudosa» si el GPS andaba mal, y al pie cuánto se marcó tarde en promedio (incluye la demora del GPS).
- **Comp.** como cuarto modo del informe: la vuelta elegida contra la modelo ★ (la más cerca del ideal); doble toque
  abre el tiempo ganado/perdido y 4 sectores.
- La pantalla de Cámara ya no se traba (~8 s): la lista de GStreamer se guarda con los datos (GST_REGISTRY_1_0).
- El vigilante lee la tensión de Salud. Capturas: los nombres con espacio ya no se cortan.

## 1.0.46 (publicada 08/11/2026)
- Tema propio: al guardar colores desde el celu la pantalla se vuelve a armar (los acentos quedaban viejos).
- Personalizar → Colores: sin botones; los círculos de Fondo y Acento de abajo eligen qué se pinta (Matías).

## 1.0.45 (publicada 08/11/2026)
- **Vigilante** (`app/vigilante.cpp`): memoria cada 10 s; si pasa del 70 %, la pantalla se congela más de 1,5 s o hay
  tirones en carrera, manda un reporte con el equipo (memoria, swap, carga, temperatura, tensión), los 5 procesos que
  más usan, cuánto crece Betómetro por minuto, las últimas pantallas, qué está andando y la memoria de la última media
  hora. Un aviso por motivo cada 10 min. Costo medido: ~0,05 s de procesador por minuto.
- **Chequeo previo → Memoria:** semáforo y «Despejar» (dos toques): reinicia Betómetro y vuelve al Chequeo diciendo
  cuánto liberó (`despejar.json`).
- **Apagar:** ventana con Apagar, Reiniciar y Cancelar.

## 1.0.44 (publicada 08/11/2026)
- La Pi se ponía lenta tras guardar muchas veces desde el celu: el Canvas de la foto del inicio no soltaba las fotos
  viejas (~12 MB por guardado). Arreglado; el fondo se repinta al cambiar el color del tema propio.
- «Personalizar desde el celu» aparece solo con el tema Propio (Matías).

## 1.0.43 (publicada 08/11/2026)
Personalizar → Colores: panal de 127 colores y 9 grises; se elige si se pinta el principal o el acento (Matías: «más colores»).

## 1.0.42 (publicada 08/11/2026)
El tema Avispón Verde siempre con su foto, el 101 y «Avispón Verde» (es el auto de Matías). Los otros temas y el propio
usan lo personalizado; lo personalizado no se borra.

## 1.0.41 (publicada 08/11/2026)
El velocímetro elegido en Personalizar vale solo en el tema propio; cada tema de fábrica vuelve al suyo (Matías).

## 1.0.40 «personalizar» (publicada 08/11/2026)
**Personalizar desde el celu** (Ajustes → Pantalla y tema → QR): foto del inicio (mover, pellizcar, acostada y parada),
número y nombre del auto, tema propio (fondo, acento, letras; parte del Deportivo) y 6 velocímetros del inicio.
Además: flecha blanca en las barras (negra en sol); los números de los botones − / + se achican si no entran; el código
de Google siempre entra; pruebas con capturas en Cabina. Probado de punta a punta (celu simulado → Pi).

## 1.0.39 (publicada 08/11/2026)
Modo tele: el sonido del video vuelve a salir por la tele (directo por ALSA, `hdmi:CARD=vc4hdmi`). Desde la 1.0.28
PulseAudio no mira la tele y la búsqueda de la salida fallaba: el sonido iba a los auriculares. Fuera del modo tele,
auriculares como siempre.

## 1.0.38 (publicada 08/11/2026)
Prueba completa: el wifi prueba la ANTENA (sin wifi o sin internet no es falla; falla solo si no está la placa o no
ve ninguna red). Las pruebas abren 10 pantallas más (compartir, wifi, editor, Drive, ajustes, recorrido, gráfico y
mapa a pantalla completa, vertical). Servicio más protegido (ProtectSystem, ProtectHome, PrivateTmp). Aviso «NO LO
APAGUES» en pantalla mientras se rearma el arranque. Arreglos: prueba de micrófono trabada, archivo temporal de Drive,
aviso del video en iPhone, mail de ejemplo, licencias ([LICENCIAS.md](LICENCIAS.md), también en el equipo).

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

## Personalizar desde el celu (decidido con Matías, 08/11/2026) — muestras en `docs/muestras/personalizar/`
- En la Pi (Ajustes → Pantalla y tema) un QR abre la página **Personalizar** en el celu, con 4 pestañas y la vista del
  inicio en vivo (acostada / parada); recién se manda a la Pi con GUARDAR, y siempre está «De fábrica».
- **Foto** del inicio: elegir del celu, mover con el dedo, pellizcar; acostada y parada por separado.
- **Auto:** número y nombre.
- **Colores** (tema propio, parte del **Deportivo**: gris casi negro + rojo + letras blancas): solo 3 → principal (fondo),
  acento, letras (blanco / crema / negro). Aviso si hay poco contraste. Bandera a cuadros y brillo cromado siempre.
  **Fijos, nunca cambian:** la barra de promedio (verde/amarillo/rojo), adelantado/atrasado (texto y caja), vuelta
  medida/libre (rojo/verde), los avisos, y el modo sol. Letra fija (la del Avispón).
- **Reloj:** 6 velocímetros para el inicio (Clásico 404, Deportivo, Digital, Rally, Aviación, Anillo).
- Hecho todo en la 1.0.40 (Matías: «sumá todo a la 40»).

## Tienda de herramientas (decidido con Matías, 08/11/2026)
- Botón **Tienda** en los dos inicios y pantalla con 8 lugares punteados vacíos (publicado en la 1.0.35). Sin brújula por ahora.
- Cada herramienta es un programa Qt para la Pi, en un paquete `.tar.gz` con su ícono y su ficha (nombre, versión, descripción).
- **Un solo repositorio `Betometro-tienda`**: una carpeta por herramienta + el catálogo `tienda.json`. GitHub arma cada paquete.
- En la Pi: baja el catálogo, Instalar (comprueba el hash), Abrir → Betómetro se cierra y abre la herramienta; al salir o si se
  traba, Betómetro vuelve solo. Nunca con una carrera en curso.
- **Primera herramienta (para otro día, Matías 08/11/2026): «El Avispón vuela»**, juego propio de un botón, **solo con el
  pulsador**. 20 niveles en 4 mundos (Autódromo, Ruta de tierra, Montaña, De noche), jefe cada 5 (La Grúa, El Escarabajo,
  El Cóndor, El Cronómetro), 3 vidas, estrellas por banderas, menú con el pulsador (apretar = jugar; 1 s = cambiar de
  nivel), X para volver. Muestras en `docs/muestras/juego/`. Orden: base de la Tienda (instalar/abrir/volver) → mundo 1 → resto.
- Antes: que Matías cree el repositorio y elija la primera herramienta (ideas: brújula, registrador de viaje, calculadora de
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
- **Para la 1.0.60 (12/11/2026, sin publicar):** se grabaron las 2 frases que faltaban («Seguí apretando para salir»
  y «Carrera terminada», Elena y Tomás; desde el 09/11 no sonaban). En la Prueba completa, el pulsador salteado
  ya no es aviso (Matías no lo usa). «Betómetro empezó a los 480 s» ahora dice «se volvió a abrir a los 8 min»
  cuando no es el arranque. Hardware: cooler (47 °C máx. grabando) y fuente nueva de 5 A.
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
- Para el auto: placa de borneras a tornillo para los GPIO, convertidor 12 → 5 V (3 A o más, ajustado a 5,1 V), fusible 3 A, cable de 0,75 mm², fichas GX12, cable espiralado. (Disipadores y cooler ya puestos, 11/11/2026.)
- Webcam USB MJPEG 1080p gran angular (para el video).
