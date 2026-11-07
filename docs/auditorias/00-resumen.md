# Resumen de las 10 auditorías (07/11/2026)

Lo más importante de los 10 informes, ordenado por urgencia. El detalle de cada punto está en su informe
(el número entre paréntesis: `05-1.1` = informe 05, punto 1.1).

## A. Urgente: puede hacer perder una carrera o dejar el equipo sin andar

| # | Qué pasa | Informe | Costo |
|---|---|---|---|
| 1 | **El recorrido libre pierde todo el recorrido si se corta la luz** antes de Terminar (la traza se guarda recién al final). Solo quedan las marcas | 05-1.1 | chico |
| 2 | **El informe se cae** si en la traza aparece una distancia negativa (confirmado con una prueba) | 02-1.1 | chico |
| 3 | **Corte de luz durante una actualización** → Betómetro puede quedar roto, sin volver solo a la versión anterior. Además cada actualización reescribe `cmdline.txt` (arranque de la Pi) aunque no cambie nada | 10-1.1, 1.2 | medio + chico |
| 4 | **Si el instalador falla a mitad**, queda apagado y la pantalla dice «se reinicia sola» | 10-1.3 | chico |
| 5 | **Si no se puede guardar (tarjeta llena), nadie se entera.** Con la tarjeta llena en carrera, además, se corta el video | 05-1.2, 06-1.3 | chico |
| 6 | **Las pruebas no frenan la publicación**: una versión rota puede llegar igual a la Raspberry | 09-1.1 | chico |
| 7 | **La cámara puede quedar «grabando» para siempre** si se cuelga o se desenchufa, y no se vuelve a grabar | 02-1.2, 06-1.4 | chico |

## B. Importante: se nota en el uso o en el video

| # | Qué pasa | Informe | Costo |
|---|---|---|---|
| 8 | **El video con sonido queda corrido** medio segundo o más respecto de los datos | 06-1.1 | chico |
| 9 | **480p está roto con tu cámara** (da 1080p, 720p y 800×600; no 480p). Se arregla con una línea | 06-1.2 | chico |
| 10 | **Guardar la base es lento y crece con cada carrera**; abrir el informe traba la pantalla unos segundos. Hay un arreglo chico (10 veces más rápido, mismo archivo) | 04-1.1, 1.2, 05-2.1 | chico |
| 11 | **Al terminar, el navegante cae en un formulario** y no ve cómo le fue | 07-1.1 | chico |
| 12 | **Dos botones dicen LARGAR** y hacen cosas distintas (uno solo va a la pantalla, el otro larga) | 07-1.2 | chico |
| 13 | **Los avisos en carrera son muy chicos** (letras de 1,3 a 1,7 mm). Sin GPS no queda claro qué hacer | 07-1.3 | chico |
| 14 | **Una barrita se anima sin parar** y hace trabajar la pantalla a 60 cuadros por segundo toda la carrera | 03-1.1 | chico |
| 15 | **Una marca de más no se puede deshacer en carrera** | 07-1.5 | medio |

## C. Para ordenar con calma

- **10 pantallas viejas** que ya no se ven pero se siguen editando por error (01-1.1).
- `datos.cpp`, `servidor.cpp` e `Informe.qml` crecieron de más (01-1.2 a 1.4).
- La base en un solo archivo: pasar a un archivo por carrera, en una versión aparte (05, sección 4).
- Compilar en GitHub en ARM de verdad: de 20 a unos 5 minutos (10-2.1).
- Betómetro corre como root y la red propia tiene la misma clave en todos los equipos (10-1.5).
- Visual: el engranaje, 4 textos cortados, colores fijos que no siguen el tema (08).

## Qué propongo hacer

**1.0.27 «sin sorpresas»** (todo chico, sin cambiar pantallas): puntos 1, 2, 4, 5, 6, 7, 9, 10 y el
`cmdline.txt` del 3. Ninguno necesita que elijas nada.

**Después**, cuando contestes las preguntas: la vuelta atrás automática (3), el video (8), lo de uso
(11 a 15) y lo visual (con imagen antes).

**Antes de todo eso**, si podés: medir en la Pi con los comandos del informe 04 (pasos A y B). Así se
sabe cuánto pesa hoy tu base y cuánto procesador usa la cámara.

## Respuestas de Matías (07/11/2026)

1. Vuelta atrás automática: **sí**.
2. No publicar si falla una prueba: **sí**.
3. Terminar → derecho al informe, con el error total grande arriba: **sí**.
4. Botón «Deshacer» después de cada marca: **no**.
5. 480p en 800×600: **sí**.
6. Poco lugar en carrera → parar el video y que la carrera siga: **sí**.
7. Copia de la base: **sí, subirla a Drive junto con el video y el informe**, cuando se conecta a la wifi favorita.
8. Clave de la red propia distinta por equipo: **no** (queda «betometro»).
9. Borrar las 10 pantallas viejas: **sí**.
10. Compilar solo desde una rama (`main`): **sí**.

## Las preguntas que te tocan (las más importantes)

1. ¿Si una versión nueva no arranca, que **vuelva sola** a la anterior y te avise? (10)
2. ¿Que **no se publique** una versión si falla alguna prueba? (09)
3. Al tocar **Terminar**, ¿ir derecho al **informe**, con el error total grande arriba? (07)
4. ¿Un botón chico **«Deshacer»** unos segundos después de cada marca? (07)
5. **480p:** ¿te sirve en 800×600 (un poco cuadrado), que hace trabajar la Pi menos de la mitad? (06)
6. Si queda **poco lugar** en carrera: ¿parar el video y que la carrera siga? (06)
7. ¿Subir una **copia de la base a Drive** al terminar cada carrera? (05)
8. ¿La clave de la red propia **distinta en cada equipo** (la ves en el QR)? (10)
9. ¿**Borrar las 10 pantallas viejas** que no se usan? (01)
10. ¿Compilar solo desde **una rama** (`main`) en vez de las 4? (10)

Cada informe tiene más preguntas al final. Las de arriba son las que traban algo.

## Los 10 informes

[01 Arquitectura](01-arquitectura.md) · [02 Código C++](02-codigo-cpp.md) · [03 Pantallas QML](03-pantallas-qml.md) ·
[04 Rendimiento en la Pi](04-rendimiento-pi.md) · [05 Datos y guardado](05-datos-guardado.md) · [06 Video](06-video.md) ·
[07 UX](07-ux.md) · [08 Diseño visual](08-diseno-visual.md) · [09 Pruebas](09-pruebas.md) · [10 Sistema](10-sistema.md)
