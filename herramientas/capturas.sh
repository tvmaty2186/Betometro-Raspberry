#!/bin/sh
# Saca capturas del programa real, sin pantalla, para comparar con las muestras.
# Uso: herramientas/capturas.sh <programa betometro> <carpeta de salida> [pantallas…]
set -e
BIN=${1:?falta el programa}
OUT=${2:?falta la carpeta}
shift 2
PANTALLAS=${*:-inicio}
mkdir -p "$OUT"
export QT_QPA_PLATFORM=offscreen LANG=C.UTF-8
for p in $PANTALLAS; do
  for o in h v; do
    "$BIN" --pantalla "$p" --orientacion "$o" --captura "$OUT/$p-$o.png"
  done
done
ls -1 "$OUT"
