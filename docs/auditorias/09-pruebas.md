# Auditoría 09 — Pruebas automáticas

Fecha: 07/11/2026 (sobre la 1.0.26). Solo lectura: no se tocó código. Lo medido se hizo en el contenedor
`beto` (Debian trixie, Qt 6.8.2, x86) y en los registros de GitHub Actions.

**En una línea:** el motor está muy bien probado (igual a la app, diferencia cero), pero **las pruebas no frenan
la publicación**: desde la 1.0.20 el workflow «Pruebas» está en rojo y se publicaron 9 versiones igual. Las
pantallas (8.400 líneas de QML) no tienen ninguna prueba, y una carrera entera por pantalla nunca se corre sola.

## Qué hay hoy

| Prueba | Qué cubre | Dónde corre |
|---|---|---|
| `motor_igual_a_la_app` (Node) | Motor C++ contra el `Motor` de la app: 65 casos, 773.041 líneas, diferencia cero | `pruebas.yml` PC y ARM |
| `informe_igual_a_la_app` (Node) | `armarRecorrido`/`datosInforme`: 30 carreras, diferencia cero | `pruebas.yml` PC y ARM |
| `prueba_nucleo` (74 chequeos) | JSON, almacén, vuelta interrumpida, solo cronómetro, voz, NEO, pulsador, unir, informe real | `pruebas.yml` PC (con sanitizers) y ARM |
| `prueba_datos` (50), `prueba_crono` (46) | Reglas de carreras; carrera entera en el `Crono` con reloj y GPS de mentira | `pruebas.yml`, job «Programa Qt» → **roto desde la 1.0.20** |
| `prueba_videos` (40), `prueba_drive` (47) | Lista de videos; lógica de Drive | **No corren en ningún lado** (`pruebas.yml:57` filtra `datos|crono`) |
| `raspios/probar.sh` | 40 s vivo sin pantalla, página web, 5 capturas | `programa.yml`, en ARM con QEMU |
| `herramientas/capturas.sh` | 48 capturas (24 pantallas × h/v) | `pruebas.yml`, roto; nadie las compara |

Tamaño: 7.800 líneas de C++ y 8.400 de QML; 1.760 de pruebas, todas del lado C++. Del QML, cero.

## 1. Los huecos más peligrosos

**1.1 Las pruebas no frenan la publicación.** `raspios/compilar.sh:15` compila con `-DBETO_PRUEBAS=OFF`;
`programa.yml` nunca corre `ctest`. Las pruebas viven en `pruebas.yml`, otro workflow que corre aparte y no
tiene relación con el paso «Publicar» (`programa.yml:99-112`). Resultado: el `[publicar]` sube el paquete aunque
las pruebas estén en rojo.
- Evidencia: «Pruebas» falla en todos los empujes desde la 1.0.20 (06/11 01:56) hasta hoy. Se publicaron 1.0.20 a
  1.0.26 así. Causa: `app/video_gst.cpp:5` pide `gst/app/gstappsink.h`; en el runner de Ubuntu (install-qt-action)
  no hay GStreamer, y `CMakeLists.txt:62` solo lo chequea para `betometro-camara`, no para `betometro`.
- Riesgo: **alto**. Es exactamente «un cambio en una pantalla rompe otra y nadie lo nota», pero peor: ya pasa con
  las pruebas que existen.
- Propuesta: una sola cadena. El paso Publicar (`needs`) espera a que pasen todas las pruebas. Costo: **chico**.

**1.2 Las pantallas no tienen prueba.** `probar.sh:39-47` saca 5 capturas y si una no sale solo dice «OJO» (no
falla). `capturas.sh` saca 48, se suben como artefacto y nadie las mira (el job está rojo hace 9 versiones).
`docs/capturas/` es del 02/11: contra la 1.0.26 difiere 37 % (inicio) y 67 % (ajustes). No sirve de referencia.
- Riesgo: alto. Un widget corrido, un texto tapado o un tema roto llegan a la Pi sin aviso.
- Propuesta: capturas comparadas contra referencias, con tolerancia (sección 3a). Costo: **medio**.

