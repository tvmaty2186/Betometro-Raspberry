#!/bin/bash
# Instala (o actualiza) Betómetro en Raspberry Pi OS Lite 64 bits.
# Uso, en la Raspberry:  sudo bash instalar.sh
# -E: el trap de ERR también vale adentro de las funciones (si no, una falla ahí dejaba la pantalla negra; auditoría 12)
set -Eeuo pipefail
cd "$(dirname "$0")"
if [ "$(id -u)" != 0 ]; then echo "Hay que correrlo con sudo:  sudo bash instalar.sh"; exit 1; fi
VERSION=$(cut -d' ' -f1 version.txt 2>/dev/null || echo '?')
RESULTADO=/var/lib/betometro/actualizacion/resultado   # Betómetro lo muestra al prender
mkdir -p /var/lib/betometro/actualizacion
ANTERIOR=/var/lib/betometro-anterior
# Si algo falla a mitad (sin lugar, paquete roto): se vuelve a la versión que andaba y Betómetro se prende
# igual. Antes quedaba apagado con la pantalla diciendo «se reinicia sola» (auditoría 10-1.3).
fallo() {
  echo "!! Falló la instalación en la línea $1: se vuelve a la versión anterior"
  if [ -f "$ANTERIOR/betometro.tar.gz" ] && [ "${YA_CAMBIADO:-0}" = 1 ]; then rm -rf /usr/share/betometro; tar -C / -xzf "$ANTERIOR/betometro.tar.gz" || true; fi
  echo "No se pudo instalar la $VERSION: quedó la versión anterior" > "$RESULTADO"
  sync
  systemctl daemon-reload || true
  systemctl start betometro || true
  exit 1
}
trap 'fallo $LINENO' ERR

echo "== 1/5 Programas que necesita (tarda unos minutos la primera vez)"
# Si una instalación anterior quedó a medias (se apagó en el medio), se termina de acomodar
dpkg --configure -a || true
faltan=$(for p in $(grep -v '^#' paquetes.txt); do dpkg -s "$p" >/dev/null 2>&1 || echo "$p"; done)
if [ -n "$faltan" ]; then
  apt-get update
  DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends $faltan
else
  echo "Ya está todo (no hace falta internet)"
fi

echo "== 1b Con internet: se actualizan solo certificados, zona horaria, ssh, dnsmasq y wpa_supplicant"
# Lo que mira a internet o escucha a extraños en la red propia (11B-2.1, Matías: sí). Nunca el núcleo, Qt,
# Mesa ni GStreamer (eso cambia cómo anda Betómetro). Sin internet, o si tarda, sigue igual: nunca falla por esto.
SEGUROS="ca-certificates tzdata openssh-server dnsmasq-base wpasupplicant"
if timeout 120 apt-get update -qq >/dev/null 2>&1; then
  DEBIAN_FRONTEND=noninteractive timeout 600 apt-get install -y -qq --only-upgrade --no-install-recommends $SEGUROS >/dev/null 2>&1 \
    || echo "   (no se pudieron actualizar: se deja como estaba)"
else
  echo "   (sin internet: se deja como estaba)"
fi
# Las versiones de lo que importa, para el reporte de errores (reportes.cpp las manda con cada issue)
mkdir -p /var/lib/betometro
{ dpkg-query -W -f='${Package} ${Version}\n' $SEGUROS libqt6core6t64 libgstreamer1.0-0 pulseaudio network-manager curl 2>/dev/null
  echo "nucleo $(uname -r)"; } > /var/lib/betometro/versiones.txt || true

echo "== 2/5 Betómetro"
# Primero se desarma la versión nueva aparte y se fuerza a la tarjeta: si se corta la luz acá, el programa
# que anda no se tocó (antes se escribía encima del viejo y podía quedar a medias) (10-1.1)
NUEVO=/var/lib/betometro-nuevo
rm -rf "$NUEVO" && mkdir -p "$NUEVO"
tar -C "$NUEVO" --no-same-owner -xzf betometro.tar.gz
sync
systemctl stop betometro 2>/dev/null || true
# Se guarda la versión que estaba, por si la nueva falla. Si se instala dos veces la misma, no se pisa
# la copia buena con esa misma (10-3). A mano:  sudo bash /var/lib/betometro-anterior/volver.sh
INSTALADA=$(cat /usr/share/betometro/version 2>/dev/null || echo '')
if [ -x /usr/bin/betometro ] && [ "$INSTALADA" != "$VERSION" ]; then
  rm -rf "$ANTERIOR" && mkdir -p "$ANTERIOR"
  # También los servicios: si la nueva cambia cómo se lanza, la anterior vuelve con los suyos (auditoría 12)
  svc=$(cd / && ls etc/systemd/system/betometro.service etc/systemd/system/betometro-volver.service etc/systemd/system/pulseaudio-sistema.service etc/systemd/system/pulseaudio-auriculares.service etc/systemd/system/despertar-touch.service 2>/dev/null || true)
  tar -C / -czf "$ANTERIOR/betometro.tar.gz" usr/bin/betometro usr/bin/betometro-camara usr/share/betometro $svc 2>/dev/null || true
  echo "${INSTALADA:-?}" > "$ANTERIOR/version"
