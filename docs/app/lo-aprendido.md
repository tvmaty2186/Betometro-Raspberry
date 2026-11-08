# Lo aprendido de la app (v2.1.1) — para rehacerla en la Raspberry

Este archivo junta **todo lo que hace la app** y **cada decisión ya tomada**. Sirve para rehacerla en Qt/QML + C++ sin volver a abrir el código.

- Fuente: `referencia/app-2.1.1/index.html` y `referencia/app-2.1.1/informe-plantilla.html`.
- Los nombres entre `comillas invertidas` son de esa copia: se buscan con grep.
- No repite lo que ya está en [arquitectura.md](arquitectura.md), [diseno.md](diseno.md) e [historial.md](historial.md). Leelos también.
- Regla: **no se vuelven a discutir** las decisiones de la app. Si algo acá dice «no está claro en el código», eso sí hay que definirlo.

---

## 1. Modelo de datos

### 1.1 Base completa (`Store.todo()`)

```json
{ "schemaVersion": 2, "config": { ...cfg }, "carreras": [ { ...carrera, "vueltas": [ ...vuelta ] } ] }
```

### 1.2 Carrera

| Campo | Tipo | Qué es |
|---|---|---|
| `id` | texto | `'c' + Date.now() en base 36 + 4 caracteres al azar` (`nuevaId`) |
| `nombre` | texto | Hasta 80 caracteres. En práctica: `«Práctica dd/mm/aa hh:mm»` (formato es-AR corto) |
| `fecha` | texto `AAAA-MM-DD` | Día de la carrera (hora local). En práctica: hoy |
| `notas` | texto | Hasta 500 caracteres |
| `lugar`, `piloto`, `navegante`, `auto`, `numero`, `categoria` | texto | **Restos de versiones viejas.** Se guardan vacíos y ya no se editan. `lugar` todavía se muestra si tiene algo |
| `tiempoIdealMs` | entero ms | Tiempo ideal por vuelta. Mínimo 5000 |
| `distanciaM` | entero m | Distancia por vuelta. 0 = sin distancia (no hay velocidad sugerida ni adelanto) |
| `patron` | `'todas'` \| `'alt1'` \| `'alt2'` | Qué vueltas son medidas (ver 2.1) |
| `practica` | bool | `true` = práctica libre |
| `creada` | texto ISO | Fecha y hora de creación |
| `vueltasTotal` | entero | Cantidad de vueltas (1 a 300). En práctica no existe o es 0 = sin fin |
| `vueltas` | lista | Las vueltas corridas, de **todas** las largadas |

- Si la carrera ya tiene vueltas, **no se pueden cambiar** `tiempoIdealMs`, `distanciaM` ni `patron`. Sí nombre, fecha, vueltas totales y notas.
- Una práctica libre **se guarda recién al largar** (primera marca). Si salís sin largar, no queda nada.

### 1.3 Vuelta (lo que guarda `alEvento('cerrada')`)

| Campo | Tipo | Qué es |
|---|---|---|
| `largada` | entero | Número de largada dentro de la carrera (1, 2, …). Se calcula como `máx(largada de las vueltas) + 1` al largar (`nuevaCarrera`) |
| `n` | entero | Número de vuelta dentro de esa largada (empieza en 1) |
| `medida` | bool | `esMedida(patron, n)` |
| `idealMs` | entero | Tiempo ideal en el momento |
| `realMs` | entero | Tiempo real de la vuelta (redondeado a ms) |
| `difMs` | entero | `realMs − idealMs`. **Positivo = llegó tarde** |
| `fuente` | texto | `'pantalla'`, `'teclado'` o `'pulsador'` |
| `fin` | entero | `Date.now()` al cerrar (hora de pared, en ms) |
| `distM` | entero m | Solo si hubo distancia GPS: `distanciaCruda × factor` |
| `velPromKmh` | número, 1 decimal | `distM / (realMs/1000) × 3,6` |
| `velMaxKmh` | número, 1 decimal | Máxima de las muestras válidas de la vuelta |
| `factorVuelta` | número, 4 decimales | Solo si la autocalibración aceptó esta vuelta (ver 2.6) |
| `traza` | lista de filas | Solo si hubo **5 filas o más**. Ver 1.4 |

### 1.4 Traza de la vuelta (`grabarTraza`)

- Una fila por lectura de GPS con latitud: `[t, d, kmh, lat, lon, prec]`.
  - `t`: segundos desde el inicio de la vuelta, 1 decimal.
  - `d`: metros recorridos = `distanciaM − restanteM` del motor en ese instante (ya con factor), mínimo 0, 1 decimal.
  - `kmh`: velocidad de la lectura, 1 decimal.
  - `lat`, `lon`: 6 decimales.
  - `prec`: precisión en metros, entera.
- Se graban **todas** las lecturas, también las que el motor descartó por precisión. El informe filtra después.
- Solo se graba con la carrera en curso. Se vacía al empezar cada vuelta.
- Tope: 20.000 filas (se tira la más vieja).

### 1.5 Dónde se guarda en la app

- SQLite (nativo): tablas `config (clave, valor)` con una sola fila `'config'` (JSON), `carreras` (columnas de la tabla 1.2) y `vueltas (carreraId, largada, n, datos JSON)` con clave `(carreraId, largada, n)`.
- En el navegador: `localStorage['betometro.v2']` con el JSON entero.
- **Respaldo automático:** después de cada cambio, a los 800 ms (se agrupan los cambios) se escribe el JSON completo en `betometro.json`.
- Si al abrir SQLite está vacía, se importa lo anterior (localStorage, `betometro.json` o la versión 1). No aplica en la Raspberry.
- **Exportar respaldo** (Ajustes → Aplicación): comparte `betometro-respaldo-AAAA-MM-DD.json` con `JSON.stringify(todo, null, 1)`.
- No hay importar respaldo en la app.

---

## 2. Motor (`Motor`)

El motor no tiene pantalla ni audio. **En la Raspberry se reescribe en C++ con la misma lógica**. Todo el tiempo es en ms de un reloj **monotónico** (`performance.now()`), salvo `fin` de la vuelta.

### 2.1 Vueltas medidas

```
esMedida(patron, n):
  'alt1' → n impar      (1.ª medida)
  'alt2' → n par        (1.ª de práctica)
  otro   → true         ('todas')
```

### 2.2 Datos de entrada por largada (`nuevaCarrera`)

```
carrera = { tiempoIdealMs, distanciaM, patron, vueltasTotal,       // de la carrera elegida
            antirreboteMs: cfg.antirreboteMs,
            umbralPrecisionM: cfg.umbralPrecisionM,
            calibrar: cfg.calibrar,
            congelarMs: max(cfg.cuentaN, 3) * 1000,
            factorGps: 1,                                           // arranca en 1 en CADA largada
            vueltas: [] }
```

- El factor del GPS **no se guarda** entre largadas ni entre carreras.
- Si cambiás en Ajustes el antirrebote, el umbral o la calibración con la carrera en curso, se aplican enseguida a la carrera.

### 2.3 Estado interno

`lapNum, lapStart, enCurso, dCruda (m), ult = {t, v} (última muestra válida), velMaxMs, vNecCongelada, factores[], factor`.

### 2.4 Muestra de velocidad (`muestra(m)`)

`m = { t, velMs, precisionM, lat?, lon? }`

```
si m.precisionM > umbralPrecisionM: emitir 'descartada'; return false
si enCurso y ult:
  dt = (m.t − ult.t) / 1000
  si 0 < dt < 10: dCruda += (ult.v + m.velMs) / 2 * dt      // trapecio
  (si dt ≥ 10 s, ese hueco NO suma distancia)
ult = { t: m.t, v: m.velMs }
si enCurso: velMaxMs = max(velMaxMs, m.velMs)
```

Distancia «al instante» (extrapola hasta 2 s con la última velocidad):

```
distCruda(ahora) = dCruda + (enCurso y ult ? ult.v * clamp((ahora − ult.t)/1000, 0, 2) : 0)
```

### 2.5 Marca (`marcar(ahora, fuente)`)

```
si !enCurso:                          // LARGADA
  enCurso = true; lapNum = 1; nuevaVuelta(ahora)
  emitir 'vuelta' {n:1, medida, inicio: ahora}
  return 'largada'
si ahora − lapStart < antirreboteMs:  // antirrebote contado desde el inicio de la vuelta
  emitir 'ignorada'; return 'ignorada'
real = ahora − lapStart; cruda = distCruda(ahora)
v = { n: lapNum, medida, idealMs: T, realMs: round(real), difMs: round(real − T), fuente, fin: Date.now() }
si cruda > 0:
  v.distM = round(cruda * factor)
  v.velPromKmh = 1 decimal(cruda * factor / (real/1000) * 3.6)
  v.velMaxKmh = 1 decimal(velMaxMs * 3.6)
  si calibrar y D > 0:
    f = D / cruda
    si 0.85 < f < 1.15:               // fuera de eso: se marcó mal o se cortó camino
      factores.push(f); factor = promedio(factores); v.factorVuelta = 4 decimales(f)
carrera.vueltas.push(v); emitir 'cerrada' v
lapNum += 1; nuevaVuelta(ahora)
emitir 'vuelta' {n: lapNum, medida, inicio: ahora}
return 'marca'

nuevaVuelta(ahora):
  lapStart = ahora; dCruda = 0; velMaxMs = ult ? ult.v : 0; vNecCongelada = null
  si ult: ult = { t: ahora, v: ult.v }   // la distancia de la vuelta nueva arranca justo en la marca
```

