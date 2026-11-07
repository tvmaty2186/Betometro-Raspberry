# Auditoría 02 — Código C++ y Qt (07/11/2026)

**En corto:** el código está prolijo. El guardado en otro hilo, el cierre con SIGTERM y el video entre hilos están bien hechos.
Encontré **un cierre seguro con datos raros** (informe), **una grabación que puede quedar trabada** (cámara) y
**procesos del wifi sin tiempo límite**. Los tres se arreglan con poco.

Qué corrí: todo compilado con `-fsanitize=address,undefined -Wall -Wextra` en Docker (`/tmp/bd-asan02`);
`ctest` 5 de 5 bien; el programa sin pantalla en 5 pantallas y una carrera de 70 s con GPS simulado, cerrado con
SIGTERM: sin errores de memoria ni pérdidas. Además, 3000 bases dañadas a propósito contra el informe y 200 000 tiras de bytes
basura contra el lector del GPS.

## 1. Lo más grave (puede cerrar o trabar el programa)

**1.1 El informe se cae con una distancia negativa en la traza** — confirmado
- Dónde: `app/nucleo/informe.cpp:70` (`std::min(1.0, p[1] / V.L)` no tiene piso), y después `:92` y `:153` usan ese valor como índice.
- Qué pasa: si una vuelta guardada trae una distancia menor que 0 (archivo dañado o editado, una base que venga de afuera),
  el informe escribe fuera de la memoria y **el programa se cierra**. Lo reproduje con ASan; es el único cierre que encontraron las 3000 bases dañadas.
- Dónde pega: en la pantalla del informe, en `/informe` y el zip del celu (`servidor.cpp:140`, `:191`), y **solo en Drive**
  (`drive.cpp:363`, `:475`, arma los informes de todas las carreras). Con Drive prendido sería un cierre cada vez que conecta el wifi.
  Nuestro motor nunca guarda distancias negativas (`modelo.cpp:14`), así que hoy solo pasa con datos raros.
- Propongo: `std::max(0.0, std::min(1.0, …))` en la línea 70. Con eso, las 3000 bases dañadas pasaron sin cierres. Sumar esa prueba a `ctest`.
- Cuesta: **chico**.

**1.2 La cámara puede quedar «grabando» para siempre** — posible
- Dónde: `app/camara.cpp:95` y `:304` paran con `terminate()` una sola vez y nunca usan `kill()`.
  `app/camara/betometro_camara.cpp:64-67` manda «fin de video» y espera sin límite a que GStreamer conteste.
- Riesgo: si una webcam o un micrófono USB se traba, el ayudante no termina nunca. `grabando()` queda en sí: no se puede volver a grabar,
  no se pasa a MP4, Drive cree que está ocupado y no sube nada. Hasta reiniciar.
- Propongo: en el ayudante, cortar solo 8 s después del «fin»; en `Camara`, matarlo si sigue vivo 12 s después del `terminate()`.
- Cuesta: **chico**. No lo pude provocar sin una cámara real.

**1.3 Wifi: `nmcli` e `iw` sin tiempo límite y sin freno** — posible
- Dónde: `app/wifi.cpp:27` (cada 10 s), `:45-56` y `:171-216`. Cada vuelta lanza 3 `nmcli` + 1 por red guardada + `iw`,
  sin esperar a que terminen los anteriores. **También en carrera.**
- Riesgo: si NetworkManager se traba, los procesos se acumulan (D-Bus corta a ~25 s, así que no crecen sin fin).
  Igual, son 5 o 6 programas cada 10 s en la Pi 3 mientras corre.
- Propongo: no empezar otra vuelta si la anterior sigue en curso; matar el `nmcli` que pase 15 s; en carrera, revisar cada 60 s.
- Cuesta: **chico**.

**1.4 Cosas que traban la pantalla (el vigilante reinicia a los 30 s)**
- `app/actualizador.cpp:155-172`: `mount`/`umount` del pendrive con `QProcess::execute`, sin límite. Un pendrive malo puede pasar los 30 s y
  systemd reinicia Betómetro. No pasa en carrera.
