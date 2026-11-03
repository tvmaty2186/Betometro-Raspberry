# Betómetro Raspberry — Pendientes

Actualizado el 03/11/2026. Plan: [docs/plan-ingenieria.md](docs/plan-ingenieria.md).

## Decisión del 03/11/2026: Raspberry Pi OS Lite en vez de Buildroot
- Betómetro se instala como **programa** en Raspberry Pi OS Lite 64 bits (Debian trixie), no como sistema propio.
  Motivo: cada imagen de Buildroot tardaba 2–3 h y fallaba por piezas del sistema. Ahora se compila solo el programa (~15 min con caché).
- Se pierde: copias A/B (queda la versión anterior guardada para volver a mano) y unos segundos de arranque.
- Buildroot (`sistema/`, `imagen.yml`) queda guardado sin usar.
- Cómo se arma e instala: `raspios/` y `.github/workflows/programa.yml`.

## Hecho
- E1 (motor igual a la app), E2 (programa completo, temas aprobados).
- Cambios de Matías: sin cantidad de vueltas, MEDIDA/LIBRE y «Unir vueltas» en el informe, libres sin error, cronómetro progresivo/regresivo, ideal a la vista, «libre» en todos lados.
- 03/11: **Betómetro anda en la Raspberry** (Pi OS Lite): arranca solo, pantalla, touch y voz por auriculares.
  - Pantalla: es la Touch Display 2 de **5"** (720×1280), no de 7".
  - Touch: se despierta con `despertar-touch.service` (el sistema lo busca antes de que la pantalla lo encienda).
  - Sonido: el Qt de Debian solo sale por PulseAudio → `pulseaudio-sistema.service`.
  - GitHub prueba cada versión en un Debian arm64 limpio antes de entregarla.

## Ahora (Matías)
1. Para publicar actualizaciones: README en `Betometro-actualizaciones` (público), token con Contents: Read and write y secreto `ACTUALIZACIONES_TOKEN` en este repo.
2. Elegir la pantalla de arranque (muestra: foto de los pilotos con la franja amarilla como barra de carga).

## Siguiente
- **Detalles de pantalla de la app que faltan** (pedido de Matías, 03/11): lista completa en
  [docs/app/diferencias-pantalla.md](docs/app/diferencias-pantalla.md). Lo principal: tamaño de los dígitos con la
  fórmula de la app (y «Números − / +»), dígitos de ancho fijo, widgets chicos al frente en el editor, cara de
  instrumento con reflejo, colores de pasado/atrasado, estilos Anillo y Barra, redondo en más widgets.
- **Ficha de la carrera (foto de Matías, 03/11):** los números grandes del resumen se salen del recuadro
  (error total cortado a la izquierda, mejor a la derecha); los chips de vueltas («V1 MEDIDA») muy chicos
  para la pantalla de 5"; en el nombre «02: 46» sale con espacio raro por la tipografía.
- Pantalla de arranque horizontal con barra de carga, sin letras (Plymouth).
- **Reporte de errores automático (pedido de Matías, 03/11):** Betómetro junta solo lo que falla (cierres,
  avisos del programa, GPS, sonido, wifi, actualizaciones; con versión y hora) y, cuando hay internet y no hay
  carrera, lo manda a un repo de reportes para mejorar las próximas versiones. Sin datos personales.
  A definir: repo (privado, por ejemplo `Betometro-reportes`, como issues) y una llave que solo pueda crear
  reportes ahí (la Raspberry la lleva adentro: que no sirva para nada más).
- Probar Ajustes → Actualizar con una versión 1.0.1 publicada.
- Conectar GPS (NEO-M8N) y pulsador (GPIO17, pin 11 / GND pin 9; pull-up 10 kΩ a 3,3 V); chequeo previo; medir demoras.
- Leer el pulsador en el programa (GPIO).
- Medir rendimiento en la Pi 3: pantalla fluida, procesador, temperatura.
- Solo lectura (overlay de Pi OS) para que cortar el contacto no rompa nada, con las carreras en un lugar aparte.
- Revisar letras chicas en la pantalla de 5" con el auto en movimiento.

## Más adelante
- E5: wifi real (hoy simulado), «Betómetro en la red», compartir por QR.
- Video: informe con video, webcam MJPEG ([docs/video.md](docs/video.md)).
- E6: prueba en pista y versión 1.0.0 definitiva.
- App del celular: el prompt está en [docs/prompt-app.md](docs/prompt-app.md) (se hace en otra sesión).

## Compras
- Auriculares con cable (3,5 mm), pulsador (10 kΩ y 100 nF).
- Para el auto: placa de borneras a tornillo para los GPIO, convertidor 12 → 5 V (3 A o más, ajustado a 5,1 V), fusible 3 A, cable de 0,75 mm², fichas GX12, cable espiralado, disipadores.
- Webcam USB MJPEG 1080p gran angular (para el video).
