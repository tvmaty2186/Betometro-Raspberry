# Cómo conectarse a la Raspberry por ssh

## Rápido

En la compu (Windows): tecla Windows → escribir `powershell` → Enter. Después:

```
ssh beto@192.168.1.83
```

Pide la clave de `beto` (no se ve mientras se escribe) → Enter.

## Para no acordarse la IP (una sola vez en la compu)

Pegar esto en PowerShell **una vez**:

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

## Desde la 1.0.28

La Pi se va a llamar `betometro`, así que también va a andar sin la IP:

```
ssh beto@betometro.local
```

## Para salir

Escribir `exit` y Enter.
