# Auditoría 11, parte A: lo que faltó del programa (07/11/2026)

Lo que las auditorías 01 a 10 no miraron: clima, modo tele y control desde el celu, la página del celu
por dentro, QR, voz, simulador de GPS, arranque/apagado, Drive y reportes. Solo lectura. No repite lo del
`00-resumen.md` ni lo del `plan-1.0.28.md`.

Archivos que ningún informe nombraba y acá se revisaron: `clima.cpp`, `control_remoto.cpp`, `hdmi.cpp`,
`qr.cpp`, `audio.cpp`, `drive_logica.cpp`, `pulsador_gpio.cpp`, `contexto.h`, `sistema.h`. Los dos últimos y
`pulsador_gpio.cpp` están bien (reloj del kernel, pull-up, sin nada que decir).

## 1. Lo más grave

**1.1 En carrera, sin internet, el clima pide el nombre del lugar 10 veces por segundo**
- Qué encontré: `main.cpp:143` manda cada lectura del GPS (10 Hz) a `Clima::ponerPosicion`. Si se movió más
  de 5 km pide el nombre a OpenStreetMap (`clima.cpp:140-158`). Si el pedido falla (sin wifi, lo normal en
  carrera), pone `nombradoLat_ = 999` (`clima.cpp:151`) y la **próxima lectura, 100 ms después, vuelve a
  pedir**. Sin red el error llega al instante: un pedido fallido por cada lectura, toda la carrera.
- Riesgo: trabajo de más en la Pi 3 justo en carrera (resolver nombres, crear y destruir conexiones). Además
  es lo único del programa que sale a internet en carrera (Drive y reportes se paran, el servidor se apaga).
- Propongo: no pedir el nombre con una carrera en curso; si falla, esperar 2 minutos antes de reintentar
  (como hace el pronóstico con `intento`, `clima.cpp:174`). OpenStreetMap además pide máximo 1 pedido por segundo.
- Cuesta: chico.

**1.2 El simulador de GPS no pierde la señal como el NEO-M8N de verdad**
- Qué encontré: con `--corte`, `simular_neo.cpp:99,115` **deja de mandar mensajes**. El módulo real nunca se
  calla: sigue mandando NAV-PVT a 10 Hz con `fixType 0`, pocos satélites y `hAcc` enorme. Son dos caminos
  distintos en `gps.cpp`: el silencio dispara «callado» a los 6 s (`gps.cpp:111`, cambia de puerto); la
  pérdida de fix es `gps.cpp:207` (`if (!l.fix3d) continue`). Hoy la prueba solo recorre el primero.
  Tampoco simula: parada en un control (velocidad nunca baja de ~40 km/h), arranque en frío (30 s sin fix),
  cambio de baudios (en un pty no existe), ni lecturas con `horaValida` falsa.
- Riesgo: el «Simulador» del Chequeo previo (plan 1.0.28, §7) daría verde en casos que en la ruta fallan.
- Propongo: en `--corte` mandar NAV-PVT con `fix3d=false` (y una variante `--silencio` para el cable
  cortado); agregar `--parada S` (velocidad 0 unos segundos) y `--frio S`. Para el Chequeo previo, **no usar
  el puerto**: correr la carrera simulada adentro del programa con `GpsSimulado` (`gps.cpp:27-56`, ya existe)
  más un pulsador simulado, en una base aparte. Sin pty, sin competir con el GPS real enchufado.
- Cuesta: chico (simulador) + medio (Chequeo).

**1.3 Reportes de errores: si la llave de GitHub vence, nadie se entera**
- Qué encontré: `reportes.cpp:170-175` ignora cualquier error del envío (401 incluido) y reintenta cada
  10 min, con 20 s de espera cada vez, para siempre. Las llaves «fine-grained» de GitHub vencen (máximo un
  año). La llave está en texto plano en `/var/lib/betometro/reportes.token`, creada a mano: si se hizo con
  `echo` queda 0644, legible por el usuario `beto` y por cualquier proceso.
