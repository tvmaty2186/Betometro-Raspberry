---
name: publicar-version
description: Publicar una versión nueva de Betómetro Raspberry (llega sola a la Pi). Usar solo cuando Matías dice «publicá», «dale», «compilá» o similar para una versión.
---

# Publicar una versión

Solo con el OK de Matías. Nunca empujar a `main` mientras compila otra versión.

1. Todo lo intermedio ya tiene que estar en la rama asignada con `[skip ci]` y con las pruebas en verde
   (`ctest --test-dir /build-t`, 12 pruebas, incluidas las capturas).
2. Subir el número en `CMakeLists.txt` (`project(betometro VERSION 1.0.N …)`), en `CLAUDE.md` («Última publicada»)
   y agregar arriba en `PENDIENTES.md` un bloque `## 1.0.N (publicada DD/MM/AAAA)` con lo que trae (corto).
3. Commit con autor `Matías Russo <tvmaty2186@gmail.com>`, mensaje en español con `[publicar]` en el título.
4. Empujar a `main` solo si `main` es ancestro: `git fetch -q origin main && git merge-base --is-ancestor origin/main HEAD && git push -q origin HEAD:main`, y después a la rama asignada.
   Si el clasificador niega el push a `main`, dejarlo en la rama y pedirle a Matías el «dale».
5. Programar un recordatorio a los ~13 min (`send_later`) y entonces comprobar:
   ```
   B=https://github.com/tvmaty2186/Betometro-actualizaciones/releases/download/v1.0.N
   curl -sL $B/betometro-pi-1.0.N.tar.gz.sha256; curl -sL $B/betometro-pi-1.0.N.tar.gz | sha256sum
   ```
   Tienen que coincidir. Si no está, mirar el run de `programa.yml` (GitHub MCP `actions_list`) y el log.
6. Avisarle a Matías en 3–5 renglones: publicada, qué trae y cómo probarlo en la Pi.

Las demás ramas (`claude/adoring-bell-disf3a`, `ccr-*`) se adelantan con un push de la misma punta cuando no hay
compilación en curso (ver CLAUDE.md, «Ramas y versiones»).