- La largada **no** tiene antirrebote.
- `terminar()`: `enCurso = false`, emite `'terminada'`.
- El factor calculado en una vuelta se usa para **la vuelta siguiente** (la vuelta que calibra ya se guardó con el factor anterior).

### 2.6 Estado (`estado(ahora)`) — lo que leen pantalla y voz

```
T = tiempoIdealMs; D = distanciaM
sinGps = !ult o (ahora − ult.t > 3000)
vActKmh = (ult y !sinGps) ? ult.v * 3.6 : null
vBaseKmh = D > 0 ? D / (T/1000) * 3.6 : null

si !enCurso:
  { n:0, medida: esMedida(1), transMs:0, restanteMs:T, proxMedida: esMedida(1),
    sinGps, vActKmh, vBaseKmh, vNecKmh: vBaseKmh, restanteM: D, deltaS: null }

trans = ahora − lapStart; restanteMs = T − trans        // puede ser negativo (vuelta pasada)
d = distCruda(ahora) * factor; restanteM = D − d
vNecKmh = null; deltaS = null
si D > 0 y !sinGps:
  deltaS = (d − D * trans / T) / (D / (T/1000))      // segundos; POSITIVO = ADELANTADO
  si restanteM <= 0:            vNecKmh = 0
  si no, si restanteMs <= congelarMs:                 // CONGELAR
    si vNecCongelada == null: vNecCongelada = restanteMs > 0 ? restanteM/(restanteMs/1000)*3.6 : null
    vNecKmh = vNecCongelada
  si no:                        vNecKmh = restanteM / (restanteMs/1000) * 3.6

return { enCurso, n: lapNum, medida: esMedida(lapNum), transMs: trans, restanteMs,
         proxMedida: esMedida(lapNum + 1), sinGps, vActKmh, vBaseKmh, vNecKmh,
         congelada: vNecCongelada != null, restanteM, deltaS }
```

- **Congelar:** en los últimos `max(cuentaN, 3)` segundos la velocidad sugerida queda fija (el cálculo se vuelve inestable). En pantalla se agrega `■` al número.
- Sin GPS (más de 3 s sin muestra válida): no hay `deltaS` ni `vNecKmh` (se muestran como «–» o «SIN GPS»).
- `sinGps` cuenta solo muestras **válidas** (las descartadas por precisión no cuentan).

### 2.7 Eventos y qué hace la app con cada uno (`alEvento`)

| Evento | Qué hace la app |
|---|---|
| `vuelta` | Pone `velMaxVuelta = 0`, vacía la traza, reinicia la voz (`resetVueltaAudio`). Si `vueltasTotal` existe y `n > vueltasTotal`, no hace nada más (la carrera ya terminó). Si no, y `avisoInicio`, dice el anuncio de vuelta (ver 3.3) |
| `cerrada` | Agrega la traza si tiene ≥5 filas. Guarda la vuelta con `largada`. Actualiza «Última vuelta». Si `n ≥ vueltasTotal`, termina la carrera con motivo «Carrera completa» |
| `ignorada` | Cartel «Marca ignorada (antirrebote)» |
| `descartada`, `terminada` | Nada |

### 2.8 Vista antes de largar (`vista()`)

Sin carrera en curso, los widgets muestran datos «en reposo» de la carrera elegida (o de `cfg` si no hay ninguna, por ejemplo en el editor de pantalla):

- `restanteMs = T`, `transMs = 0`, `restanteM = D`, `deltaS = null`, `congelada = false`.
- `vNecKmh = vBaseKmh = D/(T/1000)·3,6`.
- La velocidad actual sale de la última muestra del GPS (aunque no haya carrera), con la misma regla de 3 s.

---

## 3. Voz y sonidos

### 3.1 Programador de voz (`programador`, cada 15 ms)

Corre solo con la carrera en curso. Lee `motor.estado(ahora)`.

```
N = cuentaN; ant = (modoCuenta == 'voz') ? anticipoMs : 0

// Pitido cada X segundos (opcional), desde el inicio de la vuelta
si pitidoCadaS > 0:
  p = floor(transMs / (pitidoCadaS*1000))
  si p > 0 y p != ultPitidoCada y restanteMs > N*1000 + 500: ultPitidoCada = p; tic()

// Cuenta regresiva
si N > 0:
  k = ceil((restanteMs − ant) / 1000)
  si 1 ≤ k ≤ N y k != ultimoK y (ultimoK == null o k < ultimoK):
    ultimoK = k
    voz → número k   |   pitidos → pitido 880 Hz 90 ms

// Cero
si pitidoCero y !pitoCero y restanteMs <= 0:                 // sin anticipo
  pitoCero = true; pitido largo 1320 Hz 450 ms
si !pitidoCero y N > 0 y !pitoCero y ultimoK != null y restanteMs − ant <= 0:
  pitoCero = true; voz → «cero»   |   pitidos → pitido 880 Hz 90 ms
```

- Solo se cuenta **hacia abajo**: nunca repite ni vuelve a un número mayor.
- «cero» solo suena si la cuenta ya había empezado (`ultimoK != null`).
- Con `pitidoCero` prendido **no** se dice «cero»: suena el pitido largo, sin anticipo.
- Al empezar cada vuelta se reinicia todo (`resetVueltaAudio`).

### 3.2 Fábrica

- Cuenta en voz de los últimos **20 s**, en números.
- **Sin** pitido largo: dice «cero».
- Anticipo: **40 ms** en la app instalada (los números ya están grabados); 250 ms en la web.
- Pitido cada X s: apagado (0).

### 3.3 Todas las frases

| Frase | Cuándo |
|---|---|
| «1» … «60», «cero» | Cuenta regresiva (la app pregraba «cero» y del 1 al 60) |
| «Vuelta N, medida» / «Vuelta N, práctica» | Al empezar cada vuelta, si `avisoInicio` (fábrica: sí). Incluye la vuelta 1 en la largada |
| «Última vuelta, medida» / «Última vuelta, práctica» | En lugar de «Vuelta N» cuando `n == vueltasTotal` |
| «Carrera completa» | Al cerrar la última vuelta |
| «Pulsador desconectado» | Si se desconecta el pulsador Bluetooth asignado |
| «5», «4», «3», «2», «1» | Botón «Medir demora de la voz» (solo web) |

- Hay restos sin uso: `textoDelta` («en tiempo», «adelantado 1,5», «atrasado 12») y `avisoProx`/`vozVel`. Están forzados apagados. **No se dicen.**
- Velocidad de la voz: 1,15 en nativa (pregrabado con TextToSpeech), 1,2 en la web. Voz es-AR si hay.

### 3.4 Sonidos

| Sonido | Web (Web Audio) | Nativa |
|---|---|---|
| Pitido de la cuenta en modo pitidos | onda cuadrada 880 Hz, 90 ms, volumen 0,35 | `#corto` |
| Pitido largo del cero | 1320 Hz, 450 ms | `#largo` (todo pitido de más de 200 ms) |
| Tic (pitido cada X s) | 1000 Hz, 70 ms | `#tic` |
| Pitido de prueba del pulsador | 1320 Hz, 200 ms | `#corto` |
| Clic al marcar | ruido filtrado de 30 ms que se apaga rápido + seno 1800 Hz | `#clic` |

- El clic suena en cada marca **aceptada** (no en las ignoradas), si `clicMarca` (fábrica: sí).
- Canal: «alarma» (suena aunque el teléfono esté en silencio) o «multimedia» (sale por Bluetooth/auriculares).
- `volumenMax`: al largar sube el volumen al máximo; al terminar lo devuelve.

### 3.5 Medir demora y probar cuenta (Ajustes → Cuenta regresiva)

- **Medir demora** (oculto en la app instalada): dice «5»…«1» cada 1,1 s, mide desde la orden hasta que empieza a hablar (si no empieza, cuenta 1000 ms). Ordena, descarta el más rápido y el más lento, promedia los 3 del medio y redondea a 10 ms → `anticipoMs`. Texto: «Demora medida: a, b, c, d, e ms. Anticipo ajustado a X ms.»
- **Probar cuenta de 5 s:** misma lógica que en carrera con T = 5 s y N = 5. Muestra «Prueba: 0:04.3»… hasta «0:00.0».

---

## 4. Configuración (`DEF` y `cfg`)

`cfg = DEF + lo guardado`. Todo cambio en Ajustes se guarda al instante (`leerForm` en cada `input`/`change`).

### 4.1 Todas las claves

