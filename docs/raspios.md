# Betómetro en Raspberry Pi OS — cómo está hecho, cómo se publica y lo aprendido

Actualizado el 04/11/2026 (versión 1.0.12). Leelo antes de tocar el sistema, el instalador o las versiones.

## El equipo
- Raspberry Pi 3 B+ (1 GB) + **Touch Display 2 de 5"** (720×1280, de pie; se usa acostada: Qt gira la imagen).
- Raspberry Pi OS Lite 64 bits (Debian **trixie**), Qt 6.8.2 de Debian, `eglfs_kms`, Mesa vc4.
- Usuario `beto`. En casa la IP es `192.168.1.83` (`ssh beto@192.168.1.83` desde PowerShell). Paso a paso y atajo: [conectarse-ssh.md](conectarse-ssh.md).
- La luz de fondo es `/sys/class/backlight/panel_backlight@1`.

## Hasta cuándo sirve trixie (11B-3)
- Debian 13 «trixie» tiene soporte normal hasta mediados de **2028** y después pasa a archivo (`archive.debian.org`):
  `compilar.sh`/`pruebas.sh` (que instalan paquetes en `debian:trixie`) dejarían de poder bajarlos y en la Pi
  `apt-get install` fallaría. Cuando pase: cambiar las fuentes de apt a `archive.debian.org` o pasar a Debian 14 (Qt nuevo:
  recompilar y probar todo). No es urgente; anotado para no sorprenderse.
- `edge-tts` (la voz) va con versión fija en `programa.yml` (`edge-tts==7.3.1`, la última al 07/11/2026): si Microsoft
  cambia la API, se sube el número a propósito.

## Qué instala `raspios/instalar.sh`
**Desde la 1.0.28:** desarma la versión nueva aparte y la cambia con `mv` (un corte de luz no la deja a medias);
si falla a mitad vuelve sola a la anterior y prende Betómetro; `cmdline.txt` se escribe solo si cambia; deja
`/var/lib/betometro/actualizacion/resultado` («Se instaló la X» o el error), que Betómetro muestra al prender.
**Vuelta atrás automática:** si la versión recién instalada se cae 4 veces en 5 minutos, `betometro-volver.service`
corre `/var/lib/betometro-anterior/volver.sh` y avisa. Si anduvo un minuto, ya no vuelve sola (archivo `a-probar`).
Probado en Docker con el `config.txt` y el `cmdline.txt` reales de la Pi (07/11/2026).

1. Paquetes de `raspios/paquetes.txt` (antes repara dpkg con `dpkg --configure -a`; solo instala lo que falta).
   - 1b: **con internet**, actualiza solo `ca-certificates`, `tzdata`, `openssh-server`, `dnsmasq-base` y `wpasupplicant`
     (`--only-upgrade`, con tiempo límite; nunca núcleo, Qt, Mesa ni GStreamer) y guarda las versiones en
     `/var/lib/betometro/versiones.txt` (van en cada reporte de errores). Sin internet sigue igual (11B-2.1).
   - 3f: zona horaria `America/Argentina/Buenos_Aires` y nombre `betometro`, solo si no están (11B-2.4).
2. Guarda la versión anterior en `/var/lib/betometro-anterior` (`volver.sh` para volver a mano), copia el programa, la voz y los servicios.
3. `config.txt`: `dtoverlay=vc4-kms-dsi-ili9881-5inch`, `enable_uart=1`, `disable-bt`, audio, watchdog, `disable_splash=1`, `ramoops` (si existe).
   - `cmdline.txt`: sin consola serie (el puerto es del GPS), `console=tty3 quiet splash … video=Composite-1:d fsck.repair=yes`.
4. 3c, menos escrituras: journald guarda en la tarjeta solo avisos y errores (`MaxLevelStore=warning`); sin `apt-daily` ni `man-db`; memoria de reserva en zram (sin `dphys-swapfile`).
5. 3b, pantalla de arranque Plymouth (`raspios/arranque/`): si cambió la foto, rearma el initramfs. **Tarda varios minutos en la Pi 3 y la pantalla queda negra con un guión parpadeando: es normal, no apagar.**
6. Servicios: `betometro`, `pulseaudio-sistema` (el Qt de Debian solo suena por PulseAudio) y `despertar-touch` (el touch aparece antes que la pantalla). `getty@tty1` apagado. Reinicia.

