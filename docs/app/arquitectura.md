# Arquitectura de Betómetro

## Stack

- **Capacitor 8**: la app es una página web (`www/`) dentro de una app Android nativa (`android/`).
- **Sin frameworks ni compilación del front.** Todo es JS y CSS planos en un solo archivo, `www/index.html` (unas 2.800 líneas: estilos, HTML y script).
- **Plugins:**
  - `@capacitor-community/background-geolocation` (GPS con la pantalla apagada)
  - `@capacitor-community/sqlite` (base de datos)
  - `@capacitor-community/keep-awake`
  - `@capacitor/app`, `filesystem` y `share`
- **Plugin propio** `Beto` (Java), que hace lo que la web no puede.
- **CI:** GitHub Actions compila y firma el APK y lo publica como *release* (ver `CLAUDE.md`).
- El paquete Android es `ar.com.betometro.app` y el repo es `tvmaty2186/Betometro`.

## Archivos

| Archivo | Qué es |
|---|---|
| `www/index.html` | Toda la app |
| `www/informe.js` | Plantilla del informe ya compilada. **No se edita a mano** |
| `herramientas/informe-plantilla.html` | Fuente del informe. Después de editarla hay que correr `python3 herramientas/armar-informe.py` |
| `www/fuentes/` | Tipografías locales (woff2): Barlow, Barlow Condensed, Chakra Petch, B612, B612 Mono, Big Shoulders Display, Special Elite |
| `www/img/leon.png` | El león de Matías (668×356, fondo transparente), usado en el tema Avispón |
| `android/app/src/main/java/ar/com/betometro/app/BetoPlugin.java` | Plugin nativo |
| `MainActivity.java` | Registra el plugin y programa el aviso diario de actualizaciones |
| `ActualizacionWorker.java` | WorkManager: una vez por día consulta la última *release* y avisa con una notificación (una vez por versión) |
| `InicioReceiver.java` | `BOOT_COMPLETED`: abre la app al prender el teléfono, si el ajuste está activo |
| `docs/diseno/` | Muestras de diseño, el modelo 3D de la caja y los íconos |

## Secciones de `www/index.html`

Buscá los encabezados `/* ===== … ===== */` con grep.

- **ESTILOS:** temas base y luego bloques por versión (1.3 instrumentos, 1.4 menús, 2.0 Avispón). Las reglas que van al final ganan.
- **PLATAFORMA:** detecta `NATIVO` y obtiene los plugins (`Beto`, `BG`, `KeepAwake`…). `REPO = 'tvmaty2186/betometro'`.
- **MOTOR (`Motor`):** cálculo puro, sin pantalla ni audio.
  - `Motor.crear(carrera, oyente)` recibe muestras de velocidad y marcas.
  - Emite los eventos `vuelta`, `cerrada`, `ignorada` y `descartada`.
  - `Motor.esMedida(patron, n)`: `alt1` = vueltas impares medidas, `alt2` = vueltas pares medidas, `todas` = todas medidas.
  - Calcula la distancia recorrida integrando la velocidad del GPS. Descarta muestras con precisión peor que `umbralPrecisionM`.
  - Puede autocalibrarse (factor del GPS) con las vueltas anteriores.
  - Expone `estado(t)` con `restanteMs`, `restanteM`, `vNecKmh`, `deltaS`, etc.
- **ALMACENAMIENTO (`Store`):** en memoria guarda `{ config, carreras:[{…, vueltas:[]}] }`.
  - En la app nativa se guarda en SQLite; en el navegador, en localStorage (`betometro.v2`).
  - Siempre deja además un respaldo completo `betometro.json`.
  - Funciones: `setConfig`, `guardarCarrera`, `agregarVuelta`, `buscar`.
- **CONFIGURACIÓN:** `DEF` son los valores de fábrica que eligió Matías. `cfg = Object.assign({}, DEF, Store.config)`.
  - **Migraciones de una sola vez:** se marcan con banderas en `cfg` (`cero0`, `cuenta20`, `avispon1`, `avispon21`).
  - `avispon21` (antes `avispon1`) se fija en el arranque asíncrono, después de `Store.cargar()`, y se guarda enseguida: pone el tema Avispón **una sola vez** y después se respeta el tema que elija el usuario.
  - Para una migración nueva, copiá ese patrón (comprobá `Store.config.bandera`, no `cfg`).
