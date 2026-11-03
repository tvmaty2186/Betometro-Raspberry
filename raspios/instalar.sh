#!/bin/bash
# Instala (o actualiza) Betómetro en Raspberry Pi OS Lite 64 bits.
# Uso, en la Raspberry:  sudo bash instalar.sh
set -euo pipefail
cd "$(dirname "$0")"
if [ "$(id -u)" != 0 ]; then echo "Hay que correrlo con sudo:  sudo bash instalar.sh"; exit 1; fi

echo "== 1/5 Programas que necesita (tarda unos minutos la primera vez)"
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

echo "== 4/5 Arranque directo en Betómetro"
systemctl daemon-reload
systemctl enable pulseaudio-sistema despertar-touch betometro
systemctl disable getty@tty1 2>/dev/null || true

echo "== 5/5 Listo. Versión: $(cat version.txt 2>/dev/null || echo '?')"
echo "Se reinicia en 5 segundos..."
sleep 5
reboot
