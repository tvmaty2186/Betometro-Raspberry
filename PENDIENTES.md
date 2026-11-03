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
- 03/11 (1.0.7): foto de arranque bien ubicada en la pantalla de 5".
- 03/11 (1.0.6): aviso «Actualizar» en el inicio; informes con el tema Avispón.
- 03/11 (1.0.5): pantalla de arranque con barra de carga y sin letras; pulsador por cable (1.0.4); letras chicas
  más grandes para 5"; últimos detalles de la app (pellizco, cartel de 5 s, barra ±, MARCAR, marco en sol, brillo
  de los temas); reporte de errores automático; wifi propia «Betometro-101»; menos escrituras en la tarjeta.
- 03/11 (1.0.4): el programa lee el pulsador (GPIO17, pin 11 a GND pin 9) con la pull-up interna y la hora
  exacta del kernel; marca en la carrera y responde en el Chequeo previo. Falta probarlo con el pulsador real.
- 03/11 noche (1.0.3): dígitos con la fórmula de la app y «Números − / +», editor con los chicos al frente
  y 4 asas, cara de instrumento con reflejo, colores de pasado/atrasado, estilos Anillo y Barra, redondo en
  más widgets, velocímetro liviano (aguja sin GPS, «máx», inicio animado), ficha para temas de letra ancha,
  botón Terminar más grande. **Arreglo:** el armado guardado en el editor nunca se cargaba.
- 03/11: actualización por wifi probada (1.0.1 → 1.0.2). Wifi real y «Betómetro en la red».
- 03/11: **Betómetro anda en la Raspberry** (Pi OS Lite): arranca solo, pantalla, touch y voz por auriculares.
  - Pantalla: es la Touch Display 2 de **5"** (720×1280), no de 7".
  - Touch: se despierta con `despertar-touch.service` (el sistema lo busca antes de que la pantalla lo encienda).
  - Sonido: el Qt de Debian solo sale por PulseAudio → `pulseaudio-sistema.service`.
  - GitHub prueba cada versión en un Debian arm64 limpio antes de entregarla.

## Ahora (Matías)
1. Actualizar a la 1.0.7 y probar: pantalla de arranque, pulsador (pin 11 y 9), wifi propia (lejos de casa).
2. Reporte de errores por internet (opcional): crear el repo privado `Betometro-reportes` y una llave que solo
   pueda crear issues ahí; se copia a la Raspberry en `/var/lib/betometro/reportes.token`. Sin eso, los
   errores igual quedan en la página «Betómetro en la red» → «Reporte de errores».

## Siguiente
- Probar en el auto: GPS, pulsador, letras en movimiento, rendimiento y temperatura de la Pi 3.
- Solo lectura total (overlay): necesita regrabar la tarjeta con una partición aparte para las carreras.
  Por ahora: casi nada se escribe en la tarjeta, las carreras se guardan de forma segura y se repara sola al prender.

## Más adelante
- Video: informe con video, webcam MJPEG ([docs/video.md](docs/video.md)).
- E6: prueba en pista y versión 1.0.0 definitiva.
- App del celular: el prompt está en [docs/prompt-app.md](docs/prompt-app.md) (se hace en otra sesión).

## Compras
- Auriculares con cable (3,5 mm), pulsador (10 kΩ y 100 nF).
- Para el auto: placa de borneras a tornillo para los GPIO, convertidor 12 → 5 V (3 A o más, ajustado a 5,1 V), fusible 3 A, cable de 0,75 mm², fichas GX12, cable espiralado, disipadores.
- Webcam USB MJPEG 1080p gran angular (para el video).
