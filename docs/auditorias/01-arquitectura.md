# Auditoría 01 — Arquitectura

Hecha el 07/11/2026 sobre la versión 1.0.26. Solo lectura: no se tocó código.

## Cómo está repartido hoy

| Parte | Líneas | Archivos |
|---|---|---|
| Núcleo (motor, modelo, informe, voz) | 2.462 | `app/nucleo/` |
| Cámara y video (C++ + ayudante GStreamer) | 1.990 | `app/camara*`, `app/video_gst*`, `app/imagen_video*` |
| Drive | 1.047 | `app/drive*` |
| Datos | 790 | `app/datos.*` |
| Servidor web + control remoto | 688 | `app/servidor.*`, `app/control_remoto.*` |
| Crono | 420 | `app/crono.*` |
| Pantallas QML | 8.405 | `pantallas/` 2.938 · `tablet/` 1.522 · `componentes/` 1.476 · `video/` 1.251 · `widgets/` 828 |
| Plantilla del informe (HTML+JS) | 426 | `recursos/informe-plantilla.html` |

**Lo que está bien y hay que cuidar:** el núcleo no depende de Qt ni del hardware y se compara con la app (`ctest`). El informe se arma en un solo lugar (`app/nucleo/informe.cpp`) y de ahí comen la pantalla y el HTML. La cámara corre en un proceso aparte (`betometro-camara`). El guardado va en otro hilo (`app/datos.cpp:86`). `main.cpp` cablea todo y nadie más se conoce de más: los `.cpp` solo incluyen `datos.h`, `camara.h` y `gps.h` entre sí.

---

## 1. Lo más grave

### 1.1 Diez pantallas muertas que se siguen editando
- **Qué encontré:** `app/qml/Main.qml:33-36` manda siempre a `tablet/` (salvo el inicio). Los `case` de `Main.qml:42-56` para `pantallas/Practica, Pronostico, AjustesCamara, AjustesDrive, Ajustes, Carreras, Ficha, Chequeo, Conectividad, Actualizar` no se alcanzan nunca. Son **1.665 líneas** que igual se compilan (`app/qml/CMakeLists.txt`). Nadie más las usa (`grep` en todo el repo).
- **Riesgo:** trabajo doble y arreglos en la pantalla que no se ve. Ya pasó: el commit `6436487` (1.0.24, sin 1080p) tocó `AjustesCamara.qml` y `TCamara.qml`. Y ya están desfasadas: `Ficha.qml` y `Practica.qml` no saben del «Recorrido libre»; `TFicha` y `TPractica` sí.
- **Propongo:** borrar las 10, sacar esos `case` y las líneas del CMake. `Inicio.qml` queda (vive con Modo Tablet apagado). Después, sacar la «T» de los nombres (`TFicha.qml` → `Ficha.qml`) para que no parezcan una variante.
- **Cuesta:** chico. Se verifica con las capturas de `herramientas/capturas.sh` antes y después.

### 1.2 `Datos` hace cinco cosas
- **Qué encontré:** `app/datos.h` junta: base de carreras; configuración (`cfg`/`ponerCfg`, `datos.cpp:542-576`); archivos de video en la tarjeta (`videosGuardados`, `borrarVideo`, `renombrarVideo`, `datos.cpp:414-483`); el HTML del informe (`datos.cpp:303-322`); y las carreras de ejemplo (`cargarMuestra`, `datos.cpp:578-674`, 97 líneas de datos de prueba adentro del programa). Además `resumen` (`datos.cpp:115-215`) arma textos de pantalla («ok/medio/mal», fechas largas, duraciones).
- **Riesgo:** una sola señal `cfgChanged` para todos los ajustes. Cada vez que se mueve cualquier ajuste se vuelven a correr brillo, wifi, servidor, audio y tema (`main.cpp:156-229`) y se recalculan los enlaces a `Datos.ajustes` en 22 archivos QML. Un cambio de ajuste tiene el mismo costo que cambiar el tema.
- **Propongo, de a pasos:** (1) mover `cargarMuestra` a `app/muestra.cpp` (sigue en el programa: lo usan las capturas); (2) lo de archivos de video a `app/videos.cpp` (Cámara ya lo envuelve, `camara.cpp:321-366`); (3) `cfgChanged(clave)` con la clave que cambió, y que `main.cpp` filtre. Las pruebas `prueba_datos`, `prueba_videos` y `prueba_crono` cubren los pasos 1 y 2.
- **Cuesta:** medio (paso 1 y 2 chicos; paso 3 medio).

### 1.3 Tres páginas web escritas adentro de strings de C++
- **Qué encontré:** `app/servidor.cpp:261-517` tiene la página principal, la de Drive y la del control de la tele (HTML, CSS y JS) como texto C++; ~255 líneas. El informe, en cambio, ya está en un archivo aparte (`recursos/informe-plantilla.html`) y se carga de los recursos.
- **Riesgo:** se editan a ciegas (sin resaltado, sin abrirlas en un navegador), y cualquier retoque visual recompila el programa.
- **Propongo:** llevarlas a `recursos/web/index.html`, `drive.html` y `control.html`, cargarlas con `QFile(":/recursos/web/…")` como la plantilla, y dejar en `servidor.cpp` solo rutas y respuestas.
- **Cuesta:** chico.

