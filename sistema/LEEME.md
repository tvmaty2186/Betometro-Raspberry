# Sistema de Betómetro (Buildroot)

Árbol externo de Buildroot (`BR2_EXTERNAL`) que arma la imagen para la tarjeta de la Raspberry Pi 3 B+.

| Archivo | Qué es |
|---|---|
| `configs/betometro_rpi3_defconfig` | La configuración completa (Buildroot 2026.02.3, Qt 6.9, kernel 6.12) |
| `board/rpi3/config.txt`, `cmdline.txt` | Arranque de la Raspberry: pantalla, GPS por el UART, sonido, vigilante, sistema de solo lectura |
| `board/rpi3/genimage.cfg` | La tarjeta: arranque (64 MB), sistema A y B (600 MB cada uno), datos (se agranda sola) |
| `board/rpi3/overlay/` | Servicios: Betómetro arranca solo y se reinicia si se cae; agrandar datos; vigilante; registro en memoria |
| `board/rpi3/arranque.png` | Pantalla de arranque (se arma con `node sistema/arranque/armar.js`) |
| `package/betometro/` | El programa, compilado desde este repo, con la voz grabada |

## Armar la imagen

En GitHub: «Imagen para la Raspberry» (se corre sola al cambiar `sistema/` o con una etiqueta `v*`).

A mano (Linux, unos 25 GB libres y varias horas la primera vez):

```
python3 voz/grabar.py sistema/voz-grabada
wget https://buildroot.org/downloads/buildroot-2026.02.3.tar.xz && tar xf buildroot-2026.02.3.tar.xz
make -C buildroot-2026.02.3 BR2_EXTERNAL=$PWD/sistema O=$PWD/salida betometro_rpi3_defconfig
make -C salida
# resultado: salida/images/betometro.img.xz
```
