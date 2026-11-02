# Betómetro Raspberry — Pendientes

Actualizado el 02/11/2026. Plan: [docs/plan-ingenieria.md](docs/plan-ingenieria.md).

## Hecho
- E1 (motor igual a la app), E2 (programa completo, temas aprobados).
- Cambios de Matías: sin cantidad de vueltas, MEDIDA/LIBRE y «Unir vueltas» en el informe, libres sin error, cronómetro progresivo/regresivo, ideal a la vista, «libre» en todos lados.
- Imagen 1.0.0 con actualizador (U-Boot A/B + RAUC, por pendrive o internet) y cámara/micrófono. Primer paquete `.raucb` firmado. Secreto `RAUC_CLAVE` cargado.

## Ahora (Matías)
1. Grabar la tarjeta con `betometro-1.0.0.img.xz` y probar: león de arranque, Betómetro, toque en la pantalla.
2. Guardar `betometro-1.0.0.raucb` en un pendrive (para probar la actualización).

## Después de que arranque (E3 → E4)
- Probar la actualización por pendrive y la vuelta sola a la copia anterior.
- Conectar GPS (NEO-M8N), pulsador y auriculares; chequeo previo; medir demoras.
- Medir rendimiento en la Pi 3: pantalla fluida, procesador, temperatura.
- Pantalla de arranque con la foto de los pilotos: aprobada, guardada en `sistema/arranque/`; va por actualización.

## Más adelante (por actualización, sin regrabar)
- Repo público de actualizaciones (`Betometro-Actualizaciones`) para bajar por internet.
- E5: wifi real, «Betómetro en la red», compartir por QR.
- Video: informe con video (mitad y mitad), «Videos guardados», webcam común MJPEG comprimida por el chip de la Pi ([docs/video.md](docs/video.md)). Elegir la cámara.
- E6: prueba en pista y versión 1.0.0 definitiva.
- App del celular: el prompt está en [docs/prompt-app.md](docs/prompt-app.md) (se hace en otra sesión).

## Compras
- Fuente de 5 V 2,5 A y cable micro-USB corto y grueso.
- Auriculares con cable (3,5 mm), cables dupont, pulsador (10 kΩ y 100 nF).
- Para el auto: convertidor 12 → 5 V (mejor 5 A), diodo Schottky, TVS 24–27 V, 2 × 4.700 µF 35 V, fusible 3 A, fichas GX12, cable espiralado, disipadores.
- Webcam USB MJPEG 1080p gran angular (para el video).
