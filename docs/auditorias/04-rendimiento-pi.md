# Auditoría 04 — Rendimiento en la Pi 3 (07/11/2026)

Hecha sin la Raspberry: leí el código y medí en la PC (Docker, Xeon 2,1 GHz). **La Pi 3 B+ es entre 6 y 10 veces
más lenta por núcleo** (eso es estimado). Marco cada número como **[medido]** (en la PC) o **[estimado]** (para la Pi).

**Lo más importante:** la carrera en sí (motor, voz, GPS, pantalla) es liviana y está bien armada. El problema es
**guardar**: cada vuelta reescribe la base entera, y pasar números a texto es muy lento. Con muchas carreras guardadas
eso puede comerse un núcleo y cientos de MB de memoria **en plena carrera**.

## 1. Lo más grave

### 1.1 Guardar la base es lento y usa mucha memoria — crece con cada carrera guardada
- **Qué encontré:** `Crono::alEvento` → `Datos::agregarVuelta` (`app/datos.cpp:383-387`) llama a `guardar()` en **cada
  vuelta**. El hilo de guardado (`app/datos.cpp:86`, bien hecho: aparte y con prioridad baja) arma la base **entera**
  como árbol JSON y la escribe (`app/nucleo/almacen.cpp:86`). La traza va a 10 por segundo: una carrera de 12 vueltas
  de 4 min son ~30.000 filas.
- **Medido en la PC** (carreras de 12 vueltas a 10 Hz):

  | Base | Tamaño | Armar y escribir | Pico de memoria | Leer al prender |
  |---|---|---|---|---|
  | 1 carrera | 1,8 MB | 0,6 s | — | 0,06 s |
  | 5 carreras | 9 MB | 3,7–4,1 s | **116 MB** | 0,2 s |
  | 20 carreras | 36 MB | 15–18 s | **456 MB** | 1,0 s |
  | 50 carreras | 91 MB | 28–35 s | — | 1,6 s |

- **Estimado en la Pi:** con 20 carreras, **1,5 a 3 minutos de un núcleo por cada vuelta** y ~450 MB de pico en una Pi
  de 1 GB. Las vueltas son de ~4 min: el guardado no termina de una que ya empieza la otra. La memoria se va a zram
  (que comprime con el procesador): más trabajo y posibles trabas. Además escribe 36 MB con `fsync` en la misma
  tarjeta donde graba el video.
- **Riesgo:** alto si la base ya tiene varias carreras. Prender también se alarga (leer 91 MB ≈ 10–16 s en la Pi, con el
  vigilante de 30 s corriendo).
- **Propongo:** (a) arreglo rápido del punto 2.1; (b) de fondo, un archivo por carrera o guardar solo la carrera que
  cambió (eso es la **auditoría 05**). Mientras tanto, medir el tamaño real (plan, paso A).
- **Cuesta:** (a) chico; (b) medio/grande.

### 1.2 Abrir el informe traba la pantalla varios segundos
- **Qué encontré:** `Datos::informe` (`app/datos.cpp:291-295`) hace `datosInforme` → texto JSON → `QJsonDocument` →
  QVariant, todo en el **hilo principal**. Lo llama `Informe.qml:12` (y de nuevo en cada cambio de MEDIDA/LIBRE).
- **Medido en la PC:** 12 vueltas a 10 Hz: **0,7 s** (0,13 s el cálculo, 0,44 s pasar a texto, 0,14 s leerlo). 30 vueltas:
  **1,2 s**. Recorrido de 5 h a 1 Hz: 0,27 s. El informe manda 1,3 MB de datos a QML.
- **Estimado en la Pi:** 4 a 10 s con la pantalla congelada justo después de «Terminar» (la cámara sigue grabando esos
  10 s). No afecta la carrera, pero parece colgado.
- **Propongo:** 2.1 lo baja mucho; además armar el QVariant directo (sin pasar por texto) y calcularlo en un hilo
  aparte con un «cargando». **Cuesta:** chico (2.1) + medio (hilo).

## 2. Lo que más mejora con poco trabajo

### 2.1 Pasar números a texto: 40 veces más rápido
- **Qué encontré:** `numeroComoJs` (`app/nucleo/json.cpp:92-102`) prueba de 0 a 16 dígitos con `snprintf` + `strtod`
  hasta que el número vuelve igual. Una latitud necesita ~9 vueltas. Es lo que más tarda al guardar y en el informe.
- **Medido en la PC:** 3,6 millones de números: **7,0 s** con lo de ahora y **0,18 s** con `std::to_chars` (×40). Salen
  iguales salvo 7 casos (100000 sale «1e+05»): se arregla pidiendo los dígitos a `to_chars` en formato científico y
  armando el texto como ahora. Hay que pasar las pruebas de `pruebas/` (comparación con la app).
- **Riesgo si no se hace:** todo lo de 1.1 y 1.2. **Cuesta:** chico (una función, con pruebas).

