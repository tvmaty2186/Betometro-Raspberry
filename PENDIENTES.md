# Betómetro Raspberry — Pendientes

Actualizado el 01/11/2026.

## Diseño
- Hecho: sistema acordado, cableado del GPS, muestras horizontales de las 12 pantallas (aprobadas).
- Muestras verticales hechas (01/11/2026). **Sigue:** que Matías las apruebe.

## Dudas para charlar con Matías (salieron de leer la app)

Resueltas el 01/11/2026: largar sin hora, correr sin GPS, informes sí y respaldo no, vuelta interrumpida, simulador NMEA, MARCAR en pantalla siempre visible. Ver «Decisiones propias» en `docs/diseno-sistema.md`.

- **Precisión del GPS:** técnica, la resuelve Claude (qué dato del NEO-M8N usar; umbral de fábrica 20 m).


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
