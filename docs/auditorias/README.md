# Auditorías de Betómetro (plan aprobado por Matías, 07/11/2026)

Diez revisiones del proyecto por dentro, cada una a cargo de un agente. Se hacen de a una, en
sesiones nuevas (esta lista la pide Matías; el prompt para cada sesión está abajo).

**Regla general para cada auditoría:**
- Solo lectura al principio. Primero el informe; los arreglos recién con el OK de Matías.
- El informe va en `docs/auditorias/NN-nombre.md`: lo importante primero, corto, en español rioplatense,
  con «qué encontré», «qué riesgo tiene», «qué propongo» y «cuánto cuesta» (chico / medio / grande).
- Cada hallazgo con el archivo y la línea (`app/crono.cpp:120`).
- Nada de rehacer lo que ya se decidió (`docs/app/`, `docs/diseno-sistema.md`, `CLAUDE.md`).
- Commit con `[skip ci]`, autor Matías Russo, a las 4 ramas.

| N.º | Auditoría | Modelo | Qué revisa |
|---|---|---|---|
| 01 | Arquitectura | **Fable** | Reparto entre motor, cronómetro, datos y pantallas. Archivos que crecieron de más (`Informe.qml`, `datos.cpp`, `servidor.cpp`). Cómo ordenar sin romper |
| 02 | Código C++ y Qt | Opus | Memoria, procesos que quedan colgados (cámara, nmcli), hilos, avisos del compilador. Correr con sanitizers (`-fsanitize=address,undefined`) y las pruebas |
| 03 | Pantallas QML | Opus | Canvas y sombras que se redibujan de más, enlaces que se recalculan 20 veces por segundo, pantallas lentas en abrir |
| 04 | Rendimiento en la Pi 3 | Opus | Medir en la Pi real con video + GPS 10 Hz + pantalla: procesador por parte, temperatura, `get_throttled`, cuadros por segundo. Qué sacar del hilo principal |
| 05 | Datos y guardado | **Fable** | `betometro.json` único y reescrito entero en cada vuelta; con recorridos largos pesa MB. Evaluar un archivo por carrera o SQLite, sin perder nada ni romper el formato de la app |
| 06 | Video | **Fable** | Cadena MJPEG → H.264 con el chip, sonido, MP4, reproducción. ¿Otra cámara o menos cuadros? |
| 07 | UX (cómo se usa) | **Fable** | Toques desde prender hasta largar; qué mira el navegante en carrera y en qué orden; errar MARCAR o Terminar; avisos cuando falla GPS/cámara/wifi; nombres (práctica libre, recorrido libre, vuelta libre, medida); prueba con alguien que nunca lo vio |
| 08 | Diseño visual | Opus | Todas las pantallas contra la app y entre sí: tamaños, márgenes, íconos (engranaje que parece sol), 4 temas, horizontal y vertical, modo sol |
| 09 | Pruebas automáticas | **Fable** | Capturas de cada pantalla que se comparan antes y después; simulador de carrera completa con GPS; que corra en GitHub antes de publicar |
| 10 | Sistema y actualizaciones | Opus | Arranque, servicios, vigilante, escrituras en la SD, vuelta atrás si falla una actualización, la compilación de 20 min en GitHub |
| 11 | Lo que faltó (07/11, pedido de Matías) | **Fable** | A: [programa](11a-programa.md) (clima, tele, página del celu, QR, voz, simulador, arranque, Drive, reportes). B: [sistema operativo](11b-sistema.md) (ssh, firewall, hora, config.txt, disco, sonido, red, paquetes). Falta sumar lo medido en la Pi |
| 12 | Final, para entregar (08/11, pedido de Matías) | Fable + Opus | [12-final.md](12-final.md): carrera, C++ con sanitizers, sistema, pantallas y web, entrega. Lo que falta para entregar |

Para ahorrar: si hay que elegir, Fable solo en 01, 05 y 07.

## Prompt para cada sesión nueva

Copiar esto en una sesión nueva sobre el repo `tvmaty2186/Betometro-Raspberry`, cambiando el número:

```
Leé CLAUDE.md y docs/auditorias/README.md. Hacé la auditoría N.º __ de la tabla, con la regla general.

Cómo trabajar:
1. Primero leé los documentos que nombra CLAUDE.md para esa área (no todo el repo). Después el código.
2. No cambies código. Solo mirá, medí y anotá. Si necesitás compilar o correr algo para medir, hacelo en
   Docker con debian:trixie como dice raspios/compilar.sh, con [skip ci] y sin publicar.
3. Escribí docs/auditorias/NN-nombre.md. Orden: 1) lo más grave, 2) lo que más mejora con poco trabajo,
   3) el resto. Cada punto: qué encontré (archivo:línea), qué riesgo tiene, qué propongo, cuánto cuesta.
   Máximo 2 páginas. Español rioplatense, frases cortas.
4. Terminá con «Las 3 que haría primero» y una pregunta a Matías por cada decisión que le corresponde a él.
5. Commit «Auditoría NN: nombre [skip ci]», autor Matías Russo, a las 4 ramas. No publiques.
6. En el chat, contale a Matías en 5 líneas lo que encontraste y pasale el link al informe.
```
