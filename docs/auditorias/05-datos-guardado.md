# Auditoría 05 — Datos y guardado

07/11/2026, sobre la 1.0.26. Solo lectura: no se cambió código.

**Resumen en 4 líneas.** Hoy no se pierde una vuelta ya marcada: el guardado es seguro contra cortes de luz
(probado). Lo grave es otra cosa: **un recorrido libre guarda la traza GPS recién al terminar**; si se corta la
luz a las 3 horas, se pierde todo salvo las marcas. Y la base entera se reescribe en cada vuelta: con la
base de un par de temporadas tarda **segundos** en la Pi y come **cientos de MB de RAM**. El desgaste de la
tarjeta no es problema (el video escribe 50 veces más).

**Medido** = corrido en esta sesión (PC Xeon 2,1 GHz y el programa real en Docker). **Estimado** = calculado;
la Pi 3 es unas 4 a 6 veces más lenta que esta PC (no se midió en la Pi).

---

## 1. Lo más grave

### 1.1 El recorrido libre pierde la traza si se corta la luz
- **Qué encontré:** la traza (`traza_`) vive en memoria y va a la base solo en `Evento::Cerrada/Interrumpida`
  (`app/crono.cpp:233-235`). En un recorrido eso es **solo al tocar Terminar**. Las marcas sí se guardan al
  instante (`app/crono.cpp:181`, `ponerExtra`), pero la traza no.
- **Riesgo:** corte de corriente, reinicio del vigilante (30 s congelado) o caída del programa a las 3 h de un
  recorrido → quedan las marcas sin mapa, sin velocidad, sin resumen. Es justo el caso del auto.
  En una carrera normal pasa lo mismo con la vuelta en curso, pero ahí se pierde como mucho una vuelta
  (decisión de Matías del 05/11: la carrera en curso se corta).
- **Qué propongo:** en un recorrido, guardar la vuelta abierta (con la traza que va) cada 30 s o cada 30 filas,
  marcada como «en curso». Al terminar se reemplaza por la definitiva. Al arrancar, una vuelta «en curso» se
  muestra como recorrido cortado. Hasta que no esté el punto 4, hacerlo cada 60 s (hoy cada guardado reescribe
  toda la base).
- **Cuánto cuesta:** chico (con el formato de hoy) · se vuelve gratis con el punto 4.

### 1.2 Si no se puede guardar, nadie se entera
- **Qué encontré:** si falla la escritura, `hiloGuardar` emite `Datos::error` (`app/datos.cpp:101`) y nada lo
  escucha: no hay `connect` ni `onError` en todo `app/` (grep). Tampoco hace `qWarning`, así que no va al
  reporte de errores. Probado con disco lleno simulado: `guardar` devuelve `false`, el archivo viejo queda sano
  (bien), pero el programa sigue mudo.
- **Riesgo:** tarjeta llena (los videos ocupan 2 GB por hora; la cámara hace lugar solo cuando graba,
  `app/camara.cpp:26,308` (solo al empezar a grabar, `camara.cpp:274`)) o tarjeta fallada → cada vuelta parece guardada y al apagar no está ninguna.
- **Qué propongo:** `qWarning` (va al reporte) + cartel en la carrera «NO SE GUARDA», chico y rojo, que no se
  vaya. Y antes de largar, si hay menos de 200 MB libres, avisar.
- **Cuánto cuesta:** chico.

### 1.3 La base entera se reescribe en cada vuelta y en cada marca: tiempo y memoria crecen con la historia
- **Qué encontré:** `Datos::guardar` copia toda la base y el hilo la escribe completa (`app/datos.cpp:72-104`,
  `app/nucleo/almacen.cpp:84-101`). Medido con una base de 20 carreras de 1 h (5 Hz) + 5 recorridos llenos
  = **18,3 MB**:

  | Qué | Medido en la PC | Estimado en la Pi 3 |
  |---|---|---|
  | Armar el JSON y escribirlo (`aJson` + `escribir`) | 3,4 s | 15 a 20 s |
  | De eso, pasar números a texto (`numeroComoJs`) | 3,3 s | casi todo |
  | Grabar + fsync + renombrar | 0,2 s | 1 a 2 s |
  | Leer al arrancar (`cargar`) | 0,63 s (el programa real: +0,65 s de arranque) | 3 a 4 s |
  | **RAM pico** al leer / al guardar | **321 MB / 366 MB** | igual (misma arquitectura de 64 bits) |

  Con una base chica (3 carreras, 2,1 MB): 0,4 s y 52 MB. La RAM se va en el árbol `Json` intermedio: cada
  fila de traza (`[t,d,kmh,lat,lon,prec]`) ocupa unos 500 bytes como árbol contra 48 bytes en la `Base`.