### 2.2 Copia de la base en el hilo principal en cada vuelta
- `Datos::guardar` (`app/datos.cpp:73`) copia la base entera antes de mandarla al hilo. **Medido:** 5 ms con 20
  carreras, 12 ms con 50. **Estimado Pi:** 30–100 ms: la voz (cada 15 ms) y la pantalla se atrasan un poquito justo al
  cerrar la vuelta, que es cuando se dice «Vuelta N». **Propongo:** copiar solo la carrera que cambió (va con la 05).
  **Cuesta:** medio.

### 2.3 Cosas que guardan la base entera sin necesidad
- `Datos::ponerCfg` (`app/datos.cpp:574`): cada ajuste (brillo, volumen) reescribe todas las carreras con sus trazas.
- Recorrido libre: cada MARCAR (`app/crono.cpp:181` → `ponerExtra`) reescribe todo.
- **Propongo:** config en su propio archivo (va con la 05). **Cuesta:** chico/medio.

## 3. El resto

| # | Qué encontré | Riesgo | Propongo | Cuesta |
|---|---|---|---|---|
| 3.1 | Cámara MJPEG: `jpegdec ! videoconvert` en el procesador, en un solo hilo, sin `queue` entre medio (`app/camara/betometro_camara.cpp:217`) | **[estimado]** 60–120 % de un núcleo a 720p/30. Es el consumidor más grande en carrera; con 1080p ya hubo que bajar a 15 cuadros | Medir (plan, paso B). Si pasa de 120 %: `queue` entre `jpegdec` y `videoconvert` (reparte en 2 núcleos), o 720p a 15 cuadros, o pedirle 800×600 a la cámara | chico |
| 3.2 | Procesador «al máximo» toda la carrera (`app/salud.cpp:110`) | Más calor. La Pi 3 B+ ya baja de 1,4 a 1,2 GHz a los **60 °C**; en un auto al sol se llega | Medir la temperatura en carrera. Disipador o ventilador si pasa de 70 °C | — |
| 3.3 | Wifi: cada 10 s lanza 2 `nmcli` + 1 `iw`, también en carrera (`app/wifi.cpp:27,171`) | **[estimado]** 1–3 % de procesador. Bajo | En carrera, cada 30 s | chico |
| 3.4 | `Crono::refrescar` 20 veces/s arma un mapa de 22 datos; `gpsChanged` ~20 veces/s y lo leen 11 lugares de QML (`app/crono.cpp:258,287`) | **[medido PC]** el programa entero en carrera: 10 % de un núcleo de la PC. Bajo | Ver auditoría 03 (QML) | — |
| 3.5 | Traza con tope: `filas_.erase(begin())` al pasar 20.000 filas (`app/nucleo/modelo.cpp:19`) | Solo en vueltas de más de 33 min a 10 Hz: mueve ~1 MB por lectura | `std::deque` o anillo | chico |
| 3.6 | Drive, después de la carrera, arma el HTML de **todas** las carreras para comparar (`app/drive.cpp:363`), en el hilo principal | **[estimado]** varios segundos trabado, 20 s después de terminar, con muchas carreras | Guardar la huella al guardar la carrera | chico |
| 3.7 | `Tele.carreras` llama `Datos.informe` por cada carrera con video solo para contar vueltas (`app/qml/Tele.qml:61`) | Con 10 carreras con video: **[estimado]** 30–60 s trabado al abrir la lista en modo tele | Contar vueltas en C++ (`resumen` ya las tiene) | chico |
| 3.8 | Al apagar, el destructor espera el último guardado (`app/datos.cpp:69`) | Con base grande puede pasar el tiempo de systemd (90 s) y matarlo. No se pierde nada (`.nuevo`), pero tarda | Se arregla con 2.1 | — |

**Lo que está bien:** el GPS lee por `QSocketNotifier` y la hora de cada lectura sale del GPS (iTOW), así que una traba
de pantalla no corre los tiempos. El pulsador usa la hora del kernel. Guardar va en un hilo aparte. La voz no lee
archivos (QSoundEffect cargados al prender). Video del informe por el chip (`imagen_video`, `video_gst`). Prioridades
bien puestas (`Nice=-10`, cámara 5, wifi 10).

## 4. Plan de medición en la Pi

Hacelo con la cámara grabando, el GPS andando (alcanza con la Pi en el auto parado o en la ventana) y una práctica en
curso, 30–60 min. Primero `ssh beto@<IP>`, después pegá cada bloque.

**A. Foto rápida** (antes y durante la práctica):
```bash
vcgencmd measure_temp; vcgencmd get_throttled; vcgencmd measure_clock arm; cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor; free -m; uptime; ls -la /var/lib/betometro/; grep -E "VmRSS|VmHWM" /proc/$(pidof betometro)/status
```

