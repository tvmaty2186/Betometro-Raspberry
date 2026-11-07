# Auditoría 03 — Pantallas QML

Hecha el 07/11/2026 sobre la versión 1.0.26. Solo lectura: no se tocó código.

**En una línea:** en carrera la pantalla se redibuja **60 veces por segundo en vez de 20**, por una animación
chiquita de la barra. Sacarla es una línea y es lo que más ahorra.

## Lo que medí (PC, contenedor `beto`, 6 s de cada pantalla, contando cuadros dibujados)

| Pantalla | Con GPS simulado (10 Hz) | Sin GPS |
|---|---|---|
| Carrera, horizontal, largada | 420 cuadros (≈ 65 por segundo) | 130 (≈ 20 por segundo) |
| Carrera, vertical, largada | 411 (≈ 65 por segundo) | – |
| Inicio | 394 (≈ 65 por segundo) | 76 (≈ 12 por segundo) |
| Informe (sin video) | 4 | – |

Lo normal en carrera sería 20 por segundo (el ritmo de `Crono.estado`). Pintar un Canvas: el fondo de pantalla
completa tarda ≈ 3 ms en la PC y la cara de un widget ≈ 1,3 ms. En la Pi 3 calculo de 10 a 15 veces más
(es una estimación: hay que confirmarlo en la Pi, auditoría 04).

## 1. Lo más grave

**1.1 La barrita se anima sin parar → 60 cuadros por segundo en toda la carrera**
- Qué encontré: `app/qml/widgets/Barra.qml:35` (`Behavior on x`, 250 ms). El dato cambia cada 50 a 100 ms, así que
  la animación nunca termina. Está en el cronómetro y en el velocímetro de fábrica.
  Lo mismo pasa en `pantallas/Inicio.qml:70` (aguja suave de 220 ms con GPS real) y en `pantallas/Informe.qml:151-155`
  (el borde del botón «Ver en la tele» late siempre; con la tele en uso sigue latiendo **debajo** del control remoto,
  mientras la Pi también pasa el video).
- Riesgo: la Pi dibuja la pantalla entera tres veces más de lo necesario: más procesador, más calor (el aviso
  «Calor») y menos lugar para la voz (cada 15 ms) y la cámara grabando.
- Propongo: sacar el `Behavior` de la barra (el dato ya llega 10 a 20 veces por segundo, se ve igual de fluido).
  En el inicio, sin animación o con la aguja moviéndose sola cada 100 ms. El borde que late: solo si
  `!raiz.enTele`.
- Costo: **chico**.

**1.2 Listas que se arman de nuevo 10 veces por segundo con el video andando**
- Qué encontré: `video/VistaVideo.qml:226-237` (los chips «V3 · medida» a medias) e `pantallas/Informe.qml:547-553`
  («En este momento»). El `Repeater` recibe una lista nueva con cada posición del video, entonces borra y crea
  todos sus elementos 10 veces por segundo. En pantalla completa lo de abajo sigue haciéndolo aunque no se vea.
- Riesgo: trabajo y basura de memoria justo cuando la Pi está pasando video (lo más pesado que hace).
- Propongo: elementos fijos con su texto enlazado, sin `Repeater` (son 2 y 3 cosas fijas).
- Costo: **chico**.

**1.3 Modo tele: cálculos de más**
- Qué encontré: `Tele.qml:74-85`: el estado para el celu lee la posición del video, así que 10 veces por segundo
  rehace la lista de todas las vueltas (con 60 vueltas, unas 3.600 vueltas de bucle cada vez).
  `Tele.qml:54-64`: para contar vueltas llama a `Datos.informe()` de **cada** carrera con video; ese informe se arma
  entero y pasa por texto JSON (`app/datos.cpp:291-295`).
- Riesgo: trabas al entrar al modo tele y mientras anda.
- Propongo: separar lo fijo (vueltas, se arma una vez) de lo que cambia (posición, andando). Para contar vueltas,
  usar los datos de la carrera, no el informe.
- Costo: **medio**.

## 2. Lo que más mejora con poco trabajo

**2.1 Los dos tableros siempre creados** — `pantallas/Carrera.qml:159-174`. El de vueltas y el de recorrido libre
existen los dos; el escondido igual arma sus 6 widgets (con sus Canvas) y recalcula 20 veces por segundo.
Propongo un `Loader` con `active` según el tipo. **Chico.**

