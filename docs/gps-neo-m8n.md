# GPS: el módulo NEO-M8N que vamos a comprar

Investigación del 01/11/2026, con las fotos del módulo que mandó Matías.

## Qué es la placa (de las fotos)

- **Chip:** u-blox **NEO-M8N-0-10**. Según la especificación del protocolo de u-blox, esa versión trae el **firmware SPG 3.01** (protocolo 18), guardado en su memoria **flash**.
- **Conectores:** SMA para la antena externa, micro USB, y 5 pines: **PPS, ?XD, ?XD, GND, VCC**. En la foto, la primera letra de los pines 2 y 3 queda tapada por la antena. Las placas de este tipo traen **PPS, TXD, RXD, GND, VCC**, mirando del lado de la antena cerámica. **Hay que leerlo en la placa al recibirla.**
- **En la placa:** regulador de 3,3 V (se alimenta con 3,3 a 5 V), pila de respaldo (guarda los satélites para arrancar rápido), memoria EEPROM 24C32, LED de PPS y **antena cerámica** atrás.
- **Antenas:** tiene la cerámica atrás **y** el conector SMA. No encontré el esquema para saber si están en paralelo. Se prueba al recibirla (abajo).

## Lo que dice u-blox (y qué cambió en el programa)

| Tema | Dato oficial | Cómo estaba | Cómo quedó |
|---|---|---|---|
| Lecturas por segundo | NEO-M8N: **5 Hz con GPS + GLONASS**; **10 Hz con un solo sistema** (GPS, GLONASS, BeiDou o Galileo) | 10 Hz con GPS + GLONASS (**no se puede**) | **GPS + GLONASS a 5 Hz** de fábrica. Opción `gpsSoloGps`: GPS solo a 10 Hz, para probar en el auto (E4) |
| Precisión de posición | 2,5 m (2,0 m con SBAS), igual con GPS + GLONASS que con GPS solo | — | — |
| Precisión de la velocidad | **0,05 m/s** | Se usa la velocidad Doppler (`gSpeed`) | Igual |
| Arranque de fábrica | UART a **9600** baudios, manda **NMEA** (GGA, GLL, GSA, GSV, RMC, VTG) | El lector abría directo a 115200 | Busca la velocidad sola (115200 o 9600), lo pasa a 115200 y lo configura |
| NAV-PVT | 92 bytes; campos y posiciones | — | Revisados uno por uno contra la especificación: coinciden |
| Configuración | CFG-PRT, CFG-GNSS, CFG-RATE, CFG-NAV5 (4 = automotor), CFG-MSG, CFG-CFG | Faltaba CFG-GNSS | Agregado (GPS + QZSS + SBAS, GLONASS; Galileo y BeiDou apagados). Se espera medio segundo después, como recomienda u-blox |
| PPS | 1 pulso por segundo, de 100 ms | — | Para medir demoras (opcional, pin 7) |
| Alimentación | Módulo: 2,7 a 3,6 V. Entradas y salidas a 3,3 V | — | La placa tiene regulador (3,3 a 5 V): va al **pin 17 (3,3 V)** de la Raspberry. El pin 4 no se puede usar: la ficha de la pantalla tapa los pines 2, 4 y 6. Los pines TX/RX son de 3,3 V, igual que la Raspberry |

¿Por qué GPS + GLONASS a 5 Hz y no GPS solo a 10 Hz?
- Más satélites: se pierde menos la señal entre árboles o tribunas.
- Para el motor 5 lecturas por segundo sobran (la app anda con 1 por segundo). La distancia sale de la velocidad, que es muy precisa.
- En E4 se prueban las dos en el auto y se elige con datos.

## Módulos truchos

Hay muchos NEO-M8N falsos: chips sin memoria flash, o hasta NEO-6M reetiquetados. No guardan la configuración y algunos no tienen el mensaje NAV-PVT.

Lo que hace ahora el programa:
- Le pregunta al módulo **qué es** (UBX-MON-VER) y lo muestra en el **Chequeo previo**: «NEO-M8N-0 · SPG 3.01» en verde si es original; «¿copia?» en rojo si no.
  - Original: software «**EXT CORE** 3.01» (corre desde la flash), hardware «00080000», «MOD=NEO-M8N-0».
  - Copia típica: «ROM CORE…» (sin flash) u otro hardware.
- **Configura el módulo en cada arranque**, así anda aunque no guarde nada.
- Si no llega ningún NAV-PVT, **vuelve a NMEA** (GGA, GST, RMC), que el lector también entiende.

## Repuestos: NEO-7M y NEO-6M (01/11/2026)

Son los que más se consiguen (placas GY-GPSV3-NEO y GY-NEO6MV2). **Sirven los dos**, con el mismo cableado de 4 cables (VCC, RX, TX, GND; no tienen PPS). El programa **reconoce solo qué módulo hay** (pregunta con MON-VER) y lo configura según lo que puede. Si no lo reconoce, en Ajustes → Velocidad y marcas → **Módulo GPS** se elige a mano (Auto, M8N, 7M, 6M).