- **AUDIO:**
  - En la app nativa los números se pregraban con TextToSpeech (`Beto.prepararVoz`) y se reproducen sin demora. En la web se usa `speechSynthesis`, con voz es-AR si hay.
  - El canal es «alarma» por defecto, que suena aunque esté en silencio.
  - `Audio.clic` es el clic mecánico al marcar.
- **PROGRAMADOR DE VOZ:** corre cada 15 ms.
  - La cuenta regresiva en voz cubre los últimos `cuentaN` segundos (fábrica 20). Se adelanta `anticipoMs` (40 ms en nativa, 250 ms en web) por la demora de la voz.
  - Al llegar a cero: si `pitidoCero` está apagado (fábrica), **dice «cero»**; si está prendido, hace un pitido largo.
  - Al empezar cada vuelta anuncia «Vuelta N, medida/práctica».
- **GPS (`GpsReal`):** muestras `{t, velMs, precisionM, lat, lon}`. `grabarTraza` guarda por vuelta `[s, m, km/h, lat, lon, precisión]`, para el informe.
- **WIDGETS:** la pantalla de carrera es libre (ver abajo).
- **MARCAR y pulsador:**
  - Teclas por defecto: Enter, Enter numérico, espacio y F9 (`TECLAS_DEF = [66,160,62,139]`). Sirve un teclado USB o Bluetooth, un control selfie o un pulsador propio.
  - «Aprender botón» captura cualquier tecla.
  - Antirrebote de 3 s.
- **PANTALLAS:** `PANTALLAS = ['inicio','carreras','editor','detalle','chequeo','practica']`, más el crono (`.app`).
  - `mostrar(p)` cambia de pantalla.
  - `volver()` es el botón atrás: primero cierra el visor; desde el detalle va a Carreras; desde el crono vuelve al detalle si la carrera está guardada.
- **INFORME:** `armarRecorrido`, `datosInforme`, `informeHTML`, `abrirInforme` (visor `#visor` con iframe), `imagenInforme` y `compartirInforme`.
- **ACTUALIZADOR:** consulta la última *release* de GitHub, descarga el APK y lo instala (`Beto.descargarEInstalar`). Cuando hay versión nueva, aparece «· Actualizar» en amarillo al lado de la versión en el inicio.

## Pantalla de carrera (widgets)

- **Tipos (`TIPOS_W`):**
  - `tipo`: vuelta y si es medida o práctica
  - `cuenta`: estilos números o anillo; su dato extra son los metros que faltan
  - `delta`: adelantado/atrasado, en número o barra
  - `velocimetro`: redondo = analógico 0–160; cuadrado = digital con barra contra la sugerida
  - `necesaria`: «Velocidad sugerida»
  - `faltan`, `transcurrido`, `ideal`, `proxima`
  - `ultima`: tiempo medido de la última vuelta
  - `marcar`: único
- **Posiciones:** se guardan como fracción del tablero. La referencia vertical es 378×800 (`caja()`) y la horizontal 800×378 (`cajaH()`).
  - Cada orientación tiene su armado: `cfg.widgets` (vertical) y `cfg.widgetsH` (horizontal).
  - `capa()` y `capaDef()` eligen el que corresponde.
- **Pantallas de fábrica (las armó Matías):**
  - **Vertical:** velocímetro redondo arriba; tipo a la izquierda y sugerida a la derecha; cuenta con «faltan m»; Última vuelta; MARCAR abajo.
  - **Horizontal:** cuenta a lo ancho arriba (con tipo y sugerida); abajo velocímetro digital, Última vuelta y MARCAR.
  - En las dos, Última vuelta encaja justo entre la cuenta y MARCAR, con los mismos bordes.
- **Barras:** `PROP_BARRA = 0.10`, o sea que cada punta equivale al 10 % del tiempo ideal de la vuelta. Si vas adelantado, la marca se corre a la derecha.
  - La barra del velocímetro digital compara la velocidad con la sugerida: ±10 %, con un mínimo de 5 km/h.
- **Editor:** se abre desde «Pantalla de carrera» en el inicio. Mantené apretado un widget para moverlo; tocalo para cambiar el tamaño; ✎ abre sus opciones (el menú está dentro del panel ✎).

