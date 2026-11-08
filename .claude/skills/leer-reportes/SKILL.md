---
name: leer-reportes
description: Leer los reportes automáticos que manda la Pi (issues de tvmaty2186/Betometro-reportes) y explicarle a Matías qué pasó. Usar cuando dice «fijate el reporte», «mandé reportes» o cuenta una traba o un error.
---

# Leer los reportes de la Pi

- Están como issues en `tvmaty2186/Betometro-reportes` (GitHub MCP `list_issues`, ordenados por fecha, con `since`).
  Título «Reporte v1.0.N · fecha · N líneas». La Pi manda **solo lo nuevo**, cada ~10 minutos, y lo pendiente al
  arrancar. Si Matías recién cuenta algo, puede no haber llegado: programar un recordatorio a los 10 min.
- Cada línea: `fecha · versión · tipo · texto`. Tipos: `aviso`/`error` (Qt y el programa), `sistema` (journal),
  `cuelgue` (pstore), `cierre` («no se apagó bien» = se cortó la corriente o se colgó), `prueba` (Prueba completa),
  `simulador`, `vigilante`.
- **Vigilante** (desde 1.0.45): `⚠ Memoria al X %` o `⚠ Pantalla congelada X s` o `Tirones…`, con la pantalla, el
  equipo (memoria, swap, carga, °C, tensión), los 5 procesos que más usan (con procesador del último segundo),
  cuánto crece Betómetro por minuto, las últimas pantallas, qué estaba andando y la memoria de la última media hora.
  Crecimiento parejo = pérdida; salto = algo puntual. Betómetro usa normalmente 220–270 MB (32–45 %).
- Ya vistos: traba al abrir Cámara o el informe por GStreamer (resuelto en 1.0.47/1.0.48); «Despejar memoria» deja
  una línea; «QProcess: Destroyed while process ("nmcli")» al actualizar es inofensivo.
- Antes de concluir, preguntar lo que no se ve (si la cámara o la antena estaban conectadas, si se desenchufó).
- Para reproducir con datos reales: la copia `Betómetro/betometro.json` está en el Google Drive de Matías
  (se pone como `<datos>/betometro.json` y se migra al arrancar).
- Responderle corto: qué fue, si es un error del programa o algo esperable, y qué se hace.