**B. Grabar una hora** (temperatura, frenos, reloj, memoria y procesador por programa; quedan andando solos):
```bash
nohup bash -c 'for i in $(seq 1800); do echo "$(date +%T) $(vcgencmd measure_temp) $(vcgencmd get_throttled) $(vcgencmd measure_clock arm) $(awk "/MemAvailable/{print \"libreMB=\" int(\$2/1024)}" /proc/meminfo)"; sleep 2; done' > ~/salud.txt 2>&1 &
nohup top -b -d 5 -n 720 -w 250 > ~/top.txt 2>&1 &
```
Al terminar, el resumen:
```bash
grep -o "temp=[0-9.]*" ~/salud.txt | sort -t= -k2 -n | tail -1; grep -vc "throttled=0x0" ~/salud.txt; grep -o "libreMB=[0-9]*" ~/salud.txt | sort -t= -k2 -n | head -1
awk '/^ *[0-9]+ /{c[$12]+=$9} END{for(p in c) printf "%-22s %6.1f %%\n", p, c[p]/720}' ~/top.txt | sort -k2 -n -r | head -8
grep VmHWM /proc/$(pidof betometro)/status
```
(Línea 1: temperatura máxima, cuántas veces hubo freno, memoria libre mínima. Línea 2: procesador promedio por programa.)

**C. Qué parte de Betómetro usa el procesador** (durante la práctica):
```bash
top -H -b -d 5 -n 2 -p $(pidof betometro) | tail -20
```

**D. Cuadros por segundo de la pantalla** (temporal: se borra solo al reiniciar la Pi):
```bash
sudo mkdir -p /run/systemd/system/betometro.service.d
sudo tee /run/systemd/system/betometro.service.d/medir.conf >/dev/null <<'FIN'
[Service]
Environment=QSG_RENDER_TIMING=1
Environment=QT_LOGGING_RULES=qt.scenegraph.time.renderloop=true
Environment=QT_FORCE_STDERR_LOGGING=1
Environment="QT_MESSAGE_PATTERN=%%{time process} %%{message}"
StandardError=append:/tmp/render.log
FIN
sudo systemctl daemon-reload && sudo systemctl restart betometro
```
Después de la práctica:
```bash
awk '/rendered in/{s=int($1); n[s]++; match($0,/in [0-9]+ms/); ms=substr($0,RSTART+3,RLENGTH-5)+0; t++; if(ms>50) l++} END{for(s in n) c[n[s]]++; print t" cuadros, "l+0" lentos (más de 50 ms)"; for(k in c) print k" cuadros/s: "c[k]" segundos"}' /tmp/render.log | sort -n
sudo rm -r /run/systemd/system/betometro.service.d && sudo systemctl daemon-reload && sudo systemctl restart betometro
```
Ojo: los comandos de A, B y C los probé en la PC; el paso D no lo pude probar (en la PC sin pantalla Qt no dibuja). Si
`/tmp/render.log` queda sin líneas «rendered in», avisame y lo ajusto.

**Qué números son buenos o malos** (en `top`, 100 % = un núcleo; la Pi tiene 4):

| Qué | Bien | Ojo | Mal |
|---|---|---|---|
| Temperatura | menos de 60 °C | 60–70 °C (baja a 1,2 GHz) | más de 75 °C |
| `get_throttled` | `0x0` | — | cualquier otro (`0x50005` = tensión baja ahora) |
| Reloj en carrera | 1400000000 | 1200000000 | menos |
| `betometro` | menos de 40 % | 40–80 % | más de 80 % |
| `betometro-camara` | menos de 100 % | 100–150 % | más de 150 % |
| Memoria libre mínima | más de 300 MB | 150–300 MB | menos de 150 MB |
| `VmHWM` (pico de Betómetro) | menos de 300 MB | 300–500 MB | más de 500 MB |
| Cuadros en carrera | ~20 por segundo parejo | algunos de 15 | menos de 15 o muchos «lentos» |
| `betometro.json` | menos de 5 MB | 5–20 MB | más de 20 MB |

Y la prueba que ya pide E7: demora de voz y pulsador **igual con cámara que sin cámara**.

## Las 3 que haría primero
1. **Medir en la Pi** (pasos A y B): sobre todo el tamaño de `betometro.json`, el pico de memoria y el procesador de la cámara.
2. **Números a texto con `to_chars`** (2.1): chico, sin cambiar el formato, y baja guardar e informe a una fracción.
3. **Un archivo por carrera / guardar solo lo que cambió** (1.1, 2.2, 2.3): junto con la auditoría 05.

## Preguntas para Matías
1. ¿Cuántas carreras y prácticas tenés guardadas? ¿Me pasás lo que da el paso A (`ls -la /var/lib/betometro/`)?
2. ¿La Pi tiene disipador o ventilador? ¿Va al sol (tablero) o a la sombra?
3. Si la cámara usa mucho procesador: ¿preferís 720p a 15 cuadros (más liviano) o seguir en 30?
