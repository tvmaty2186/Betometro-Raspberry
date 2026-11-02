#!/usr/bin/env python3
"""Graba con Piper todas las frases de Betómetro y arma los sonidos (pitidos y clic).

Uso: python3 voz/grabar.py <carpeta de salida>

- Voz: «Daniela», español de Argentina (rhasspy/piper-voices, licencia CC BY-SA 4.0).
  Se baja una sola vez a ~/.cache/betometro-voz.
- Se recorta el silencio del principio y del final: así la voz sale enseguida (importa para el anticipo).
- En la Raspberry esto se hace al armar la imagen (E3); el programa solo reproduce los archivos.
"""
import array
import io
import math
import os
import random
import sys
import urllib.request
import wave

VOZ = "es_AR-daniela-high"
URL = "https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/es/es_AR/daniela/high/"
TASA = 22050


def modelo():
    cache = os.path.expanduser("~/.cache/betometro-voz")
    os.makedirs(cache, exist_ok=True)
    for ext in (".onnx", ".onnx.json"):
        ruta = os.path.join(cache, VOZ + ext)
        if not os.path.exists(ruta):
            print("Bajando", VOZ + ext)
            urllib.request.urlretrieve(URL + VOZ + ext, ruta)
    return os.path.join(cache, VOZ + ".onnx")


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
    f["prueba"] = "Betómetro listo"
    return f


def recortar(muestras, umbral=500, margen=int(TASA * 0.01)):
    a = 0
    while a < len(muestras) and abs(muestras[a]) < umbral:
        a += 1
    b = len(muestras)
    while b > a and abs(muestras[b - 1]) < umbral:
        b -= 1
    return muestras[max(0, a - margen):min(len(muestras), b + margen)]


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
    from piper import PiperVoice
    voz = PiperVoice.load(modelo())
    for nombre, texto in frases().items():
        buf = io.BytesIO()
        with wave.open(buf, "wb") as w:
            voz.synthesize_wav(texto, w)
        buf.seek(0)
        with wave.open(buf, "rb") as r:
            datos = array.array("h", r.readframes(r.getnframes()))
        guardar(os.path.join(salida, nombre + ".wav"), recortar(datos))
    guardar(os.path.join(salida, "corto.wav"), tono(880, 90))
    guardar(os.path.join(salida, "largo.wav"), tono(1320, 450))
    guardar(os.path.join(salida, "tic.wav"), tono(1000, 70))
    guardar(os.path.join(salida, "clic.wav"), clic())
    print("Listo:", len(os.listdir(salida)), "archivos en", salida)


if __name__ == "__main__":
    main()