### 1.4 `Informe.qml` son seis pantallas en una
- **Qué encontré:** 668 líneas: gráfico de la vuelta (Canvas, `Informe.qml:211-290`), mapa (`298-341`), tabla de vueltas con unir/medida (`342-474`), marcas del recorrido (`475-530`), video y tele (`38-64`, `569-592`), menú y temporizadores de captura (`593-640`).
- **Riesgo:** cualquier cambio del informe pasa por el archivo más grande del proyecto. Y la tele depende de él: `Tele.qml:35` guarda un puntero `property Item reproductor` a un ítem de esta pantalla; si alguien olvida el `Component.onDestruction: Tele.terminar()` (`Informe.qml:47`), la tele queda apuntando a algo borrado.
- **Propongo:** carpeta `app/qml/informe/` con `GraficoVuelta.qml`, `Mapa.qml`, `TablaVueltas.qml`, `Marcas.qml`. `Informe.qml` queda como armado. Cada pieza recibe `inf`, `vueltas`, `lap` por propiedades, sin leer `Datos` directo.
- **Cuesta:** medio. Capturas antes y después (`docs/muestras/`).

---

## 2. Lo que más mejora con poco trabajo

### 2.1 Formatos repetidos aunque existe `Formato.qml`
- `reloj` y `coma` se reescriben en `Informe.qml:16-20`, `Reproductor.qml:210`, `TableroRecorrido.qml:12-16` y `mmss` en `ListaVideos.qml:199`. Si cambia un formato, hay que acordarse de cuatro lugares. Pasarlos a `Formato.qml`. **Chico.**

### 2.2 Las pruebas recompilan el programa de a pedazos
- `CMakeLists.txt:97-115`: `prueba_datos`, `prueba_crono` y `prueba_videos` compilan de nuevo `datos.cpp`, `gps.cpp`, `camara.cpp` cada una. Armar una biblioteca `betometro_base` (datos, crono, gps, camara, drive_logica) y enlazarla del programa y de las pruebas. Compila menos y las pruebas prueban lo mismo que corre. **Chico.**

### 2.3 La plantilla del informe es una copia de la app que creció sola
- `recursos/informe-plantilla.html` (426 líneas) salió de `referencia/app-2.1.1/informe-plantilla.html` (148). El script tiene 186 líneas distintas; el video (`319-426`) es solo de la Raspberry. `ctest informe_igual_a_la_app` compara los datos, no el HTML. Si la app cambia su informe, hay que fusionar a mano sin marcas. Marcar los bloques propios (`<!-- RASPBERRY -->`) y dejar el JS del video en `recursos/informe-video.js`, que `herramientas/armar-informe.py` ya podría pegar. **Chico.**

### 2.4 `Tele.qml` hace trabajo de datos
- `Tele.qml:53-63`: para listar las carreras con video arma el informe completo (`Datos.informe(id)`, con recorrido y trazas) de cada una solo para contar vueltas. Agregar `vueltas` al `resumen` de `Datos` (ya devuelve 30 claves) y usar eso. **Chico.**

---

## 3. El resto

### 3.1 Objetos globales como «context properties»
- `main.cpp:237-251` registra 15 objetos (`Datos`, `Crono`, `Wifi`…) como nombres globales. Los singletons `Tema.qml:15` y `Tele.qml` dependen de ellos sin declararlo; `qmllint` y Qt Creator no los ven. Qt 6 prefiere `qmlRegisterSingletonInstance`. Cambiarlo es mecánico pero toca 29 archivos. **Medio**; conviene hacerlo cuando ya no estén las pantallas muertas.

### 3.2 `Crono.estado` es un mapa de 21 claves que se rearma 20 veces por segundo
- `crono.cpp:276-299`. Todo widget atado a `Crono.estado.x` se recalcula entero en cada tic. Para el tamaño actual anda; si la auditoría 04 mide lentitud en carrera, el camino es una `Q_PROPERTY` por dato. **Medio.** No tocar antes de medir.

### 3.3 El plan y la carpeta real no coinciden
- `docs/plan-ingenieria.md:72-90` dice `app/hw/` con «versión real y simulada de cada pieza» y hilos para GPS y voz. Hoy todo está plano en `app/`, solo el GPS tiene simulado (`GpsSimulado`); pulsador, salud, hdmi y sistema se apagan sin `BETO_RASPBERRY`; crono y voz van por timers de 50 y 15 ms en el hilo principal (`crono.cpp:31-38`). No mover carpetas ahora (rompe CMake y el historial); **actualizar el plan** para que diga lo que hay. **Chico.**

### 3.4 Video en QML: tamaño justo, pero con componentes propios
- `ControlTele.qml` (374) define sus propias `Ficha`, `Texto`, `Etiq` (`ControlTele.qml:27-62`) en vez de usar `componentes/`. `VistaVideo.qml` (422) mezcla imantado de los datos y dibujo. Está en límite; no hace falta tocarlo hasta que crezca.

---

## Las 3 que haría primero
1. **Borrar las 10 pantallas muertas** (1.1). Es el riesgo más concreto: ya se editaron por error.
2. **Sacar las páginas web de `servidor.cpp` a `recursos/web/`** (1.3) y marcar lo propio en la plantilla del informe (2.3). Mismo tipo de cambio, mismo día.
3. **Partir `Datos`** (1.2) en los dos pasos chicos (muestra y archivos de video) y armar la biblioteca `betometro_base` (2.2) en el mismo commit, porque toca el mismo CMake.

Después, con calma y con capturas: partir `Informe.qml` (1.4).

## Preguntas para Matías
1. Las 10 pantallas viejas (`pantallas/Ficha.qml`, `Practica.qml`, etc.) no se ven nunca desde la 1.0.16. ¿Las borro? Queda el inicio chico para cuando Modo Tablet está apagado.
2. Si se borran, ¿les saco la «T» a las grandes (`TFicha.qml` → `Ficha.qml`)? Es solo el nombre del archivo; no cambia nada en pantalla.
3. El plan (`docs/plan-ingenieria.md`) describe una estructura con `app/hw/` e hilos que no es la que quedó. ¿Lo actualizo para que diga lo que hay, o preferís que quede como meta?
