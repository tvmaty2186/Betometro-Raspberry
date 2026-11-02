# Sistema de Betómetro (Buildroot)

Árbol externo de Buildroot (`BR2_EXTERNAL`) que arma la imagen para la tarjeta de la Raspberry Pi 3 B+.

| Archivo | Qué es |
|---|---|
| `configs/betometro_rpi3_defconfig` | La configuración completa (Buildroot 2026.02.3, Qt 6.9, kernel 6.12) |
| `board/rpi3/config.txt` | Firmware de la Raspberry: pantalla, GPS por el UART, sonido, vigilante. Arranca U-Boot |
| `board/rpi3/boot.cmd`, `uboot.fragment` | U-Boot: elige la copia A o B (3 intentos cada una), carga su kernel (`a/Image` o `b/Image`) y arranca de solo lectura |
| `board/rpi3/rauc/` | Actualizaciones: `system.conf` (copias A y B), `certificado.pem` (verifica la firma), `hook.sh` (pasa el kernel nuevo a la partición de arranque) |
| `board/rpi3/genimage.cfg` | La tarjeta: arranque (128 MB: firmware, U-Boot, 2 kernels), sistema A y B (600 MB cada uno, iguales de fábrica), datos (se agranda sola) |
| `board/rpi3/overlay/` | Servicios: Betómetro arranca solo y se reinicia si se cae; agrandar datos; vigilante; registro en memoria |
| `board/rpi3/arranque.png` | Pantalla de arranque (se arma con `node sistema/arranque/armar.js`) |
| `package/betometro/` | El programa, compilado desde este repo, con la voz grabada |

## Armar la imagen

En GitHub: «Imagen para la Raspberry». Corre con un commit que diga **`[imagen]`** o con una etiqueta `v*` (versión: además la publica). Deja `betometro-X.Y.Z.img.xz` (para grabar la tarjeta) y, si está el secreto `RAUC_CLAVE`, `betometro-X.Y.Z.raucb` (la actualización).

## Actualizaciones (RAUC)

- La imagen se graba **una sola vez**. Después todo llega como paquete `.raucb` firmado: programa, sistema y kernel.
- Se instala desde **Actualizar** (por internet, de la última *release* de `tvmaty2186/Betometro-Actualizaciones`, o desde un **pendrive** con el `.raucb`).
- RAUC escribe la copia que no se usa, el gancho copia su kernel a `a/` o `b/` y U-Boot arranca la nueva. Si Betómetro no anda 30 s seguidos, no se marca «buena» y a los 3 intentos U-Boot vuelve a la anterior.
- Firma: la clave privada va solo en el secreto `RAUC_CLAVE` de GitHub (nunca en el repo). Sin ella no se pueden hacer actualizaciones que la Raspberry acepte.
- Solo hace falta otra imagen si cambia lo que no se actualiza: firmware, U-Boot o las particiones.

A mano (Linux, unos 25 GB libres y varias horas la primera vez):

```
python3 voz/grabar.py sistema/voz-grabada
wget https://buildroot.org/downloads/buildroot-2026.02.3.tar.xz && tar xf buildroot-2026.02.3.tar.xz
make -C buildroot-2026.02.3 BR2_EXTERNAL=$PWD/sistema O=$PWD/salida betometro_rpi3_defconfig
make -C salida
# resultado: salida/images/betometro.img.xz
```