## `betometro.service`
- `BETO_RASPBERRY=1`, `BETO_DATOS=/var/lib/betometro`, `BETO_GPS=auto`, `BETO_VOZ=/usr/share/betometro/voz`.
- `Restart=always`, **`WatchdogSec=30` + `NotifyAccess=main`**: el programa avisa «sigo vivo» cada 5 s (`app/salud.cpp`); si se congela, systemd lo reinicia.
- Prioridad: `Nice=-10`. El programa baja a sus hijos: nmcli `nice 10` y cámara `nice 5`.
- `ExecStartPre=plymouth quit --retain-splash`: la foto queda hasta que Betómetro dibuja.

## Cómo se arma y se publica una versión
- El número está en `CMakeLists.txt` (`project(betometro VERSION x.y.z)`). Las versiones son propias de este repo y empezaron en 1.0.0.
- Push a `claude/adoring-bell-disf3a`. El workflow `.github/workflows/programa.yml` compila en un Debian trixie arm64 (QEMU), corre `raspios/probar.sh` (40 s sin pantalla, servidor web, capturas) y arma `betometro-pi-X.Y.Z.tar.gz`.
- **Publicar:** el mensaje del commit tiene que tener `[publicar]` (los tags no se pueden empujar desde la sesión: 403). Sube el paquete al repo público `tvmaty2186/Betometro-actualizaciones` con el secreto `ACTUALIZACIONES_TOKEN`. Falla si esa versión ya existe.
- **Commits intermedios con `[skip ci]`**, para no lanzar compilaciones. Nunca empujar mientras compila una (Matías se enojó mucho con los reinicios).
- Tarda unos 15 minutos con caché. Para ver el estado:
  `curl -H "User-Agent: x" https://api.github.com/repos/tvmaty2186/Betometro-Raspberry/actions/runs/<id>`.
- Desde la sesión, el proxy no deja leer `Betometro-actualizaciones` por API. Para comprobar la publicación, bajar los primeros bytes con
  `curl -L -r 0-10 https://github.com/tvmaty2186/Betometro-actualizaciones/releases/download/vX.Y.Z/betometro-pi-X.Y.Z.tar.gz` (206 = está).

## Actualización en la Raspberry (`app/actualizador.cpp`)
- Busca a los 20 s de prender, al conectar el wifi y cada 15 min. Si falla, reintenta cada minuto.
- Primero pregunta a la API (`…/releases/latest`). **La API deja 60 consultas por hora por casa**; pasado eso contesta 403 y antes se veía «sin internet». Ahora, si falla, lee la redirección de `https://github.com/…/releases/latest` y baja `…/download/vX.Y.Z/betometro-pi-X.Y.Z.tar.gz`.
- Instala con `systemd-run --unit=betometro-actualizar bash instalar.sh` (no se corta si se cierra Betómetro). Sin botón REINICIAR: cortaba la instalación.
- Pantalla Actualizar: botón **BUSCAR** por internet y «Buscar en pendrive»; dice «Sin internet» o «la última publicada es la X».
- **Instalar a mano** (si la Pi tiene una versión vieja que no encuentra la nueva):
  ```bash
  cd /tmp && curl -LO https://github.com/tvmaty2186/Betometro-actualizaciones/releases/download/vX.Y.Z/betometro-pi-X.Y.Z.tar.gz && tar xzf betometro-pi-X.Y.Z.tar.gz && sudo bash betometro-pi-X.Y.Z/instalar.sh
  ```

## Dónde quedan los datos (`/var/lib/betometro`, desde la 1.0.29)
Un archivo por carrera (auditoría 05, sección 4, opción B; `app/nucleo/almacen.cpp`, `app/datos.cpp`):
- `config.json`: la configuración guardada (sin mezclar con la de fábrica).
- `carreras/<id>.json`: cada carrera, exactamente una «carrera» del formato de la app. Se guarda **solo la que
  cambió** (una vuelta: ~50 KB, no la base entera). Escritura segura por archivo (`.nuevo` + fsync + renombrar;
  queda `.anterior`). Un archivo roto se aparta como `.danado` y se usa su `.anterior`; los demás se leen igual.
