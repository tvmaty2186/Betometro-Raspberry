# Betómetro Raspberry — Plan de ingeniería

Versión del plan: 1 (01/11/2026). Estado: **aprobado por Matías (01/11/2026).** E1 hecha; sigue E2.

Este documento ordena el proyecto en etapas. Para cada etapa dice qué se hace, cómo se hace, qué se entrega y cómo se prueba que está bien.

**Documentos que manda este plan** (no se repiten acá):
- Qué es y qué se decidió: [diseno-sistema.md](diseno-sistema.md)
- Qué hace la app, con números exactos: [app/lo-aprendido.md](app/lo-aprendido.md)
- Estética: [app/diseno.md](app/diseno.md) y las muestras aprobadas
- Piezas: [hardware.md](hardware.md)

**Reglas:**
- La app Android no se toca. Este repo es solo la Raspberry.
- Lo decidido en la app vale acá, salvo lo que cambia `diseno-sistema.md`.
- Cada etapa termina con su **criterio de aceptación** cumplido y con este documento actualizado.

---

## 1. Resumen de etapas

| Etapa | Qué | Necesita la Raspberry | Entrega |
|---|---|---|---|
| **E1** | Motor y datos (C++) | No | Biblioteca del motor + pruebas que dan igual que la app |
| **E2** | Programa completo en PC | No | Programa Qt con las 12 pantallas, 4 temas, 2 orientaciones, simulador, voz, informe |
| **E3** | Sistema operativo | Sí (pantalla) | Imagen `.img` que arranca directo en Betómetro |
| **E4** | Equipo real | Sí (GPS, pulsador, auriculares) | GPS por cables, pulsador, voz por auriculares, hora del GPS |
| **E5** | Wifi, compartir y actualizar | Sí (wifi) | Wifi, informe por QR, actualización A/B |
| **E6** | Prueba en pista y 1.0.0 | Sí, en el auto | Versión 1.0.0 publicada |

E1 y E2 se hacen sin hardware. Mientras tanto Matías junta las piezas que faltan.

---

## 2. Arquitectura

### Capas

```
 Pantalla táctil 7"  ·  GPS NEO-M8N (UART)  ·  Pulsador (GPIO)  ·  Auriculares (3,5 mm)
 ───────────────────────────────────────────────────────────────────────────────
 HW: lectores y salidas      gps/  pulsador/  audio/  brillo/  wifi/  sistema/
 ───────────────────────────────────────────────────────────────────────────────
 NÚCLEO (C++ puro, sin Qt de pantalla)   motor · modelo de datos · almacén · voz (programador) · informe
 ───────────────────────────────────────────────────────────────────────────────
 PANTALLA (QML)                          12 pantallas · widgets · editor · 4 temas · vertical/horizontal
```

- **Un solo programa** (`betometro`), en C++ 17 con Qt 6 (Quick/QML).
- El **núcleo no depende del hardware ni de la pantalla.** Así se prueba en PC y se compara con la app.
- Cada pieza de hardware tiene una **versión real** y una **versión simulada** con la misma interfaz. En PC se usan las simuladas.

### Hilos

| Hilo | Qué hace |
|---|---|
| Principal (Qt) | Pantalla y tacto |
| GPS | Lee el UART, arma las muestras y se las pasa al motor |
| Pulsador | Espera el flanco del GPIO con `libgpiod` (sin consultar en bucle) |
| Audio | Reproduce los sonidos ya cargados en memoria (ALSA, baja demora) |
| Programador de voz | Cada 15 ms mira el estado del motor y decide qué decir (como la app) |

### Tiempo

- Todo cálculo usa **reloj monotónico** (`CLOCK_MONOTONIC`), como `performance.now()` en la app.
- La **hora del día** (fechas de carreras, nombre del informe) sale del GPS. Sin hora del GPS se puede largar igual.
- Cada marca y cada muestra del GPS lleva su hora monotónica del momento en que llegó.

