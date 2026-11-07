# Cómo conectarse a la Raspberry por ssh

## Rápido (anda desde el 07/11/2026)

En la compu (Windows), en la ventana negra (cmd o PowerShell, cualquiera):

```
ssh beto@betometro.local
```

Si no anda por nombre, con la IP: `ssh beto@192.168.1.83`.

Pide la clave de `beto` (no se ve mientras se escribe) → Enter.

## Atajo opcional: escribir solo `ssh beto` (una sola vez en la compu)

Se pega **en Windows, no en la Pi**, y en **PowerShell** (tecla Windows → `powershell`; no en el cmd que dice
`C:\Users\...>`):

```powershell
mkdir -Force $HOME\.ssh | Out-Null; Add-Content $HOME\.ssh\config "`nHost beto`n  HostName 192.168.1.83`n  User beto`n"
```

Desde ahí alcanza con:

```
ssh beto
```

## Si no conecta

- La compu y la Pi tienen que estar en la **misma wifi** (la de casa).
- El router pudo darle **otra IP**: se ve en la Pi, en **Ajustes → Conectividad**. Cambiar `192.168.1.83` por esa
  (y en `$HOME\.ssh\config` si se usó el atajo).
- Lejos de casa: conectar la compu a la red propia **«Betometro-101»** (clave `betometro`). Desde la 1.0.28 ssh
  queda cerrado en esa red (plan, sección 8).

## Para salir

Escribir `exit` y Enter.

## Copia de las carreras (antes de una actualización grande)

En PowerShell, en la compu (no en la Pi):

```
scp beto@betometro.local:/var/lib/betometro/betometro.json "$HOME\Desktop\betometro-copia.json"
```

Desde la 1.0.29 la base está en una carpeta (`/var/lib/betometro/carreras/`); la copia completa se baja desde el
celu en «Betómetro en la red» → «Copia de la base», o con:

```
scp -r beto@betometro.local:/var/lib/betometro/carreras "$HOME\Desktop\carreras-copia"
```

## Prueba completa desde la terminal (desde la 1.0.30)

La **Prueba completa** (Ajustes → Prueba completa) la Pi la corre sola: sistema, simulador, voz, pulsador, GPS,
cámara, rendimiento, reproducción, tarjeta, wifi, Drive y pantallas. Tarda 3 a 5 minutos y el resultado llega
solo al desarrollador si hay llave de reportes. También se puede arrancar y leer por ssh (el programa ya corre
como servicio, por eso no hay un `betometro --prueba-completa`; se le pide al servidor de «Betómetro en la red»):

```
curl -X POST http://localhost/prueba/empezar
```

Contesta `{"ok":true}` (o el motivo: hay una carrera en curso, o ya está corriendo). En la pantalla de la Pi se
abre la prueba: ahí hay que contestar **«¿Se escuchó?»** y **apretar el pulsador** cuando lo pida (si no, a los
20 s quedan como «sin probar»).

Para ver el resultado:

```
cat /var/lib/betometro/prueba-completa.txt
```

(También en JSON: `/var/lib/betometro/prueba-completa.json`.) Desde el celu, en la misma wifi:
`http://<IP de la Pi>/prueba` muestra lo mismo, con el botón «Empezar» y la descarga del .txt.
