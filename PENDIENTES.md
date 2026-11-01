# Betómetro Raspberry — Pendientes

Actualizado el 01/11/2026.

## Diseño
- Hecho: sistema acordado, cableado del GPS, muestras horizontales de las 12 pantallas (aprobadas).
- **Sigue:** muestras de las mismas pantallas en vertical (720×1280).
  - Ojo: el armado vertical de fábrica de la app no entra igual en 720×1280 (el velocímetro redondo se mete en la cuenta). Hay que reacomodarlo y mostrarlo (ver `docs/app/lo-aprendido.md`, sección 6).

## Dudas para charlar con Matías (salieron de leer la app)
- **Fecha sin hora del GPS:** LARGAR se habilita solo si la carrera es de hoy. ¿Qué pasa si todavía no hay hora del GPS?
- **Pulsador conectado:** en la app decide si MARCAR en pantalla es obligatorio. En la Raspberry el pulsador va fijo por GPIO: ¿MARCAR en pantalla siempre visible?
- **Exportar respaldo:** ¿se quiere en la Raspberry (por ejemplo, por QR como el informe)?
- **Precisión del GPS:** qué dato del NEO-M8N usar como «precisión» (el umbral de fábrica es 20 m).
- **Terminar a mano** pierde la vuelta en curso (así es en la app). ¿Se deja igual?

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