**1.3 El recorrido libre (1.0.26) no tiene prueba.** `app/crono.cpp:79-302` tiene 12 ramas `recorrido_`
(marcas que no cortan, traza a 1 por segundo, terminar normal, sin voz). Ninguna está en `prueba_crono`. El informe
del recorrido tampoco. Riesgo: medio-alto (es lo último que se hizo y «falta probarlo en el auto»). Costo: **chico**.

**1.4 Nada corre la carrera como la corre Matías.** `prueba_crono` le habla al `Crono` directo (bien). Pero no pasa
por `Carrera.qml` (MARCAR, Terminar con doble toque, lo que se muestra), ni por `GpsNeo` leyendo un puerto serie
real, ni por el guardado en segundo plano, ni por el informe que se ve después. Costo de cubrirlo: **medio** (3b).

**1.5 Wifi y cámara sin dobles en la cadena.** `app/wifi.cpp` tiene 13 ramas `real_`; en la PC es «simulado» y
`nmcli` nunca se ejercita. La red propia falló lejos de casa (06/11) justo ahí. La cámara sí tiene doble
(`BETO_CAMARA_PRUEBA`, `camara.cpp:90`, `videotestsrc`), pero `prueba_crono.cpp:112` lo saltea en GitHub porque
el runner no arma `betometro-camara`. `prueba_videos` y `prueba_drive` no corren nunca. Costo: **chico/medio** (3c).

**1.6 La cadena tarda 25 min y la mitad es apt.** Medido en la 1.0.26: voz 1 min; compilar 17 min (**apt-get
dentro de QEMU: 7 min**, cmake + ninja 10 min con ccache al 84 %); probar 7 min (**apt otra vez: 5,5 min**, el
resto 1,5 min). Esto lo mira también la auditoría 10; acá importa porque cualquier prueba nueva se suma.

## 2. Lo que más protege con poco trabajo

| | Qué | Costo |
|---|---|---|
| a | Publicar solo si pasan todas las pruebas (`needs`) | chico |
| b | Arreglar el job «Programa Qt»: correrlo en `debian:trixie` x86 (mismo Qt 6.8.2 y GStreamer que la Pi) en vez de install-qt-action 6.8.3. Correr `ctest` entero (datos, crono con video, videos, drive) | chico |
| c | `probar.sh`: que falle si una captura no sale; sumar «Unsupported image format» (ícono SVG sin plugin) al grep de la línea 20 | chico |
| d | Recorrido libre en `prueba_crono`: largar, 3 marcas, terminar; chequear `marcas[].m/kmh`, traza a 1/s, `interrumpida=false`, sin frases | chico |
| e | Capturas comparadas, primero solo tema Avispón h/v (el prototipo ya anda, ver 3a) | medio |

## 3. El plan completo, por etapas

### P1 (1 día): una sola cadena y las pruebas que ya existen
Mover todo a `programa.yml` como jobs en paralelo: `arm` (lo de hoy: compilar + probar.sh + paquete) y
`pruebas-x86` (trixie nativo: ctest completo + capturas + carrera simulada). El job `publicar` con
`needs: [arm, pruebas-x86]`. Como el ARM tarda 17 min solo en compilar, el x86 (≈10 min) **no alarga nada**.
Borrar `pruebas.yml` o dejarlo solo para el núcleo con sanitizers (1 min, vale la pena).

### P2: capturas comparadas (a)
Medido en el contenedor: una captura tarda **1,6 s** en x86 (5 s en ARM con QEMU); 8 en paralelo de a 4, 3,3 s;
comparar 4 capturas, 1 s (`/tmp/…/comparar_capturas.py`, 40 líneas con Pillow + numpy).
- **Qué cambia entre dos corridas iguales:** solo el reloj y el chip «GPS 11» de la barra de arriba
  (x 940-1280, y 0-70), el velocímetro animado del inicio y los datos del Chequeo. Tapando la barra,
  Ajustes y Carreras dan **0,000 %** de diferencia; Inicio 0,02 %; Chequeo 0,24 %.
