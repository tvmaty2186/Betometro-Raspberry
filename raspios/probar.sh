#!/bin/bash
# Prueba de humo: en un debian:trixie arm64 LIMPIO se instala solo lo de paquetes.txt,
# se copia Betómetro y se lo deja andar 40 s sin pantalla. Falla si se cae, si falta
# algún módulo de QML o si no puede cargar las tipografías.
set -euo pipefail
cd "$(dirname "$0")/.."
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends $(grep -v '^#' raspios/paquetes.txt) >/dev/null
cp -a salida-pi/raiz/. /
mkdir -p /tmp/datos /tmp/run
set +e
QT_QPA_PLATFORM=offscreen QT_FORCE_STDERR_LOGGING=1 LANG=C.UTF-8 XDG_RUNTIME_DIR=/tmp/run \
  BETO_DATOS=/tmp/datos BETO_VOZ=/usr/share/betometro/voz \
  timeout 40 /usr/bin/betometro --sin-gps > /tmp/log.txt 2>&1
codigo=$?
set -e
grep -v "soundeffect: Failed to update audio output" /tmp/log.txt | head -40 || true
if [ "$codigo" != 124 ]; then echo "MAL: Betómetro terminó solo (código $codigo)"; exit 1; fi
if grep -qiE "No pude cargar la fuente|is not installed|module .* not found|Cannot load library|failed to load component|QQmlApplicationEngine failed" /tmp/log.txt; then
  echo "MAL: faltan piezas (ver arriba)"; exit 1
fi
echo "BIEN: Betómetro anduvo 40 s sin problemas"
