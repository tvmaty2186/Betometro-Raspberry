# Plan de la 1.0.30 (07/11/2026)

Lo que quedó sin hacer de las 1.0.28 y 1.0.29 (todo aprobado por Matías). El detalle de cada punto está en
[plan-1.0.28.md](plan-1.0.28.md) y en el informe de su auditoría (`docs/auditorias/NN-…md`).
Commits intermedios con `[skip ci]`; publicar al final con el OK de Matías.

- [x] **Prueba completa** (Matías, 07/11/2026): la Pi se revisa sola de punta a punta y manda el resultado. Hecho:
      `app/prueba_completa.h/.cpp` (objeto `PruebaCompleta`, 12 pasos sin trabar la pantalla: sistema, simulador,
      voz con «¿Se escuchó?», pulsador con 20 s, GPS, cámara 30 s con video+sonido y MP4, rendimiento grabando,
      reproducción con `VideoGst` contando cuadros, tarjeta con `dd` de 50 MB, wifi e internet, Drive, pantallas con
      `QQmlComponent`), pantalla `tablet/TPrueba.qml` (mismo estilo que el Simulador), botón en Ajustes (11 botones:
      2 filas de 4 y una de 3), resultado en `<datos>/prueba-completa.json` y `.txt`, al reporte de errores (y se manda
      si hay llave), y `/prueba` en «Betómetro en la red» con «Empezar» (`POST /prueba/empezar`, docs/conectarse-ssh.md).
      No corre en carrera. En la PC los pasos de hardware dicen «no se puede en la PC» (naranja).
- [ ] Pasarle a Matías los comandos para pegar por ssh y anotar los números: peso de la base y procesador de la
      cámara (04, sección 4, pasos A y B); arranque, servicios y lugar ocupado (10, «Para mirar en la Pi»).
- [ ] Blindar el servicio sin dejar de ser root (`ProtectSystem=full`, `ProtectHome=yes`, `PrivateTmp=yes`),
      probándolo en la Pi (10-1.5 b).
- [ ] Firmar los paquetes de actualización (opcional, medio) (10-3).
- [x] Carrera y recorrido libre de punta a punta con `simular_neo` en las pruebas (09 P3). Hecho: prueba `carrera_simulada`
      (`pruebas/programa/prueba_carrera.cpp`): `simular_neo --fija` por un puerto virtual, el `GpsNeo` real y el `Crono`
      con el reloj de verdad; 3 vueltas de 6, 8 y 10 s a 48 km/h; chequea vueltas, tiempos (±0,15 s), distancias (±3 %),
      traza, voz, informe y el archivo guardado; después `--corte` (pierde el fix sin callarse). ~40 s. El recorrido
      libre ya estaba en `prueba_crono`.
- [x] Probar compilar en ARM de verdad (`ubuntu-24.04-arm`) para bajar de 20 a ~5 min (10-2.1). ARM **solo al
      publicar**; en cada subida, solo x86 (Matías). Hecho: el ARM (emulado) corre solo al publicar; el ARM de
      verdad (`ubuntu-24.04-arm`) queda para la 1.0.29.
- [x] Capturas comparadas contra referencias: primero Avispón h/v, después los 4 temas (09-1.2, P2). Hecho el Avispón
      h/v: prueba `capturas` (`herramientas/comparar-capturas.sh`, 11 pantallas × h/v, 9 s) contra `pruebas/capturas/`,
      tolerancia 1 % de píxeles; `BETO_FIJO=1` (GPS sin azar, hora 12:00) y `faketime` (fecha fija). Si una cambia a
      propósito: `herramientas/comparar-capturas.sh <betometro> <comparar_capturas> <salida> --aceptar` y commitear.
      Faltan los otros 3 temas.
- [x] Dobles de wifi y cámara para que corran en GitHub (09-1.5, P4). La cámara de prueba necesita
      `gstreamer1.0-plugins-ugly` (x264enc): con eso `prueba_crono` y `prueba_videos` pasan en Docker. Wifi hecho:
      `nmcli` e `iw` de mentira en `pruebas/dobles/` (escenarios casa, nada, dnsmasq) y prueba `wifi`
      (`prueba_wifi.cpp`): conectar, clave mala, olvidar, propia a los 90 s (reloj adelantado), redes apagadas, falla dnsmasq.
- [ ] Una biblioteca `betometro_base` para que las pruebas no recompilen el programa (01-2.2).
- [x] `-Wall -Wextra` también en el programa y la cámara (02-2).
- [x] Borrar las fotos de referencia viejas de `docs/capturas/` (02/11); las nuevas salen de las capturas comparadas (09-1.2).
- [x] Listas del video que se rearman 10 veces por segundo → elementos fijos; aliviar el modo tele (03-1.2, 1.3).
- [x] Relojes del cronómetro prendidos solo con una carrera preparada (02-2).
- [x] GPS con puerto fijo: apagar el aviso si se desenchufa (02-2).
- [x] Traza larga: no se cambia. Solo pasa en vueltas de más de 33 min a 10 Hz y cambiar el tope tocaría la igualdad
      con la app (02-3, 04-3.5).
