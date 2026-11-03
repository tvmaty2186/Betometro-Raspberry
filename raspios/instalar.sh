#!/bin/bash
# Instala (o actualiza) Betómetro en Raspberry Pi OS Lite 64 bits.
# Uso, en la Raspberry:  sudo bash instalar.sh
set -euo pipefail
cd "$(dirname "$0")"
if [ "$(id -u)" != 0 ]; then echo "Hay que correrlo con sudo:  sudo bash instalar.sh"; exit 1; fi

echo "== 1/5 Programas que necesita (tarda unos minutos la primera vez)"
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends $(grep -v '^#' paquetes.txt)

echo "== 2/5 Betómetro"
systemctl stop betometro 2>/dev/null || true
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