## Inicio

`layoutInicio()` arma todo en proporción a la pantalla (`k = min(W/390, H/844)`):

- **Velocímetro:** centro en `cx = W − 60k`, `cy = 0,717·H`, diámetro `330k`.
- **Íconos:** van en un arco de radio `262k`, a 150°, 180°, 210°, 240° y 270°, en este orden: Carreras, Práctica libre, Chequeo previo, Pantalla de carrera, Ajustes. Ajustes queda justo arriba del centro del velocímetro.
- **Tema Avispón:** el redondel del número está en `(80k, cy − 300k)`, mide `104k` y va sobre la banda blanca y amarilla.

## Informe de carrera

- **Recorrido:** se arma con **todas** las vueltas.
  - Se toma la mediana ponderada de los puntos buenos cada ~5 m.
  - Los huecos se completan con las otras vueltas.
  - Cada vuelta se ajusta al trazado común; se suaviza (±4 tramos, 3 pasadas) y se cierra el circuito.
  - No se inventan velocidades.
- **Contenido:** mapa del recorrido coloreado, gráfico de velocidad, gráfico de aceleración y tabla de todas las vueltas.
- **Formato:** HTML interactivo que se ve dentro de la app o se comparte. El archivo se llama «🏁 nombre dd-mm-aaaa.html» y lleva favicon e imagen PNG de vista previa.
- **No hay** exportación CSV/JSON: se sacó a pedido de Matías.

## Parte nativa (plugin `Beto`)

**Métodos:**

| Tema | Métodos |
|---|---|
| Ubicación | `ubicacionPrecisa`, `pedirUbicacionPrecisa` |
| Notificaciones | `pedirNotificaciones` |
| No molestar | `estadoNoMolestar`, `pedirNoMolestar` |
| Pantalla | `estadoPantalla`, `pedirPantalla`, `orientacion`, `protegerPantalla`, `modoCarrera` |
| Arranque | `iniciarAlPrender` |
| Audio | `configurarAudio`, `volumenCarrera`, `prepararVoz`, `reproducir`, `hablar`, `estadoVoz` |
| Sensor de luz | `sensorLuz` |
| Pulsador | `configurarTeclas`, `listarDispositivos` |
| Actualizaciones | `puedeInstalar`, `pedirPermisoInstalar`, `descargarEInstalar` |

**Comportamientos:**

- **Pantalla protegida en carrera:** los botones de volumen no hacen nada y la pantalla se muestra sobre el bloqueo.
  - Si se apaga, vuelve sola: notificación de pantalla completa más `startActivity`. El *wake lock* solo no anda en el S23 Ultra.
  - La opción de **fijar la app** (y bloquear inicio/atrás) se probó y **se sacó** a pedido de Matías.
- **Modo carrera:** No molestar, pantalla completa y volumen al máximo.
- **Modo sol:** con el sensor de luz pasa a un modo claro (`:root.sol`) para leer a pleno sol.

**Permisos del manifiesto:** ubicación precisa, servicio en primer plano (ubicación), notificaciones, wake lock, `RECEIVE_BOOT_COMPLETED`, `USE_FULL_SCREEN_INTENT`, `SYSTEM_ALERT_WINDOW`, `REQUEST_INSTALL_PACKAGES`, No molestar y vibración.

- Por esta mezcla de permisos, **Play Protect a veces marca la app como dañina**. Matías decidió no tocar nada (ver `CLAUDE.md`).

## Valores de fábrica importantes (`DEF`)

| Ajuste | Valor |
|---|---|
| Tiempo ideal | 4:00 |
| Distancia | 3.200 m |
| Patrón | `alt1` |
| Cuenta en voz | 20 s |
| Pitido del cero | apagado (dice «cero») |
| Clic al marcar | prendido |
| Antirrebote | 3 s |
| Color medida | `#b3132b` |
| Color práctica | `#0f6e3a` |
| Indicador | pantalla completa de color |
| Fuente de velocidad | GPS |
| Umbral de precisión | 20 m |
| Tema | `avispon` |
| Número del auto | `101` |
| Modo sol | automático |
| Volumen al máximo en carrera | sí |
| Canal de audio | alarma |