- **Propuesta:** opción `--prueba fijo` (o `BETO_FIJO=1`): hora fija, GPS simulado con semilla fija
  (`gps.cpp:44` usa `QRandomGenerator::global()`), sin animación en el inicio. Así la tolerancia puede ser 0.
  Mientras tanto: tolerancia 0,3 % + máscara de la barra.
- **Matriz:** las 24 pantallas de `capturas.sh` en h/v con Avispón (48) + 6 pantallas clave (inicio, carrera,
  largada, informe, ajustes, chequeo) en los otros 3 temas y h/v (36) + modo sol, tablet-inicio, recorrido en
  carrera e informe, cámara, pronóstico, drive (≈14). **≈100 capturas: 40 s en x86 de a 4.** En ARM/QEMU
  serían 8 min: por eso van en el job x86. Las 5 de `probar.sh` en ARM quedan «para mirar».
- **Referencias:** `docs/capturas-ref/` en el repo (hoy `docs/capturas` ya pesa lo mismo). Se sacan en el mismo
  sistema donde se comparan (trixie x86): las letras pueden rasterizarse distinto en otra arquitectura o versión
  de Qt. Cuando un cambio es a propósito: `herramientas/capturas.sh --aceptar` regenera y se commitea.
- **Cuando falla:** el job sube `capturas-diferencias/` (la referencia atenuada con lo que cambió en rojo) y la
  lista «CAMBIO inicio-h.png: 2,1 % x951..1149 y10..59». Para Matías: una imagen, no un número.
- Costo: medio (el `--prueba fijo`, el script, la lista, las referencias iniciales que Matías aprueba una vez).

### P3: una carrera completa de punta a punta (b)
Hecho un prototipo con lo que ya hay: `simular_neo --largo 200 --kmh 48` + `betometro --gps … --pantalla carrera
--prueba recorrido-largo` sin pantalla. Resultado en `betometro.json`: recorrido de 42,0 s, 578 m, 6 marcas con
metros y km/h, 41 puntos de traza (1/s), `fuente: terminar`. Funciona; falta que **termine solo y chequee**.
- **Propuesta:** opción `--prueba guion:ARCHIVO`. El guion dice qué hacer y cuándo, como el navegante:
  `0 largar · 15.0 marcar · 30.1 marcar · 45.0 marcar · 52 terminar-doble-toque · salir`. Lo lee
  `Carrera.qml` (ya tiene el `Timer` de `recorrido-largo`, líneas 37-45, y `Crono.marcar(fuente, edadMs)`).
  Al terminar sale con código 0. Dos corridas: **carrera** de 3 vueltas de 15 s con patrón `alt1` (la vuelta 3
  queda interrumpida) y **recorrido libre** de 40 s con 4 marcas. Tiempo total ≈ 2 min.
- **Qué se verifica** (script en Python sobre `betometro.json` y sobre `http://127.0.0.1:8080/informe/<id>`):
  cantidad de vueltas, `realMs` ± 150 ms de lo que dice el guion, `difMs` contra el ideal, `medida`/libre según
  patrón, `distM` ≈ 200 m ± 5 %, traza con muestras, `interrumpida`, las frases de voz emitidas (`Crono::frase`),
  que el informe HTML tenga N filas y el mapa, y en el recorrido: marcas con `m` y `kmh`, sin frases.
- También: una carrera con `--corte 5` (sin señal entre árboles) y una con `--mala 0.1`; el guardado en segundo
  plano (`betometro.json` y `.anterior` sanos al salir); y la misma corrida con `--sin-gps` (solo cronómetro).
