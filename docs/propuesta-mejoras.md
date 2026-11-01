# Propuesta: conectividad y mejoras por usar la Raspberry

Estado: **propuesta (01/11/2026), falta que Matías elija.** Nada de esto está programado.

Salió de lo que pidió Matías: un menú de conectividad para la wifi de casa, ver todos los informes por la red y revisar qué se puede hacer mejor que en el celular, sobre todo con el GPS.

---

## 1. Conectividad

### 1.1 Menú «Conectividad» (en Ajustes)

Reemplaza a la fila «Wifi» de Ajustes. Es una pantalla propia (la 9, ampliada):

- **Estado:** conectado a «Casa», señal, dirección en la red (`betometro.local`).
- **Buscar redes:** lista con la señal y un candado si tiene clave. Se toca una y se escribe la clave con el teclado en pantalla (con ñ y un ojo para ver lo que escribiste).
- **Redes guardadas:** la de casa y la zona wifi del celular. Se conecta sola a la que encuentre. Se puede «Olvidar» una red.
- **Hora:** con internet, la hora se pone en hora sola (además de la del GPS).

### 1.2 Betómetro en la red (en casa)

Con la Raspberry en la wifi de casa, desde el celular o la compu se entra a **http://betometro.local**:

- Lista de **todas las carreras**, con fecha y vueltas.
- **Informe de cada una** (el mismo HTML de la app), para ver o descargar.
- **Descargar todos** juntos en un `.zip`.
- Solo se ve en la red de casa o del celular. **Solo lectura**: desde ahí no se borra ni se cambia nada.
- Se apaga sola durante la carrera (para no gastar nada del equipo).

El QR de un informe (pantalla 11) sigue igual; ahora lleva a esta misma página.

### 1.3 Wifi propia de la Raspberry (opcional)

Si no encuentra ninguna red guardada, la Raspberry puede armar **su propia wifi** («Betometro-101»). El celular se conecta directo y entra a la misma página. Sirve en la pista, sin depender de la zona wifi del celular.

### 1.4 Actualizar

Sin cambios: «· Actualizar» en amarillo cuando hay versión nueva. Nunca durante una carrera.

---

## 2. GPS: sacarle todo al NEO-M8N

El GPS del celular da 1 lectura por segundo y una precisión que inventa Android. El NEO-M8N da mucho más. Lo que propongo:

| # | Mejora | Qué se gana |
|---|---|---|
| G1 | **10 lecturas por segundo con GPS + GLONASS** (más satélites), en vez de solo GPS. Se confirma en E4 que el módulo lo sostiene | Más satélites → menos error y menos cortes entre árboles o edificios |
| G2 | **Hora de cada lectura según el reloj del GPS**, no la hora a la que llegó por el cable | Las lecturas quedan exactas cada 100 ms. La distancia sale más pareja |
| G3 | **PPS** (el pulso por segundo del GPS, pin 7) para atar el reloj de la Raspberry al del GPS | Se sabe cuánto tarda en llegar cada lectura y se corrige |
| G4 | Usar el **error de la velocidad** que da el GPS (`sAcc`), además del de la posición | La distancia sale de la velocidad: descartar las velocidades dudosas es lo que de verdad importa |
| G5 | **«Sin GPS» a 1 s** en vez de 3 s (con 10 Hz, 1 s son 10 lecturas perdidas) | Avisa antes cuando se pierde la señal |
| G6 | Solo lecturas con **posición 3D buena** (el GPS lo marca) | Se evitan lecturas sueltas malas al arrancar |
| G7 | **Arranque en caliente:** con la pila del módulo, el GPS guarda los satélites | Tiene señal en segundos al prender |

**Ojo:** G1, G2, G3, G6 y G7 están en la lectura del GPS: **el motor no cambia** y sigue dando igual que la app. G4 y G5 sí cambian el motor. Se hacen como agregado con su propia prueba y se puede volver a lo de la app desde Ajustes.

---

## 3. Otras mejoras por ser Raspberry

| # | Mejora | Qué se gana |
|---|---|---|
| R1 | **Hora de la marca tomada por el sistema** en el instante en que se aprieta el pulsador (no cuando el programa se entera) | Marca exacta al milisegundo. En el celular el Bluetooth agregaba demora variable |
| R2 | **Voz cargada en memoria**, salida de audio de baja demora | La cuenta suena justo, siempre con la misma demora |
| R3 | **Prueba de demora real** en el chequeo previo: se mide la voz contra el reloj | El anticipo se ajusta con números, no a oído |
| R4 | **Pantalla siempre prendida**, sin avisos ni llamadas | Nada interrumpe la carrera |
| R5 | **Guardado al instante** de cada vuelta, a prueba de cortes de luz | No se pierde una vuelta aunque se corte el contacto |

R1 a R5 ya estaban en el plan; se dejan escritas para que quede claro qué se gana.

---

## 4. Qué cambia en el plan si Matías dice que sí

- **E2:** pantalla de Conectividad (muestra visual primero) y la página «Betómetro en la red» con datos de prueba.
- **E4:** G1 a G7 con el módulo real y medición en el auto.
- **E5:** wifi real, `betometro.local`, wifi propia (si se elige) y actualizar.
