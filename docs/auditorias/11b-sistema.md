# Auditoría 11 B — Sistema operativo y código base

07/11/2026 · versión revisada: 1.0.27 (repo) · solo lectura, desde los archivos. Lo que no se ve
desde el repo queda marcado **[Pi]**: se confirma con los comandos del final. No repite lo de 10-sistema.md.

**En una línea:** el sistema está bien pensado para cortes de luz, pero **en boxes cualquiera puede
probar contraseñas por ssh** (la red propia es pública), **nada pone la Pi en hora desde el GPS** aunque la
documentación lo dice, y hay dos detalles de arranque (config.txt y PulseAudio) que pueden fallar sin aviso.

---

## 1. Lo más grave

**1.1 ssh por contraseña, abierto a la red propia, y `sudo` sin clave**
- Qué encontré: el instalador no toca ssh ni pone firewall (`raspios/instalar.sh`, nada). `sshd` escucha en todas
  las redes, también en la propia (`10.42.0.1`, la que arma `app/wifi.cpp:139` con clave `betometro`, pública).
  Raspberry Pi OS deja al primer usuario con `sudo` **sin pedir clave** (`/etc/sudoers.d/010_pi-nopasswd`) **[Pi]**.
- Riesgo: en boxes, cualquiera se conecta a «Betometro-101» y prueba contraseñas de `beto` sin límite
  (`MaxAuthTries 6` por defecto). Si acierta, es root: borra carreras, cambia el programa, apaga el equipo.
- Propongo (mínimo, sin cambiar cómo entra Matías):
  (a) `nftables` con 4 reglas: en la IP de la red propia (`10.42.0.1`) solo entran el 80 (la página del celu) y
  DHCP/DNS; el 22 **no**. Y en cualquier red, más de 6 conexiones nuevas al 22 por minuto se tiran. Nada de
  escrituras en la tarjeta (fail2ban escribe su base y suma un servicio: no vale la pena).
  (b) `/etc/ssh/sshd_config.d/betometro.conf`: `MaxAuthTries 3`, `LoginGraceTime 20`, `AllowUsers beto`.
  Lo pone el instalador. En casa Matías sigue entrando igual con `ssh beto@IP` y contraseña.
- Cuesta: **chico** (agregar `nftables` a `paquetes.txt` si no viene **[Pi]**).

**1.2 Nada pone la Pi en hora desde el GPS (y la documentación dice que sí)**
- Qué encontré: `app/gps.cpp:202-205` arma la hora UTC del GPS y la manda a la pantalla (`emit datos`), nada
  más. El reloj del sistema no se toca. `docs/diseno-sistema.md:36` dice «la hora del sistema se toma del GPS»:
  no está hecho. El instalador no configura NTP: queda lo de fábrica (`systemd-timesyncd`, solo con internet)
  y `fake-hwclock`, que al prender pone la hora de la última vez que se apagó **[Pi]**.
- Riesgo: sin wifi, la Pi arranca con la hora de la última apagada (la «fecha mentira» de 07-1.4). Y si queda
  muy atrasada (meses), **fallan las conexiones seguras**: actualizar, Drive y el pronóstico dicen «sin
  internet» porque el certificado «todavía no vale». Matías pidió tomar la hora siempre del GPS o de internet.
- Propongo: en `gps.cpp`, con `horaValida` y `fullyResolved` del NAV-PVT, si la hora del equipo difiere más de
  2 s y NTP no sincronizó (`timedatectl show -p NTPSynchronized`), poner el reloj una vez con
  `clock_settime(CLOCK_REALTIME)` (Betómetro ya es root) y correr `fake-hwclock save`. El cronómetro no se mueve:
  usa el reloj monotónico (`gps.cpp:13-23`). Hay que revisar los lugares que miden tiempos con
  `QDateTime::currentMSecsSinceEpoch` (p. ej. `wifi.cpp:149`), que saltarían una sola vez. Con wifi, NTP sigue
  mandando (las dos fuentes coinciden). Sin ninguna, no se bloquea nada (decisión de Matías). Sin gpsd ni chrony:
  el puerto es de Betómetro.