| Clave | Fábrica | Qué hace | Dónde se cambia | Valores |
|---|---|---|---|---|
| `tiempoIdealMs` | 240000 (4:00) | Tiempo ideal de la **práctica libre** y valor inicial de una carrera nueva | Práctica libre | ≥ 5000 ms. Si escribís menos, queda el anterior |
| `distanciaM` | 3200 | Distancia de práctica y de carrera nueva | Práctica libre | 0 a 1.000.000 m |
| `patron` | `'alt1'` | Vueltas medidas de práctica y de carrera nueva | Práctica libre | `todas`, `alt1`, `alt2` |
| `cuentaN` | 20 | Segundos de cuenta regresiva hablada. También fija el congelado: `max(cuentaN,3)` s | Ajustes → Cuenta regresiva | 0 a 60 (0 = sin cuenta) |
| `modoCuenta` | `'voz'` | Números o pitidos | Ajustes → Cuenta regresiva | `voz`, `pitidos` |
| `pitidoCero` | false | Pitido largo al llegar a cero (si no, dice «cero») | Ajustes → Cuenta regresiva | bool |
| `clicMarca` | true | Clic al marcar | Ajustes → Cuenta regresiva | bool |
| `anticipoMs` | 40 nativa / 250 web | Adelanta la voz por su demora | Ajustes → Cuenta regresiva | 0 a 1500, paso 10 |
| `pitidoCadaS` | 0 | Tic cada X s desde el inicio de la vuelta | Ajustes → Cuenta regresiva | 0 a 600 (0 = apagado) |
| `avisoInicio` | true | Anuncio «Vuelta N, medida» | Ajustes → Avisos por voz | bool |
| `avisoProx` | false | Sin uso; se fuerza a false | — | — |
| `antirreboteMs` | 3000 | Ignora una segunda marca durante X s | Ajustes → Botón y pantalla (en segundos) | 0 a 60 s, paso 0,5 |
| `colMedida` | `#b3132b` | Color de vuelta medida | Ajustes → Botón y pantalla | color |
| `colPractica` | `#0f6e3a` | Color de vuelta de práctica | Ajustes → Botón y pantalla | color |
| `velActiva` | true | Usa GPS. Si está apagado, se esconden los widgets de velocidad | Ajustes → Velocidad | bool |
| `fuenteVel` | `'gps'` | Se fuerza siempre a `'gps'` | — | — |
| `umbralPrecisionM` | 20 | Descarta lecturas con error mayor | Ajustes → Velocidad | 1 a 200 m |
| `calibrar` | true | Autocalibración del factor GPS | Ajustes → Velocidad | bool |
| `simVelKmh` | 48 | Velocidad del simulador | Widget simulador (hoy oculto) | 0 a 160 |
| `teclas` | null | Códigos de tecla Android del pulsador (null = por defecto) | Ajustes → Pulsador | lista |
| `teclaWeb` | null | Tecla aprendida en la web | Ajustes → Pulsador | texto |
| `teclaNombre` | `''` | Nombre de la tecla asignada. **Si está vacío, MARCAR en pantalla es obligatorio** | Ajustes → Pulsador | texto |
| `pulsadorNombre` | `''` | Nombre del dispositivo Bluetooth asignado | Ajustes → Pulsador | texto |
| `modoCarrera` | true | Al largar: No molestar + pantalla completa | Ajustes → Modo carrera | bool |
| `tema` | `'avispon'` | Tema | Ajustes → Tema y pantalla | `avispon`, `rally`, `sport`, `cabina` |
| `numeroAuto` | `'101'` | Número del redondel (tema Avispón) | Ajustes → Tema y pantalla | hasta 4 caracteres; vacío → `101` |
| `modoSol` | `'auto'` | Fondo claro para pleno sol | Ajustes → Tema y pantalla | `no`, `si`, `auto` |
| `indicador` | `'fondo'` | Cómo se ve el tipo de vuelta | Ajustes → Tema y pantalla | `marco`, `fondo` |
| `volumenMax` | true | Volumen al máximo en carrera | Ajustes → Sonido | bool |
| `canalAudio` | `'alarma'` | Salida del sonido | Ajustes → Sonido | `alarma`, `multimedia` |
| `widgets` | null → fábrica | Armado vertical | Editor de pantalla | lista (ver 6) |

Claves que no están en `DEF` pero se guardan:

| Clave | Qué es |
|---|---|
| `widgetsH` | Armado horizontal. Si no existe, se copia el de fábrica |
| `widgetsV` | Versión del armado vertical. Si no es **6**, el vertical vuelve a fábrica |
| `cero0`, `cuenta20`, `avispon1`, `avispon21` | Banderas de migración (4.2) |
| `vozVel` | Sin uso; se fuerza a false |
| `simModo` (`'variable'`), `simRuido` (bool) | Solo para el simulador; no hay dónde cambiarlas en la interfaz |

- «Abrir Betómetro al prender el teléfono» no está en `cfg`: lo guarda Android (`Beto.iniciarAlPrender`). Fábrica: apagado.

### 4.2 Migraciones de una sola vez (`fijarCfg` y arranque)

| Bandera | Qué hace (una sola vez) |
|---|---|
| `cero0` | `pitidoCero = false` |
| `cuenta20` | Si `cuentaN == 60`, pasa a 20 |
| `avispon21` (y `avispon1`) | En el arranque, después de cargar: si lo **guardado** no tiene `avispon21`, pone `tema = 'avispon'`, marca las dos banderas y guarda enseguida |
| `widgetsV = 6` | Si es otra, el armado vertical vuelve a fábrica |

Siempre (no son de una sola vez): `fuenteVel = 'gps'`, `vozVel = false`, `avisoProx = false`, se sacan los widgets `simulador`, y si no hay `teclaNombre` se vuelve a poner MARCAR en las dos orientaciones.

**Patrón para una migración nueva:** preguntar por la bandera en lo **guardado** (`Store.config.bandera`), no en `cfg` (que ya tiene `DEF` mezclado). Aplicar el cambio, poner la bandera y guardar.

---

## 5. Pantallas

### 5.0 Navegación (`mostrar`, `volver`)

- `mostrar(p)` pone la pantalla en **vertical** siempre. Solo el crono deja girar (`orientacion('libre')`).
- `volver()` es el botón atrás y todos los «Volver»:

| Dónde estás | Qué hace |
|---|---|
| Visor del informe abierto | Lo cierra |
| Ajustes abierto | Guarda y cierra |
| Editor de pantalla | Sale del modo edición y va al inicio |
| Crono con carrera en curso | Cartel «Para salir, terminá la carrera». No sale |
| Crono sin carrera | Al detalle si la carrera está guardada; si no (práctica sin largar), a Práctica libre |
| Editor de carrera | Al detalle si la carrera existe; si es nueva, a Carreras |
| Detalle | Siempre a Carreras |
| Práctica libre | Guarda los valores y va al inicio |
| Carreras, Chequeo | Al inicio |
| Inicio | Cierra la app |

- **Cartel** (`avisar`): pastilla amarilla arriba, 2,2 s.
- **Confirmar con doble toque:** borrar, terminar y restablecer piden un segundo toque dentro de **3 s**. El botón cambia el texto a «Tocá otra vez…» y vuelve solo.

### 5.1 Inicio

- Logo «BETÓMETRO», «Regularidad para el navegante» y la versión. Al lado, «· Actualizar a X.Y.Z» en amarillo si hay versión nueva.
- Botón **Compartir** (comparte el link del APK con el texto «Betómetro: cronómetro de regularidad para el navegante. Descargá la app para Android acá:»). En la Raspberry no aplica.
- Velocímetro de adorno que se mueve solo: aguja `47 + 2,5·sin(0,9t) + 1,2·sin(2,3t)`, sugerida 48.
- 5 botones en arco (posiciones en arquitectura.md): Carreras, Práctica libre, Chequeo previo, Pantalla de carrera, Ajustes.
- El inicio **no** larga carreras: se larga desde la ficha.

### 5.2 Carreras

- Cabecera: Volver, título, botón **+** (carrera nueva).
- Vacío: «Todavía no hay carreras. Tocá + para cargar la primera.»
- Ayuda: «Mantené apretada una carrera para borrarla.»
- **PRÓXIMAS:** no práctica, fecha ≥ hoy y no finalizada. Orden: fecha ascendente.
- **CORRIDAS:** todo lo demás (prácticas, pasadas, finalizadas). Orden: fecha descendente, después `creada` descendente.
- **Tarjeta:** redondel con día y mes (`ENE`…`DIC`); nombre; línea `lugar · N vueltas · ideal`; mini barras de las últimas 16 vueltas medidas (alto `6 + min(18, |dif s|·12)` px, color por nivel); chip de estado.
- Tocar la tarjeta → detalle.
- Mantener apretado **600 ms** → vibra y aparece «¿Borrar «nombre»?» con Borrar / Cancelar.
- Tema `rally`: tarjetas tipo «carnet» (`tarjetaCarnet`), con casillas de las primeras 6 medidas y un sello «ERROR …» rojo o verde.

**Chip de estado (`estadoCarrera`), en este orden:**