- `app/camara.cpp:272-273`: al grabar desde Ajustes de cámara espera hasta 4 s a que se suelten la vista y el micrófono.
- `app/reportes.cpp:86-89`: `journalctl` al prender, hasta 4 s (y si no terminó, `~QProcess` sigue esperando).
- Propongo: pasarlos a `start()` + señal `finished`, con un límite de tiempo. Cuesta: **medio**.

## 2. Mucha mejora con poco trabajo

- **Avisos del compilador solo en el núcleo** (`CMakeLists.txt:23`). Compilé todo con `-Wall -Wextra` y está limpio: solo 6 «missing initializer»
  en `programador.cpp:23-45` (inofensivos) y una función vieja de GLib (`betometro_camara.cpp:98`). Propongo prender
  `-Wall -Wextra` también en el programa y en la cámara. **Chico.**
- **Los números del JSON dependen del idioma del sistema** — latente. `json.cpp:101` y `:312` usan `snprintf`/`strtod`, que siguen
  el idioma (Qt lo toma del sistema al arrancar). Hoy anda porque `betometro.service` pone `LANG=C.UTF-8`. Si algún día la Pi queda
  en `es_AR`, el decimal sería coma y **la base se escribiría mal**. Propongo `setlocale(LC_NUMERIC, "C")` en `main.cpp` después de
  crear la app, o pasar a `std::to_chars`/`std::from_chars`. **Chico.**
- **Relojes del cronómetro siempre prendidos**: `crono.cpp:31-38` despierta 20 + 66 veces por segundo aunque no haya carrera.
  Prenderlos solo con una carrera preparada. **Chico.**
- **GPS con puerto fijo** (`--gps /dev/…`): `gps.cpp:196-198`, si se desenchufa, el aviso queda prendido y el procesador se va al 100 %.
  En la Pi se usa `auto` y no pasa, pero conviene apagar el aviso igual. **Chico.**

## 3. El resto (menor)

- `servidor.cpp:79-82`: sigue juntando lo que manda el celu aunque ya se haya contestado, y una conexión a medio pedido no tiene límite de
  tiempo. Riesgo bajo (red propia). **Chico.**
- `actualizador.cpp:191-195`: la descarga no tiene límite de tiempo (las otras sí, 15 s). Si el wifi se cae a mitad, queda «descargando»
  hasta reiniciar. **Chico.**
- `camara.cpp:167`: al cerrar no corta la conversión a MP4 (`mp4_`); Qt la mata y queda un `.mp4.parte`. **Chico.**
- `modelo.cpp:19`: al llegar a 20 000 puntos, cada muestra borra el primero y mueve ~1 MB. Solo en vueltas larguísimas. **Chico.**
- `datos.cpp:73`: cada cambio (también deslizar el brillo) copia toda la base en el hilo principal. Lo ve la auditoría 05.

**Lo que está bien:** el guardado en otro hilo (`datos.cpp:72-105`: copia, candado, gana el último, termina de escribir al cerrar);
el guardado a prueba de cortes (`almacen.cpp`, con `fsync` y `.anterior`); el orden en que se cierra todo en `main.cpp`;
`VideoGst`/`ImagenVideo` (candado + `seVa` para no usar una imagen ya borrada); los lambdas siempre con `this` como contexto;
JSON con límite de profundidad; el lector del GPS aguantó 200 000 tiras de basura sin un error.

## Las 3 que haría primero
1. El piso en `informe.cpp:70`, más la prueba de bases dañadas en `ctest` (1.1).
2. Matar la cámara si no termina sola (1.2).
3. Freno y tiempo límite para `nmcli`, y menos chequeos de wifi en carrera (1.3).

## Preguntas para Matías
- ¿Pensás pasar alguna vez la base de la app Android a la Raspberry? Eso traería datos que el informe no espera.
- ¿Alguna vez la cámara quedó «grabando» y no pudiste volver a grabar sin reiniciar?
- ¿Usás el wifi del equipo durante la carrera, o se puede chequear menos seguido mientras corrés?