- Cuesta: **chico/medio** (el cambio es chico; probarlo en la Pi con y sin wifi).

**1.3 `config.txt`: las líneas se agregan al final, confiando en que la última sección sea `[all]`**
- Qué encontré: `instalar.sh:42-48` (`agregar`) hace `echo >> config.txt`. El `config.txt` de fábrica termina
  en secciones `[cm4]`, `[cm5]`, `[all]`. Si en alguna tarjeta el archivo termina en otra sección (una imagen
  distinta, un ajuste a mano), `dtoverlay=…5inch`, `enable_uart`, `watchdog` y `ramoops` **no se aplican en la
  Pi 3** y nadie avisa: pantalla negra, sin GPS por pines, sin vigilante.
- Propongo: que el bloque `# Betómetro` empiece con una línea `[all]` (`instalar.sh:43`). Una línea.
- Cuesta: **chico**. **[Pi]**: confirmar con `tail -20 /boot/firmware/config.txt`.

**1.4 PulseAudio arranca «antes» pero Betómetro no espera a que esté listo**
- Qué encontré: `pulseaudio-sistema.service` no tiene `Type=notify`: systemd lo da por arrancado al lanzar el
  proceso, y `betometro.service:3` (`After=`) sigue enseguida. El socket `/var/run/pulse/native` tarda 1-2 s en
  aparecer. `reportes.cpp:32` ya filtra el error «PulseAudioService: pa_context_connect»: la carrera existe.
- Riesgo: al prender, la primera voz puede no sonar o Qt queda sin salida de audio hasta reabrir.
- Propongo: `Type=notify` en `pulseaudio-sistema.service` (PulseAudio avisa a systemd; su servicio de usuario
  lo usa así). Y `Nice=-5` para que la voz no se corte mientras la cámara comprime.
- Cuesta: **chico**.

## 2. Lo que más mejora con poco trabajo

**2.1 Sistema congelado (decisión de Matías: nunca `full-upgrade`): qué conviene subir igual**
- Qué encontré: `paquetes.txt` instala «lo que trixie tenga ese día» y nunca más se toca (`instalar.sh:11-17`).
  Versiones de trixie hoy (confirmar **[Pi]** con `apt-cache policy`): Qt 6.8.2, GStreamer 1.26, PulseAudio 17,
  NetworkManager 1.52, dnsmasq 2.91, OpenSSH 10, curl 8.14, Mesa 25.0, núcleo 6.12.
- Riesgo real del congelado (lo demás no importa en un equipo de carrera):
  1. **`ca-certificates` y `tzdata`**: si vence una raíz o Argentina cambia el horario, actualizar/Drive/clima
     dejan de andar o la hora sale corrida. Son datos, no programas: no cambian el comportamiento.
  2. **Lo que escucha a extraños en la red propia**: `sshd`, `dnsmasq` (DHCP/DNS de la red propia), `wpa_supplicant`.
     Son los únicos con avisos de seguridad que importan acá. GStreamer tiene muchos avisos, pero solo lee videos
     propios: no importa.
  3. Dos equipos instalados en fechas distintas quedan con Qt/GStreamer distintos y nadie lo sabe.
- Propongo: en `instalar.sh`, un paso **opcional** con tiempo límite: si hay internet,
  `apt-get install --only-upgrade ca-certificates tzdata openssh-server dnsmasq-base wpasupplicant curl libssl3t64`;
  si no hay, sigue igual. Nunca núcleo, firmware, Qt, Mesa ni GStreamer (eso sí cambia el comportamiento). Y
  guardar `dpkg -l` de esos paquetes en `/var/lib/betometro/reportes/paquetes.txt` para el reporte.
- Cuesta: **chico**.

