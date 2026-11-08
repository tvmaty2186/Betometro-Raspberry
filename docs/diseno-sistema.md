# Betómetro para Raspberry Pi — diseño del sistema

> **Ojo (03/11/2026, repasado el 08/11):** el sistema ya no es Buildroot sino Raspberry Pi OS Lite con Betómetro como programa. Lo de Buildroot, las copias A/B, Piper y el amplificador I2S de abajo quedan como historia: hoy la voz está grabada (edge-tts) y sale por los auriculares, y el simulador está en el Chequeo previo. Lo vigente está en [raspios.md](raspios.md). El cableado, las pantallas y las decisiones siguen valiendo.

Estado: **diseño acordado con Matías (01/11/2026). Muestras horizontales y verticales aprobadas («están preciosas», «perfectas»). Todavía no se programó nada.**

**Regla:** la app Android vive en otro repo ([tvmaty2186/Betometro](https://github.com/tvmaty2186/Betometro)) y **no se toca**. Este repo es solo la Raspberry.

## Qué es

- Equipo **autónomo**: no necesita el celular para nada.
- **Betómetro es el sistema operativo:** se prende y arranca directo en Betómetro. No hay escritorio, ni usuario, ni navegador.
- Programa **nativo, hecho de cero**. No es un emulador de la app del celular.

## Equipo

- Raspberry Pi 3 B+
- Pantalla Raspberry Pi Touch Display 2 de 5" (DSI, 720×1280)
- GPS GY-NEO-M8N por UART (cableado en [cableado-gps.png](cableado-gps.png))
- **Auriculares con cable** en la salida de audio de la Raspberry (ficha de 3,5 mm), para la voz
- Pulsador en GPIO

### Pines

| Pin | Uso |
|---|---|
| 2 y 6 | Alimentación de la pantalla |
| 17 (3,3 V), 14 (GND) | GPS (no el pin 4: lo tapa la ficha de 3 lugares de la pantalla, que ocupa 2, 4 y 6) |
| 8 (TX) → RXD del GPS | GPS |
| 10 (RX) ← TXD del GPS | GPS |
| 7 (GPIO4) | PPS del GPS (opcional) |
| 11 (GPIO17) y 9 (GND) | Pulsador (propuesto) |

- El Bluetooth se apaga (`dtoverlay=disable-bt`) para usar el UART bueno con el GPS.
- El GPS se configura al arrancar: **GPS + GLONASS a 5 Hz** (el NEO-M8N no da más con dos sistemas; opción GPS solo a 10 Hz), 115200 baudios. Ver [gps-neo-m8n.md](gps-neo-m8n.md).
- La hora del sistema se toma del GPS (la Raspberry no tiene reloj propio).

## Sistema operativo

**Buildroot**: un Linux armado a medida, solo con lo necesario.

Por qué (para que no se tilde ni se trabe):
- Pocos programas corriendo: arranca rápido (unos 10–15 s) y no hay nada que compita con Betómetro.
- **Sistema de solo lectura:** cortar el contacto no lo rompe.
- Las carreras van en una partición aparte, con escritura segura (archivo nuevo y después reemplazo).
- **Vigilante (watchdog) del procesador:** si el programa se colgara, la Raspberry se reinicia sola y vuelve a Betómetro.
- **Dos copias del sistema (A y B):** la actualización se escribe en la copia que no se está usando. Si la nueva no arranca bien, vuelve sola a la anterior.

GitHub Actions arma la imagen completa (`.img`). La primera vez se graba en la tarjeta con Raspberry Pi Imager.

## Programa

- **Qt 6 con QML**, dibujando directo en la pantalla (EGLFS), con aceleración de la placa de video. Es lo que se usa en tableros de autos.
- **Motor:** se reescribe en C++ con la misma lógica que `Motor` de la app. Se prueba con los mismos datos que el motor de la app: tienen que dar igual.
- **Vertical u horizontal:** lo elige el usuario desde el inicio o desde Ajustes. Cada orientación tiene su armado de pantalla.
- **Audio:** salida de auriculares de 3,5 mm de la Raspberry (`dtparam=audio=on`), con auriculares con cable. No hay parlante ni amplificador.
- **Voz:** Piper (voz natural, español argentino). Los números y frases se graban al armar la imagen; la Raspberry solo los reproduce, sin demora.
- **Diseño:** misma línea estética que la app (temas, Avispón Verde). Antes de programar, se muestran imágenes de muestra.

## Wifi y conectividad

**Aprobado el 01/11/2026:** todo lo de [propuesta-mejoras.md](propuesta-mejoras.md): menú Conectividad, informes en `betometro.local`, wifi propia «Betometro-101» si no hay red, y las mejoras del GPS (G1 a G7).


- Ajustes → Wifi: elegir red y escribir la clave con teclado en pantalla.
- En la pista lo normal es conectarse a la **zona wifi del celular**.

## Informes

- Se ven **en la pantalla de la Raspberry** (mapa, velocidad, aceleración, tabla), dibujados por el programa.
- **Compartir por wifi:** la Raspberry muestra un **código QR**. Con el celular en la misma red, se escanea y se descarga el informe en HTML, igual al de la app.

## Actualizaciones

- Solo cuando el usuario quiere: Ajustes → Actualizar, o tocando la versión con el texto amarillo «· Actualizar» en el inicio.
- Busca la última *release* de GitHub, descarga el paquete del sistema y lo instala en la otra copia (A/B). Reinicia en la versión nueva.

## Pantallas (muestras aprobadas, horizontal 1280×720)

Imágenes: [muestras-horizontal-1.png](muestras-horizontal-1.png), [-2](muestras-horizontal-2.png), [-3](muestras-horizontal-3.png).
Fuente editable: [muestras/pantallas-horizontal.html](muestras/pantallas-horizontal.html) (usa las fuentes y las imágenes de `recursos/`). Para regenerar, se abre con Playwright a 1280×720 y se saca captura de cada `#s1`…`#s12`.

**Temas:** los 4 de la app, con sus mismos colores, fuentes y estilos: Avispón Verde (de fábrica), Rally clásico (menús tipo «carnet» de papel), Deportivo moderno y Cabina de avión. Las variables de cada tema están en [app/diseno.md](app/diseno.md) y en `referencia/app-2.1.1/index.html` (bloques `:root[data-tema="…"]`). Las muestras se hicieron solo en Avispón; los otros 3 se muestran con capturas del programa real en la etapa 2.

**Regla de estética:** los temas, colores, tipografías y componentes son **iguales a la app** en todo lo posible (ver [app/diseno.md](app/diseno.md), copia de la guía de la app).

| # | Pantalla | Qué tiene |
|---|---|---|
| 1 | Inicio | Cabecera Avispón (león, título cromo, rayas, versión con «· Actualizar» amarillo). La foto de los pilotos ocupa casi toda la izquierda (a Matías le parece bien). 101 sobre la banda, nombre del auto y chips de GPS y wifi. Velocímetro crema a la derecha y los 5 botones en arco, con el nombre a la izquierda de cada disco (de abajo hacia arriba: Carreras, Práctica libre, Chequeo previo, Pantalla de carrera, Ajustes). |
| 2 | Carreras | Bandas PRÓXIMAS y CORRIDAS, fecha en redondel cromado, LARGAR en cada próxima, Informe en cada corrida, «+ Nueva». |
| 3 | Ficha de la carrera | A la izquierda los datos editables (nombre, fecha, vueltas, distancia, tiempo ideal, vueltas medidas, velocidad promedio). A la derecha el resumen, las vueltas medida/práctica y LARGAR grande. |
| 4 | Carrera en curso | Arriba: tipo de vuelta (V3 MEDIDA), cuenta «Falta» enorme con metros y barra, Sugerida. Abajo: velocidad digital con barra, Última vuelta y MARCAR. Línea superior con la carrera, el estado del GPS y Terminar. |
| 5 | Práctica libre | Tiempo ideal, distancia, vueltas medidas y LARGAR grande. |
| 6 | Chequeo previo | GPS (satélites, precisión, lecturas por segundo, hora del GPS), pulsador, voz y auriculares, wifi, lugar libre, TODO LISTO. |
| 7 | Pantalla de carrera (editor) | Barra con Vertical/Horizontal, + Widget, De fábrica, Guardar. Widgets con borde punteado amarillo y ✎. |
| 8 | Ajustes | Mi auto, Tema y pantalla (tema, **orientación**, modo sol, **brillo**), Voz y sonido (**volumen auriculares** con Probar, cuenta regresiva), Conexión y sistema (**wifi**, **versión y Actualizar**, **Apagar el equipo**). |
| 9 | Wifi (clave) | Lista de redes, clave y teclado en pantalla con ñ. Se abre al tocar una red en Conectividad. |
| 13 | Conectividad | Estado, Betómetro en la red (QR y `betometro.local`), redes guardadas con Olvidar, redes cerca y Buscar. Se entra desde Ajustes → Conexión y sistema → Conectividad. Muestra: [horizontal](muestras-conectividad-horizontal.png), [vertical](muestras-conectividad-vertical.png) (aprobadas). |
| 10 | Informe | Recorrido coloreado por velocidad, gráfico de velocidad, tabla de vueltas con error, Compartir. |
| 11 | Compartir por QR | Código QR y los 3 pasos para bajarlo al celular en la misma red. |
| 12 | Actualizar | Versión actual → nueva, qué trae, barra de descarga. Se instala en la copia de reserva; si falla vuelve sola a la anterior. |

Lo nuevo respecto de la app: orientación, brillo, volumen de auriculares, wifi, apagar el equipo, chequeo con hora del GPS y lugar libre, compartir por QR y la pantalla de actualizar.

## Decisiones propias de la Raspberry (01/11/2026)

Cambian lo que hace la app en estos puntos:

- **Largar sin hora:** se puede largar aunque el GPS todavía no haya dado la hora (no se exige que la carrera sea «de hoy»).
- **Correr sin GPS (solo cronómetro):** si no hay GPS, la carrera sigue solo con el tiempo: cuenta regresiva, voz, MARCAR y tiempos de vuelta. Sin velocidad sugerida, sin metros y sin recorrido en el informe.
- **Respaldo:** no se exporta respaldo de la base.
- **Informes:** se exporta el informe **de cada carrera** (HTML, por QR).
- **Terminar a mano:** la vuelta en curso **no se pierde**: se guarda como **vuelta interrumpida** (se ve en la ficha y en el informe, marcada así, y no cuenta como vuelta completa).
- **Volumen:** un solo ajuste, en Ajustes → «Voz y sonido» → «Volumen auriculares», con botón ▶ Probar. **De fábrica viene al máximo (100 %).** Es el volumen con que se escucha la voz al largar y durante la carrera. Si el usuario lo cambia, se guarda y se usa siempre el último que eligió. (No se fuerza el máximo al largar.)
- **Fondo en carrera:** como la app: toda la pantalla roja en vuelta medida y verde en práctica (indicador «fondo»). Decidido el 01/11/2026.
- **MARCAR en pantalla:** siempre visible, aunque el pulsador esté conectado (por si el pulsador falla en carrera). Los dos marcan.
- **± segundos en la pantalla de carrera (07/11/2026):** cuadro «delta» con el número grande (+2,4 / −1,0) y abajo
  ADELANTADO (verde), ATRASADO (amarillo) o EN TIEMPO. De fábrica: acostada arriba a la derecha (la Sugerida baja al lado
  de la velocidad), de pie abajo al lado de la última vuelta. Distinto de la app, decidido por Matías.
- **Recorrido libre (07/11/2026):** en Práctica libre. Una sola «vuelta» abierta y libre, sin tiempo ideal ni distancia
  (`extra.recorrido`), sin voz. MARCAR deja marcas (`extra.marcas`: `{n, ms, m, kmh}`), no corta. Al terminar no queda
  «interrumpida». Pantalla: `widgets/TableroRecorrido.qml`; informe: resumen y marcas (`datosInforme` → `resumen`, `marcas`).
  Muestras: [muestras/recorrido/](muestras/recorrido/).
- **Simulador:** solo para desarrollo, nunca en el equipo final. Simula **los datos que manda el NEO-M8N** (frases NMEA por el mismo camino que el UART), así se prueba toda la cadena: lectura, motor, voz y pantalla.

## Pantallas en vertical (720×1280, aprobadas)

Imágenes: [muestras-vertical-1.png](muestras-vertical-1.png), [-2](muestras-vertical-2.png), [-3](muestras-vertical-3.png). Fuente: [muestras/pantallas-vertical.html](muestras/pantallas-vertical.html) (`#v1`…`#v12`).

Las mismas 12 pantallas que en horizontal, apiladas en una columna. Diferencias:
- **Inicio:** como la app: foto arriba, banda con el 101, velocímetro crema abajo a la derecha (sale un poco del borde) y los 5 botones en arco con la fórmula de `layoutInicio()` (k = min(W/390, H/844); centro en W − 60k y 0,717·H; diámetro 330k; radio 262k; ángulos 150° a 270°).
- **Carrera:** armado de fábrica de la app reacomodado para que entre: velocímetro redondo de 420 px arriba; V3 a la izquierda y Sugerida a la derecha, montados sobre el borde del velocímetro; la cuenta abajo, sin tocarlo; Última vuelta en una franja; MARCAR abajo a todo el ancho.
- **Informe:** la tabla muestra la **vuelta interrumpida** (sin error).

## Decisiones del 08/11/2026 (1.0.40 a 1.0.48)

**Personalizar y temas**
- **Personalizar desde el celu** (QR con llave que cambia en cada arranque): foto del inicio (acostada y parada por
  separado, se mueve y se pellizca), número y nombre del auto, colores y velocímetro del inicio. El botón del QR
  aparece **solo con el tema Propio** (Ajustes → Pantalla y tema, debajo de Tema).
- **Tema propio:** parte del Deportivo y cambia solo fondo, acento y letras (blanco, crema o negro). Paleta de **127
  colores en panal + 9 grises**; los **círculos de Fondo y Acento** (abajo del panal) eligen qué se pinta, sin botones.
  Avisa si hay poco contraste. **No cambian:** barra (verde/amarillo/rojo), adelantado/atrasado, medida/libre
  (rojo/verde), avisos y modo sol. Se conserva la bandera a cuadros.
- **Gráficos, mapas y tablas de datos** (informe y video): en el tema propio siempre sobre el **gris oscuro del
  Deportivo** (`Tarjeta { datos: true }`); el error total del informe sobre una pastilla oscura.
- **El Avispón Verde es el 101 de Matías:** siempre su foto, el 101 y «Avispón Verde», aunque se haya personalizado.
  Los otros temas usan lo personalizado (no se borra).
- **Velocímetro del inicio:** 6 estilos (clásico, deportivo, digital, rally, aviación, anillo); el elegido vale
  **solo en el tema propio** (de fábrica, Deportivo). Cada tema de fábrica usa el suyo.
- Cursor de las barras: **triángulo blanco con borde negro**, centrado sobre la barra (negro en modo sol).

**Informe**
- **Pulsador virtual (GPS)**, solo de referencia, nunca cambia las marcas: la línea es donde se largó; tiempo y error al
  lado del manual, «dudosa» si el GPS andaba mal, y al pie cuánto se marcó tarde en promedio. **Solo en el informe.**
- **Vuelta modelo ★** = la medida más cerca del ideal. **Comp.** es el cuarto modo (Vel. · Acel. · Adel. · Comp.):
  la vuelta elegida contra la modelo; doble toque abre el tiempo ganado/perdido y 4 sectores.
- Con el video: **Mapa | Gráfico** al lado (el gráfico con los modos, raya donde va el video, toque para saltar).
- Sin el aviso de «video cortado» (no aporta).

**Equipo**
- **Apagar** abre una ventana: Apagar, Reiniciar, Cancelar.
- **Chequeo previo → Memoria** con semáforo (verde < 60 %, naranja < 80 %, rojo) y **Despejar** (dos toques: reinicia
  Betómetro, no la Raspberry, y vuelve al Chequeo diciendo cuánto liberó).
- **Vigilante:** reporte solo si la memoria pasa del 70 %, la pantalla se congela más de 1,5 s o hay tirones en carrera;
  con procesos, crecimiento, últimas pantallas y la memoria de la última media hora. Un aviso por motivo cada 10 min.
- Prueba completa: la falta de wifi no es una falla (se prueba la antena).

## Pendiente de definir

- Después: empezar a programar (imagen Buildroot, motor en C++, pantallas en Qt).