- **Riesgo:** la Pi 3 tiene 1 GB. Con una base de 30 MB (estimado: dos temporadas con recorridos), guardar
  pide ~600 MB extra con el programa, la pantalla y la cámara andando → se queda sin memoria y el vigilante lo
  reinicia **en plena carrera**. Antes de eso: vueltas de 17 s con guardados de 20 s (se escribe solo la última,
  bien, pero la vuelta queda hasta 20 s sin estar en la tarjeta).
- **Qué propongo:** puntos 2.1 (ya) y 4 (formato).
- **Cuánto cuesta:** ver cada uno.

---

## 2. Lo que más mejora con poco trabajo

### 2.1 `numeroComoJs` es 80 veces más lento de lo necesario
- **Qué encontré:** prueba hasta 17 veces `snprintf("%.*e")` + `strtod` por número (`app/nucleo/json.cpp:92-102`).
  Medido: 4,4 µs por coordenada, 0,9 µs por número de 1 decimal. `std::to_chars(x, chars_format::fixed)` da
  **exactamente el mismo texto** (probado con 2,2 millones de números: 0 diferencias dentro de 1e-6…1e21) en
  0,05 µs.
- **Riesgo de no hacerlo:** es el 90 % del tiempo de guardar (1.3).
- **Qué propongo:** `to_chars` fijo; para `|x| < 1e-6` o `≥ 1e21` dejar el camino de hoy (no aparecen en la base).
  Guardar la base de 18 MB pasaría de 3,4 s a ~0,4 s en la PC (estimado), y en la Pi de ~18 s a ~2 s.
  Agregar la prueba de igualdad a `pruebas/nucleo`.
- **Cuánto cuesta:** chico (10 líneas + prueba).

### 2.2 Escribir el texto directo desde la `Base`, sin el árbol `Json`
- **Qué encontré:** `Base::aJson()` arma el árbol completo (200 MB con 18 MB de base) solo para `escribir()`.
- **Qué propongo:** una función `escribirEn(std::string&)` en `Vuelta`/`Carrera` que escriba la traza
  directo (la parte que pesa). El resto puede seguir por `Json`. Saca el pico de RAM al guardar.
- **Cuánto cuesta:** medio · innecesario si se hace el punto 4.

---

## 3. El resto (probado: el guardado seguro anda)

- **Corte a mitad del guardado — bien.** `.nuevo` + `fsync` + rename + `fsync` de la carpeta
  (`app/nucleo/almacen.cpp:31-60`). Probado cortando `.nuevo` a la mitad: lee `betometro.json`, el pedazo queda en
  `.nuevo.danado`. Dos guardados a la vez no pueden pasar (un solo hilo; `base_` solo se toca desde el hilo
  principal). Al apagar, `~Datos` espera que termine de escribir (`app/datos.cpp:63-70`) y `main.cpp:74` cierra
  prolijo con SIGTERM.
- **Detalle:** si el corte pasa en el momento normal (escribiendo `.nuevo`), al prender dice «La base estaba
  dañada» (`app/datos.cpp:59`) y queda un `.nuevo.danado` de varios MB para siempre. No es un daño: es lo
  esperado. Propongo no avisar si lo que está roto es `.nuevo` y borrar el `.danado` cuando `betometro.json`
  leyó bien. Chico.
- **JSON válido pero vacío:** si `betometro.json` se lee bien pero no tiene `carreras` (no debería pasar con
  nuestro escritor), arranca con base vacía sin mirar `.anterior` (`almacen.cpp:67-80`) y el siguiente guardado
  lo pisa. Propongo: si la base leída tiene 0 carreras y `.anterior` tiene más, avisar y usar `.anterior`. Chico.
- **Lectura robusta — bien.** `Json::leer` solo tira `ErrorJson` (números con `strtod`, tope de 200 niveles);
  `desdeJson` usa valores por defecto para todo (`app/nucleo/modelo.cpp:79-100`). No encontré forma de que una
  base rota tire el programa abajo al arrancar.
- **Desgaste de la tarjeta — no es problema.** Medido: una carrera de 1 h (15 vueltas a 5 Hz) escribe
  **5,7 MB** con la base vacía, **38 MB** con 2 MB de historia y **280 MB** con 18 MB de historia. El video
  escribe **2 GB por hora** (`app/camara.cpp:25`). Una temporada de 30 carreras con base grande: ~8 GB; una
  tarjeta común aguanta decenas de TB. Lo que sí importa es el tiempo (1.3).
- **Tamaños medidos:** 41,5 bytes por fila de traza; una vuelta de 4 min a 5 Hz = 50 KB; un recorrido lleno
  (20 000 filas, 5,5 h) = 0,83 MB; carrera de 1 h = 0,75 MB.