**2.2 Escrituras «escondidas» del sistema (aparte de Betómetro)**
- Qué encontré: el instalador apaga apt y man-db (`instalar.sh:81`), pero quedan **[Pi]**: `fake-hwclock`
  (escribe la hora **cada hora** y al apagar), NetworkManager (`/var/lib/NetworkManager/`: en cada conexión y
  cada cliente de la red propia), PulseAudio (`/var/lib/pulse/*.tdb` en cada `set-sink-volume`, o sea cada
  arranque por `pulseaudio-sistema.service:8`), `timesyncd` (toca un archivo en cada sincronización),
  `random-seed` al prender y apagar. Y `wifi.cpp:138-139` **borra y vuelve a crear** la conexión de la red propia
  cada vez que la arma: dos escrituras en `/etc/NetworkManager/system-connections/`.
- Riesgo: chico. Son pocas y chicas; la tarjeta aguanta años. Lo nombro para que la medición de la Pi lo cuente.
- Propongo: dejar `fake-hwclock` (sin él la Pi arranca en 1970 y las conexiones seguras fallan hasta tener hora).
  Crear la conexión propia una sola vez y después `connection up` (`wifi.cpp:138`). El resto, nada.
- Cuesta: **chico**.

**2.3 NetworkManager: dos ajustes de fábrica que suelen molestar en la Pi 3 [Pi]**
- Qué encontré: el instalador no configura NM. De fábrica: ahorro de energía del wifi (la página del celu y
  `nmcli` tardan o se cortan) y MAC al azar al buscar redes (el chip de la Pi 3, `brcmfmac`, a veces falla la
  búsqueda con eso; `reportes.cpp:39` ya filtra errores de `brcmf`).
- Propongo: `/etc/NetworkManager/conf.d/betometro.conf` con `wifi.powersave=2` y `wifi.scan-rand-mac-address=no`.
  Solo si la medición en la Pi muestra búsquedas que fallan o cortes.
- Cuesta: **chico**.

**2.4 Zona horaria y nombre del equipo: los pone el Imager, no el instalador [Pi]**
- Qué encontré: `instalar.sh` no fija zona horaria ni `hostname`. Si la tarjeta se graba sin configurarlo, la
  hora en pantalla sale en UTC (3 horas corridas) y el equipo se llama `raspberrypi`.
- Propongo: `timedatectl set-timezone America/Argentina/Buenos_Aires` y `hostnamectl set-hostname betometro`
  en el instalador (si no están). Así `ssh beto@betometro.local` también anda en casa.
- Cuesta: **chico**.

## 3. El resto

- **Disco:** el instalador no toca `/etc/fstab`. Raspberry Pi OS ya monta `/` con `noatime` **[Pi]**; `commit=5`
  y `data=ordered` de fábrica están bien para cortes de luz (lo que se corta a medias queda en cero, y
  `almacen.cpp` lo cubre con fsync + renombrar). `/tmp` es memoria en trixie **[Pi]**. `fstrim.timer` semanal:
  dejarlo, le hace bien a la tarjeta. No cambiaría `commit`: gana poco y arriesga más datos por corte.
- **Vigilante de hardware:** `instalar.sh:55` pide `RebootWatchdogSec=30`, pero el chip de la Pi aguanta 15 s
  como máximo: el núcleo lo recorta a 15. Anda igual; poner 15 para que diga la verdad. **Chico.**
- **Overclock / `gpu_mem`:** no hay overclock (bien: la Pi 3 B+ ya tira calor). `gpu_mem` queda de fábrica; con
  el driver KMS no importa (usa CMA). Nada que tocar.
- **Bluetooth:** apagado por `disable-bt` (`instalar.sh:46`), pero `bluetooth.service` sigue habilitado y
  arranca sin adaptador **[Pi]**. `systemctl disable bluetooth` en el instalador. **Chico.**
- **Red propia y reenvío:** `nmcli hotspot` usa modo «compartido»: prende el reenvío de IP y NAT. En boxes no hay
  nada que compartir (eth0 suelto). Sin riesgo hoy; si algún día la Pi va con cable a una red, los del hotspot
  entran a esa red. Lo cubre la regla de 1.1 (a).
- **Compilación (`programa.yml:67`):** `debian:trixie` sin fijar versión y `pip install edge-tts` sin número
  (`:58`). Hoy no rompe nada (trixie es estable y no cambia Qt). A largo plazo: cuando trixie pase a archivo
  (≈2028), `compilar.sh` deja de poder instalar paquetes; anotarlo en `docs/raspios.md`.