- Costo: medio. Protege lo que más importa el día de la carrera.

### P4: dobles de wifi y cámara (c)
- **Wifi:** `wifi.cpp:45-55` arranca `nmcli` por nombre. Agregar `BETO_NMCLI=/ruta/nmcli-falso` (un script de
  shell en `pruebas/dobles/`) que contesta según los argumentos y anota lo que le pidieron. Casos: red guardada
  cerca → se conecta; nada en 90 s → arma «Betometro-101»; llave «Conectarse a redes wifi» apagada → corta y arma
  la propia enseguida; falla `dnsmasq` → el motivo se ve en Conectividad. Prueba nueva `prueba_wifi` con
  `BETO_RASPBERRY=1` y `BETO_NMCLI`. Costo: **medio** (el doble es chico; lo caro es separar el reloj de 90 s para
  poder adelantarlo, como `ponerReloj` en `Crono`).
- **Cámara:** ya existe (`BETO_CAMARA_PRUEBA` → `videotestsrc`). Solo hay que correr `prueba_crono` donde esté
  `betometro-camara` (el job trixie x86, GStreamer incluido). Sumar un caso MJPEG (`pattern=smpte ! jpegenc`,
  `betometro_camara.cpp:281`) y la conversión a MP4. Costo: **chico**.
- **Pulsador:** `prueba_nucleo` ya prueba el antirrebote; `PulsadorGpio` real no se puede sin Pi. Queda en el
  Chequeo previo.

### P5: cuánto tarda y dónde corre (d)

| Job | Dónde | Hoy | Con el plan |
|---|---|---|---|
| Voz | runner | 1 min | 1 min |
| ARM: compilar + probar.sh + paquete | QEMU | 24 min | 24 min (13 con imagen Docker con los paquetes ya instalados, auditoría 10) |
| x86: ctest + ≈100 capturas + carrera simulada | trixie nativo | — | ≈10 min en paralelo (apt 1,5 + compilar 6 sin caché / 1 con ccache + pruebas 3) |
| Núcleo con sanitizers | runner | 3 min | 1,5 min (sin el job roto) |
| **Publicar** | runner | sin esperar pruebas | **espera a todos** |

Total: no se pasa de los 25 de hoy (manda el ARM). Para no llenar GitHub: el job x86 corre en todo empuje sin
`[skip ci]`; el ARM solo con `[publicar]` o a pedido (hoy corre en cada empuje a cualquier rama que toque `app/`).

## Las 3 que haría primero
1. **Publicar solo si pasan las pruebas** y arreglar el job roto corriéndolo en `debian:trixie` x86 (2.a + 2.b + 2.c).
   Un día. Es lo que cierra el agujero por el que pasaron 9 versiones.
2. **Carrera y recorrido libre de punta a punta** con `simular_neo` y el guion (P3 + 2.d). Es lo que protege el día
   de la carrera: tiempos, vueltas, guardado e informe.
3. **Capturas comparadas** con `--prueba fijo`, primero Avispón h/v, después los 4 temas (P2).

## Preguntas para Matías
1. ¿Querés que el `[publicar]` **no salga** si falla una prueba, aunque sea una captura que cambió a propósito?
   (Mi consejo: sí. Si el cambio es querido, se aceptan las capturas nuevas y se vuelve a publicar.)
2. Cuando cambie una pantalla a propósito, ¿querés **ver la imagen de antes y después** antes de aceptarla, o te
   alcanza con que te avise «cambió Ajustes 2 %»?
3. ¿Preferís que el ARM de 24 min corra **solo cuando se publica** (y en cada empuje solo el x86 de 10 min)?
   Ahorra tiempo de GitHub; la contra es que un error solo de ARM se ve recién al publicar.
4. Para la carrera simulada, ¿hay algún circuito de verdad (distancia e ideal) que quieras que sea «el caso de
   referencia», además de los 200 m de prueba?