| Condición | Texto | Color |
|---|---|---|
| Tiene vueltas medidas | `ERROR <suma>` | por nivel del promedio |
| Es práctica | `PRÁCTICA` | neutro |
| Finalizada | `FINALIZADA` | neutro |
| Es hoy | `HOY` | amarillo |
| Es mañana | `MAÑANA` | amarillo |
| Faltan N días | `EN N DÍAS` | neutro |
| Ya pasó | `SIN CORRER` | neutro |

**Nivel de error (`nivelErr`, en segundos):** ≤ 0,2 = `ok` (verde) · ≤ 0,5 = `medio` (amarillo) · más = `mal` (rojo).

**Formato de errores (`durNum`, `dur`, `difDur`):**
- Menos de 59,95 s: `12,3 s`.
- Desde 1 minuto: `m:ss,d` (sin « s»). Desde 1 hora: `h:mm:ss,d`.
- Con signo: `+` tarde, `−` (signo menos tipográfico) temprano, `±` justo.

### 5.3 Editor de carrera

Campos en este orden:

| Campo | Detalle |
|---|---|
| Nombre de la carrera | Texto, máx. 80 |
| Fecha | Selector de fecha. Nueva: hoy |
| Vueltas | Con − y +, de 1 a 300 |
| Distancia por vuelta (metros) | Número ≥ 0 |
| Tiempo ideal por vuelta | Tres casillas MIN : SEG . DÉC (solo dígitos) |
| (texto) | «Velocidad promedio: 48,0 km/h» en vivo (si hay distancia y tiempo) |
| Vueltas medidas | Tres botones con puntitos: Todas (●●●●), 1.ª medida (●○●○), 1.ª práctica (○●○●) |
| Notas | Texto, máx. 500 |

- Carrera nueva: toma tiempo, distancia y patrón de `cfg` (los de la última práctica).
- Si ya tiene vueltas: tiempo, distancia y patrón quedan bloqueados con el aviso «Esta carrera ya tiene vueltas corridas: el tiempo, la distancia y las vueltas medidas no se pueden cambiar.»
- **Validación al guardar** (mensaje en rojo debajo):
  - sin nombre → «Poné un nombre a la carrera.»
  - sin fecha → «Elegí la fecha de la carrera.»
  - vueltas < 1 → «Poné la cantidad de vueltas de la carrera.»
  - tiempo < 5 s → «El tiempo ideal tiene que ser de al menos 5 segundos.»
- El tiempo se calcula como `(min·60 + seg)·1000 + déc·100`. Por eso **90 s se guarda como 1:30** y así se ve al volver a abrir.
- GUARDAR CARRERA → guarda y va al detalle.
- «Borrar carrera» (solo al editar): doble toque → borra y va a Carreras.
- Título: «Nueva carrera» o «Editar carrera».

### 5.4 Detalle / ficha de la carrera

- Título: nombre. Debajo, la fecha larga («sábado, 1 de noviembre de 2026»).
- **Recuadro principal:**
  - Con vueltas medidas: «Error total · vueltas medidas», la suma grande coloreada por nivel del promedio; debajo Promedio, Mejor (menor error absoluto) y Vueltas (`corridas/total`).
  - Sin medidas: chip «Se corre hoy» / «Faltan N días» / «Sin correr», el ideal grande y «tiempo ideal por vuelta».
  - Tira de 3 datos: Ideal (o Vueltas, «∞» si no hay total), Distancia en m, Vel. promedio en km/h (1 decimal con coma).
  - Patrón: puntitos + «Todas las vueltas medidas» / «Alternadas · la 1.ª medida» / «Alternadas · la 1.ª de práctica».
- **Acciones:**
  - Si está finalizada: cartel «CARRERA FINALIZADA», sin LARGAR.
  - Si no: **LARGAR**, habilitado solo si es práctica o **la fecha es hoy**. Si no, el botón está gris y dice «Se larga el 15 nov 2026» o «La fecha de la carrera ya pasó».
  - Editar (no en prácticas).
  - Borrar (doble toque) → Carreras.
- Notas, si hay.
- **Vuelta por vuelta** (si hay vueltas): una fila por vuelta con chip del número (color medida/práctica), barra y diferencia.
  - Escala: `max(1000 ms, T·0,10)`; ancho `min(1, |dif|/escala)·48 %`.
  - **Acá la barra va a la derecha si llegó tarde** (`difMs ≥ 0`) y a la izquierda si llegó antes. Es al revés que la barra del crono (ver 6.4). Así está en la app.
  - Si hubo más de una largada, separadores «Largada N».
- Botón **VER INFORME**.
- **Finalizada** (`estadoLargada`): alguna largada tiene `≥ vueltasTotal` vueltas.
- Se puede volver a largar una carrera no finalizada: arranca una largada nueva (vueltas desde 1).
- Tema `rally`: «CARNET DE RUTA» con renglones y casillas de todas las vueltas.

### 5.5 Práctica libre

- Campos: Distancia, Tiempo ideal (MIN:SEG.DÉC), Vueltas medidas. Se guardan en `cfg` al escribir.
- Texto: «Velocidad promedio necesaria: X km/h».
- **Normalización:** al salir del campo de segundos, si es > 59, se reacomoda (`0:90.0 → 1:30.0`). Los segundos aceptan hasta 5999.
- Si el tiempo total queda < 5 s, se ignora y queda el anterior.
- Botón **IR A LARGAR**: crea una carrera de práctica (sin guardar todavía) y va al crono.
- Pie: «Estos valores quedan guardados para la próxima práctica.»
- Si hubiera carrera en curso, los campos se bloquean con «Con la carrera en curso no se puede cambiar el tiempo, la distancia ni las vueltas medidas.»

### 5.6 Crono (pantalla de carrera)

- Arriba a la izquierda, chico: `«nombre · 4:00.0»` (en práctica: «Práctica libre · ideal»).
- Arriba a la derecha: **Salir** (sin carrera en curso) o **Terminar** (con carrera en curso, doble toque).
- El tablero de widgets (ver 6). Ocupa toda la pantalla menos 6 px de margen.
- MARCAR dice **LARGAR / «Tocá en la largada»** antes de largar y **MARCAR / «Fin de vuelta N»** en carrera.
- Al tocar: el botón se achica un 3 % y se aclara durante 120 ms; el teléfono vibra 40 ms.
- Orientación libre: acostando el teléfono se usa el armado horizontal.

### 5.7 Ajustes (panel `#config`)

Grupos y orden (en la app es un panel encima de la pantalla actual):

1. **Tema y pantalla:** Tema · Número del auto · Modo sol · Cómo se marca el tipo de vuelta. Hint: «Los widgets de la pantalla de carrera se arman desde «Configuración de pantalla», en el inicio.»
2. **Sonido:** Subir el volumen al máximo durante la carrera · Salida del sonido.
3. **Cuenta regresiva:** Contar en voz los últimos (s) · Cómo contar · Pitido cada (s, 0 = apagado) · Pitido largo al llegar a cero · Clic al marcar · Anticipar la voz (ms) · botones Medir demora / Probar cuenta de 5 s · texto de resultado (al abrir Ajustes muestra el diagnóstico de voz).
4. **Velocidad:** Mostrar velocidad y adelantado/atrasado · estado de ubicación precisa + botón · Descartar lecturas con error mayor a (m) · Calibración automática al marcar cada vuelta · hint del congelado.
5. **Avisos por voz:** Al empezar cada vuelta («vuelta 3, medida»).
6. **Botón y pantalla:** Ignorar una segunda marca durante (s) · Color de vuelta medida · Color de vuelta de práctica.
7. **Modo carrera:** No molestar y pantalla completa · permiso No molestar · Abrir al prender el teléfono · permiso de pantalla que vuelve sola.
8. **Pulsador:** estado · Aprender botón · Usar teclas por defecto · hint.
9. **Aplicación:** versión instalada · Buscar actualización · Exportar respaldo · resultado.

### 5.8 Visor del informe

- Pantalla completa con barra: Volver · «Informe de la carrera» · Compartir. Adentro, el HTML del informe (ver 9).

### 5.9 Editor de pantalla («Pantalla de carrera» en el inicio)

- Es **el crono real a tamaño completo**, en modo edición, sin carrera elegida (los datos salen de `cfg`).
- Cartel de ayuda 5 s: «Mantené apretado un widget para moverlo · tocalo para cambiar el tamaño · ✎ para sus opciones».
- Detalle en 6.6.

---

## 6. Widgets

### 6.1 Tipos (`TIPOS_W`)

| Clave | Nombre en el menú | Estilos | Forma redonda | Dato extra | Necesita GPS (`vel`) |
|---|---|---|---|---|---|
| `tipo` | Vuelta y tipo (medida o práctica) | — | no | no | no |
| `cuenta` | Cuenta regresiva | `numeros` (Números), `anillo` (Anillo) | sí | sí | no |
| `delta` | Adelantado / atrasado | `numero` (Número), `barra` (Barra) | sí | no | sí |
| `velocimetro` | Velocímetro | redondo = analógico; cuadrado = digital | sí | sí | sí |
| `necesaria` | Velocidad sugerida | — | sí | sí | sí |
| `faltan` | Metros que faltan | — | sí | sí | sí |
| `transcurrido` | Tiempo transcurrido | — | sí | no | no |
| `ideal` | Tiempo ideal | — | sí | no | no |
| `proxima` | Próxima vuelta | — | sí | no | no |
| `ultima` | Última vuelta | — | sí | sí | no |
| `marcar` | Botón MARCAR | — | no | no | no — **único**, `pideBoton` |