fi
if [ -f "$ANTERIOR/betometro.tar.gz" ]; then
  # Volver a la anterior: a mano por ssh, o solo si la nueva no arranca (betometro-volver.service)
  cat > "$ANTERIOR/volver.sh" <<'VOLVER'
#!/bin/bash
# Vuelve a la versión anterior de Betómetro (la usa betometro-volver.service si la nueva no arranca)
systemctl stop betometro 2>/dev/null || true
rm -rf /usr/share/betometro.viejo; mv /usr/share/betometro /usr/share/betometro.viejo 2>/dev/null
tar -C / -xzf /var/lib/betometro-anterior/betometro.tar.gz || { mv /usr/share/betometro.viejo /usr/share/betometro; exit 1; }
rm -rf /usr/share/betometro.viejo
systemctl daemon-reload 2>/dev/null || true   # por si volvieron los servicios de la anterior
mkdir -p /var/lib/betometro/actualizacion
echo "La versión nueva no arrancaba: se volvió a la $(cat /var/lib/betometro-anterior/version 2>/dev/null)" > /var/lib/betometro/actualizacion/resultado
sync
systemctl reset-failed betometro 2>/dev/null || true
systemctl start betometro
VOLVER
  chmod +x "$ANTERIOR/volver.sh"
fi
# El cambio: cada archivo se reemplaza con mv (instantáneo, nunca queda a medias)
YA_CAMBIADO=1
for f in usr/bin/betometro usr/bin/betometro-camara; do
  [ -e "$NUEVO/$f" ] && mv -f "$NUEVO/$f" "/$f"
done
rm -rf /usr/share/betometro.viejo
[ -d /usr/share/betometro ] && mv /usr/share/betometro /usr/share/betometro.viejo   # la voz vieja no queda mezclada
mv "$NUEVO/usr/share/betometro" /usr/share/betometro
rm -rf /usr/share/betometro.viejo
# Lo que el paquete traiga además (por si algún día suma archivos fuera de esas carpetas)
cp -a "$NUEVO/usr/." /usr/ 2>/dev/null || true
rm -rf "$NUEVO"
echo "$VERSION" > /usr/share/betometro/version
sync
# El paquete trae /usr/share/betometro/kms.json (pantallas: la de 5" manda, la tele por HDMI a 1280×720).
# Si viniera suelto al lado del instalador, también se usa.
[ -f kms.json ] && install -D -m 0644 kms.json /usr/share/betometro/kms.json
[ -f /usr/share/betometro/kms.json ] || echo "OJO: falta kms.json: el modo tele no anda (lo demás sí)"
install -m 0644 betometro.service betometro-volver.service pulseaudio-sistema.service pulseaudio-auriculares.service despertar-touch.service /etc/systemd/system/
# Sonido: un solo PulseAudio para todo el equipo (el de las sesiones de usuario no)
usermod -aG pulse-access root
systemctl --global disable pulseaudio.service pulseaudio.socket 2>/dev/null || true
systemctl --global mask pulseaudio.service pulseaudio.socket 2>/dev/null || true

