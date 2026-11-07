# Google Drive: subir informes y videos solos

El Betómetro sube solo, a tu Drive, los informes y los videos de cada carrera.
Quedan en la carpeta **Betómetro**, con una carpeta por carrera: «2026-11-08 Santa Clara».
En la misma carpeta **Betómetro** deja también `betometro.json`: una copia de toda la base (todas las carreras
y la configuración, en el formato de la app). Se vuelve a subir, pisando la anterior, cada vez que cambia una carrera.

Hay que prepararlo **una sola vez**. Son 6 pasos. Los primeros 4 se hacen en la compu.

## Los pasos

1. **Crear el proyecto.** Abrí [console.cloud.google.com/projectcreate](https://console.cloud.google.com/projectcreate).
   Nombre: `Betometro`. Tocá **Crear**.

2. **Activar la API de Drive.** Abrí [la API de Drive](https://console.cloud.google.com/apis/library/drive.googleapis.com)
   (fijate que arriba esté el proyecto Betometro). Tocá **Habilitar**.

3. **La pantalla de permiso.** Abrí [Google Auth Platform](https://console.cloud.google.com/auth/overview) y tocá **Comenzar**.
   - Nombre de la app: `Betómetro`. Correo: el tuyo.
   - Público: **Externo**.
   - Contacto: tu correo. Aceptá y tocá **Crear**.
   - Después abrí [Público](https://console.cloud.google.com/auth/audience) y tocá **Publicar app** → **Confirmar**.

   > Importante: si queda en «Prueba», Google corta el permiso cada 7 días.
   > No hace falta que Google la revise: Betómetro solo ve los archivos que sube él.

4. **El cliente.** Abrí [Clientes](https://console.cloud.google.com/auth/clients/create).
   - Tipo: **TVs y dispositivos de entrada limitada**. Nombre: `Betómetro`. Tocá **Crear**.
   - Tocá **Descargar JSON**. Queda un archivo `client_secret_….json` en la compu.

   > Ese archivo es una clave: no lo mandes por WhatsApp ni por chat.

5. **Pasarlo al Betómetro.** Con la compu en la misma wifi que el Betómetro, abrí `http://<IP del Betómetro>/drive`
   (la IP está en Ajustes → Google Drive y en Ajustes → Conectividad). Elegí el JSON y tocá **Subir**.

6. **Conectar.** En el Betómetro: **Ajustes → Google Drive → Conectar**.
   Aparece un código grande. En el celu abrí [google.com/device](https://www.google.com/device) (o escaneá el QR),
   escribí el código, elegí tu cuenta y tocá **Continuar**. Listo: dice «Conectado como …@gmail.com».

## Cuándo sube

- Solo con wifi, sin carrera y sin la cámara grabando. Si empieza una carrera, para enseguida.
- Revisa al terminar cada carrera, al conectarse la wifi y cada 10 minutos. «Subir ahora» lo hace ya.
- De a un archivo y despacito, para no trabar el equipo. Si se corta, sigue donde quedó.
- Si cambiás algo de una carrera (vuelta medida o libre), el informe se vuelve a subir encima del anterior.

## Opciones (Ajustes → Google Drive)

- **Subir informes** (prendido).
- **Subir videos** (prendido), pero **solo con las wifi que elijas**. De fábrica no hay ninguna:
  elegí la de casa. Una hora de video son unos 2 GB y con el celu se gastan los datos.
- **Borrar del equipo los videos ya subidos** (apagado). Nunca borra el que se está grabando ni los informes.

## Si algo falla

La pantalla dice el problema en palabras:

| Dice | Qué hacer |
|---|---|
| Falta un paso: desde la compu abrí http://…/drive | Hacé el paso 5 |
| Se venció el permiso: tocá Conectar | Tocá Conectar otra vez (si pasa cada semana, falta «Publicar app» del paso 3) |
| Drive lleno | Liberá lugar en Drive |
| Esperando wifi de casa para los videos | Elegí la wifi de casa en «Videos: solo con estas wifi» |
| El cliente de Google no sirve | Repetí el paso 4 con el tipo «TVs y dispositivos de entrada limitada» |
| Falta activar la API de Drive | Hacé el paso 2 |

**Desconectar** le saca el permiso a Betómetro en tu cuenta. Lo que ya está en Drive queda.

## Para el programa (técnico)

- Código: `app/drive.*` (red y cola) y `app/drive_logica.*` (sin red, probado en `pruebas/programa/prueba_drive.cpp`).
- Permiso: flujo de Google para equipos sin teclado, alcance `drive.file`.
- `<datos>/drive-cliente.json` (0600): `client_id` y `client_secret`, lo sube `POST /drive`.
- `<datos>/drive.json` (0600): `refreshToken`, `cuenta`, `raiz` (id de la carpeta Betómetro),
  `carreras: {id: {carpeta, informe: {id, version (sha1 del HTML)}, videos: {archivo: {id, bytes}}}}` y
  `sesion` (la subida a medias: `clave`, `url`, `bytes`, `version`).
- Subida resumable de a 4 MB con `curl` (nice 10, disco en «idle», 2 MB/s como mucho, pausa de 1,5 s entre pedazos).
- En la PC no sale a internet. Capturas: `--pantalla drive --prueba drive-codigo|drive-subiendo|drive-listo|drive-esperando|drive-error`
  (con `tablet-` adelante, en Modo Tablet). Con `BETO_DRIVE_SERVIDOR=http://127.0.0.1:PUERTO` usa un Google de mentira
  y `--prueba drive-conectar` toca «Conectar» solo.