- Los widgets con `vel` se **esconden** si `velActiva` está apagado (salvo en el editor).
- Hay un tipo viejo `simulador` que se borra siempre.

### 6.2 Qué muestra cada uno

| Widget | Etiqueta | Número | Dato extra |
|---|---|---|---|
| `tipo` | — | En carrera: `VUELTA n` o `VUELTA n/tot`. Antes: `tot VUELTAS` o `LISTO` | Debajo: `MEDIDA` / `PRÁCTICA`. **Todo el widget** se pinta del color medida/práctica, texto blanco |
| `cuenta` | `Falta`; cuando se pasa, `Pasado` | `m:ss.d` del restante (en carrera redondea **hacia arriba** a la décima). Pasado: `+m:ss.d` en color de alerta | `faltan 1.234 m` (miles con punto) o `faltan – m` |
| `cuenta` anillo | igual | igual, centrado | igual. Aro r=86 en 200×200, trazo 12, que se vacía con `restanteMs/T` desde arriba |
| `delta` número | — | `EN TIEMPO` (si \|Δ\| < 0,05), `ADELANTADO 1,2 s`, `ATRASADO 1,2 s` (atrasado en color de alerta). Sin dato: `SIN DATOS` en carrera, `EN TIEMPO` antes | — |
| `delta` barra | `Adelantado / atrasado` | Barra de ±5 s: centro en 150 de 300, 28 unidades por segundo; derecha (color zona) = adelantado, izquierda (alerta) = atrasado | `En tiempo` / `Adelantado 1,2 s` / `Atrasado 1,2 s`; sin dato: `sin datos de velocidad` o `en tiempo al largar` |
| `velocimetro` redondo | — | Analógico (6.3) | `máx N` arriba del centro |
| `velocimetro` cuadrado | `Velocidad km/h` | km/h redondeado o `SIN GPS` | `máx N` (máxima de la vuelta) + barra contra la sugerida (6.4) |
| `necesaria` | `Velocidad sugerida` | km/h redondeado, `–` sin dato, con `■` si está congelada | `base N` (velocidad promedio de la vuelta ideal) |
| `faltan` | `Faltan` | `1.234 m` | `de 3.200 m` |
| `transcurrido` | `Transcurrido` | `m:ss.d` | — |
| `ideal` | `Ideal` | `m:ss.d` | — |
| `proxima` | `Próxima vuelta` | `MEDIDA` / `PRÁCTICA` | — |
| `ultima` | `Última vuelta` o `Última vuelta · V3` | tiempo real `m:ss.d` o `–` | `+1,2 s del ideal` |
| `marcar` | — | `LARGAR` / `MARCAR` | `Tocá en la largada` / `Fin de vuelta N` |

- `fmt(ms)`: `m:ss.d` con **punto** para la décima; corta hacia abajo salvo la cuenta en carrera.
- «máx» = máxima velocidad de la vuelta medida por la pantalla (`velMaxVuelta`, se pone en 0 en cada vuelta).
- «Última vuelta» se borra al largar.

### 6.3 Velocímetro analógico (`velocimetro`)

- Dibujo en 240×240, centro (120,120), radio 96.
- Escala **fija 0–160 km/h**. Arco de 240°, desde −210° (abajo a la izquierda), en sentido horario.
- Rayitas cada 5 km/h; mayores (más gruesas y largas) cada 20, con el número.
- **Aguja** = velocidad real. Sin GPS: aguja en 0 con opacidad 0,25 y número `SIN GPS` (más chico).
- **Marca** de la sugerida (color `--g-req`) y **franja verde** de sugerida ±2 km/h (`--g-zona`).
- Textos: `VELOCIDAD` arriba del número, número grande, `KM/H` abajo; dato extra arriba del centro.

### 6.4 Barras (`PROP_BARRA = 0.10`)

Barra de colores: rojo `#d7263d` 0 % · amarillo `#f5c518` 25 % · verde `#2ecc71` 45–55 % · amarillo 75 % · rojo 100 %. Un cursor blanco con borde negro. Sin dato: cursor al medio con opacidad 0,35. Ancho 62 % del widget. El cursor se mueve con transición de 0,25 s.

| Barra | Rango de cada punta | Valor | Derecha = |
|---|---|---|---|
| En `cuenta` (si no es anillo) | `max(1 s, T/1000 · 0,10)` segundos | `deltaS` | **adelantado** |
| En `velocimetro` digital | `max(5 km/h, vNec · 0,10)` | `vAct − vNec` | más rápido que la sugerida |

```
d = clamp(valor, −rango, +rango)
cursor.left = 50 + d / rango * 50   (%)
```

- El centro de la barra de velocidad es **dinámico**: siempre es la sugerida de ese momento.
- La barra de velocidad solo tiene dato con GPS y `vNec > 0`.
- Se puede sacar cada barra desde ✎ («Sin barra ±» / «Barra ±»). Se guarda como `barra: false`.

### 6.5 Tamaño de los números

- Cada número se ajusta al ancho y al alto de su widget: `tamaño = min(A% del alto, k · B% del ancho / n / ancho)`.
  - `n` = cantidad de caracteres (mínimo 3). `k` = cuánto entra por carácter según la tipografía del tema (Avispón: 190).
  - `ancho` = opción «Números −/+» del widget (0,7 a 1,8, paso 0,1; fábrica 1). Además estira el número en horizontal (`scaleX(ancho)`).

| Widget | A (% alto) | B (% ancho) | n por defecto |
|---|---|---|---|
| General | 58 | 100 | 4 |
| `cuenta` | 82 (66 con barra) | 105 | 6 |
| `velocimetro` digital con barra | 46 | 100 | 4 |
| `delta` número | 70 | 95 | 12 |
| `tipo` (número) | 36 | 90 | 8 |
| `tipo` (MEDIDA/PRÁCTICA) | 34 | 90 / 8 | — |
| MARCAR | 52 | 18 % del ancho (fijo) | — |

- Etiqueta: entre 10 y 15 px (11 % del alto). Dato extra: entre 11 y 17 px (11 % del alto), sin cortar.
- Widget: bordes redondeados 22 px, relleno 8×12 px; redondos: círculo con relleno 16 %.
- Se dibujan **primero los más grandes** (por área), así los chicos quedan siempre adelante.

### 6.6 Posiciones guardadas

- Cada widget: `{ t, e?, forma?: 'redondo', x, y, w, h, extra?, barra?: false, ancho? }`.
- `x, y, w, h` son **fracciones del tablero** (0 a 1).
- Redondos: `w` es el **diámetro como fracción del ancho**; `h` no se usa.
- Las de fábrica se escribieron en una caja de referencia: vertical **378×800** (`caja`), horizontal **800×378** (`cajaH`).
- Tablero = pantalla menos 6 px de cada lado.
- `horizontal` = el tablero es más ancho que alto. Se usa `cfg.widgetsH`; si no, `cfg.widgets`.

### 6.7 Pantallas de fábrica

**Vertical (`WIDGETS_DEF`, referencia 378×800):**

| Widget | x | y | w | h | Fracciones (x, y, w, h) |
|---|---|---|---|---|---|
| `velocimetro` analógico, redondo | 21 | 0 | 333 (diámetro) | — | 0,0556 · 0 · 0,8810 · — |
| `tipo` | 0 | 246 | 105 | 88 | 0 · 0,3075 · 0,2778 · 0,1100 |
| `necesaria` | 276 | 242 | 102 | 92 | 0,7302 · 0,3025 · 0,2698 · 0,1150 |
| `cuenta` números, `ancho 1.2`, con extra y barra | 0 | 338 | 378 | 141 | 0 · 0,4225 · 1 · 0,1762 |
| `ultima` | 0 | 485 | 378 | 52 | 0 · 0,6062 · 1 · 0,0650 |
| `marcar` | 0 | 543 | 378 | 209 | 0 · 0,6787 · 1 · 0,2612 |

- Tipo y sugerida van en las esquinas de abajo del velocímetro, a los costados del círculo.
- Última vuelta queda justo entre la cuenta y MARCAR, con 6 px de separación y los mismos bordes.
- **Ojo en la Raspberry:** el teléfono es más angosto (≈ 378:800) que la pantalla vertical de la Pi (720:1280). Como el diámetro del redondo es fracción del **ancho**, en 708×1268 mide 624 px de alto y **se encima con la cuenta** (que empieza en 536 px). Hay que reacomodar el armado vertical de fábrica para la Pi (y mostrar muestra a Matías).

**Horizontal (`WIDGETS_DEF_H`, referencia 800×378):**