- **`app/terceros/qrcodegen`:** Nayuki, MIT, solo codifica texto propio. Sin riesgo. Falta anotar la versión
  en `LEEME.md` para saber qué copia es. **Chico.**
- **Qt 6.8.2 de Debian:** es la versión que trixie va a tener siempre (Debian solo corrige errores, no sube).
  Lo que se compila en GitHub y lo que está en la Pi son la misma 6.8.2: sin riesgo de incompatibilidad.
- **`despertar-touch.service:5`:** `After=systemd-udev-settle.service` (servicio viejo). Como es solo `After=`, no
  lo arranca ni frena. Dejarlo.

## Para medir en la Pi (pegar por ssh, solo lectura)

```bash
ssh beto@<IP>
# 1.1 ssh y sudo
sudo sshd -T | grep -iE 'passwordauth|maxauthtries|permitroot|allowusers|logingrace'; sudo cat /etc/sudoers.d/* 2>/dev/null
sudo ss -tulnp; sudo nft list ruleset 2>/dev/null | head -20; dpkg -s nftables 2>/dev/null | grep -i status
# 1.2 / 2.4 hora
timedatectl; systemctl status systemd-timesyncd fake-hwclock --no-pager | head -20; ls -l /etc/fake-hwclock.data; hostname
# 1.3 config.txt y cmdline
tail -25 /boot/firmware/config.txt; cat /boot/firmware/cmdline.txt; vcgencmd get_mem gpu; vcgencmd measure_clock arm
# 1.4 sonido
systemctl show pulseaudio-sistema -p Type -p NRestarts; journalctl -b -u betometro --no-pager | grep -i pulse | head
# 2.1 versiones
apt-cache policy qt6-base-abi libgstreamer1.0-0 pulseaudio network-manager dnsmasq-base openssh-server curl ca-certificates tzdata | grep -E '^[a-z]|Installed'
uname -r; cat /etc/debian_version
# 2.2 escrituras del sistema (correr dos veces con 1 hora entre medio y restar)
cat /sys/block/mmcblk0/stat | awk '{print "sectores escritos:", $7}'; systemctl list-timers --all --no-pager
sudo find /var /etc -newermt '-1 hour' -type f 2>/dev/null | grep -v betometro | head -30
# 2.3 NetworkManager
nmcli -f GENERAL.STATE,WIFI-PROPERTIES device show wlan0 | head; cat /etc/NetworkManager/conf.d/* 2>/dev/null; iw dev wlan0 get power_save
# 3 disco y servicios
findmnt / -o OPTIONS; findmnt /tmp; systemctl is-enabled bluetooth fstrim.timer
```

## Las 3 que haría primero

1. **Cerrar el ssh a la red propia + límite de intentos** (1.1): en boxes nadie entra; en casa todo igual.
2. **Hora desde el GPS** (1.2) y zona horaria en el instalador (2.4): fecha real sin wifi y las conexiones
   seguras no se caen por reloj atrasado.
3. **`[all]` en config.txt + `Type=notify` en PulseAudio** (1.3 y 1.4): dos líneas que sacan dos formas de arrancar mal.

## Preguntas para Matías

1. `sudo` hoy no pide clave (a confirmar en la Pi). ¿Dejamos que la pida? Vos tendrías que escribir tu
   contraseña una vez más por sesión de ssh. Si decís que no, queda como está (con 1.1 ya alcanza bastante).
2. ¿Aceptás que el instalador, **solo si hay wifi**, suba esos pocos paquetes (certificados, zona horaria, ssh,
   dnsmasq, curl) y nada más? No cambia nada de cómo se ve ni se usa Betómetro.

## Medido en la Pi (07/11/2026, Matías por ssh, versión 1.0.26/1.0.27, 18 min prendida)