---

## 3. Estructura del repo (a medida que se crea)

```
app/
  nucleo/        motor, modelo, almacén, programador de voz, informe (C++ sin pantalla)
  hw/            gps, pulsador, audio, brillo, wifi, sistema (real + simulado)
  qml/           pantallas, widgets, temas
  main.cpp
pruebas/
  motor/         casos de prueba y comparación con el Motor de la app
  datos/         recorridos de ejemplo
simulador/       generador de datos del NEO-M8N (UBX y NMEA)
voz/             lista de frases y armado de los audios con Piper
sistema/         Buildroot: placa, configuración, overlays, RAUC, servicios
docs/            documentación (este plan, diseño, muestras, app/)
recursos/        fuentes, imágenes, ícono
referencia/      copia del código de la app 2.1.1 (no se edita)
.github/workflows/
```

---

## 4. Etapas en detalle

### E1 — Motor y modelo de datos

**Objetivo:** el cálculo de la app, en C++, dando exactamente los mismos resultados.

**Especificación:**
- Pasar `Motor` de la app (sección 2 de `lo-aprendido.md`) a C++: muestras, descarte por precisión, integración de distancia, vueltas medidas/práctica, antirrebote, autocalibración del GPS, congelado de la sugerida, `estado(t)` con `restanteMs`, `restanteM`, `vNecKmh`, `deltaS`.
- Agregar lo propio de la Raspberry: **vuelta interrumpida** al terminar a mano y **modo solo cronómetro** (sin GPS).
- Modelo de datos: carrera, vuelta, configuración, con los mismos campos y valores de fábrica que la app (`DEF`), más los nuevos (orientación, brillo, volumen = 100 %, wifi).
- Almacén: archivo JSON en la partición de datos. Escritura segura: se escribe un archivo nuevo, se fuerza a disco (`fsync`) y se reemplaza (`rename`). Se guarda una copia anterior.

**Pruebas:**
- Se extrae el `Motor` de `referencia/app-2.1.1/index.html` y se corre con Node.
- Los mismos recorridos (simulados y de borde: huecos de GPS, mala precisión, marcas dobles) se pasan por los dos motores.
- Se comparan todos los eventos y `estado(t)` cada 100 ms.

**Aceptación:** diferencias menores a 1 ms en tiempos y a 1 cm en distancias en todos los casos. Pruebas corriendo solas en GitHub Actions en cada cambio.

**Entrega:** `app/nucleo/` + `pruebas/motor/` + informe de la comparación.

**Estado: hecha (01/11/2026).** Diferencia cero contra la app en 65 casos. Ver [pruebas/motor/INFORME.md](../pruebas/motor/INFORME.md).
- El JSON es propio (`app/nucleo/json.*`): el núcleo no depende de nada, ni de Qt.
- La vuelta interrumpida no se guarda si dura menos que el antirrebote. Su `fuente` es `"terminar"`.
- Orientación de fábrica: horizontal. Brillo mínimo: 10 %. Las redes wifi las guarda `wpa_supplicant` (E5), no la base.
- Falta para E2: el programador de voz (`programador`) y el informe (`armarRecorrido`) también van al núcleo.

### E2 — Programa completo en PC

**Objetivo:** todo Betómetro funcionando en una PC, con datos simulados.