| Widget | x | y | w | h | Fracciones (x, y, w, h) |
|---|---|---|---|---|---|
| `cuenta` números, `ancho 1.2`, con extra y barra | 3 | 3 | 795 | 164 | 0,0037 · 0,0079 · 0,9938 · 0,4339 |
| `tipo` (encima de la cuenta, a la izquierda) | 3 | 3 | 109 | 101 | 0,0037 · 0,0079 · 0,1363 · 0,2672 |
| `necesaria` (encima de la cuenta, a la derecha) | 676 | 65 | 122 | 102 | 0,8450 · 0,1720 · 0,1525 · 0,2698 |
| `velocimetro` **digital** (cuadrado) | 3 | 175 | 206 | 200 | 0,0037 · 0,4630 · 0,2575 · 0,5291 |
| `ultima` | 215 | 175 | 140 | 200 | 0,2687 · 0,4630 · 0,1750 · 0,5291 |
| `marcar` | 361 | 175 | 437 | 200 | 0,4512 · 0,4630 · 0,5463 · 0,5291 |

- Fila de abajo: mismo alto y 6 unidades de separación.
- Sin redondos, así que en 1280×720 se ve igual que en el teléfono.

### 6.8 Editor de pantalla (interacción)

- **Tocar** un widget (sin mover más de 12 px) lo **elige**: borde punteado amarillo y asas.
  - Cuadrado: 4 asas en las esquinas.
  - Redondo: 1 asa a 45° abajo a la derecha, sobre el borde.
  - Tocar el vacío lo deselecciona.
- El toque cuenta solo dentro de la forma real (los redondos son círculos). Gana el que está más adelante.
- **Mantener apretado 350 ms** levanta el widget (opacidad 0,6, vibra 30 ms) y se arrastra.
- **Asa:** cambia el tamaño. Mínimo 60×44 px; redondo, mínimo 70 px de diámetro (crece desde el centro).
- **Pellizco** con dos dedos sobre el elegido: escala desde el centro (mínimos 60×44).
- **Imán:** al mover, se pega (8 px) a bordes y centros de los otros widgets y del tablero (0, mitad, borde). Muestra líneas guía punteadas fucsia (`#ff3df0`).
- Al soltar: se recorta para que quepa en el tablero, se guarda en fracciones y se redibuja. El imán **no** se usa al cambiar de tamaño.
- **Botón ✎** (flotante) abre el panel:
  - ▯ Vertical / ▭ Horizontal (gira la pantalla para editar ese armado).
  - Menú del widget elegido, «Widget: nombre»:
    - **Estilo** (pasa al siguiente estilo).
    - **Redondo / Cuadrado.** A redondo: diámetro = el menor entre ancho y alto. A cuadrado: alto = diámetro (cuadrado real).
    - **Dato extra / Sin dato extra.**
    - **Barra ± / Sin barra ±** (cuenta que no sea anillo; velocímetro cuadrado).
    - **Números − / Números +** (no en MARCAR, velocímetro redondo ni delta barra).
    - **✕ Quitar.** MARCAR no se puede quitar sin pulsador asignado: «Para quitar MARCAR, primero asigná un pulsador en Ajustes → Pulsador».
  - **+ Agregar widget…** (lista). MARCAR solo aparece si no está. El nuevo va a:
    - velocímetro: redondo, `caja(80, 280, 220, 220)`;
    - otros: `caja(70, 340, 240, 90)`, con el primer estilo;
    - MARCAR: su lugar de fábrica.
    - Nota: usa la caja **vertical** también en horizontal (así está en el código).
  - **Restablecer** (doble toque, «Tocá otra vez para restablecer»): vuelve a fábrica **solo la orientación actual**.
  - **Listo**: sale al inicio.
- Todo cambio se guarda al instante.

---

## 7. Marcar y pulsador

### 7.1 Entradas

| Entrada | Detalle |
|---|---|
| Botón MARCAR | Al **apoyar** el dedo (`pointerdown`), no al soltar |
| Teclas por defecto (nativa) | Códigos Android `66` Enter, `160` Enter numérico, `62` espacio, `139` F9 (`TECLAS_DEF`) |
| Teclas en la web | Enter, espacio, F9, Enter numérico. Ignora la repetición de tecla |
| Pulsador aprendido | Una sola tecla. Reemplaza a las de fábrica |

- La hora de la marca se toma **antes que nada**. Desde lo nativo llega `edadMs` (cuánto hace que se apretó) y se resta: `t = ahora − edadMs`.
- **Aprender botón:** «Apretá ahora el botón del pulsador…». La próxima tecla queda asignada: guarda código, nombre de la tecla y nombre del dispositivo. Cartel «Botón asignado: X».
- **Usar teclas por defecto:** borra lo aprendido.
- Texto de estado: «Botón asignado: X (dispositivo).» o «Teclas por defecto: Enter, espacio y F9.»
- Pulsador Bluetooth asignado: si se desconecta, dice «Pulsador desconectado» y cartel «Pulsador desconectado: usá el botón de la pantalla». Al reconectar: «Pulsador conectado».

### 7.2 Qué pasa al marcar (`marcar`)

| Situación | Resultado |
|---|---|
| Editor de pantalla | Nada |
| Chequeo, esperando la prueba | Prueba del pulsador (ver 10) |
| Sin carrera en curso y fuera del crono (o con Ajustes abierto, o sin carrera elegida) | Nada. **Nunca se larga sin querer** |
| Crono, sin carrera en curso | **LARGADA** (abajo) |
| En carrera, antes del antirrebote | Ignorada: cartel «Marca ignorada (antirrebote)», **sin clic** |
| En carrera | Cierra la vuelta y empieza la siguiente |
| Cierra la última vuelta (`n ≥ vueltasTotal`) | Guarda, dice «Carrera completa», termina |

**Al largar:**
1. Crea el motor de esta largada (`nuevaCarrera`).
2. Si la carrera no estaba guardada (práctica), la guarda.
3. Borra «Última vuelta», mejor diferencia y máxima.
4. Modo carrera: pantalla protegida; No molestar y pantalla completa si `modoCarrera`.
5. Volumen al máximo si `volumenMax`.
6. Pantalla siempre encendida.
7. Marca en el motor, clic, y le pasa la **última lectura del GPS con la hora de la marca**, para que la distancia arranque justo en la largada.
8. Vibra 40 ms y efecto del botón.

### 7.3 Terminar (`terminarCarrera(motivo)`)

- Manual: **Terminar** con doble toque → motivo «Carrera terminada».
- Automático: última vuelta → «Carrera completa» (y lo dice en voz).
- Hace: `motor.terminar()`, suelta pantalla encendida, devuelve el volumen, sale de modo carrera.
- Cartel «<motivo>: N vueltas guardadas» y va al **detalle**.
- La vuelta en curso al apretar Terminar **se pierde** (no se guarda).

---

## 8. Indicador de vuelta, modo sol y modo carrera

### 8.1 Indicador medida / práctica

- Solo con la carrera en curso.
- `indicador = 'fondo'` (fábrica): **todo el fondo** de la pantalla toma el color (`colMedida` rojo `#b3132b` o `colPractica` verde `#0f6e3a`). Los widgets siguen con su cara; el fondo propio del crono se vuelve transparente. Transición de 0,25 s.
- `indicador = 'marco'`: un marco interior de 7 px del color (11 px en modo sol).
- Antes de largar: fondo normal del tema.

### 8.2 Modo sol

- `si`: siempre. `no`: nunca. `auto` (fábrica): con el sensor de luz.
- **Automático:**
  - más de **8000 lux** sostenidos **3 s** → se prende;
  - menos de **2000 lux** sostenidos **5 s** → se apaga;
  - entre 2000 y 8000 → se reinician los dos contadores (no cambia).
- Qué cambia: fondo `#fbfaf5`, tinta negra, etiquetas en negrita, widgets blancos con borde negro de 4 px sin brillo ni reflejo, MARCAR negro con letras blancas, delta con fondo negro y letras `#ffd400`, sugerida `#d99a00`, zona `#0a9a3c`, alerta `#a33a00`. Le gana a cualquier tema.

### 8.3 Modo carrera (Android)

- Siempre en carrera: «pantalla protegida» (los botones de volumen no hacen nada, se ve sobre el bloqueo, vuelve sola si se apaga).
- Si `modoCarrera`: además No molestar y pantalla completa. Si falta el permiso: «Para silenciar llamadas y notificaciones, permití «No molestar» en Ajustes → Modo carrera» (una vez).
- Ver sección 13 para la Raspberry.

---

## 9. Informe

### 9.1 Datos que usa

Por carrera: nombre, fecha, ideal, distancia y **todas** las vueltas de todas las largadas (n, largada, medida, realMs, difMs, velPromKmh, velMaxKmh, distM, traza).

### 9.2 Armado del recorrido (`armarRecorrido`)

