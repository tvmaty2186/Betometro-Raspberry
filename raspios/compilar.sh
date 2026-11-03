#!/bin/bash
# Compila Betómetro para Raspberry Pi OS (Debian trixie, arm64). Corre adentro de un
# contenedor debian:trixie arm64 (en GitHub, con QEMU). Deja todo en salida-pi/.
set -euo pipefail
cd "$(dirname "$0")/.."
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
  ca-certificates cmake ninja-build g++ ccache pkg-config file \
  qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-declarative-dev-tools \
  qt6-svg-dev qt6-multimedia-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev >/dev/null
export CCACHE_DIR="$PWD/.ccache"
cmake -S . -B build-pi -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBETO_PRUEBAS=OFF -DBETO_PROGRAMA=ON -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build build-pi
rm -rf salida-pi/raiz
DESTDIR="$PWD/salida-pi/raiz" cmake --install build-pi --prefix /usr
test -x salida-pi/raiz/usr/bin/betometro
mkdir -p salida-pi/raiz/usr/share/betometro/voz
if ls salida-pi/voz/elena/*.wav >/dev/null 2>&1; then
  cp -r salida-pi/voz/. salida-pi/raiz/usr/share/betometro/voz/
else
  echo "OJO: no hay voz grabada (salida-pi/voz): el programa queda sin voz"
fi
file salida-pi/raiz/usr/bin/betometro
ccache -s | head -5 || true