**2.2 Un fondo pintado de nuevo en cada pantalla** — `componentes/Fondo.qml` está en 19 pantallas: cada vez que se
abre una, se vuelve a pintar el degradé de 1280×720 y el damero (≈ 760 cuadraditos) y se sube una imagen de 3,7 MB.
En carrera queda tapado por el color de la vuelta (`Carrera.qml:63-70`) y se sigue dibujando.
Propongo un solo `Fondo` en `Main.qml`, detrás de las pantallas, y apagarlo en carrera. **Chico a medio.**

**2.3 La foto del inicio se copia entera para saber su tamaño** — `componentes/FotoInicio.qml:18`
(`createImageData`: 1200×800, 3,8 MB, solo para leer ancho y alto). Poner 1200×800 fijos. **Chico.**

**2.4 Cada widget son 9 capas (14 en Rally, Deportivo y Cabina)** — `widgets/Caja.qml:11-75`: 4 sombras, 2 aros,
la cara y el reflejo (2 Canvas) y 5 anillos de brillo según el tema. El chip las dibuja todas en cada cuadro, y al
abrir la pantalla se pinta cada cara (estimo 15-20 ms por widget en la Pi). Propongo juntar todo en un solo Canvas
por widget, con el mismo aspecto (comparar capturas antes y después). **Medio.** Rinde más después de arreglar 1.1.

## 3. El resto

- **Modo anillo siempre creado** — `widgets/WCuenta.qml:143-184`: aunque de fábrica se usan números, el anillo
  existe y recalcula. Si se usa, su Canvas se repinta cada 0,25 % de la vuelta: en vueltas de 17 s, unas 20 veces
  por segundo. Propongo `Loader` y repintar por píxel de avance. **Chico.**
- **Franja de la sugerida** — `componentes/Velocimetro.qml:304-327`: repinta un Canvas del tamaño del velocímetro
  (420×420 de pie) cada medio km/h. La marca amarilla puede ser un rectángulo girado como la aguja. **Chico a medio.**
- **`Crono.estado` es un solo paquete** — `app/crono.cpp:278-297`: cada 50 ms llega entero y se recalculan unos
  60 enlaces aunque cambie un dato. Hoy no es grave (estimo menos del 2 % del procesador). A futuro: datos sueltos.
  **Medio (C++), no urgente.**
- **`Datos.cfg()` dentro de enlaces** — `Carrera.qml:65-66, 73-74`: se llama 20 veces por segundo porque el enlace
  también lee `est`, y no se entera si cambia la configuración. Leer de `Datos.ajustes`. **Chico.**
- **Cualquier cambio de configuración rehace el tablero** — `Carrera.qml:14` (`Armado.cargar(h, Datos.ajustes)`).
  En carrera hoy nadie cambia la configuración: riesgo bajo, solo tenerlo en cuenta.
- **Imágenes** — `tablet/TBarra.qml:26`: el león sin `sourceSize` (668×356 para 52 px). La vista de la cámara
  (`AjustesCamara.qml:48`, `TCamara.qml:37`) se lee en el hilo principal con cada foto nueva. **Chico.**
- **Lo que está bien:** el velocímetro en capas (la aguja solo gira), el punto de la pista sin repintar, los datos
  del video 10 veces por segundo, la imagen del video propia (`ImagenVideo`), los títulos cromo pintados una vez.
  El giro de 90° de `Main.qml` con `clip` no cuesta de más.
- **Informe:** en la PC abre igual con 8 que con 60 vueltas. Medirlo en la Pi (auditoría 04).

## Las 3 que haría primero

1. Sacar la animación de la barra, la del inicio y el latido debajo del control de la tele (1.1).
2. Cambiar los dos `Repeater` del video por elementos fijos y aliviar el modo tele (1.2 y 1.3).
3. Un solo fondo en `Main.qml` y crear solo el tablero que se usa (2.1 y 2.2).

## Preguntas para Matías

1. ¿La rayita de la barra tiene que deslizarse suave, o está bien que salte 10 a 20 veces por segundo?
   (Sin animación igual se ve fluida.)
2. ¿Usás en carrera el cronómetro con anillo o el velocímetro redondo? Así sé si vale la pena arreglarlos.
3. ¿Te parece que la auditoría 04 mida en la Pi los cuadros por segundo y la temperatura antes y después del punto 1?