```
vs = vueltas con traza de ≥ 5 filas;  si no hay → sin recorrido
umbral = max(10, cfg.umbralPrecisionM)
p0 = primera fila de vs[0] con prec ≤ umbral (o la primera)
proyección plana en metros: x = (lon − lon0) · 111320 · cos(lat0), y = (lat − lat0) · 110540

// 1) puntos buenos por vuelta
para cada vuelta:
  L = max(50, distM o d de la última fila o 1)
  cada fila → { f = min(1, d / L), q = (x,y), ok = prec ≤ umbral }
  descartar saltos imposibles, comparando con el último punto bueno:
    dt = max(0.5, t − t_prev); dd = distancia entre q
    si dd/dt > max(kmh, kmh_prev)/3.6 · 1.6 + 8 + (prec + prec_prev)/dt → ok = false

Lm = promedio de L;  B = clamp(round(Lm / 5), 40, 1500)     // tramos de ~5 m

construir():
  para cada punto ok: tramo = min(B−1, floor(f·B)); peso = 1 / max(3, prec)²
  P[i] = (mediana ponderada de x, mediana ponderada de y) del tramo, o vacío
  cerrada = distancia(primer lleno, último lleno) < max(40, Lm · 0.04)
  tramos vacíos: interpolar entre el lleno anterior y el siguiente
     (si es circuito cerrado, dando la vuelta; si no, copiar el vecino que haya)
  suavizar: 3 pasadas de promedio móvil de ±4 tramos (circular si cerrada)
  return P

pista = construir()
// 2) encaje: cada punto ok busca el tramo de la pista más cercano dentro de ±max(3, round(B·0.06))
//    alrededor de su tramo, y su f pasa a (mejor + 0.5)/B
pista = construir() (con los f corregidos)  // si falla, queda la anterior
```

- Mediana ponderada: ordenar por valor y devolver el primero donde el peso acumulado llega a la mitad.
- Las vueltas que perdieron señal se completan con las otras. **No se inventan velocidades:** los puntos sin señal quedan marcados.
- El comentario del código dice «dos pasadas», pero hace **3**.

### 9.3 Datos para dibujar (`datosInforme`)

- Pista escalada a un lienzo de **364×330**, norte arriba, margen 20, centrada.
- Por cada punto de cada vuelta: `[t, d, kmh, acel, adelanto, x, y, ok]`.
  - `acel` (m/s²) = diferencia de velocidad entre el vecino de antes y el de después / su diferencia de tiempo (o con el anterior si no hay siguiente). Recortada a ±9, 2 decimales.
  - `adelanto` (s) = `d / (D/T) − t` (positivo = adelantado). 0 si no hay distancia.
  - `x, y` = posición **sobre la pista común** según su `f` (interpolando), no la cruda.
  - `ok` = 1 si el punto es bueno **y** no hubo más de 4 s sin lecturas antes.

### 9.4 Plantilla del informe (HTML de una sola página)

- Fondo `#121417` con rayas diagonales; ancho máx. 560 px. Título «Betómetro · nombre». Favicon de cronómetro.
- Colores: rojo `#d7263d`, amarillo `#f5c518`, verde `#2ecc71`, azul `#3d8bfd`, celeste `#7fd3ff`, texto `#eef0f2`, secundario `#8a929b`, bordes `#2c3137`.
- **Cabecera:** nombre; «sábado, 1 de noviembre de 2026 · ideal 4:00,0 · 3.200 m».
- **Chips de vueltas** con recorrido: `V3` (+ ` L2` si hubo varias largadas) y «medida/práctica». Empieza en la primera con datos.
- **4 datos** de la vuelta elegida: TIEMPO · VS IDEAL (color por adelanto) · KM/H PROM · KM/H MÁX (máxima de los puntos buenos).
- **Lectura fija arriba** del punto elegido: DISTANCIA (m) · KM/H · M/S² · adelanto en s con ADELANTADO/ATRASADO (o TIEMPO si no hay distancia).
- **Mapa** (364×330): pista común en gris `#3a3f45` (trazo 11), encima la vuelta coloreada (trazo 8, solo tramos con señal). Punto blanco «LARGADA» al inicio de la pista. Marca «frenada −x,x» en la mayor frenada si es menor a −0,3 m/s².
  - Modos: **Velocidad**, **Aceleración**, **Adelanto** (este solo si hay distancia).

| Modo | Colores |
|---|---|
| Velocidad (`v − sugerida`) | > +6 rojo «muy rápido» · > +2 amarillo «rápido» · entre ±2 verde «en sugerida» · < −2 celeste · < −6 azul «muy lento» · sin señal `#555`. Sin distancia: todo verde |
| Aceleración | > 0,8 verde · > 0,25 `#8fe3b0` · < −0,8 rojo · < −0,25 `#f08a96` · si no `#9aa3ad` («parejo») |
| Adelanto | \|s\| < 1 verde · < 3 amarillo · más rojo |

- **Gráfico de velocidad** (364×160): eje x = distancia (`x = 30 + 328·d/L`); escala `max(20, ceil(max(v, sugerida+10)/10)·10)`, 4 divisiones; sugerida en línea blanca punteada; huecos sin señal como línea gris abajo. Texto: «sugerida 48,0».
- **Gráfico de aceleración** (364×150): barras verdes (acelera) y rojas (frena) desde el centro, escala `max(0,5, |a| máx)`. Texto: «máx +x,x · frenada −x,x».
- **Cursor:** arranca en un tercio de la vuelta. Tocar o deslizar de costado en los gráficos lo mueve por distancia; tocar el mapa elige el punto más cercano. Línea blanca en los gráficos y círculo blanco en el mapa.
- **Tabla «Todas las vueltas»:** VUELTA · TIPO (punto rojo medida / verde práctica) · TIEMPO · DIF. (color por adelanto) · PROM (1 decimal) · MÁX (entero). Fila final «Error total · N medidas» con la suma. Arriba a la derecha «promedio X». Tocar una fila con recorrido la elige y sube.
- Sin trazas: «Esta carrera no tiene el recorrido grabado. Se graba con el GPS en las carreras corridas desde la versión 1.8 de Betómetro.» (la tabla se muestra igual).
- Barra de desplazamiento propia a la derecha (20 px, se arrastra). Pie: «Generado con Betómetro».
- Formato de tiempo en el informe (`tfmt`): `m:ss,d` con **coma** (`h:mm:ss,d` desde 1 hora).

### 9.5 Imagen de vista previa (`imagenInforme`, PNG 1080×1080)

- Fondo `#121417` con rayas finas.
- «BETÓMETRO · INFORME DE CARRERA» (rojo, 30 px), nombre (64 px, máx. 26 caracteres), «1 de noviembre de 2026 · ideal 4:00.0 · 3.200 m».
- Mapa en la caja (90, 250, 900×560): pista gris (30 px) y la **primera vuelta medida con datos** (o la primera con datos) coloreada por velocidad (20 px). Punto blanco de largada. «Vuelta N · coloreada por velocidad».
- Sin recorrido: «Resultados de la carrera».
- Abajo, 3 recuadros: VUELTAS (`corridas/total`), ERROR TOTAL, MEJOR VUELTA.

### 9.6 Compartir

- Nombre: `🏁 <nombre> dd-mm-aaaa` (nombre sin `\ / : * ? " < > |`, hasta 40 caracteres).
- Se comparten `.png` (primero) y `.html`.
- **No** hay CSV ni JSON (decisión de Matías).

---

## 10. Chequeo previo

Se actualiza cada **2 s**. Cada fila: ✓ (verde), ! (rojo) o – (neutro), nombre y texto.

| Fila | Bien | Mal | Neutro |
|---|---|---|---|
| Pulsador | Hay botón asignado: «Botón asignado: X.» Si es Bluetooth y está conectado: «X: conectado.» | Bluetooth asignado sin conectar: «X: NO conectado. Prendelo o revisá el Bluetooth.» | Sin asignar: «Solo botón en pantalla y teclas por defecto (Enter, espacio, F9).» |
| GPS | Lectura de hace ≤ 3 s y precisión ≤ umbral: «Precisión N m.» | Sin lectura en 3 s: «Sin señal de GPS todavía. Esperá al aire libre.» · Precisión mala: «Precisión N m: todavía es mucho, esperá.» | Velocidad apagada: «Velocidad desactivada en Ajustes.» |
| Voz | El texto tiene «60 de 60» o «disponible» | «preparando…» o «no tiene» voz | otro. Ojo: se pregraban 61 textos («cero» + 1–60), así que en nativa el texto sería «61 de 61» y quedaría neutro. No está claro en el código cuánto da `total` |
| Batería | ≥ 30 % o cargando | < 30 %: «N %: conviene cargar antes de largar.» | no se pudo leer |
| Ubicación precisa (solo Android) | Activada | «Desactivada: Ajustes → Velocidad → Activar ubicación precisa.» | — |
| No molestar (solo Android) | «Al largar se silencian llamadas y notificaciones.» | «Falta el permiso: Ajustes → Modo carrera.» | no se pudo consultar |
| Datos | Siempre bien: «Base de datos del teléfono, con respaldo.» | — | — |

- **Probar el pulsador:** botón «Probar pulsador» → «Esperando… apretá el pulsador ahora.» La próxima marca **no larga nada**: «¡Recibido! Llegó por el pulsador / el teclado / la pantalla.» y pitido 1320 Hz 200 ms.
- No hay un «TODO LISTO» en la app (es nuevo en la Raspberry).

