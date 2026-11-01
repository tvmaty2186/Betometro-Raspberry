# Hardware

## Comprado
- Raspberry Pi 3 B+ (Element14)
- Raspberry Pi Touch Display 2 de 7" (DSI)
- microSD SanDisk Ultra de 16 GB
- Módulo GPS GY-NEO-M8N (u-blox NEO-M8N-0-10) con SMA, micro USB, pila de respaldo y LED PPS
- Antena GPS activa de 1 m, SMA macho

## Falta comprar (recomendado)
- Fuente de 5 V 2,5 A micro-USB y cable USB-A a micro-USB
- Amplificador **MAX98357A** (I2S) con parlante de 3 W 4 Ω, para la voz
- Cables dupont y un pulsador
- Más adelante: convertidor de 12 V a 5 V 3 A, fichas GX12 y cable espiralado

## GPS
- Va por **cables (UART), no por USB** (decisión de Matías). Cableado: [cableado-gps.png](cableado-gps.png) y la tabla de pines en [diseno-sistema.md](diseno-sistema.md).
- Pines del módulo, vistos del lado de la antena cuadrada, de izquierda a derecha: PPS, TXD, RXD, GND, VCC.
- Usar la **antena externa en el techo**; verificar que el módulo la esté usando y no la cerámica de la placa.

## Descartado
- Caja con celular Galaxy A37 (cancelada el 01/11/2026).
- Kiosco con la web de la app en Chromium: Matías quiere un sistema nativo hecho de cero.
- Placas ESP32 Waveshare: peores para la voz.
- SIM868 HAT y Pico SIM7080G como GPS: se eligió el NEO-M8N.
