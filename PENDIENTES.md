# Betómetro Raspberry — Pendientes

Actualizado el 01/11/2026.

## Diseño
- Hecho: sistema acordado, cableado del GPS, muestras horizontales de las 12 pantallas (aprobadas).
- Muestras verticales aprobadas (01/11/2026). El diseño visual está cerrado.

## Dudas para charlar con Matías (salieron de leer la app)

Resueltas el 01/11/2026: largar sin hora, correr sin GPS, informes sí y respaldo no, vuelta interrumpida, simulador NMEA, MARCAR en pantalla siempre visible. Ver «Decisiones propias» en `docs/diseno-sistema.md`.

- **Precisión del GPS:** técnica, la resuelve Claude (qué dato del NEO-M8N usar; umbral de fábrica 20 m).


## Programar

Se sigue [docs/plan-ingenieria.md](docs/plan-ingenieria.md) (E1 a E6). Plan aprobado por Matías el 01/11/2026.

- **E1 hecha** (01/11/2026): motor y modelo de datos en C++, idénticos a la app. Informe: [pruebas/motor/INFORME.md](pruebas/motor/INFORME.md).
- **Propuesta aprobada entera** (01/11/2026): conectividad, GPS y mejoras → [docs/propuesta-mejoras.md](docs/propuesta-mejoras.md). Muestra de Conectividad aprobada.
- **E2 completa y aceptada** (01/11/2026): temas aprobados. Detalle en el plan.
- **Sigue E3:** sistema operativo (Buildroot) para la Raspberry.
- ~~E2 en curso:~~ base del programa, Inicio, Carreras, Ficha, Práctica libre, pantalla de carrera, Ajustes, Conectividad, clave del wifi, Chequeo, Informe, Compartir y Actualizar hechas. Faltan: editor de pantalla, los otros 3 temas, modo sol, la voz y el simulador del NEO-M8N. Fondo en carrera: rojo/verde según la vuelta, como la app (decidido). Siguen las otras 12 pantallas, los widgets de carrera, los otros 3 temas, la voz y el informe.

- Imagen Buildroot (solo lectura, A/B, watchdog) armada por GitHub Actions.
- Pantallas en Qt/QML, con los 4 temas de la app. Mostrarle a Matías capturas de cada tema (Rally clásico, Deportivo moderno, Cabina de avión) antes de cerrar.
- GPS por UART, pulsador por GPIO, voz con Piper pregrabada por la salida de auriculares, volumen en Ajustes.

## Compras que faltan
- Fuente de 5 V 2,5 A y cable micro-USB.
- Auriculares con cable (ficha de 3,5 mm), si no tenés.
- Cables dupont y pulsador.
- Más adelante: convertidor de 12 V a 5 V 3 A, fichas GX12 y cable espiralado.