- `carreras/papelera/`: las carreras borradas; se vacía a los 30 días (al prender).
- `betometro.json.migrado`: la base vieja de un solo archivo (hasta la 1.0.28). Al prender por primera vez con la
  1.0.29 se migra: se escribe `config.json` y cada carrera en `carreras.nuevo/`, se vuelve a leer todo y se compara
  con lo original; recién ahí pasa a `carreras/` y el viejo queda como `.migrado` (**nunca se borra**). Si se corta
  a mitad, la próxima vez repite. Si algo no coincide, avisa en el reporte y sigue con el archivo viejo.
- Lo demás: `videos/`, `informes/`, `reportes/errores.log`, `clima.json`, `drive.json`, `drive-cliente.json`.
- **Copia de todo** en el formato de la app (`{schemaVersion, config, carreras}`): `http://<IP>/betometro.json`
  (link «Copia de la base» al pie de la página) y Drive la sube como `Betómetro/betometro.json` junto con los
  informes, pisando la anterior.
- Bajar la base a la compu: `scp beto@betometro.local:/var/lib/betometro/betometro.json .` (hasta la 1.0.28) o
  `scp -r beto@betometro.local:/var/lib/betometro/carreras .` (desde la 1.0.29).

## Partes del programa propias de la Raspberry
| Archivo | Qué hace |
|---|---|
| `app/gps.cpp` | GPS u-blox (M8, 7 o 6; lo detecta con MON-VER). `auto`: busca por USB (`ttyACM*`, `ttyUSB*`) y si no por los pines (`ttyAMA0`); se puede enchufar con el programa andando. La hora de cada lectura sale del iTOW del GPS. Ver [gps-neo-m8n.md](gps-neo-m8n.md) |
| `app/pulsador_gpio.cpp` | Pulsador en GPIO17 (pin 11 a GND pin 9), pull-up interna, hora del kernel en cada flanco |
| `app/salud.cpp` | Tensión baja y calor (`get_throttled`, `thermal_zone0`), vigilante de systemd, procesador al máximo en carrera salvo con calor o tensión baja (vuelve 30 s después) |
| `app/sistema.h` | Apagar y **brillo** (escribe la luz de fondo; 100 % = máximo de la pantalla) |
| `app/wifi.cpp` | nmcli. Wifi propia «Betometro-<número>» (clave `betometro`) si en 90 s no hay red guardada cerca. **«Conectarse a redes wifi»** (Conectividad, cfg `wifiRedes`): apagado, se desconecta, no se conecta a ninguna red y arma la propia enseguida (si NetworkManager se conecta solo a una guardada, la corta) |
| `app/servidor.cpp` | «Betómetro en la red»: lista, informes (tema Avispón), zip, reporte de errores, `/betometro.json` (toda la base, formato de la app), `/drive` (subir el cliente de Google). Puerto 80 en la Pi |
| `app/reportes.cpp` | `<datos>/reportes/errores.log`. Al prender distingue «se cayó el programa» de «se cortó la corriente o se colgó», y suma errores del sistema (journal) y del pstore. Manda issues a `Betometro-reportes` si existe `/var/lib/betometro/reportes.token` |
| `app/actualizador.cpp` | Ver arriba |
| `app/clima.cpp` | Pronóstico de Open-Meteo (16 días) y buscador de localidades (Open-Meteo, si no OpenStreetMap). Guarda el último en `<datos>/clima.json` para sin wifi. Pantalla `Pronostico.qml` (no se llama `Clima.qml` porque chocaba con el objeto `Clima`) |
| `app/drive.cpp` | **Google Drive**: sube solo informes (HTML) y videos a «Betómetro/AAAA-MM-DD Nombre», y una copia de la base (`Betómetro/betometro.json`, se pisa cuando cambia alguna carrera), con wifi, sin carrera y sin la cámara grabando. Videos solo con las wifi elegidas. Subida de a 4 MB con `curl` en prioridad baja; si se corta sigue. Se conecta con un código (google.com/device); el cliente de Google se sube en `http://IP/drive`. Usa `curl` (viene en Raspberry Pi OS). Ver [google-drive.md](google-drive.md) |
| `raspios/arranque/` | Pantalla de arranque: foto girada (`fondo.png`, 720×1280) y barra amarilla que se llena |