| Qué | Resultado | Qué significa |
|---|---|---|
| **Hora** | La Pi decía **06/11 23:57** cuando eran las **07/11 ~11:20**. `System clock synchronized: no` con wifi de casa y `timesyncd` activo | **Lo más grave de la medición.** 11 horas atrasada *con* internet. Las carreras de hoy salen con fecha de ayer y el clima / Drive / Actualizar pueden fallar. NTP no está sincronizando (falta ver por qué) |
| Tensión y calor | `throttled=0x0`, 49,9 °C, 1,4 GHz | Bien |
| `sudo` | **Pide la clave** (beto está en el grupo sudo, sin archivo NOPASSWD) | Bien, como quería Matías |
| ssh | `PasswordAuthentication yes`, escucha en todas las redes; `PermitRootLogin without-password` (root solo con llave) | Confirma 1.1 |
| Firewall | Ninguna regla de nftables | Confirma 1.1 |
| Escuchan en la red | ssh 22, Betómetro 80, avahi 5353 (lo que hace andar `betometro.local`) | Lo esperado |
| Nombre | La Pi ya se llama **BETOMETRO** y `ssh beto@betometro.local` anda | 2.4: el nombre ya está; dejarlo igual en el instalador |
| Zona horaria | America/Argentina/Buenos_Aires | Bien (la puso el Imager) |
| **Arranque** | 27,4 s (4,3 núcleo + 23,1 sistema). Betómetro espera a **PulseAudio: 10,4 s**, que espera a **cloud-init (~5 s)** | PulseAudio se demora probando el sonido HDMI y falla («Failed to find a working profile»). Apagar esa tarjeta en PulseAudio y apagar cloud-init: unos **12 s menos** |
| Servicios que sobran | `cloud-init` (5 servicios), `bluetooth`, `rpi-eeprom-update` (la Pi 3 no tiene eeprom), `NetworkManager-wait-online` (6 s), `udisks2`, `sshswitch` | Apagarlos en el instalador. **Dejar** `avahi` (hace andar `betometro.local`) y `fake-hwclock`/`timesyncd` |
| `config.txt` | Las líneas de Betómetro están bajo `[all]` | Hoy anda; 1.3 sigue valiendo para que no dependa de la suerte |
| `cmdline.txt` | `video=Composite-1:d` **dos veces** y sin salto de línea al final | Confirma 10-1.2 (se reescribe en cada actualización) |
| Núcleo | Instalado también el de la **Pi 5** (`linux-image-rpi-2712`) | Ocupa lugar y alarga cada actualización del núcleo. Se puede quitar |
| Disco | 3,3 GB usados de 15 GB (11 GB libres); `/var/lib/betometro` 498 MB; versión anterior 22 MB | Bien. Los 498 MB incluyen los paquetes de actualización viejos (ya en el plan) |
| Escrituras | ~6 MB en 18 min | Bien |
| zram | 904 MB, sin uso | Bien |
| Versiones | GStreamer 1.26.2, PulseAudio 17, openssh 10.0p1, núcleo 6.18.50, firmware 2026-09-15 | Las de trixie. Qt no salió (cambiar el filtro) |
| Errores | Solo PulseAudio HDMI y avisos de wpa_supplicant (`bgscan`) | Nada grave |

**Faltó** (el comando no lo mostró): opciones de montaje del disco, versión de Qt y el detalle de
`/var/lib/betometro`. Se piden con el segundo bloque de comandos.

### Segunda medición (07/11, la hora)

- `timesyncd` activo pero **nunca mandó un paquete** (`Packet count: 0`, `Server: n/a`) en 38 minutos con la wifi
  de casa. No es el router bloqueando: ni siquiera lo intenta. El registro no dice por qué porque journald guarda
  solo avisos (`instalar.sh:61`, `MaxLevelStore=warning`).
- **No hay `fake-hwclock`.** Al prender, la Pi arranca con la hora en que se apagó (la guarda `timesyncd`) y,
  como NTP no corrige, queda atrasada todo el tiempo que estuvo apagada. Por eso decía 06/11 23:57: es la hora
  del apagado de anoche más los minutos prendida.
- Disco: `/` con `noatime` (bien). `/var/lib/betometro/actualizacion` = **494 MB** de paquetes viejos (de 498
  MB); la base pesa 364 KB.
- Qt 6.8.2 (`libqt6core6t64 6.8.2+dfsg-9+deb13u2`).