echo "== 3/5 Pantalla, GPS, sonido y vigilante (config.txt)"
CFG=/boot/firmware/config.txt
CMD=/boot/firmware/cmdline.txt
agregar() {
  grep -qxF "$1" "$CFG" && return 0
  [ "$(grep -E '^\[' "$CFG" | tail -1)" = "[all]" ] || echo "[all]" >> "$CFG"
  echo "$1" >> "$CFG"
}
# Lo de Betómetro va siempre bajo «[all]»: si la última sección fuera otra ([pi5], [cm4]…), la Pi 3 no lo
# leería y arrancaría sin pantalla, GPS ni vigilante (11B-1.3)
grep -q "^# Betómetro" "$CFG" || printf '\n[all]\n# Betómetro\n' >> "$CFG"
# cmdline.txt (FAT): se escribe solo si cambia algo, de una vez y con sync. Antes se reescribía en cada
# actualización y un corte justo ahí dejaba la Pi sin arrancar (10-1.2). También saca palabras repetidas.
cmd_cambiar() {   # cmd_cambiar "<texto nuevo de la línea>"
  if [ "$1" != "$(head -1 "$CMD")" ]; then
    cp "$CMD" "$CMD.nuevo" && printf '%s\n' "$1" > "$CMD.nuevo" && sync && mv "$CMD.nuevo" "$CMD" && sync
  fi
}
cmd_armar() {   # la línea con lo que pide Betómetro, sin repetidos y sin consola en el puerto del GPS
  local linea
  linea=$(head -1 "$CMD" | sed -e 's/console=serial0,[0-9]*//' -e 's/console=tty1/console=tty3/')
  local out="" w
  for w in $linea "$@"; do
    case " $out " in *" $w "*) ;; *) out="$out $w" ;; esac
  done
  echo "${out# }"
}
agregar "dtoverlay=vc4-kms-dsi-ili9881-5inch"
agregar "enable_uart=1"
agregar "dtoverlay=disable-bt"
agregar "dtparam=audio=on"
agregar "dtparam=watchdog=on"
# El puerto serie es del GPS: sin consola de texto ahí (se saca al armar la línea, más abajo)
systemctl disable --now hciuart 2>/dev/null || true
systemctl disable --now serial-getty@ttyAMA0 2>/dev/null || true
# Vigilante: si el sistema se colgara, la Raspberry se reinicia sola
mkdir -p /etc/systemd/system.conf.d
# (15 s: lo máximo que aguanta el chip de la Pi; con 30 el núcleo lo recortaba igual) (11B-3)
printf '[Manager]\nRuntimeWatchdogSec=15\nRebootWatchdogSec=15\n' > /etc/systemd/system.conf.d/betometro-vigilante.conf

echo "== 3c Para que cortar el contacto no rompa nada: casi nada se escribe en la tarjeta"
# - Registros del sistema: solo los avisos y errores se guardan en la tarjeta (son pocos), para saber
#   qué pasó si la Raspberry se colgó o se cortó la corriente. El resto, nada.
mkdir -p /etc/systemd/journald.conf.d /var/log/journal
printf '[Journal]\nStorage=persistent\nMaxLevelStore=warning\nSystemMaxUse=16M\nSyncIntervalSec=30s\n' > /etc/systemd/journald.conf.d/betometro.conf
# - Si el núcleo se cuelga, deja lo último en una memoria que sobrevive al reinicio (Betómetro lo
#   suma al reporte de errores al prender)
[ -f /boot/firmware/overlays/ramoops.dtbo ] && agregar "dtoverlay=ramoops"
# - Memoria de reserva comprimida en la RAM (zram) en vez de un archivo en la tarjeta: más rápida y
#   no gasta la SD. La Pi 3 tiene 1 GB.
if systemctl list-unit-files dphys-swapfile.service >/dev/null 2>&1; then
  dphys-swapfile swapoff 2>/dev/null || true
  systemctl disable --now dphys-swapfile 2>/dev/null || true
  rm -f /var/swap
fi
if dpkg -s rpi-swap >/dev/null 2>&1; then
  # Raspberry Pi OS nuevo: su propio armado de la memoria de reserva; se le pide solo zram
  mkdir -p /etc/rpi/swap.conf.d
  printf '[Main]\nMechanism=zram\n' > /etc/rpi/swap.conf.d/80-betometro.conf
else
  dpkg -s systemd-zram-generator >/dev/null 2>&1 || apt-get install -y -qq systemd-zram-generator || true
  printf '[zram0]\nzram-size = ram / 2\ncompression-algorithm = zstd\n' > /etc/systemd/zram-generator.conf
fi
# - Sin tareas de fondo que escriben (actualizar la lista de programas, manuales)
systemctl disable --now apt-daily.timer apt-daily-upgrade.timer man-db.timer 2>/dev/null || true
# - Si la tarjeta quedó a medio escribir, se repara sola al prender (fsck.repair=yes, va en la línea de abajo)
# - Las carreras se guardan siempre en un archivo nuevo, forzado a disco, y recién ahí reemplaza al anterior
#   (si se corta a la mitad, queda la versión de antes: almacen.cpp)