- **Sin respaldo fuera de la tarjeta.** `diseno-sistema.md:111` dice «no se exporta respaldo». La única copia de
  todas las carreras está en la SD del auto; Drive sube solo informes y videos. Ver pregunta 2.

---

## 4. Formato: qué conviene

| Opción | Guardar una vuelta | RAM | Exportar formato app | Cambio de código |
|---|---|---|---|---|
| **A. JSON único + 2.1 + 2.2** | 2 s en la Pi con 18 MB (crece con la historia) | arreglada con 2.2 | ya está | chico |
| **B. Un archivo por carrera** (`carreras/<id>.json` + `config.json`) | ~50 ms en la Pi (solo esa carrera, ≤ 1 MB) | pico por archivo (≤ 10 MB) | juntar los archivos en `{schemaVersion, config, carreras}` | medio (solo `Almacen` y `Datos::guardar`; el resto sigue usando `base_` en memoria) |
| C. SQLite (como la app: `config`, `carreras`, `vueltas` con JSON) | ~ms | solo si se carga perezoso (toca `informe`, `servidor`, `resumen`…) | SELECT + armar JSON | grande + paquete `libqt6sql6-sqlite` |

**Recomendación: B.** Da el 95 % de lo que da SQLite con un cuarto del trabajo, mantiene el formato de la app
archivo por archivo (cada `<id>.json` es exactamente una «carrera» de la app), y la escritura segura de hoy
(`.nuevo` + fsync + rename) se reutiliza tal cual. SQLite se justifica solo si algún día hay que buscar o
mostrar miles de carreras sin cargarlas; hoy no.

**Plan de migración seguro (en este orden, cada paso publicable solo):**
1. **Antes de nada:** 2.1 (`to_chars`), 1.2 (aviso si no guarda) y 1.1 (recorrido en curso cada 60 s). Tres
   cambios chicos, sin tocar el formato. Publicar y que Matías lo pruebe.
2. **Copia de seguridad de Matías:** antes de actualizar a la versión con el formato nuevo, bajar la base:
   `scp beto@<IP>:/var/lib/betometro/betometro.json .` (comando listo en la nota de la versión).
3. **`Almacen` nuevo:** `config.json` y `carreras/<id>.json`; cada archivo con la misma escritura segura.
   `Datos::guardar(id)` guarda solo esa carrera (o la config). Borrar carrera = mover a `carreras/papelera/`
   (se vacía a los 30 días; también resuelve el pedido «borrar carrera = borrar su video» con calma).
4. **Migración al arrancar**, si existe `betometro.json` y no `carreras/`: leer, escribir cada carrera, **volver
   a leer todo y comparar con lo original** (`aJson()` igual); recién entonces renombrar
   `betometro.json` → `betometro.json.migrado` (no borrar nunca). Si se corta a mitad, la próxima vez repite
   (las carreras ya escritas se pisan, no se duplican). `Datos` vieja y nueva se prueban con la misma
   `prueba_datos.cpp` + una prueba de migración con la base de muestra.
5. **Exportar igual que la app:** `GET /betometro.json` en «Betómetro en la red» (junta todo; `JSON.stringify`
   compacto) y, si Matías quiere, subirlo a Drive al terminar cada carrera (pesa menos que un informe con video).
6. Sacar 2.2 de la lista (ya no hace falta) y dejar la traza en curso cada 30 s (punto 1.1) como definitivo.

Costo total de 3 a 5: **medio** (2 o 3 sesiones, con pruebas).

---

## Las 3 que haría primero
1. **Recorrido libre: guardar la traza en curso** (1.1). Es la única pérdida real hoy. Chico.
2. **`to_chars` en `numeroComoJs`** (2.1): guardar 10 veces más rápido, mismo archivo byte a byte. Chico.
3. **Avisar cuando no se puede guardar** (1.2) + aviso de poco lugar antes de largar. Chico.

Después, el formato B (sección 4) en una versión aparte, con la copia de la base de Matías hecha antes.

## Preguntas para Matías
1. En un recorrido cortado por la luz, ¿preferís ver el recorrido «hasta donde llegó» (con un cartel de que se
   cortó) o que no aparezca? Yo lo mostraría.
2. Hoy todas las carreras viven solo en la tarjeta del auto. ¿Querés que al terminar cada carrera se suba una
   copia de la base a Drive (como los informes)? Es chico y no cambia lo que dice el diseño de la app.
3. Para pasar al archivo por carrera hay que actualizar una vez con la base copiada a la PC antes. ¿Lo hacemos
   después de la temporada o ahora, mientras hay pocas carreras?
4. Si se llena la tarjeta en medio de una carrera, ¿preferís un cartel chico fijo («NO SE GUARDA») o que la voz
   lo diga una vez? Por lo decidido el 04/11 (nada de carteles de calor en carrera), propongo el cartel chico.
