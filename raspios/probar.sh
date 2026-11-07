#!/bin/bash
# Prueba de humo: en un debian:trixie arm64 LIMPIO se instala solo lo de paquetes.txt,
# se copia Betómetro y se lo deja andar 40 s sin pantalla. Falla si se cae, si falta
# algún módulo de QML o si no puede cargar las tipografías.
set -euo pipefail
cd "$(dirname "$0")/.."
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends $(grep -v '^#' raspios/paquetes.txt) curl unzip >/dev/null
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
if grep -qiE "No pude cargar la fuente|is not installed|module .* not found|Cannot load library|failed to load component|QQmlApplicationEngine failed|Unsupported image format" /tmp/log.txt; then
  echo "MAL: faltan piezas (ver arriba)"; exit 1
fi
echo "BIEN: Betómetro anduvo 40 s sin problemas"

# «Betómetro en la red»: la página del celular (en la PC/GitHub escucha en el 8080)
mkdir -p /tmp/srv
QT_QPA_PLATFORM=offscreen LANG=C.UTF-8 XDG_RUNTIME_DIR=/tmp/run timeout 150 /usr/bin/betometro --sin-gps --datos /tmp/srv --muestra >/tmp/srv.log 2>&1 &
for i in $(seq 1 60); do curl -sf -o /tmp/pag.html http://127.0.0.1:8080/ && break; sleep 2; done
grep -q "Santa Clara" /tmp/pag.html || { echo "MAL: la página de carreras no lista las carreras"; cat /tmp/srv.log | head -20; exit 1; }
id=$(grep -o 'href="/informe/[^"?]*"' /tmp/pag.html | head -1 | sed 's|href="/informe/||; s|"||')
curl -sf -o /tmp/inf.html "http://127.0.0.1:8080/informe/$id" && grep -qi "<html" /tmp/inf.html || { echo "MAL: no abre el informe $id"; exit 1; }
curl -sf -D /tmp/h.txt -o /dev/null "http://127.0.0.1:8080/informe/$id?descargar=1" && grep -qi "attachment" /tmp/h.txt || { echo "MAL: no descarga el informe"; exit 1; }
curl -sf -o /tmp/todos.zip http://127.0.0.1:8080/todos.zip && unzip -tq /tmp/todos.zip || { echo "MAL: el .zip de todos no está bien"; exit 1; }
echo "BIEN: Betómetro en la red: $(unzip -l /tmp/todos.zip | tail -1)"
kill %1 2>/dev/null || true

# Capturas de pantallas (para mirarlas antes de entregar): quedan en salida-pi/capturas
mkdir -p salida-pi/capturas /tmp/cap
faltan=0
cap() { n=$1; shift
  QT_QPA_PLATFORM=offscreen LANG=C.UTF-8 XDG_RUNTIME_DIR=/tmp/run BETO_VOZ=/usr/share/betometro/voz \
    timeout 120 /usr/bin/betometro --datos /tmp/cap --muestra --orientacion h --captura "salida-pi/capturas/$n.png" "$@" \
    >"/tmp/cap-$n.log" 2>&1
  [ -s "salida-pi/capturas/$n.png" ] || { echo "MAL: no salió la captura $n"; faltan=1; }
  # Una pantalla que no carga (por ejemplo un módulo de QML que no está en paquetes.txt) sale en negro:
  # en la 1.0.29 pasó con QtQuick.Shapes y todas las pantallas tablet quedaron negras en la Pi
  if grep -qiE "is not installed|module .* not found|failed to load component|Type .* unavailable|Cannot load library" "/tmp/cap-$n.log"; then
    echo "MAL: la pantalla $n no carga:"; grep -iE "is not installed|not found|failed|unavailable|Cannot load" "/tmp/cap-$n.log" | head -5; faltan=1
  fi; }
cap inicio --pantalla inicio
cap informe --pantalla informe --carrera "Santa Clara"
cap informe-unir --pantalla informe --carrera "Santa Clara" --prueba unir
cap informe-menu --pantalla informe --carrera "Santa Clara" --prueba menu
cap ajustes --pantalla ajustes
# Las pantallas tablet (las que se ven después del inicio) y la carrera
for p in practica carreras chequeo conectividad actualizar camara clima simulador; do cap "tablet-$p" --pantalla "$p"; done
cap ficha --pantalla ficha --carrera "Santa Clara"
cap carrera --pantalla carrera --carrera "Gualeguaychú" --prueba largar
cap inicio-tablet --pantalla inicio --prueba tablet
cap salir-de-practica --pantalla carrera --prueba salir   # Salir sin largar vuelve a Práctica (no a «nueva carrera»)
ls -la salida-pi/capturas
[ "$faltan" = 0 ] || { echo "MAL: faltan capturas (una pantalla no abre)"; exit 1; }