| | NEO-M8N (el principal) | NEO-7M | NEO-6M |
|---|---|---|---|
| Sistemas | GPS + GLONASS a la vez | Uno solo (GPS) | Solo GPS |
| Lecturas por segundo | 5 (10 con GPS solo) | 10 | 5 |
| Precisión de la velocidad | 0,05 m/s | 0,1 m/s | 0,1 m/s |
| Mensaje que usa Betómetro | NAV-PVT (92 bytes) | NAV-PVT (84 bytes, mismos campos) | NMEA (GGA + GST + RMC) |
| Guarda la configuración | Sí (flash) | Con la pila de la placa | Con la pila de la placa |

- Los tres arrancan a 9600 con NMEA; el programa los pasa a 115200 y los configura en cada arranque.
- Con el 6M y el 7M hay menos satélites (sin GLONASS): se corta más fácil la señal entre árboles. **Conviene el M8N; los otros, de repuesto.**
- Las antenas de esas placas son cerámicas chicas con ficha U.FL. Para la antena del techo hace falta un cable U.FL a SMA.
- Fuentes: hoja de datos NEO-7 (UBX-13003830): https://content.u-blox.com/sites/default/files/products/documents/NEO-7_DataSheet_(UBX-13003830).pdf · protocolo u-blox 7 (NAV-PVT de 84 bytes): https://www.olimex.com/Products/Modules/GPS/MOD-GPS/resources/u-blox7-V14_ReceiverDescriptionProtocolSpec_Public_-GPS.G7-SW-12001-.pdf · hoja de datos NEO-6 (5 Hz): https://www.nyang-tech.com/datasheet/1894238505/u-blox/NEO-6M-0-001.pdf

## Al recibir el módulo (prueba en E4)

1. **Leer los pines** en la placa (TXD y RXD) y anotarlos acá.
2. **Chequeo previo:** tiene que decir «NEO-M8N-0 · SPG 3.01» en verde y 5 lecturas por segundo.
3. **Antena:** con la antena externa en el techo, comparar la señal tapando la cerámica con la mano. Si la cerámica sigue influyendo, están en paralelo: conviene sacar la cerámica (se desuelda un pin) o no conectar la externa.
4. **Antena activa:** ver si el conector SMA da tensión (3,3 V) para la antena activa. Si no la da, la antena activa no amplifica.
5. Probar **GPS solo a 10 Hz** contra **GPS + GLONASS a 5 Hz** en una vuelta: cortes de señal y precisión.

## Fuentes

- Hoja de datos NEO-M8, revisión R14 (30-sep-2025), UBX-15031086: https://content.u-blox.com/sites/default/files/NEO-M8-FW3_DataSheet_UBX-15031086.pdf
- Hoja de datos NEO-M8, revisión R10, UBX-13003366 (valores de fábrica y tabla de 5/10 Hz): https://content.u-blox.com/sites/default/files/NEO-M8_DataSheet_(UBX-13003366).pdf
- Especificación del protocolo u-blox M8 (UBX-13003221, pública): https://cdn.sparkfun.com/assets/learn_tutorials/8/6/9/u-blox8-M8_ReceiverDescrProtSpec__UBX-13003221__Public.pdf
- Notas del firmware SPG 3.01: https://content.u-blox.com/sites/default/files/GNSS-FW3.01_ReleaseNotes_(UBX-16000319)_Public.pdf
- Módulos falsos: https://hagensieker.com/2022/08/01/beware-the-fake-gps-modules/ y https://forum.arduino.cc/t/u-blox-neo-m8n-attenzione-ai-fake/493575
- Placas con antena cerámica y externa: https://www.rocketryforum.com/threads/issues-with-new-gps-modules.173178/

## Por USB (desde la 1.0.9)
- El programa busca el GPS solo: **primero por USB** (`/dev/ttyACM*`: VK-172, VK-162, NEO-M8N por su micro USB;
  `/dev/ttyUSB*`: adaptador USB-serie) y si no hay, **por los pines** (`/dev/ttyAMA0`).
- Se puede enchufar o desenchufar con el programa andando: si un puerto se calla 6 s, prueba el siguiente.
- La configuración es la misma (reconoce M8, 7 o 6). Por USB no se cambia la velocidad del puerto (no hace falta).
- En el Chequeo previo, «Módulo» dice «· USB» o «· pines».
- **La placa de Matías (04/11/2026): el micro USB solo da corriente, no pasa datos.** Va por los pines.
- **Pines de su placa, leídos en la foto (04/11/2026), de izquierda a derecha con la antena arriba: PPS · RXD · TXD · GND · VCC**
  (RXD y TXD al revés de lo que se suponía). Cables soldados: amarillo PPS, marrón RXD, naranja TXD, azul GND, blanco VCC.
  Conexión: blanco → pin 17 (3,3 V) · azul → pin 14 (GND) · naranja (TXD) → pin 10 (RX) · marrón (RXD) → pin 8 (TX) ·
  amarillo (PPS) → pin 7 (GPIO4, opcional) o aislado.
- **Ojo con la placa NEO-M8N:** en algunas el micro USB solo da corriente. Prueba: enchufarla a la PC con un cable
  de datos; si en el Administrador de dispositivos aparece «u-blox GNSS receiver» o un puerto COM nuevo, pasa datos.

## Quieto (static hold), 06/11/2026
Con el auto parado, el ruido del GPS daba 0,5 a 3 km/h y el informe mostraba aceleraciones y frenadas falsas. CFG-NAV5 ahora prende *static hold*: por debajo de 1 m/s (3,6 km/h) el módulo da velocidad 0 y deja la posición fija; sale al pasar esa velocidad o al moverse más de 5 m.
