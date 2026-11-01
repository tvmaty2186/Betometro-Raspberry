# Betómetro Raspberry — Pendientes

Actualizado el 01/11/2026.

## Diseño
- Hecho: sistema acordado, cableado del GPS, muestras horizontales de las 12 pantallas (aprobadas).
- **Sigue:** muestras de las mismas pantallas en vertical (720×1280).

## Programar (después de aprobar las verticales)
- Imagen Buildroot (solo lectura, A/B, watchdog) armada por GitHub Actions.
- Motor en C++, comparado con el `Motor` de la app.
- Pantallas en Qt/QML.
- GPS por UART, pulsador por GPIO, voz con Piper pregrabada.

## Compras que faltan
- Fuente de 5 V 2,5 A y cable micro-USB.
- Amplificador MAX98357A con parlante de 3 W 4 Ω.
- Cables dupont y pulsador.
- Más adelante: convertidor de 12 V a 5 V 3 A, fichas GX12 y cable espiralado.