echo "== 3b Pantalla de arranque: foto de los pilotos con barra de carga, sin letras"
if command -v plymouth-set-default-theme >/dev/null; then
  T=/usr/share/plymouth/themes/betometro
  cambio=0
  for f in arranque/*; do cmp -s "$f" "$T/$(basename "$f")" || cambio=1; done
  install -d "$T"
  install -m 0644 arranque/* "$T/"
  # Escala 1: la pantalla de 5" tiene mucha definición y Plymouth la agrandaba al doble
  mkdir -p /etc/plymouth
  grep -q "^DeviceScale=1" /etc/plymouth/plymouthd.conf 2>/dev/null || { printf '[Daemon]\nTheme=betometro\nDeviceScale=1\n' > /etc/plymouth/plymouthd.conf; cambio=1; }
  agregar "disable_splash=1"   # sin el arcoíris de la placa
  # La Pi 3 tiene además una salida de TV (720×480) y Plymouth tomaba ese tamaño: la foto quedaba
  # en un pedazo de la pantalla. Se apaga esa salida (video=Composite-1:d, en la línea de abajo).
  # Si cambió la foto o el tema, se rearma el arranque (la foto va adentro del initramfs)
  if [ "$(plymouth-set-default-theme)" != betometro ] || [ "$cambio" = 1 ]; then
    echo "   (rearmando el arranque: tarda unos minutos, no apagues)"
    # Que se vea en la pantalla (Betómetro está parado): letras grandes y el aviso (auditoría 11A-3 / 12)
    { f=/usr/share/consolefonts/Lat15-TerminusBold32x16.psf.gz
      if [ -f "$f" ]; then setfont -C /dev/tty1 "$f" || true; fi
      printf '\033c\033[?25l\n\n\n   ACTUALIZANDO BETOMETRO\n\n   NO LO APAGUES\n\n   Tarda unos minutos y se reinicia solo.\n' > /dev/tty1
      chvt 1 || true; } 2>/dev/null || true
    plymouth-set-default-theme -R betometro || { plymouth-set-default-theme betometro; update-initramfs -u; } || true
  fi
fi

# cmdline.txt: todo de una vez (y solo si cambió)
cmd_cambiar "$(cmd_armar fsck.repair=yes video=Composite-1:d quiet splash plymouth.ignore-serial-consoles logo.nologo vt.global_cursor_default=0 loglevel=3)"

echo "== 3f Zona horaria Argentina y nombre «betometro» (solo si no están)"
# Normalmente ya los puso el Imager; si la tarjeta se grabó sin eso, la hora salía en UTC y el equipo se
# llamaba «raspberrypi» (11B-2.4). Si ya se llama betometro (con mayúsculas o sin), se deja como está.
ZONA=America/Argentina/Buenos_Aires
if [ "$(timedatectl show -p Timezone --value 2>/dev/null || cat /etc/timezone 2>/dev/null || true)" != "$ZONA" ]; then
  timedatectl set-timezone "$ZONA" 2>/dev/null || { ln -sf "/usr/share/zoneinfo/$ZONA" /etc/localtime && echo "$ZONA" > /etc/timezone; } || true
fi
NOMBRE=$(hostname 2>/dev/null || cat /etc/hostname 2>/dev/null || true)
if [ "$(echo "$NOMBRE" | tr '[:upper:]' '[:lower:]')" != betometro ]; then
  hostnamectl set-hostname betometro 2>/dev/null || echo betometro > /etc/hostname || true
  # /etc/hosts también, si no «sudo» se queja de que no resuelve el nombre
  [ -n "$NOMBRE" ] && sed -i "s/^\(127\.0\.1\.1[[:space:]]\+\)$NOMBRE\([[:space:]]\|\$\)/\1betometro\2/" /etc/hosts 2>/dev/null || true
  grep -q "^127\.0\.1\.1[[:space:]]\+betometro" /etc/hosts 2>/dev/null || printf '127.0.1.1\tbetometro\n' >> /etc/hosts || true
fi

echo "== 3d Arranque más rápido: servicios que no se usan"
# cloud-init (lo usa el Imager solo la primera vez), esperar la red, la eeprom (la Pi 3 no tiene),
# discos automáticos (el pendrive lo monta Betómetro), bluetooth (apagado en config.txt) (11B, medido 07/11)
touch /etc/cloud/cloud-init.disabled 2>/dev/null || true
for s in NetworkManager-wait-online rpi-eeprom-update udisks2 sshswitch bluetooth; do
  systemctl disable "$s" 2>/dev/null || true
done
# PulseAudio tardaba 10 s en arrancar probando el sonido por HDMI (y fallaba igual): que no lo mire
mkdir -p /etc/udev/rules.d
# Ni la tele ni el micrófono de la webcam (Betómetro lo graba directo por ALSA; PulseAudio lo probaba y fallaba)
printf 'SUBSYSTEM=="sound", ATTRS{id}=="vc4hdmi*", ENV{PULSE_IGNORE}="1"\nSUBSYSTEM=="sound", ENV{ID_BUS}=="usb", ENV{PULSE_IGNORE}="1"\n' > /etc/udev/rules.d/80-betometro-sin-hdmi.rules
# Que la Pi no duerma la cámara (ni nada USB): a los 2 s sin usarla la ponía a dormir y al despertarla la
# grabación fallaba con «failed during initialization» (07/11/2026). En el auto el ahorro no importa.
printf 'ACTION=="add", SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", TEST=="power/control", ATTR{power/control}="on"\n' > /etc/udev/rules.d/81-betometro-usb-despierto.rules
for c in /sys/bus/usb/devices/*/power/control; do echo on > "$c" 2>/dev/null || true; done
# El núcleo de la Pi 5 no se usa en la Pi 3: ocupa lugar y alarga cada actualización del núcleo
if ! uname -r | grep -q 2712; then
  k5=$(dpkg -l 'linux-image-*' 2>/dev/null | awk '/^ii/ && $2 ~ /rpi-2712/ {print $2}' || true)   # sin ninguno, dpkg sale con 1 (pipefail)
  [ -n "$k5" ] && { DEBIAN_FRONTEND=noninteractive apt-get purge -y -qq $k5 >/dev/null 2>&1 || true; }