- Riesgo: los reportes dejan de llegar en silencio; la llave se puede copiar de la tarjeta.
- Propongo: con 401/403, anotar «llave vencida» una vez, parar hasta el próximo arranque y mostrarlo en el
  Chequeo previo («Reportes: sin llave»). Al arrancar, si el archivo no es 0600, ponerlo (`chmod`) y avisar.
  Además `reportes.cpp:167` manda solo los últimos 60 KB pero marca todo como enviado: lo del medio se pierde.
- Cuesta: chico.

**1.4 Publicar depende de una API no oficial de Microsoft (edge-tts)**
- Qué encontré: `programa.yml:39-59` graba las frases con `edge-tts` en cada compilación. La caché de GitHub
  (`~/.cache/betometro-voz`) ayuda, pero GitHub la borra a los 7 días sin uso. `edge-tts` ya se rompió varias
  veces (Microsoft cambia la API) y la actualización tarda días.
- Riesgo: un día no se puede publicar ninguna versión, aunque el programa esté perfecto.
- Propongo: guardar los WAV ya grabados (unos 15 MB) como archivo adjunto de un «release» del repo (o en una
  carpeta `voz/grabada/`). El workflow los baja; corre `grabar.py` solo si cambió la lista de frases.
- Cuesta: chico.

## 2. Lo que mejora con poco trabajo

**2.1 La caché del clima crece con cada kilómetro.** La clave es lat/lon con 2 decimales (≈1 km,
`clima.cpp:38`). En el inicio (`TInicio.qml:13`) el pronóstico sigue al GPS: viajando a la carrera, cada
pocos km se pide uno nuevo (40 KB con 16 días de datos) y `clima.json` se reescribe entero con todos los
lugares de la semana (`clima.cpp:53-61`). Propongo: clave redondeada a 0,05° (≈5 km) y como mucho 20 lugares
guardados. Chico.

**2.2 Si falta un audio, no suena y nadie avisa.** `audio.cpp:49-54` ignora el nombre que no está (por
ejemplo `n37` o `vuelta_12_m`): en carrera se saltea un número en silencio. Propongo: al arrancar, revisar
la lista de 186 nombres esperados (la de `grabar.py:56-70`) y mostrar en el Chequeo «Voz: faltan N frases»;
«Probar la voz» podría decir un número y una vuelta, no solo «Betómetro listo». Chico.

**2.3 `despertar-touch` puede demorar el arranque 15 s sin decir nada.** Si el touch no aparece, hace 5
intentos de 3 s (`despertar-touch.service:10`) **antes** de que arranque Betómetro (`Before=`), y termina
con «exit 0»: no queda rastro. Propongo: 3 intentos, anotar en el registro si no apareció, y que el Chequeo
diga «Touch no encontrado» (hoy, sin touch, la pantalla queda muda). Chico.

**2.4 `/control/accion` acepta GET aunque el comentario diga «POST solo».** `servidor.cpp:113` responde
cualquier GET con `responder()`, y ahí está `/control/accion` (`servidor.cpp:171`). Una página cualquiera
abierta en el celu puede meter `<img src="http://betometro.local/control/accion?tipo=salir">`. Es un caso
concreto dentro de 10-1.5. Propongo: en `/control/accion`, solo POST. Chico.

**2.5 La página Drive del celu muestra el mail de la cuenta de Google** a cualquiera de la wifi
(`servidor.cpp:337`). Propongo: «Conectado» sin el mail (el mail se ve en la pantalla de la Pi). Chico.

**2.6 QR: librería sin versión anotada y una excepción sin atajar.** `app/terceros/qrcodegen/LEEME.md` es el
README de la versión Java, sin decir qué versión se copió (la licencia MIT está bien incluida). Y
`qr.cpp:10`: `encodeText` tira `data_too_long` si el texto pasa ~2.300 bytes; corre en el hilo de imágenes
de Qt, así que una excepción cierra el programa. Hoy los textos son cortos (una dirección), pero el QR del
reporte de la 1.0.28 va a crecer. Propongo: anotar la versión (es la 1.8.0, 2022) y atajar la excepción
devolviendo un QR vacío. Chico.

