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