fi

echo "== 3e ssh: cerrado en la red propia «Betometro-…» (su clave la sabe cualquiera en boxes)"
# En casa se entra igual: ssh beto@betometro.local con la contraseña. Desde la red propia (10.42.0.1), no.
# Sin firewall ni paquetes nuevos: lo hace el mismo ssh. OpenSSH 10 ya castiga a quien prueba claves de más
# (PerSourcePenalties); además, 3 intentos por conexión y 20 s para entrar (11B-1.1).
if [ -d /etc/ssh/sshd_config.d ]; then
  cat > /etc/ssh/sshd_config.d/betometro.conf <<'SSH'
# Betómetro (raspios/instalar.sh)
MaxAuthTries 3
LoginGraceTime 20
PermitRootLogin no
AllowUsers beto
Match LocalAddress 10.42.0.1
    DenyUsers *
SSH
  if command -v sshd >/dev/null && ! sshd -t 2>/dev/null; then
    echo "OJO: ssh no aceptó el ajuste: se deja como estaba"; rm -f /etc/ssh/sshd_config.d/betometro.conf
  else
    systemctl reload ssh 2>/dev/null || true
  fi
fi

echo "== 4/5 Arranque directo en Betómetro"
# Ajuste de prueba del video (06/11/2026, mandaba los mensajes a /tmp): ya no hace falta
rm -f /etc/systemd/system/betometro.service.d/video.conf
rmdir /etc/systemd/system/betometro.service.d 2>/dev/null || true
systemctl daemon-reload
systemctl enable pulseaudio-sistema despertar-touch betometro
systemctl disable getty@tty1 2>/dev/null || true

# Paquetes de actualizaciones viejas: solo se deja el de esta versión (eran ~500 MB) (10-2.4)
find /var/lib/betometro/actualizacion -maxdepth 1 -name 'betometro-pi-*.tar.gz' ! -name "betometro-pi-$VERSION.tar.gz" -delete 2>/dev/null || true
echo "Se instaló la $VERSION" > "$RESULTADO"
# Hasta que la nueva ande un minuto, si no arranca se vuelve sola a la anterior (betometro-volver.service)
[ -f "$ANTERIOR/betometro.tar.gz" ] && touch /var/lib/betometro/actualizacion/a-probar
trap - ERR
sync

echo "== 5/5 Listo. Versión: $(cat version.txt 2>/dev/null || echo '?')"
echo "Se reinicia en 5 segundos..."
sleep 5
reboot