## Cosas de la pantalla y del arranque (lo que costó)
- La pantalla es de **5"**, no de 7". Con `dtoverlay=vc4-kms-dsi-ili9881-5inch`.
- La Pi 3 tiene además una **salida de TV (720×480)**. En Plymouth, `Window.GetWidth()` sin número da la pantalla **más chica**: la foto salía apretada en un costado. El script busca la pantalla más grande (`Window.GetWidth(i)`) y vuelve a medir mientras arranca.
- `DeviceScale=1` en `/etc/plymouth/plymouthd.conf` (si no, la agrandaba al doble).
- Al instalar se ve un rato la consola negra con el cursor: el instalador ya cerró Betómetro y está trabajando.

## En carrera (decisiones de Matías)
- **Nada de carteles de calor o tensión durante la carrera**: hay que aguantar. Se anotan en el reporte con la vuelta.
- El procesador va al máximo; solo baja si hay calor o tensión baja.
- Si el vigilante reinicia Betómetro en medio de una carrera, **la carrera en curso se corta** (las vueltas marcadas quedan). No se retoma: Betómetro tarda casi 2 min en volver y en regularidad ya no sirve (decisión de Matías, 05/11/2026).

## Probar en la PC (la sesión de Claude)
- Qt 6.8.2 con aqtinstall en `/opt/qt/6.8.2/gcc_64`; compilar el programa con `cmake --build <bq>` y el núcleo + pruebas con `ctest`.
- Capturas: `QT_QPA_PLATFORM=offscreen QT_PLUGIN_PATH=/opt/qt/6.8.2/gcc_64/plugins betometro --datos D --muestra --pantalla X --captura f.png`.
- Pruebas de la 1.0.30 (todas en `ctest`, las corre `raspios/pruebas.sh` antes de publicar):
  `carrera_simulada` (simular_neo por un puerto virtual + GpsNeo + Crono, 3 vueltas, ~40 s), `wifi` (nmcli e iw de
  mentira en `pruebas/dobles/`) y `capturas` (11 pantallas Avispón h/v contra `pruebas/capturas/`, con `faketime` y
  `BETO_FIJO=1`). Si una pantalla cambió a propósito, se aceptan las nuevas y se commitean:
  `herramientas/comparar-capturas.sh build/betometro build/comparar_capturas /tmp/cap --aceptar`.
- Simulador de GPS: `simular_neo --enlace /tmp/betometro-gps` y `betometro --gps /tmp/betometro-gps` (con `--enlace /dev/ttyACM0` se prueba el modo USB).
- Con `BETO_RASPBERRY=1` se activan las partes de la Raspberry (sin sensores en la PC).

## Errores que ya pasaron (para no repetirlos)
- Actualizar volvía a la 1.0.3: la instalación se cortaba (apt + initramfs tardan minutos; el botón REINICIAR la mataba). Arreglado.
- «No encuentra la actualización» = el wifi se había desconfigurado, o la API de GitHub frenaba (60 por hora). Arreglado con el plan B.
- El brillo solo se guardaba y no se aplicaba. Arreglado en 1.0.10.
- 06/11/2026: la compilación se colgó 17 min en «apt-get install ffmpeg» (paso Voz) sin decir nada. Ahora ese paso tiene tiempo límite y reintentos.
- 06/11/2026: el video del informe salía negro en la Pi 3. El VideoOutput de Qt usa un programa del chip gráfico que la VideoCore IV (OpenGL ES 2) no arma («Failed to build graphics pipeline state»). Ahora el video se dibuja como imagen (`app/imagen_video.*`): el procesador pasa cada cuadro a RGB.
- Buildroot: cada imagen tardaba 2–3 h y fallaba por piezas del sistema; por eso se pasó a Raspberry Pi OS (`sistema/` e `imagen.yml` se borraron el 07/11/2026; quedan en el historial de git).
