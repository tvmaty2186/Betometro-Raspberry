# Tarjeta SD nueva: rearmar el Betómetro desde cero

Para cuando la tarjeta se rompe, se pierde o se quiere armar otro equipo. Se hace desde la compu (Windows),
en unos 40 minutos (casi todo es esperar). Las carreras se recuperan de la copia.

## Antes: tener la copia de las carreras

La copia es un archivo `betometro.json` con todas las carreras y la configuración. Está en dos lugares:

- **Google Drive**, en la carpeta **Betómetro** (si Drive estaba conectado: se sube sola con la wifi de casa).
- **El celu o la compu**, si la bajaste: en «Betómetro en la red», abajo, **Copia de la base**.

Bajala a la compu antes de empezar. Si no hay copia, el equipo arranca igual, sin carreras.

## 1. Grabar el sistema en la tarjeta

1. Bajá **Raspberry Pi Imager**: https://www.raspberrypi.com/software/
2. Elegí: **Raspberry Pi 3** → **Raspberry Pi OS (other)** → **Raspberry Pi OS Lite (64-bit)** → tu tarjeta.
3. Cuando pregunte por los ajustes, tocá **Editar ajustes** y poné:
   - Nombre del equipo: `betometro`
   - Usuario: `beto` y una contraseña (anotala en un lugar seguro).
   - Wifi: la de tu casa, con su clave. País: `AR`.
   - Zona horaria: `America/Argentina/Buenos_Aires`.
   - En **Servicios**: activar **SSH** con contraseña.
4. Grabá, poné la tarjeta en la Raspberry y dale corriente. Esperá 3 minutos (la primera vez tarda).

## 2. Instalar Betómetro

1. En la compu, abrí **PowerShell** y entrá a la Raspberry:
   ```
   ssh beto@betometro.local
   ```
   (Si no la encuentra, usá la IP que muestra tu router. Más ayuda: [conectarse-ssh.md](conectarse-ssh.md).)
2. Mirá cuál es la última versión: https://github.com/tvmaty2186/Betometro-actualizaciones/releases/latest
   (por ejemplo `1.0.36`).
3. Pegá esto, cambiando `X.Y.Z` por esa versión (las 3 veces):
   ```
   cd /tmp && curl -LO https://github.com/tvmaty2186/Betometro-actualizaciones/releases/download/vX.Y.Z/betometro-pi-X.Y.Z.tar.gz && tar xzf betometro-pi-X.Y.Z.tar.gz && sudo bash betometro-pi-X.Y.Z/instalar.sh
   ```
4. La primera vez baja programas del sistema y rearma el arranque: **20 a 30 minutos**. La pantalla puede quedar
   negra con un guión: es normal, **no la apagues**. Al terminar se reinicia sola y arranca en Betómetro.

## 3. Recuperar las carreras

1. Desde la compu, en una PowerShell **nueva** (no la de la Raspberry), en la carpeta donde está la copia:
   ```
   scp betometro.json beto@betometro.local:/tmp/
   ```
2. En la PowerShell de la Raspberry:
   ```
   sudo systemctl stop betometro && sudo rm -f /var/lib/betometro/config.json && sudo cp /tmp/betometro.json /var/lib/betometro/ && sudo systemctl start betometro
   ```
3. Al arrancar, Betómetro la lee, arma una carrera por archivo y guarda la copia como `betometro.json.migrado`.
   (El `config.json` que se borra es el de fábrica de la tarjeta nueva: así vuelven tus ajustes.)
   Las carreras y los ajustes vuelven como estaban. Los videos no: quedan en Drive.

## 4. Lo demás

- **Google Drive:** Ajustes → Google Drive. Si pide el cliente de Google, abrí desde la compu la dirección que muestra
  la pantalla, escribí el **código** que aparece ahí y subí el `.json` del cliente (los pasos están en esa página).
  Después **Conectar**.
- **Reporte de errores por internet** (opcional): hace falta la llave de GitHub, que se pone a mano en
  `/var/lib/betometro/reportes.token`. Que sea *fine-grained*, solo para `Betometro-reportes` y con permiso de Issues.
  **Nunca la pegues en un chat.**
- **Pulsador, GPS, cámara y auriculares:** se conectan igual que antes (ver el manual, parte 12). Hacé el
  **Chequeo previo** y una **Prueba completa** (Ajustes → Prueba completa) para ver que todo ande.
