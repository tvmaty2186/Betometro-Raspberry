#!/bin/bash
# Instala (o actualiza) Betómetro en Raspberry Pi OS Lite 64 bits.
# Uso, en la Raspberry:  sudo bash instalar.sh
set -euo pipefail
cd "$(dirname "$0")"
if [ "$(id -u)" != 0 ]; then echo "Hay que correrlo con sudo:  sudo bash instalar.sh"; exit 1; fi

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

echo "== 2/5 Betómetro"
systemctl stop betometro 2>/dev/null || true
# Se guarda la versión que estaba, por si la nueva falla:  sudo bash /var/lib/betometro-anterior/volver.sh
if [ -x /usr/bin/betometro ]; then
  rm -rf /var/lib/betometro-anterior && mkdir -p /var/lib/betometro-anterior
  tar -C / -czf /var/lib/betometro-anterior/betometro.tar.gz usr/bin/betometro usr/bin/betometro-camara usr/share/betometro 2>/dev/null || true
  printf '#!/bin/bash\nsystemctl stop betometro; tar -C / -xzf /var/lib/betometro-anterior/betometro.tar.gz && systemctl start betometro\n' > /var/lib/betometro-anterior/volver.sh
  chmod +x /var/lib/betometro-anterior/volver.sh
fi
rm -rf /usr/share/betometro/voz   # la voz vieja no queda mezclada con la nueva
tar -C / --no-same-owner -xzf betometro.tar.gz
# El paquete trae /usr/share/betometro/kms.json (pantallas: la de 5" manda, la tele por HDMI a 1280×720).
# Si viniera suelto al lado del instalador, también se usa.
[ -f kms.json ] && install -D -m 0644 kms.json /usr/share/betometro/kms.json
[ -f /usr/share/betometro/kms.json ] || echo "OJO: falta kms.json: el modo tele no anda (lo demás sí)"
install -m 0644 betometro.service pulseaudio-sistema.service despertar-touch.service /etc/systemd/system/
# Sonido: un solo PulseAudio para todo el equipo (el de las sesiones de usuario no)
usermod -aG pulse-access root
systemctl --global disable pulseaudio.service pulseaudio.socket 2>/dev/null || true
systemctl --global mask pulseaudio.service pulseaudio.socket 2>/dev/null || true

echo "== 3/5 Pantalla, GPS, sonido y vigilante (config.txt)"
CFG=/boot/firmware/config.txt
agregar() { grep -qxF "$1" "$CFG" || echo "$1" >> "$CFG"; }
grep -q "^# Betómetro" "$CFG" || printf '\n# Betómetro\n' >> "$CFG"
agregar "dtoverlay=vc4-kms-dsi-ili9881-5inch"
agregar "enable_uart=1"
agregar "dtoverlay=disable-bt"
agregar "dtparam=audio=on"
agregar "dtparam=watchdog=on"
# El puerto serie es del GPS: sin consola de texto ahí
sed -i 's/console=serial0,[0-9]* //' /boot/firmware/cmdline.txt
systemctl disable --now hciuart 2>/dev/null || true
systemctl disable --now serial-getty@ttyAMA0 2>/dev/null || true
# Vigilante: si el sistema se colgara, la Raspberry se reinicia sola
mkdir -p /etc/systemd/system.conf.d
printf '[Manager]\nRuntimeWatchdogSec=15\nRebootWatchdogSec=30\n' > /etc/systemd/system.conf.d/betometro-vigilante.conf

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
# - Si la tarjeta quedó a medio escribir, se repara sola al prender
grep -qw "fsck.repair=yes" /boot/firmware/cmdline.txt || sed -i '1 s/$/ fsck.repair=yes/' /boot/firmware/cmdline.txt
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
  CMD=/boot/firmware/cmdline.txt
  sed -i 's/console=tty1/console=tty3/' "$CMD"
  # La Pi 3 tiene además una salida de TV (720×480) y Plymouth tomaba ese tamaño: la foto quedaba
  # en un pedazo de la pantalla. Se apaga esa salida.
  for p in video=Composite-1:d quiet splash plymouth.ignore-serial-consoles logo.nologo vt.global_cursor_default=0 loglevel=3; do
    grep -qw -- "$p" "$CMD" || sed -i "1 s/\$/ $p/" "$CMD"
  done
  # Si cambió la foto o el tema, se rearma el arranque (la foto va adentro del initramfs)
  if [ "$(plymouth-set-default-theme)" != betometro ] || [ "$cambio" = 1 ]; then
    echo "   (rearmando el arranque: tarda unos minutos, no apagues)"
    plymouth-set-default-theme -R betometro || { plymouth-set-default-theme betometro; update-initramfs -u; } || true
  fi
fi

echo "== 4/5 Arranque directo en Betómetro"
systemctl daemon-reload
systemctl enable pulseaudio-sistema despertar-touch betometro
systemctl disable getty@tty1 2>/dev/null || true

echo "== 5/5 Listo. Versión: $(cat version.txt 2>/dev/null || echo '?')"
echo "Se reinicia en 5 segundos..."
sleep 5
reboot
