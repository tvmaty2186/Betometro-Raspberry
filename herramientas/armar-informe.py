#!/usr/bin/env python3
"""Arma recursos/informe-plantilla.html desde recursos/informe/fuente.html:
mete adentro (en base64) las tipografías de Betómetro y el león, para que el informe se vea igual
en cualquier celular y sin internet, y pega el JS del video (recursos/informe-video.js, propio de la
Raspberry) donde dice __VIDEO_JS__, así el informe descargado sigue siendo un solo archivo.
Uso: python3 herramientas/armar-informe.py"""
import base64, io, os
from PIL import Image
R = os.path.join(os.path.dirname(__file__), '..', 'recursos')
b64 = lambda p: base64.b64encode(open(os.path.join(R, p), 'rb').read()).decode()
leon = Image.open(os.path.join(R, 'img/leon.png')).convert('RGBA')
leon.thumbnail((180, 180))
buf = io.BytesIO(); leon.save(buf, 'PNG', optimize=True)
s = open(os.path.join(R, 'informe/fuente.html'), encoding='utf-8').read()
for marca, valor in {
    '__F_B600__': b64('fuentes/barlow-latin-600-normal.woff2'),
    '__F_B600I__': b64('fuentes/barlow-latin-600-italic.woff2'),
    '__F_B800I__': b64('fuentes/barlow-latin-800-italic.woff2'),
    '__F_BC700__': b64('fuentes/barlow-condensed-latin-700-normal.woff2'),
    '__LEON__': base64.b64encode(buf.getvalue()).decode(),
    '__VIDEO_JS__': open(os.path.join(R, 'informe-video.js'), encoding='utf-8').read().rstrip('\n'),
}.items():
    assert marca in s, marca
    s = s.replace(marca, valor)
open(os.path.join(R, 'informe-plantilla.html'), 'w', encoding='utf-8').write(s)
print('informe-plantilla.html:', len(s) // 1024, 'KB')