**Especificación:**
- **Pantallas:** las 12 aprobadas, en vertical (720×1280) y horizontal (1280×720). Navegación y «volver» como la app.
- **Temas:** los 4 de la app, como conjuntos de variables (colores, fuentes, radios, estilo de menús). Rally clásico con sus menús tipo carnet. Modo sol encima de cualquier tema.
- **Widgets:** todos los tipos de la app, con sus estilos y opciones (✎). Posiciones guardadas como fracción del tablero, un armado por orientación. Editor: mantener apretado para mover, tocar para cambiar tamaño, ✎ para opciones, agregar, sacar, volver a fábrica. Armados de fábrica: horizontal igual a la app; vertical reacomodado según la muestra aprobada.
- **Orientación:** se elige en el inicio o en Ajustes. Se gira la pantalla desde el programa (se gira el elemento raíz de QML y el tacto con él).
- **Simulador del NEO-M8N:** genera los mensajes que manda el módulo (UBX `NAV-PVT` y frases NMEA) con un recorrido de vueltas y los entrega por una terminal virtual (`pty`). El programa los lee igual que del UART real. Solo existe en desarrollo.
- **Voz:** las frases de la app (números de la cuenta, «cero», «Vuelta N, medida/práctica», clic, pitido) se graban con Piper (voz en español de Argentina) al compilar. Se cargan en memoria al arrancar. Anticipo configurable (valor inicial 40 ms, se ajusta en E4).
- **Informe:** pantalla propia (recorrido coloreado, velocidad, aceleración, tabla con la vuelta interrumpida) con el mismo cálculo que la app (`armarRecorrido`). Exportación a HTML con la plantilla de la app (`informe-plantilla.html`), mismo nombre de archivo.
- **Teclado en pantalla** con ñ, para el wifi y los nombres.
- **Conectividad** (pantalla 13) y la página **Betómetro en la red** con datos de prueba. Ver [propuesta-mejoras.md](propuesta-mejoras.md).

**Pruebas:**
- Capturas automáticas de cada pantalla, en las 2 orientaciones y los 4 temas, comparadas a ojo con las muestras.
- Carrera completa simulada de punta a punta: largar, vueltas, marcas, terminar, informe, exportar.

**Avance (01/11/2026):** base del programa (Qt 6.8, QML), tema Avispón, navegación, giro para la pantalla real y la pantalla de **Inicio** en las dos orientaciones (95 % de los píxeles iguales a la muestra).
- **Carreras, Ficha y Práctica libre** (01/11/2026) con datos reales (núcleo + almacén), teclado en pantalla (letras con ñ, números, tiempo m:ss,d y fecha), borrar con doble toque o manteniendo apretado. Prueba `prueba_datos` (30 chequeos).
  - La ficha une el editor y el detalle de la app, como la muestra. En una carrera corrida el recuadro muestra Error total, Promedio y Mejor.
  - LARGAR solo el día de la carrera; **sin hora del GPS se deja largar** (decisión de la Raspberry). Si no se puede, el botón dice por qué.
  - La práctica **sí se guarda** en Corridas al largar (como la app); se cambió el texto de la muestra que decía lo contrario.
- **Pantalla de carrera** (01/11/2026): motor de E1 conectado (`app/crono.*`), armados de fábrica de las muestras (horizontal y vertical), widgets cuenta, tipo, sugerida, velocidad digital y analógica, última vuelta, MARCAR (al apoyar el dedo, con efecto) y los demás tipos. Teclado USB (Enter, espacio, F9). Terminar con doble toque guarda la vuelta interrumpida. Carrera completa termina sola. Sin GPS anda como cronómetro. Prueba `prueba_crono` (29 chequeos).
  - GPS simulado interno a 10 Hz para la PC (`app/gps.*`). El simulador de los mensajes del NEO-M8N por terminal virtual va en la parte del GPS.
  - Indicador de vuelta como la app (fábrica «fondo»: toda la pantalla roja en medida y verde en práctica). Las muestras no lo mostraban; **Matías eligió «como la app»** (01/11/2026).
  - Falta: editor de pantalla (mover y cambiar widgets), voz y modo sol. Capturas en [capturas/](capturas/), se sacan con `herramientas/capturas.sh`.
- Dibujo con `Canvas` (se pinta una vez y queda): no depende de efectos de la placa de video y sale igual en captura que en la Raspberry.
- El sentido del giro (90° o −90°) depende de cómo se monte la pantalla: se ajusta en E3.

