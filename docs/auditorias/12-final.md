# Auditoría 12 — Final, para entregar (08/11/2026)

Versión revisada: **1.0.33**. Solo lectura: no se cambió código. Cinco revisiones en paralelo
(carrera, C++ con sanitizers, sistema, pantallas y web, entrega) y lo grave verificado a mano.

**En una línea:** el programa está sano (sanitizers limpios, sin imports que falten en la Pi, motor igual a
la app), pero hay **2 errores que pueden arruinar una carrera o dejar el equipo muerto**, y para entregarlo
como producto **falta el manual y la guía para rearmar una SD**.

---

## A. Arreglar antes de entregar (todo chico)

| # | Qué pasa | Dónde | Costo |
|---|---|---|---|
| 1 | **El pulsador puede perder una marca sin avisar.** Si apretás y soltás rápido (50–100 ms) justo cuando la pantalla está ocupada (abrir algo, arrancar la cámara), los dos avisos llegan juntos y la marca se resuelve y se tira. Verificado leyendo el código | `app/nucleo/pulsador.cpp:6` (`flanco()` llama a `revisar()` y descarta el resultado) | chico + prueba |
| 2 | **Si una versión nueva se cuelga al abrir (no se cae), nunca vuelve atrás.** Cada intento espera 180 s; nunca suma 4 fallas en 300 s, así que la vuelta atrás no entra. Equipo muerto en el auto | `raspios/betometro.service:10,18` | chico |
| 3 | **Si el instalador falla en una función, queda pantalla negra** sin aviso ni reinicio: falta `set -E` y el `trap` no entra | `raspios/instalar.sh:4` | chico |
| 4 | **La vuelta atrás no repone los `.service`.** Si una versión cambia el servicio, el programa viejo arranca con el servicio nuevo y puede quedar en bucle | `raspios/instalar.sh:63`, `volver.sh` | chico |
| 5 | **La hora del equipo se corrige con la carrera en curso** (el GPS fija tarde): las vueltas ya cerradas quedan con otra fecha | `app/main.cpp:178-195` | chico |
| 6 | **Docs viejos en lo básico:** `PENDIENTES.md` dice 1.0.17, README dice pantalla de 7" (es 5"), `raspios.md` habla de otra rama y de 3 servicios (son 5), `diseno-sistema.md` todavía habla de Buildroot y Piper | varios | chico |

## B. Para que sea un producto (documentación)

| # | Qué falta | Costo |
|---|---|---|
| 7 | **Manual del navegante** (no existe): prender, Chequeo, largar, MARCAR, Terminar, informe, QR, wifi, Actualizar, qué hacer si falla GPS, cámara, voz o pulsador. También en «Betómetro en la red» (`/ayuda`) | medio |
| 8 | **SD nueva desde cero en 8 pasos:** qué imagen, qué poner en el Imager, primer paquete, restaurar carreras, Drive y `reportes.token` | medio |
| 9 | **Licencias:** las fuentes son OFL (hay que incluir el texto). Las voces son de edge-tts (Microsoft): para uso propio no hay problema; para dar o vender equipos, es el punto flojo. La foto de inicio tiene personas | chico (doc) |

## C. Mejoras chicas (para la 1.0.34 o después)

- **Botones del celu desde otra página (CSRF):** pedir una cabecera propia en `/control/accion`, `/prueba/empezar` y `/drive` (`app/servidor.cpp:124`).
- **`POST /drive` sin clave:** cualquiera en la wifi puede desconectar tu Drive (`servidor.cpp:97`).
- **Cada aviso del log escribe en la SD desde la pantalla:** con la SD lenta puede trabar 1–3 s (`reportes.cpp:140`). Juntar y escribir cada 2 s.
- **Actualización sin firma:** sumar un `.sha256` al release y compararlo (`actualizador.cpp:216`).
- **`workflow_dispatch` y los tags `v*` pueden publicar desde cualquier rama** (`programa.yml`).
- **Largada con lectura de GPS vieja** (`crono.cpp:280`) y **tiempo ideal 0 desde el celu** (`motor.cpp:313`).
- **`probar.sh` no abre 8 pantallas** (compartir, wifi, editor, drive, prueba, recorrido libre, tele, vertical): un error tipo 1.0.29 en una de esas pasaría.
- **«En construcción» llama a `Navegacion.volver()`, que no existe** (`EnConstruccion.qml:13`).
- **229 colores fijos fuera del tema** en 40 archivos (en sol o en otro tema quedan del Avispón).
- **Reiniciar en plena carrera:** hay que LARGAR de nuevo; la vuelta en curso se pierde. Las cerradas no.
- Menores: prueba de micrófono trabada si `aplay` no arranca, `.drive-curl` que queda al fallar, 405 que sale como 400, video .mkv en iPhone con un cartel que culpa al celu.

## Lo medido

- **Sanitizers (ASan + UBSan):** 11 de 11 pruebas pasan, sin errores de memoria.
- **25 pedidos web malformados** al servidor (`../etc/passwd`, rangos locos, 1 GB, 60 conexiones a medias): todos rechazados, sin errores. No se puede salir de la carpeta.
- **Avisos del compilador:** 6, todos inofensivos (`programador.cpp`).
- **Pantallas:** solo usan QtQuick, QtQuick.Window y QtMultimedia (lo que hay en la Pi). Ningún Timer corre escondido.
- **Motor:** reloj monotónico en todo; la hora del sistema no mueve el cronómetro; 5 o 10 lecturas por segundo dan lo mismo.

## Lo que está bien

Instalación atómica con vuelta atrás que anda si el programa se cae; un archivo por carrera con escritura
segura y papelera; copia completa a Drive; vigilante doble; Prueba completa de 12 pasos; pruebas que frenan
la publicación; secretos en 0600 y nunca en los reportes; ssh cerrado en la red propia; sin textos de prueba.

## Veredicto

- **Para que lo uses vos en carrera: sí, después de los puntos 1 y 2** (el pulsador y la vuelta atrás).
  Con 3 a 6 en la misma versión, es una tarde.
- **Para entregarlo a otro navegante: todavía no.** Faltan el manual (7) y la guía de SD nueva (8). Si se
  da o se vende, resolver además las voces (9).

## Las 3 que haría primero

1. El pulsador que pierde marcas (1).
2. La vuelta atrás cuando la versión se cuelga (2) y el `set -E` (3).
3. El manual del navegante (7).

## Preguntas para Matías

1. **¿Arreglo la parte A ahora** (puntos 1 a 6) y publico la 1.0.34?
2. **¿Querés «Seguir carrera»** al prender si se reinicia en plena carrera? (solo con hora del GPS o internet)
3. **¿El manual** lo hago como página en la Pi (`/ayuda`, se ve desde el celu), como PDF, o los dos?
4. **¿Va a otros navegantes?** Si es así, hay que ver las voces y la foto de inicio.
5. **El token de los reportes:** fijate en https://github.com/settings/tokens que sea *fine-grained*, solo para
   `Betometro-reportes` y con permiso Issues. Si es «clásico» con `repo`, quien tenga la SD puede tocar todos
   tus repos.