- [x] Drive: no armar el informe de todas las carreras en el hilo principal (04-3.6).
- [x] Modo tele: contar vueltas en C++, sin armar el informe de cada carrera (04-3.7, 01-2.4).
- [x] Anillo de la cuenta solo si se usa (hecho 1.0.29). La franja de la Sugerida se deja: repinta cada medio km/h, poco (03-3).
- [x] `Datos.cfg()` fuera de los enlaces de la carrera (hecho 1.0.29); león y vista de la cámara con `sourceSize` (03-3).
- [x] Textos: ~~«undefined satélites»~~ (hecho), ~~banderita «?» en el nombre del informe~~ (hecho), línea de práctica de
      pie cortada (`Carrera.qml:92`, falta), «MARCA 1 · solo deja una marca» en el recorrido (`widgets/TableroRecorrido.qml:40`,
      falta) (07-2.5); ~~hora «--:--» sin hora válida~~ (hecho) (07-1.4).
- [x] Partir `datos.cpp` (hace cinco cosas) (01-1.2). Hecho: `datos.cpp` + `datos_resumen/informe/videos/config/guardado/muestra.cpp`; la clase `Datos` sigue igual.
- [x] Sacar las 3 páginas web de adentro de `servidor.cpp` a archivos (01-1.3). Hecho: `recursos/web/index.html`, `drive.html` y `control.html`, cargadas de los recursos.
- [x] Partir `Informe.qml` (seis pantallas en una) (01-1.4). Hecho: `app/qml/pantallas/informe/` (cabecera, mapa, gráfico, tabla, recorrido libre, video); capturas iguales antes y después.
- [x] Formatos repetidos a `Formato.qml` (01-2.1). Hecho: `Formato.hms`, `Formato.horaVideo` y `Formato.coma` (Informe, Reproductor, TableroRecorrido, ListaVideos). Los tiempos «m:ss:d» ya estaban (`reloj`, `tiempo`).
- [x] Plantilla del informe: marcar los bloques propios y el video en `recursos/informe-video.js` (01-2.3). Hecho: marcas `RASPBERRY … /RASPBERRY` en `recursos/informe/fuente.html`; `armar-informe.py` pega el JS del video. Ojo: la fuente estaba atrasada respecto de la plantilla (recorrido libre); se emparejó.
- [ ] Objetos globales como singletons de Qt 6, después de borrar las pantallas viejas (01-3.1).
- [x] `plan-ingenieria.md` que diga cómo están las carpetas hoy (01-3.3).
- [ ] `Crono.estado` en datos sueltos, solo si lo medido en la Pi muestra lentitud (01-3.2, 03-3).
- [x] **Hora: la Pi estaba 11 h atrasada** porque la wifi no tenía internet: arranca con la hora del apagado y nada
      la corrige. **La hora del GPS es lo primero del sistema.** Hecho en la 1.0.28 (falta probarlo en la Pi sin
      wifi). Queda para la 1.0.29: mostrar en el Chequeo previo de dónde salió la hora (GPS, internet o sin hora).
- [ ] NetworkManager: sin ahorro de energía del wifi y sin MAC al azar, si la Pi muestra fallas (B-2.3).
- [x] Simulador de GPS que pierda la señal como el NEO-M8N real (A-1.2). El **simulador del Chequeo previo va
      adentro del programa** (GPS y pulsador simulados) (Matías: sí). Hecha la primera parte: `simular_neo --corte`
      sigue mandando NAV-PVT con fixType 0 (3 satélites, precisión enorme); `--silencio` es el cable cortado; `--fija`
      velocidad constante. Falta el Chequeo previo.
- [x] **Voz guardada en el repo** (los WAV), para no depender de edge-tts ni de la caché de 7 días de GitHub
      (Matías: sí) (A-1.4).
- [ ] Arranque: que «no apagues» se vea mientras se rearma la foto de arranque (A-3).
- [x] **Gráficos del informe a pantalla completa (Pi y celu)** (muestras aprobadas en `docs/muestras/1.0.30/graficos/`).
      Doble toque sobre el gráfico de velocidad o el mapa del informe → pantalla completa acostada (de pie se ve girada):
      cuadrícula cada 100 m y 5 km/h, todas las lecturas con color por tramo, raya por vuelta con tiempo y error,
      MÁX/MÍN/PROM de lo visible, Vel./Acel./Adel., toque = globito, doble toque = acercar, pellizco = zoom, arrastrar,
      barrita de toda la carrera, X. El mapa: zoom/arrastre, rayitas cada km, marcas con km y hora, escala de colores,
      mapita. Sin sugerida (recorrido libre) el color de la velocidad es contra el promedio, también en el mapa normal.
      Pi: `app/qml/pantallas/informe/GraficoCompleto.qml` y `MapaCompleto.qml` (Canvas, solo QtQuick). Celu:
      `recursos/informe-zoom.js` (pegado en la plantilla por `armar-informe.py`). Capturas: `--prueba grafico | mapa`;
      `BETO_MUESTRA_RECORRIDO=1` suma un recorrido libre de muestra.

## Medido en la Pi con la 1.0.29 (07/11/2026)

- [x] Pantallas tablet en negro: QtQuick.Shapes no está en la Pi → Canvas; probar.sh abre todas las pantallas.
- [x] Drive «Sin internet» con «Unknown protocol specified»: pedidos a Google por HTTP/1.1.
- [x] Arranque 33 s, PulseAudio 13 s: probaba el micrófono de la webcam y esperaba hasta 10 s para elegir los
      auriculares. Ahora ignora todo el sonido USB y elige los auriculares por detrás (pulseaudio-auriculares.service).
- [ ] Calor: 62 °C quieta y frenada (`throttled=0x80008`). Revisar disipador y aire de la caja (Matías).