**Aceptación:** Matías aprueba las capturas (incluidos los 3 temas que todavía no vio). La carrera simulada da los mismos tiempos que el motor de E1.

**Entrega:** programa que corre en PC + capturas.

### E3 — Sistema operativo (Buildroot)

**Objetivo:** prender la Raspberry y que arranque directo en Betómetro.

**Especificación:**
- **Buildroot** (versión estable de largo plazo), placa `raspberrypi3_64` o 32 bits según rinda mejor con Qt.
- **Pantalla:** driver `vc4-kms-v3d` y el overlay de la Touch Display 2. Qt con EGLFS (dibuja directo, sin escritorio). Gráficos por OpenGL ES 2 de la placa.
- **Arranque:** `systemd` lanza `betometro` y lo reinicia si se cae. **Watchdog** del procesador activo: si el sistema se cuelga, se reinicia solo.
- **Particiones de la tarjeta:**

| Partición | Qué tiene | Modo |
|---|---|---|
| boot | firmware y U-Boot | solo lectura |
| sistema A | sistema completo | solo lectura (squashfs) |
| sistema B | copia para actualizar | solo lectura (squashfs) |
| datos | carreras, ajustes, wifi | lectura y escritura (ext4 con diario) |

- **Pantalla de arranque** con el león y el título, en vez de los mensajes de Linux.
- **Apagar el equipo** desde Ajustes: apagado ordenado.
- **Compilación:** GitHub Actions arma la imagen. Se guardan las descargas y la caché entre compilaciones para que tarde menos.

**Aceptación:**
- Arranca en Betómetro en **15 s o menos**.
- Se le corta la corriente 20 veces seguidas en medio del uso y vuelve a arrancar siempre, sin perder carreras guardadas.
- La pantalla de carrera se mueve fluida (30 cuadros por segundo o más).

**Entrega:** `betometro-raspberry-X.Y.Z.img.xz` en la *release* + guía para grabarla con Raspberry Pi Imager.

### E4 — Equipo real

**Objetivo:** GPS, pulsador y voz de verdad.

**Especificación:**
- **GPS por UART** (cableado en [cableado-gps.png](cableado-gps.png)): `enable_uart=1`, `dtoverlay=disable-bt`, puerto `/dev/ttyAMA0`.
- **Configuración del NEO-M8N al arrancar** (mensajes UBX): 115200 baudios, **solo GPS a 10 Hz**, modelo dinámico «automotor», salida `NAV-PVT`. Se guarda en el módulo.
- **Mejoras G1 a G7** de [propuesta-mejoras.md](propuesta-mejoras.md): GPS + GLONASS a 10 Hz, hora de cada lectura según el GPS (`iTOW`) atada al PPS, error de velocidad (`sAcc`), «sin GPS» a 1 s, solo posición 3D buena, arranque en caliente. G4 y G5 cambian el motor: van aparte, con su prueba, y en Ajustes se puede volver a lo de la app.
- **Datos que se usan de `NAV-PVT`:** velocidad sobre el suelo (`gSpeed`, Doppler), posición, **precisión horizontal (`hAcc`) como «precisión»** contra el umbral de 20 m, hora UTC válida, cantidad de satélites. Así se resuelve la duda de la precisión.
- **PPS** (opcional): para medir la demora del GPS.
- **Hora:** cuando el GPS da hora válida, se pone en el reloj del sistema (hora de Argentina).
- **Pulsador** en GPIO17 (pin 11) contra GND (pin 9), con la resistencia interna hacia arriba. Filtro eléctrico de 20 ms + el antirrebote de 3 s de la app.
- **Audio** por la salida de 3,5 mm (`dtparam=audio=on`, `audio_pwm_mode=2` para menos ruido). Volumen de fábrica al máximo; se usa el último elegido.
- **Brillo** de la pantalla desde Ajustes.

