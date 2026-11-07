# Auditoría 10 — Sistema y actualizaciones

07/11/2026 · versión revisada: 1.0.26 · solo lectura (no se cambió nada).

**En una línea:** el sistema está bien armado (vigilante doble, journald con pocas escrituras, zram, carreras
guardadas con fsync, TLS al bajar), pero **una actualización cortada por la luz puede dejar a Betómetro sin
arrancar y no vuelve sola a la versión anterior**. Eso es lo primero a arreglar.

---

## 1. Lo más grave

**1.1 Corte de luz durante la actualización → Betómetro roto, sin vuelta atrás automática**
- Qué encontré: `raspios/instalar.sh:29` desarma el programa nuevo **encima** del viejo (`tar -xzf` sobre
  `/usr/bin/betometro`). Probado en Docker: GNU tar escribe en el mismo archivo (mismo inodo), sin `sync`.
  Si se corta la luz en esos ~30 s, el programa queda a medias o en cero. `volver.sh` (línea 25) existe,
  pero **solo a mano por ssh**.
- Riesgo: la pantalla queda con la foto de arranque para siempre. Sin la app andando no se puede volver a
  actualizar (ni por wifi ni por pendrive). Hace falta una compu y ssh.
- Propongo: (a) desarmar en una carpeta aparte, `sync`, y recién ahí cambiar con `mv` (es instantáneo y no
  queda a medias); (b) **vuelta atrás sola**: si Betómetro se cae 4 veces seguidas al arrancar,
  `OnFailure=` en `betometro.service` corre `volver.sh` y lo vuelve a prender.
- Cuesta: **medio**.

**1.2 Cada actualización reescribe `cmdline.txt` en la partición de arranque (FAT)**
- Qué encontré: `instalar.sh:50` y `:99` usan `sed -i` **siempre**, aunque no haya nada que cambiar.
  Probado: `sed -i` crea un archivo nuevo y reemplaza el viejo cada vez. FAT no aguanta bien un corte en ese
  momento. Igual pasa con el initramfs al rearmar la foto de arranque (línea 108, pocas veces).
- Riesgo: si se corta justo ahí, `cmdline.txt` queda vacío o roto → **la Raspberry no arranca nada**
  (hay que sacar la tarjeta y arreglarla en la PC).
- Propongo: hacer el `sed` solo si `grep` encuentra el texto (como ya se hace en las líneas 83 y 103) y un
  `sync` después de tocar `/boot/firmware`.
- Cuesta: **chico**.

**1.3 Si el instalador falla a mitad, Betómetro queda apagado y la pantalla dice «se reinicia sola»**
- Qué encontré: `actualizador.cpp:37-39` marca «lista» cuando `systemd-run` **arrancó** el instalador, no cuando
  terminó. Si `instalar.sh` falla después de la línea 20 (`systemctl stop betometro`) por `set -e` (sin lugar,
  tar roto), no hay `reboot` ni se vuelve a prender: pantalla negra hasta cortar la luz.
- Propongo: en `instalar.sh`, un `trap` que ante error vuelva a la versión anterior y haga `systemctl start
  betometro`; y que deje escrito el resultado (`/var/lib/betometro/actualizacion/resultado`) para mostrarlo
  al prender («Se instaló la 1.0.27» o «No se pudo: …»).
- Cuesta: **chico**.

**1.4 Arranque lento + vigilante de 30 s = puede reiniciarse en bucle**
- Qué encontré: `betometro.service:9` es `Type=simple`, así que el vigilante (`WatchdogSec=30`) cuenta **desde
  que arranca el proceso**. El primer «sigo vivo» sale recién después de cargar la base y todas las pantallas
  (`main.cpp:112` y `:254-256`). Con una base grande (carreras a 5 Hz, varios MB) en la Pi 3 puede pasar de 30 s.
- Riesgo: systemd lo mata antes de que termine de abrir, una y otra vez. Además, si se cae muy rápido,
  después de 5 intentos en 10 s systemd **deja de intentar** (límite por defecto) y queda la foto fija.
- Propongo: `Type=notify` + `TimeoutStartSec=180` (el programa ya manda `READY=1` en `salud.cpp:53`): así el
  vigilante empieza a contar recién cuando Betómetro está abierto. Y fijar `StartLimitBurst`/
  `StartLimitIntervalSec` a propósito (sirve también para 1.1).
- Cuesta: **chico**.

**1.5 Betómetro corre como root y abre el puerto 80 a cualquier red**
- Qué encontré: `betometro.service` no tiene `User=`: todo corre como root, incluido el servidor web hecho a mano
  (`servidor.cpp:124`, escucha en todas las redes, sin clave). En la red propia la clave es `betometro` para
  todos los equipos (está en la documentación).
- Riesgo: cualquiera en la misma wifi (casa, autódromo, la red propia en boxes) ve y baja las carreras, usa
  `/control/accion` y puede cambiar el cliente de Google en `/drive`. Un error en el servidor = control total
  del equipo.
- Propongo, en orden de costo: (a) clave de la red propia distinta por equipo (sale en el QR, no hay que
  recordarla) — chico; (b) blindar el servicio sin dejar de ser root: `ProtectSystem=full`, `ProtectHome=yes`,
  `PrivateTmp=yes` (el instalador corre en otra unidad, no lo afecta) — chico, probar en la Pi;
  (c) correr como `beto` con permisos puntuales (brillo, GPIO, nmcli, apagar) — grande.

## 2. Lo que más mejora con poco trabajo

