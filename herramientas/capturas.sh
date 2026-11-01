#!/bin/sh
# Saca capturas del programa real, sin pantalla, con carreras de ejemplo, para comparar con las muestras.
# Uso: herramientas/capturas.sh <programa betometro> <carpeta de salida>
set -e
BIN=${1:?falta el programa}
OUT=${2:?falta la carpeta}
mkdir -p "$OUT"
DATOS=$(mktemp -d)
export QT_QPA_PLATFORM=offscreen LANG=C.UTF-8
cap() {   # nombre pantalla [opciones…]
  n=$1; p=$2; shift 2
  for o in h v; do
    "$BIN" --datos "$DATOS" --muestra --pantalla "$p" --orientacion "$o" --captura "$OUT/$n-$o.png" "$@"
  done
}
cap inicio inicio
cap carreras carreras
cap ficha ficha --carrera "Gualeguaychú"
cap ficha-corrida ficha --carrera "Santa Clara"
cap practica practica
cap ajustes ajustes
cap conectividad conectividad
cap wifi wifi --prueba red-vecino
cap carrera carrera --carrera "Gualeguaychú"
cap carrera-largada carrera --carrera "Gualeguaychú" --prueba largar
cap carrera-sin-gps carrera --carrera "Gualeguaychú" --prueba largar --sin-gps
cap teclado-texto ficha --carrera "Gualeguaychú" --prueba teclado-texto
cap teclado-tiempo ficha --carrera "Gualeguaychú" --prueba teclado-tiempo
cap teclado-fecha ficha --carrera "Gualeguaychú" --prueba teclado-fecha
rm -rf "$DATOS"
ls -1 "$OUT"