---

## 11. Actualizador y arranque

### 11.1 Actualizador

- Consulta `https://api.github.com/repos/<repo>/releases/latest`. Versión = `tag_name` sin la «v». Busca un adjunto `.apk`.
- Compara `X.Y.Z` número por número (`cmpVer`).
- **Al abrir**, a los 4 s busca en silencio. Si hay nueva: en el inicio aparece «· Actualizar a X.Y.Z» en amarillo, y en Ajustes «Hay una versión nueva: X.Y.Z.» con el botón «Descargar e instalar X.Y.Z».
- Tocar el aviso del inicio **instala directo**.
- Botón de Ajustes: «Buscando…» → «Tenés la última versión.» o «No se pudo buscar: <error>. Revisá la conexión a internet.»
- Instalar:
  - con carrera en curso: «Terminá la carrera antes de actualizar.» y no hace nada;
  - progreso «Descargando… N %» (en Ajustes **y** en el aviso del inicio);
  - al final: «Descarga completa: confirmá la instalación.»
- Además, Android avisa una vez por día con una notificación (una vez por versión).
- **No** actualiza solo: siempre lo pide el usuario.

### 11.2 Arranque

1. Ubica el inicio, aplica tema y colores, estado del pulsador.
2. Programa la búsqueda de actualización (4 s).
3. Arma el tablero y empieza a dibujar (cada cuadro).
4. Carga los datos. Mezcla `DEF` + lo guardado, corre `fijarCfg` y la migración Avispón.
5. Aplica tema, teclas, fuente de velocidad (prende el GPS si `velActiva`), sensor de luz y canal de audio.
6. A los 1,5 s pide ubicación precisa si falta; a los 7 s pide permiso de notificaciones.
7. Muestra el inicio.

- El GPS queda **prendido todo el tiempo** que la app está abierta (si `velActiva`), no solo en carrera. Así el chequeo y el velocímetro en reposo tienen dato.

---

## 12. Simulador de velocidad (`Simulador`)

- Una lectura por segundo: `{ t, velMs = simVelKmh/3,6, precisionM = 5 }`, sin lat/lon (no graba traza).
- `simModo = 'variable'`: `v = simVelKmh · (1 + 0,12·sin(s/9) + 0,06·sin(s/2,7))`, con `s` = segundos desde que arrancó.
- `simRuido`: suma ±1,5 km/h al azar; precisión entre 4 y 12 m, y un 8 % de las veces 45 m (para probar el descarte).
- Control: deslizador 0–160, botones −/+ de a 2 km/h.
- **En la 2.1.1 está apagado:** `actualizarFuente` tiene `sim = false` fijo y la versión web no tiene GPS. O sea que hoy la web de prueba **no tiene ninguna velocidad**.
- En la Raspberry conviene tenerlo como fuente de prueba (por ejemplo, una opción oculta o de desarrollo), entrando por el mismo camino que el GPS.

---

## 13. Qué cambia en la Raspberry

| Cosa de Android | En la Raspberry |
|---|---|
| Play Protect | No aplica |
| Permisos (ubicación precisa, notificaciones, instalar, mostrar encima) | No aplica. Sacar sus filas del chequeo y sus botones de Ajustes |
| No molestar / modo carrera | No aplica (no hay llamadas). El ajuste `modoCarrera` se va |
| Pantalla protegida, *wake lock*, que la pantalla vuelva sola | La pantalla **nunca se apaga** (sin protector de pantalla). No aplica |
| Fijar la app | No aplica: Betómetro es el sistema |
| Abrir al prender (`BOOT_COMPLETED`) | Siempre: el equipo arranca directo en Betómetro |
| Botón atrás de Android | No hay: cada pantalla tiene su «Volver» con las mismas reglas de 5.0. En el inicio no se «cierra la app»: hay **Apagar el equipo** |
| Orientación por sensor (crono libre, menús en vertical) | La **elige el usuario** (inicio o Ajustes). Toda la interfaz usa esa. Cada orientación con su armado |
| GPS del teléfono (`BackgroundGeolocation`) | GPS por UART, **10 Hz**. Misma muestra `{t, velMs, precisionM, lat, lon}`. Velocidad Doppler del GPS; si no viene, calcular entre puntos (como `GpsReal`). Revisar el umbral y la regla de 3 s con 10 Hz |
| Precisión `accuracy` de Android | Sacarla del GPS (por ejemplo, error horizontal estimado). **No está claro qué valor usar**: definirlo |
| TextToSpeech / `speechSynthesis` | **Piper pregrabado**: «cero», 1–60 y todas las frases de 3.3 (incluidas «Vuelta 1…N, medida/práctica» y «Última vuelta, …»). Anticipo de fábrica: como el nativo (40 ms) hasta medirlo |
| Canal alarma / multimedia, volumen al máximo | No aplica. Una sola salida (amplificador I2S). Volumen fijo alto o un ajuste de volumen |
| Medir demora de la voz | Opcional: el audio es pregrabado. Mantener «Probar cuenta de 5 s» |
| Pitidos y clic | Archivos de sonido cortos pregrabados (`#corto`, `#largo`, `#tic`, `#clic`) |
| Teclas, control selfie, «Aprender botón» | Pulsador en **GPIO** con su antirrebote eléctrico. «Aprender botón» no hace falta. Se puede aceptar además un teclado USB. MARCAR en pantalla sigue siendo obligatorio si no hay pulsador (definir cómo se sabe que hay uno) |
| Pulsador Bluetooth conectado/desconectado | No aplica (Bluetooth apagado). Opcional: avisar si el GPIO no responde en la prueba |
| Sensor de luz (modo sol automático) | No hay sensor. Modo sol `si`/`no` (`auto` no aplica) más el ajuste nuevo de **brillo** |
| Batería | No hay batería. La fila se cambia por las nuevas: hora del GPS, lugar libre, wifi |
| SQLite + `betometro.json` | Archivo JSON en la partición de datos, con escritura segura (archivo nuevo y reemplazo). Mismo formato de 1.1 |
| Exportar respaldo | Por wifi con QR (igual que el informe). **No está claro** si se quiere: preguntar |
| Notificación diaria de versión nueva | No aplica. Se busca al arrancar si hay wifi y se muestra «· Actualizar» en amarillo |
| Descargar e instalar APK | Descargar el paquete del sistema desde las *releases* del repo de la Raspberry e instalar en la copia A/B. No se deja actualizar con carrera en curso |
| Compartir (archivos y link del APK) | Informe: QR por wifi. El botón «Compartir Betómetro» del inicio no aplica |
| Visor del informe (iframe con HTML) | Informe dibujado en Qt. El HTML (plantilla igual) se genera solo para compartir |
| Vibración al marcar | No aplica. Queda el clic y el efecto visual |
| Hora (`Date.now`, fecha de hoy) | Tomarla del GPS (la Pi no tiene reloj). **Ojo:** LARGAR solo se habilita si la fecha de la carrera es hoy; sin hora válida no se puede saber |
| Reloj monotónico (`performance.now`) | Reloj monotónico de C++ (`steady_clock`). La hora de la marca se toma en la interrupción del GPIO |
| Ajustes como panel encima | Pantalla propia, con los grupos nuevos (Mi auto, Wifi, Apagar, Orientación, Brillo) |
| Temas | Los mismos cuatro y el modo sol (ver diseno.md), más el **tema propio** (Personalizar desde el celu, 08/11/2026). Ver «Decisiones del 08/11/2026» en diseno-sistema.md |
| Cantidad de vueltas de cada carrera (`vueltasTotal`, «Última vuelta», «Carrera completa») | **No hay** (Matías, 02/11/2026): en la carrera pueden agregar vueltas. Se termina a mano. La carrera pasa a «corridas» cuando tiene vueltas y su fecha pasó; el mismo día se puede volver a largar |
| Vuelta medida o práctica según el patrón | Igual al correr, pero se dice **LIBRE** en todos lados (pantalla, voz «Vuelta 2, libre», informe). **En el informe** cada vuelta tiene la etiqueta MEDIDA / LIBRE: se toca y cambia. Las libres (por patrón o a mano) **nunca suman error** (Matías, 02/11/2026) |
| Marca de más (dos marcas seguidas parten una vuelta) | **En el informe:** se toca la vuelta y aparece «Unir V5 con V6» (pide tocar dos veces). Junta tiempo, distancia y recorrido; queda el tipo de la primera y se renumeran las siguientes. Sin aviso en voz durante la carrera (Matías, 02/11/2026, después de la práctica real de Beto del 31/10) |
| Cuenta regresiva (widget) | «Cronómetro de la vuelta» con dos modos: **progresiva** (de fábrica: cuenta para arriba, rojo al pasar el ideal) o regresiva (como la app). Se cambia en el editor (✎). Mismas funciones: dato extra y barra ± (Matías, 02/11/2026) |
| Tiempo ideal a la vista | Siempre en la línea de arriba y, chiquito, en la esquina del cronómetro: «IDEAL 4:00,0» (Matías, 02/11/2026) |
