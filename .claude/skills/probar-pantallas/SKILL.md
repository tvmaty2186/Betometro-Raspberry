---
name: probar-pantallas
description: Compilar Betómetro en el contenedor, sacar capturas de pantallas, medir tiempos o memoria y correr las pruebas. Usar antes de mostrarle algo a Matías o de publicar.
---

# Probar pantallas en la PC

**Contenedor «beto»** (Qt de Debian trixie, como la Pi): repo en `/src`, compilación en `/build-t`.
```
docker info >/dev/null 2>&1 || { (dockerd --pidfile /tmp/dockerd-$RANDOM.pid >/tmp/dockerd.log 2>&1 &); sleep 8; }
docker start beto
docker exec beto bash -c 'cmake --build /build-t -j8 2>&1 | grep -E "error|Linking CXX executable betometro$"'
docker exec beto bash -c 'ctest --test-dir /build-t 2>&1 | grep -E "tests passed|\*\*\*"'
```
Si se agrega un `.qml` o `.cpp`, sumarlo a `app/qml/CMakeLists.txt` o `CMakeLists.txt` y correr `cmake -S /src -B /build-t`.

**Captura de una pantalla** (1280×720 acostada, 720×1280 parada):
```
docker exec beto bash -c 'BETO_FIJO=1 QT_QPA_PLATFORM=offscreen /build-t/betometro --datos /tmp/d --muestra \
  --orientacion h --tema avispon --pantalla informe --carrera "Santa Clara" --prueba comp --captura /tmp/x.png'
docker cp beto:/tmp/x.png <scratchpad>/
```
- `config.json` en la carpeta de datos para ajustes (`tema`, `temaPropio`, `relojInicio`, `modoSol`…).
- `--prueba`: ver la lista en `docs/raspios.md` («Probar en la PC»). `--espera MS` para dar tiempo.
- Variables: `BETO_FIJO=1`, `BETO_MUESTRA_HZ=5`, `BETO_MUESTRA_LARGA=1`, `BETO_VIGILANTE_UMBRAL=1`.

**Capturas de referencia** (`pruebas/capturas/`): si cambió a propósito,
`herramientas/comparar-capturas.sh /build-t/betometro /build-t/comparar_capturas /build-t/capturas --aceptar`,
y **revertir con `git checkout` las que cambiaron sin querer** (carrera-*, chequeo-* suelen moverse solas).
Nombres con espacio: `--carrera '"Santa Clara"'`.

**Página del celu**: el programa con `--muestra` escucha en 8080; Playwright desde afuera a
`http://172.17.0.2:8080/personalizar?c=fija` (390×844).

**Medir**: promedio de 5 arranques hasta `--captura`; memoria `VmRSS` de `/proc/<pid>/status`; procesador con los
campos 14+15 de `/proc/<pid>/stat`. La Pi 3 es unas 5 a 8 veces más lenta que la PC.

Mostrarle a Matías las capturas (SendUserFile) antes de publicar; si es algo visual nuevo, una muestra antes de programar.
