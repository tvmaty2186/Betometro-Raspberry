# Modo Tablet (aprobado por Matías, 05/11/2026)

Un **interruptor dentro de cada tema** (Ajustes → Pantalla y tema → «Modo Tablet»). Prendido: todo en botones
grandes, como apps de una tablet, para tocar con el auto en marcha. Apagado: como siempre. Usa los colores, bordes y
letras del tema elegido (Avispón, Rally, Deportivo, Cabina), así que sale solo en los cuatro.

## Reglas de diseño
- Barra fija arriba: volver grande (64 px), título, GPS, wifi y hora grande. Siempre igual.
- Botones cuadrados con bordes redondeados: ícono grande, nombre (30 px) y un dato útil abajo en el color de acento.
- Sin textos de ayuda. Nada de letras de menos de 21 px.
- Nada de deslizadores ni cositas chicas: **− / +** grandes, interruptores grandes, opciones en botones.
- Horizontal es lo principal; vertical, las mismas cosas en 2 columnas.

## Muestras (docs/muestras/tablet/)
inicio-h / inicio-v (Avispón), inicio-rally / inicio-sport / inicio-cabina, ajustes-h, voz-h (una sección por
dentro), camara-h, practica-h, clima-h, carrera-h. Se arman con `gen.py` (Playwright).

## Pantalla de carrera
**Queda como está** (ya está pensada para el copiloto). En Modo Tablet solo cambia la franja de arriba: nombre de la
carrera más grande (24 px), GPS más grande y «Terminar» grande.

## Partes
1. Interruptor + barra de arriba + **Inicio** + franja de la **pantalla de carrera**.
2. **Práctica libre**, **Clima**, **Cámara**.
3. **Ajustes** (menú de 8 botones + cada sección con filas grandes).
4. **Carreras**, **Ficha**, **Chequeo previo**, **Conectividad**, **Actualizar**, **Informe**.
