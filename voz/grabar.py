#!/usr/bin/env python3
"""Graba todas las frases de Betómetro con dos voces y arma los sonidos (pitidos y clic).

Uso: python3 voz/grabar.py <carpeta de salida>
     → <salida>/elena/*.wav, <salida>/tomas/*.wav (frases) y <salida>/*.wav (sonidos)

- Voces: Elena y Tomás, español de Argentina (voces neuronales de Microsoft, con edge-tts).
  Elegidas por Matías el 03/11/2026 (la de Piper sonaba peor y cortada). Se eligen en Ajustes.
- Velocidad 1,15, como la voz pregrabada de la app nativa.
- Cada frase se baja una sola vez y queda en ~/.cache/betometro-voz (no se vuelve a pedir).
- Se recorta el silencio del principio (la voz sale enseguida, importa para el anticipo) y
  del final, pero con margen: un recorte fuerte se comía la última sílaba («medida» → «media»).
- Se hace al compilar (GitHub); la Raspberry solo reproduce los archivos, sin internet.
"""
import array
import asyncio
import math
import os
import random
import subprocess
import sys
import wave

VOCES = {"elena": "es-AR-ElenaNeural", "tomas": "es-AR-TomasNeural"}
VELOCIDAD = "+15%"
TASA = 22050
CACHE = os.path.expanduser("~/.cache/betometro-voz/edge")


async def bajar(voz, nombre, texto):
    """mp3 de la frase (de la caché si ya se bajó)."""
    import edge_tts
    carpeta = os.path.join(CACHE, voz)
    os.makedirs(carpeta, exist_ok=True)
    ruta = os.path.join(carpeta, "%s-%s.mp3" % (nombre, VELOCIDAD.replace("%", "")))
    if not os.path.exists(ruta) or os.path.getsize(ruta) == 0:
        for intento in range(4):
            try:
                await asyncio.wait_for(edge_tts.Communicate(texto, VOCES[voz], rate=VELOCIDAD).save(ruta + ".parte"), 60)
                os.replace(ruta + ".parte", ruta)
                break
            except Exception as e:
                print("Reintento", voz, nombre, e)
                await asyncio.sleep(2 * (intento + 1))
        else:
            raise SystemExit("No se pudo grabar «%s» con %s" % (texto, voz))
    return ruta


def muestras_de(mp3):
    crudo = subprocess.run(["ffmpeg", "-v", "error", "-i", mp3, "-ac", "1", "-ar", str(TASA), "-f", "s16le", "-"],
                           check=True, capture_output=True).stdout
    return array.array("h", crudo)


def frases():
    f = {"n00": "cero"}
    for k in range(1, 61):
        f["n%02d" % k] = str(k)
    for n in range(1, 61):
        f["vuelta_%02d_m" % n] = "Vuelta %d, medida" % n
        f["vuelta_%02d_p" % n] = "Vuelta %d, libre" % n
    f["ultima_m"] = "Última vuelta, medida"
    f["ultima_p"] = "Última vuelta, libre"
    f["medida"] = "medida"
    f["practica"] = "libre"
    f["completa"] = "Carrera completa"
    f["pulsador"] = "Pulsador desconectado"
    f["seguir_salir"] = "Seguí apretando para salir"   # Salir con el pulsador (09/11/2026)
    f["terminada"] = "Carrera terminada"
    f["prueba"] = "Betómetro listo"
    return f


def recortar(muestras, umbral=300, margen=int(TASA * 0.01), umbral_fin=60, margen_fin=int(TASA * 0.08)):
    """Saca el silencio: al principio casi todo; al final con margen (las sílabas finales son suaves)."""
    a = 0
    while a < len(muestras) and abs(muestras[a]) < umbral:
        a += 1
    b = len(muestras)
    while b > a and abs(muestras[b - 1]) < umbral_fin:
        b -= 1
    m = array.array("h", muestras[max(0, a - margen):min(len(muestras), b + margen_fin)])
    n = min(len(m), int(TASA * 0.01))   # 10 ms de bajada al final: sin chasquido
    for i in range(n):
        m[len(m) - n + i] = int(m[len(m) - n + i] * (n - i) / n)
    return m


def guardar(ruta, muestras):
    with wave.open(ruta, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(TASA)
        w.writeframes(array.array("h", muestras).tobytes())


def tono(hz, ms, vol=0.35, cuadrada=True):
    n = int(TASA * ms / 1000)
    out = []
    for i in range(n):
        s = math.sin(2 * math.pi * hz * i / TASA)
        if cuadrada:
            s = 1.0 if s >= 0 else -1.0
        env = min(1.0, i / 60, (n - i) / 120)   # sin chasquidos al principio y al final
        out.append(int(32767 * vol * s * env))
    return out


def clic():
    random.seed(1)
    n = int(TASA * 0.03)
    out = []
    for i in range(n):
        caida = math.exp(-i / (TASA * 0.004))
        s = (random.uniform(-1, 1) * 0.6 + 0.4 * math.sin(2 * math.pi * 1800 * i / TASA)) * caida
        out.append(int(32767 * 0.8 * s))
    return out


def main():
    salida = sys.argv[1] if len(sys.argv) > 1 else "voz-grabada"
    os.makedirs(salida, exist_ok=True)

    async def todas():
        for voz in VOCES:
            carpeta = os.path.join(salida, voz)
            os.makedirs(carpeta, exist_ok=True)
            for nombre, texto in frases().items():
                mp3 = await bajar(voz, nombre, texto)
                guardar(os.path.join(carpeta, nombre + ".wav"), recortar(muestras_de(mp3)))
            print(voz, ":", len(os.listdir(carpeta)), "frases")

    asyncio.run(todas())
    guardar(os.path.join(salida, "corto.wav"), tono(880, 90))
    guardar(os.path.join(salida, "largo.wav"), tono(1320, 450))
    guardar(os.path.join(salida, "tic.wav"), tono(1000, 70))
    guardar(os.path.join(salida, "clic.wav"), clic())
    print("Listo en", salida)


if __name__ == "__main__":
    main()