**2.1 Compilación: de ~20 min a ~5 min** — `programa.yml:33` compila en Intel y **emula ARM con QEMU**
(líneas 61-70): compilar y probar emulado es lo lento. GitHub tiene máquinas ARM de verdad
(`runs-on: ubuntu-24.04-arm`), gratis para repos públicos. Se compila directo, sin Docker ni QEMU.
Cuesta: **chico** (cambiar 3 pasos y probar una vez).

**2.2 Empujar durante una compilación ya no rompe nada** — `programa.yml:27-29` cancela la compilación
anterior en cada push. Propongo un grupo aparte para las de `[publicar]` (nunca se cancelan). Y que solo
compile en una rama (`main`), no en las 4 (`branches: ['**']`, línea 17): hoy un push a las 4 ramas son 4
compilaciones. Cuesta: **chico**.

**2.3 Publicar solo si pasan las pruebas del motor** — `pruebas.yml` corre aparte; `programa.yml` puede
publicar aunque el motor falle. Propongo correr `ctest` dentro de `programa.yml` antes de «Publicar».
Cuesta: **chico**.

**2.4 Paquetes viejos que se acumulan** — `actualizador.cpp:184` guarda cada versión bajada
(`/var/lib/betometro/actualizacion/betometro-pi-X.tar.gz`, ~24 MB) y nunca la borra, más la carpeta `nueva/`.
Con 26 versiones son ~600 MB que le sacan lugar a los videos. Propongo borrar todo menos la última al
instalar. Cuesta: **chico**.

**2.5 Servicios que sobran en el arranque** — no tengo la Pi; Raspberry Pi OS trixie trae `cloud-init`
(configura el primer arranque y suma segundos en cada uno), `ModemManager`, `avahi`, etc. Medir con los
comandos de abajo y apagar lo que no se usa. Cuesta: **chico**.

## 3. El resto

- **Seguridad de la descarga (está bien):** se baja por HTTPS y Qt verifica el certificado; alguien en la misma
  wifi **no** puede meter otro paquete. Si el gzip llega cortado, `tar` falla y no se instala nada. Lo que
  manda es la cuenta de GitHub: quien tenga `ACTUALIZACIONES_TOKEN` o la cuenta de Matías puede poner
  cualquier cosa en el auto. Firmar los paquetes (clave pública dentro de Betómetro) protege contra eso
  solo si la clave privada está fuera de GitHub — **medio**, opcional.
- **Pendrive:** quien tenga el equipo en la mano puede instalar lo que quiera (es esperable). Detalle: el
  nombre del archivo va entre comillas simples en un comando de shell (`actualizador.cpp:225`); un nombre con
  `'` rompe el comando. Pasar los argumentos sin shell — **chico**.
- **Sistema sin actualizaciones de seguridad:** `apt-daily` está apagado (`instalar.sh:81`) y el instalador
  solo agrega paquetes que faltan. Para un equipo de carrera está bien, pero conviene un `apt full-upgrade`
  a mano en casa cada tanto (ver preguntas).
- **Copia anterior se pisa:** si se instala dos veces la misma versión, `instalar.sh:23` borra la copia buena y
  guarda la misma. Guardar solo si la versión cambió — **chico**.
- **Reporte de errores:** cada aviso abre y escribe `errores.log` en la tarjeta (`reportes.cpp:123-136`); tiene
  tope de 200 KB, bien. `journalctl` al prender frena hasta 4 s el arranque (`reportes.cpp:89`) — pasarlo a
  después de abrir — **chico**.
- **`imagen.yml`** (Buildroot, sin usar) todavía se dispara con etiquetas `v*`: 2-3 h de GitHub tiradas.
  Quitarle el disparo — **chico**.
- **Si la compilación falla a mitad:** no pasa nada en el auto (se publica al final). Solo si falla la subida,
  puede quedar un borrador que bloquea ese número: hay que subir la versión.
- **Bien hecho y no tocar:** vigilante de hardware (`RuntimeWatchdogSec=15`), journald solo avisos,
  `fsck.repair=yes`, zram, carreras con fsync + renombrar (`almacen.cpp:45`), ramoops, `Nice=-10`.

## Para mirar en la Pi (pegar por ssh)

```bash
ssh beto@<IP>
systemd-analyze; systemd-analyze blame | head -15; systemd-analyze critical-chain betometro.service
systemctl list-unit-files --state=enabled --no-pager
journalctl -b -u betometro -o short-monotonic --no-pager | head -5   # cuánto tarda en abrir
du -sh /var/lib/betometro/actualizacion /var/lib/betometro-anterior; df -h /
sudo ss -tlnp; ls -l /boot/firmware/cmdline.txt; cat /boot/firmware/cmdline.txt
```

## Las 3 que haría primero

1. **Vuelta atrás automática + instalación sin quedar a medias** (1.1 y 1.3): que una actualización nunca deje
   el equipo sin arrancar.
2. **No tocar `cmdline.txt` si no hace falta + `Type=notify`** (1.2 y 1.4): dos cambios chicos que sacan dos
   formas de quedar trabado.
3. **Compilar en ARM de verdad y no cancelar las de publicar** (2.1 y 2.2): de 20 a ~5 minutos y sin el
   problema de empujar mientras compila.

## Preguntas para Matías

1. ¿Querés que si una versión nueva no arranca, **vuelva sola** a la anterior y te avise en pantalla?
2. ¿La clave de la red propia puede ser **distinta por equipo** (la ves en el QR) en vez de `betometro`?
3. ¿Entrás por ssh con **clave escrita** o con llave? Si es clave, ¿es fuerte? (con la red propia, cualquiera
   en boxes puede intentar entrar).
4. ¿Compilamos solo desde **una rama** (`main`) en vez de las 4?
5. ¿Hacemos un `apt full-upgrade` en casa de vez en cuando (con cargador, no en el auto), o preferís no tocar
   el sistema nunca?