**Pruebas:**
- GPS: con antena en el techo, 10 lecturas por segundo estables, tiempo hasta tener señal.
- Pulsador: 500 apretadas, ninguna perdida ni doble.
- Voz: demora entre la hora prevista y el sonido, medida con grabación. Se ajusta el anticipo.

**Aceptación:** demora de la voz menor a 50 ms; el pulsador nunca falla; el GPS da 10 Hz en el auto andando.

### E5 — Wifi, compartir y actualizar

**Especificación:**
- **Wifi:** buscar redes, conectar con clave, recordar y olvidar redes (`wpa_supplicant`). La clave se guarda en la partición de datos. Hora por internet (NTP) cuando hay conexión.
- **Betómetro en la red:** página en `http://betometro.local` (mDNS) con todas las carreras, cada informe y «Descargar todos» (.zip). Solo lectura. Se apaga durante la carrera.
- **Wifi propia:** si no encuentra red guardada, arma «Betometro-<número del auto>» para conectar el celular directo.
- **Compartir informe por QR:** un servidor web chico dentro del programa, solo mientras está abierta la pantalla de compartir. El QR lleva a `http://<ip>:<puerto>/<código>` y descarga el HTML. Se cierra al salir de la pantalla.
- **Actualizar:** consulta la última *release* de **este repo**. Si hay versión nueva aparece «· Actualizar» en amarillo. Al tocarlo, descarga el paquete RAUC, verifica la firma, lo instala en la copia que no está en uso y reinicia. Si la nueva no arranca bien 3 veces, vuelve sola a la anterior.
- **Firma de las actualizaciones:** clave privada en los secretos de GitHub (la carga Matías; nunca se escribe en el repo). La pública va dentro del sistema.

**Aceptación:** actualización completa por wifi del celular; una actualización rota a propósito vuelve sola a la versión anterior; el informe se baja al celular escaneando el QR.

### E6 — Prueba en pista y versión 1.0.0

- Una práctica libre y una carrera completas en el Avispón Verde.
- Comparar los tiempos con la app del celular corriendo al mismo tiempo.
- Ajustar lo que haga falta y publicar la **1.0.0**.

**Aceptación:** Matías da el OK después de correr.

---

## 5. Especificaciones generales

| Tema | Especificación |
|---|---|
| Lenguajes | C++ 17, QML (Qt 6) |
| Versiones | Propias de este repo, empiezan en 1.0.0. Mismo criterio que la app: grande / función nueva / arreglo |
| Commits | Autor Matías Russo, mensajes en español |
| Publicación | GitHub Actions: pruebas en cada cambio; imagen y paquete de actualización al crear una versión |
| Datos | JSON en la partición de datos, escritura segura, copia anterior |
| Secretos | Solo en GitHub, los carga Matías. Nunca en el repo |
| Idioma de la pantalla | Español rioplatense, como la app |

## 6. Riesgos y qué hacemos

| Riesgo | Qué hacemos |
|---|---|
| Qt 6 lento en la Raspberry 3 (placa de video vieja) | Medir en E3 al principio. Si no da, bajar efectos (sombras, degradés) o probar 32 bits |
| Ruido en la salida de 3,5 mm | `audio_pwm_mode=2`. Si igual molesta, una placa de sonido USB chica |
| Compilación de Buildroot muy larga en GitHub | Guardar caché entre compilaciones |
| Touch Display 2 con la Raspberry 3 | Probar al empezar E3 |
| Cortes de corriente | Sistema de solo lectura + escritura segura + prueba de 20 cortes |

## 7. Qué necesita Matías y cuándo

| Etapa | Qué |
|---|---|
| E2 | Aprobar capturas (incluye los 3 temas que faltan ver) |
| E3 | Raspberry, pantalla, tarjeta y fuente. Grabar la imagen y probar |
| E4 | GPS cableado, pulsador, auriculares. Antena en el techo del auto |
| E5 | Cargar en GitHub el secreto de firma de actualizaciones (se le explica cómo) |
| E6 | Ir a la pista |
