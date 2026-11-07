# Plan de la 1.0.30 (07/11/2026)

Lo que quedó sin hacer de las 1.0.28 y 1.0.29 (todo aprobado por Matías). El detalle de cada punto está en
[plan-1.0.28.md](plan-1.0.28.md) y en el informe de su auditoría (`docs/auditorias/NN-…md`).
Commits intermedios con `[skip ci]`; publicar al final con el OK de Matías.

- [ ] Pasarle a Matías los comandos para pegar por ssh y anotar los números: peso de la base y procesador de la
      cámara (04, sección 4, pasos A y B); arranque, servicios y lugar ocupado (10, «Para mirar en la Pi»).
- [ ] Blindar el servicio sin dejar de ser root (`ProtectSystem=full`, `ProtectHome=yes`, `PrivateTmp=yes`),
      probándolo en la Pi (10-1.5 b).
- [ ] Firmar los paquetes de actualización (opcional, medio) (10-3).
- [ ] Carrera y recorrido libre de punta a punta con `simular_neo` en las pruebas (09 P3).
- [ ] Probar compilar en ARM de verdad (`ubuntu-24.04-arm`) para bajar de 20 a ~5 min (10-2.1). ARM **solo al
      publicar**; en cada subida, solo x86 (Matías). Hecho: el ARM (emulado) corre solo al publicar; el ARM de
      verdad (`ubuntu-24.04-arm`) queda para la 1.0.29.
- [ ] Capturas comparadas contra referencias: primero Avispón h/v, después los 4 temas (09-1.2, P2).
- [ ] Dobles de wifi y cámara para que corran en GitHub (09-1.5, P4). La cámara de prueba necesita
      `gstreamer1.0-plugins-ugly` (x264enc): con eso `prueba_crono` y `prueba_videos` pasan en Docker.
- [ ] Una biblioteca `betometro_base` para que las pruebas no recompilen el programa (01-2.2).
- [ ] `-Wall -Wextra` también en el programa y la cámara (02-2).
- [ ] Borrar las fotos de referencia viejas de `docs/capturas/` (02/11); las nuevas salen de las capturas comparadas (09-1.2).
- [ ] Listas del video que se rearman 10 veces por segundo → elementos fijos; aliviar el modo tele (03-1.2, 1.3).
- [ ] Relojes del cronómetro prendidos solo con una carrera preparada (02-2).
- [ ] GPS con puerto fijo: apagar el aviso si se desenchufa (02-2).
- [ ] Traza larga: `std::deque` en vez de borrar el primero (02-3, 04-3.5).
- [ ] Drive: no armar el informe de todas las carreras en el hilo principal (04-3.6).
- [ ] Modo tele: contar vueltas en C++, sin armar el informe de cada carrera (04-3.7, 01-2.4).
- [ ] Anillo de la cuenta y franja de la Sugerida sin repintar de más (03-3).
- [ ] `Datos.cfg()` fuera de los enlaces de la carrera; león y vista de la cámara con `sourceSize` (03-3).
- [ ] Textos: ~~«undefined satélites»~~ (hecho), ~~banderita «?» en el nombre del informe~~ (hecho), línea de práctica de
      pie cortada (`Carrera.qml:92`, falta), «MARCA 1 · solo deja una marca» en el recorrido (`widgets/TableroRecorrido.qml:40`,
      falta) (07-2.5); ~~hora «--:--» sin hora válida~~ (hecho) (07-1.4).
- [ ] Partir `datos.cpp` (hace cinco cosas) (01-1.2).
- [ ] Sacar las 3 páginas web de adentro de `servidor.cpp` a archivos (01-1.3).
- [ ] Partir `Informe.qml` (seis pantallas en una) (01-1.4).
- [ ] Formatos repetidos a `Formato.qml` (01-2.1). Juntarlo con los tiempos «m:ss:d».
- [ ] Plantilla del informe: marcar los bloques propios y el video en `recursos/informe-video.js` (01-2.3).
- [ ] Objetos globales como singletons de Qt 6, después de borrar las pantallas viejas (01-3.1).
- [ ] `plan-ingenieria.md` que diga cómo están las carpetas hoy (01-3.3).
- [ ] `Crono.estado` en datos sueltos, solo si lo medido en la Pi muestra lentitud (01-3.2, 03-3).
- [ ] **Hora: la Pi estaba 11 h atrasada** porque la wifi no tenía internet: arranca con la hora del apagado y nada
      la corrige. **La hora del GPS es lo primero del sistema.** Hecho en la 1.0.28 (falta probarlo en la Pi sin
      wifi). Queda para la 1.0.29: mostrar en el Chequeo previo de dónde salió la hora (GPS, internet o sin hora).
- [ ] NetworkManager: sin ahorro de energía del wifi y sin MAC al azar, si la Pi muestra fallas (B-2.3).
- [ ] Simulador de GPS que pierda la señal como el NEO-M8N real (A-1.2). El **simulador del Chequeo previo va
      adentro del programa** (GPS y pulsador simulados) (Matías: sí).
- [ ] **Voz guardada en el repo** (los WAV), para no depender de edge-tts ni de la caché de 7 días de GitHub
      (Matías: sí) (A-1.4).
- [ ] Arranque: que «no apagues» se vea mientras se rearma la foto de arranque (A-3).
