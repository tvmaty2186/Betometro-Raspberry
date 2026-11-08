#!/bin/bash
# Capturas comparadas (auditoría 09, P2): saca las pantallas principales (tema Avispón, acostada y de pie) con
# el programa real sin pantalla y las compara con las referencias de pruebas/capturas/. Falla si una cambió más
# que la tolerancia (1 % de los píxeles). Matías no mira diferencias: si una cambió a propósito, se aceptan las
# nuevas con --aceptar y se commitean.
# Para que dos corridas den lo mismo: BETO_FIJO=1 (GPS simulado sin azar, hora «12:00», sin animación) y faketime
# (la fecha fija: las carreras de ejemplo son «hace 3 días», «en 14 días»). Lo corre ctest (prueba «capturas»).
# Uso: herramientas/comparar-capturas.sh <betometro> <comparar_capturas> <carpeta de salida> [--aceptar] [--tolerancia 1]
set -u
BIN=${1:?falta el programa betometro}
CMP=${2:?falta comparar_capturas}
OUT=${3:?falta la carpeta de salida}
shift 3
ACEPTAR=0
TOL=1
while [ $# -gt 0 ]; do
  case "$1" in
    --aceptar) ACEPTAR=1 ;;
    --tolerancia) TOL=$2; shift ;;
    *) echo "Opción desconocida: $1"; exit 2 ;;
  esac
  shift
done
REF="$(cd "$(dirname "$0")/.." && pwd)/pruebas/capturas"
command -v faketime >/dev/null || { echo "Falta faketime (apt-get install faketime)"; exit 2; }
rm -rf "$OUT"
mkdir -p "$OUT/nuevas" "$OUT/diferencias" "$OUT/datos"
export QT_QPA_PLATFORM=offscreen LANG=C.UTF-8 BETO_FIJO=1 FAKETIME_DONT_FAKE_MONOTONIC=1
export XDG_RUNTIME_DIR="$OUT/datos/run"
mkdir -p "$XDG_RUNTIME_DIR"

# Una captura: nombre pantalla [opciones…]; sale acostada (h) y de pie (v), de a 4 a la vez
LISTA=()
cap() { LISTA+=("$*"); }   # los nombres con espacios van con comillas adentro: --carrera '"Santa Clara"'
# La carrera largada depende del reloj (el crono va andando: «0:01:1», «± segundos» 0,0 o −0,1): más tolerancia
declare -A TOLS=([carrera-largada]=3)
una() {   # nombre orientacion pantalla [opciones…]
  n=$1; o=$2; p=$3; shift 3
  mkdir -p "$OUT/datos/$n-$o"
  faketime -f "@2026-11-07 12:00:00" "$BIN" --datos "$OUT/datos/$n-$o" --muestra --tema avispon --pantalla "$p" --orientacion "$o" \
    --captura "$OUT/nuevas/$n-$o.png" "$@" >"$OUT/datos/$n-$o.log" 2>&1
}
cap inicio inicio
cap carreras carreras
cap ficha ficha --carrera "Gualeguaychú"
cap ficha-corrida ficha --carrera '"Santa Clara"'
cap practica practica
cap ajustes ajustes
cap conectividad conectividad
cap chequeo chequeo
cap informe informe --carrera '"Santa Clara"'
cap comparar informe --carrera '"Santa Clara"' --prueba comparar
cap informe-comp informe --carrera '"Santa Clara"' --prueba comp
cap carrera carrera --carrera "Gualeguaychú"
cap carrera-largada carrera --carrera "Gualeguaychú" --prueba largar
# Con la letra más ancha (Cabina): en la 1.0.39 la fecha pisaba los botones − y + y no lo vio ninguna prueba,
# porque todas eran en el Avispón (Matías, 08/11/2026). El último --tema gana.
cap ficha-cabina ficha --carrera '"Santa Clara"' --tema cabina
cap practica-cabina practica --tema cabina
cap voz-cabina ajustesSeccion --prueba tablet-voz --tema cabina

t0=$(date +%s)
en_vuelo=0
for item in "${LISTA[@]}"; do
  for o in h v; do
    eval "set -- $item"   # nombre pantalla opciones (con las comillas de los nombres)
    n=$1; shift
    una "$n" "$o" "$@" &
    en_vuelo=$((en_vuelo + 1))
    if [ $en_vuelo -ge 4 ]; then wait -n; en_vuelo=$((en_vuelo - 1)); fi
  done
done
wait
echo "Capturas: $(ls "$OUT/nuevas" | wc -l) en $(( $(date +%s) - t0 )) s"

falla=0
cambios=()
for item in "${LISTA[@]}"; do
  eval "set -- $item"
  n=$1
  for o in h v; do
    f="$n-$o.png"
    if [ ! -s "$OUT/nuevas/$f" ]; then echo "FALTA  $f: no salió la captura (ver $OUT/datos/$n-$o.log)"; falla=1; continue; fi
    if [ "$ACEPTAR" = 1 ]; then cp "$OUT/nuevas/$f" "$REF/$f"; echo "ACEPTADA $f"; continue; fi
    if [ ! -f "$REF/$f" ]; then echo "NUEVA  $f: no hay referencia (correr con --aceptar)"; falla=1; continue; fi
    r=$("$CMP" "$REF/$f" "$OUT/nuevas/$f" --tolerancia "${TOLS[$n]:-$TOL}" --diferencia "$OUT/diferencias/$f")
    c=$?
    printf '%-7s%s: %s\n' "${r%% *}" "$f" "${r#* }"
    if [ $c -ne 0 ]; then falla=1; cambios+=("$f"); fi
  done
done
if [ "$ACEPTAR" = 1 ]; then echo "Referencias nuevas en $REF (commitealas)"; exit $falla; fi
if [ $falla -ne 0 ]; then
  echo "CAMBIARON: ${cambios[*]:-} — las diferencias (en rojo) están en $OUT/diferencias/."
  echo "Si el cambio es a propósito: herramientas/comparar-capturas.sh $BIN $CMP $OUT --aceptar"
  exit 1
fi
echo "BIEN: todas las capturas iguales a las referencias"
