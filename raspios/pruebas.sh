#!/bin/bash
# Todas las pruebas (ctest) en un debian:trixie de PC: mismo Qt 6.8.2 y GStreamer que la Raspberry.
# Las usa programa.yml ANTES de publicar (si una falla, no se publica) y pruebas.yml en cada subida.
# Uso:  docker run --rm -v "$PWD:/src" debian:trixie bash /src/raspios/pruebas.sh
set -euo pipefail
cd "$(dirname "$0")/.."
export DEBIAN_FRONTEND=noninteractive
for i in 1 2 3; do
  timeout 600 bash -c 'apt-get update -qq && apt-get install -y -qq --no-install-recommends \
    ca-certificates cmake ninja-build g++ pkg-config nodejs \
    qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-declarative-dev-tools \
    qt6-svg-dev qt6-multimedia-dev qt6-shadertools-dev qt6-base-private-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly >/dev/null' && break   # ugly: x264enc, la cámara de prueba
  echo "apt no anduvo (intento $i), reintento"
done
cmake -S . -B build-pruebas -G Ninja -DCMAKE_BUILD_TYPE=Release -DBETO_PRUEBAS=ON -DBETO_PROGRAMA=ON >/dev/null
cmake --build build-pruebas
LANG=C.UTF-8 ctest --test-dir build-pruebas --output-on-failure