## 3. El resto

- **Control de la tele**: `ControlTele.qml:135,141,159` usan `c.rep` sin chequear que exista; si la tele
  termina mientras se toca un botón queda un error JS en el reporte. Chico. `control_remoto.cpp` revisa bien
  todo lo que llega del celu (rangos, vuelta con video, carrera de la lista).
- **Página del celu por dentro**: el informe mete los datos con `<` escapado (`informe.cpp:348`) y el nombre
  con HTML escapado: bien, un nombre de carrera raro no rompe nada. El JS del control y del informe anda en
  iPhone y Chrome (`fetch`, eventos pointer, `playsinline`, `webkitRequestFullscreen`); `min()` en CSS
  necesita iOS 13.4 o más. Lo que el iPhone no abre es el MKV, y la página ya lo avisa (`plantilla:367`); el
  MP4 lo cubre el informe 06. El control pregunta cada 500 ms con una conexión nueva cada vez: anda, pero
  con 2 celus son 4 conexiones por segundo en la Pi.
- **Reportes, dos chicos**: cuando el registro pasa de 200 KB se recorta y se borra «enviado hasta»
  (`reportes.cpp:126-129`): los 100 KB que quedan se mandan de nuevo, repetidos. Y `journalctl` al prender
  ya está en el plan (10-3).
- **Drive**: la llave de Google va en archivos 0600 escritos aparte y renombrados (`drive.cpp:43-57`): bien.
  Si se corta la luz a mitad de una subida, `.drive-curl` (con el permiso de 1 hora) queda en la tarjeta hasta
  la próxima subida (`drive.cpp:604-633`). Chico: borrarlo al arrancar. «Desconectar» sin internet no llega a
  revocar el permiso en Google (`drive.cpp:306-310`): queda vivo hasta que vence, pero sin copia local. Aceptable.
- **Arranque y apagado**: el tema de Plymouth está bien (busca la pantalla más grande, escala 1). Ojo:
  cambiar la foto (`fondo.png`, 1,1 MB) rearma el initramfs en cada instalación con cambio
  (`instalar.sh:106-108`), minutos en la Pi 3; un corte de luz ahí deja la Pi sin arrancar. Hoy el `cmp` lo
  evita si la foto no cambió. Chico: avisar en pantalla «no apagues» ya está en el texto, falta que sea visible.
- **Tele enchufada después**: `Hdmi::reiniciar` sale con código 3 y systemd vuelve a abrir (`Restart=always`,
  `RestartSec=2`); no llega al límite de 5 reinicios en 10 s. Bien.
- **Clima en la PC** inventa un pronóstico (`clima.cpp:205-232`): las capturas no prueban nunca la lectura
  del JSON real de Open-Meteo. Chico: una prueba con un JSON guardado de verdad.

## Las 3 que haría primero

1. **1.1** Clima en carrera: no pedir el nombre con carrera en curso y esperar 2 min entre fallas.
2. **1.2** Simulador: perder el fix sin callarse, y el Simulador del Chequeo adentro del programa.
3. **1.3 + 1.4** Reportes que avisen cuando la llave vence, y los WAV de la voz guardados para no depender
   de edge-tts al publicar.

## Preguntas para Matías

1. ¿El repo `Betometro-reportes` es **privado**? Si es público, los registros de errores (nombres de wifi,
   rutas, versiones) los puede leer cualquiera. No pude verificarlo desde acá.
2. Los WAV de la voz, ¿los guardamos **en el repo** (15 MB, más simple) o **como adjunto de un release**?
3. El Simulador del Chequeo previo, ¿lo hacemos **adentro del programa** (sin puerto serie, con `GpsSimulado`)?
   Es lo que propongo; el `simular_neo` queda para las pruebas de GitHub.
