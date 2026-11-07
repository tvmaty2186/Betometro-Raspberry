# Muestras de «Recorrido libre» (07/11/2026). Uso: python3 ../tablet/gen.py <salida> recorrido.py
import math
# ---------- 1. Práctica libre con el botón nuevo ----------
def dato(n,v,u=''):
    return f'''<div class="fila" style="height:118px;flex-direction:column;align-items:flex-start;justify-content:center;gap:2px">
 <div style="font:800 italic 22px B;letter-spacing:2px;opacity:.85">{n}</div>
 <div style="display:flex;align-items:center;width:100%;justify-content:space-between">
 <b style="width:84px;height:64px;border-radius:22px;border:3px solid #c9ced3;display:grid;place-items:center;font:800 44px B;background:#0b2a17">−</b>
 <span style="font:700 64px C">{v}<span style="font-size:32px"> {u}</span></span>
 <b style="width:84px;height:64px;border-radius:22px;border:3px solid #c9ced3;display:grid;place-items:center;font:800 44px B;background:#0b2a17">+</b></div></div>'''
ruta='<svg width="70" height="70" viewBox="0 0 64 64" fill="none" stroke="#fff" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="50" r="5"/><path d="M17 48c14-4 4-22 18-26s14 10 20-8"/><circle cx="55" cy="12" r="4" fill="#fff"/></svg>'
pra=f'''<div class="grid" style="left:22px;top:96px;width:560px;grid-template-columns:1fr;gap:14px">
 {dato("TIEMPO IDEAL","4:00,0")}{dato("DISTANCIA","3.200","m")}
 <div class="fila" style="height:110px"><div class="e">Medidas</div><div class="seg"><div class="on">Impares</div><div>Pares</div><div>Todas</div></div></div>
 <div class="t" style="height:118px;flex-direction:row;justify-content:flex-start;gap:20px;padding-left:26px;border-color:#f4c531;border-width:4px">{ruta}
  <div style="text-align:left"><div class="n" style="text-align:left">Recorrido libre</div><div class="s" style="text-align:left">Sin vueltas, sin tiempo, sin distancia</div></div></div>
</div>
<div class="t y" style="position:absolute;left:604px;top:96px;width:654px;height:606px">{ico("bandera","#111",120)}
 <div style="font:800 italic 120px B;letter-spacing:4px">LARGAR</div>
 <div style="font:600 26px B;color:#3a2c00">Práctica con vueltas · promedio 48,0 km/h</div></div>'''
page('1-practica',1280,720,barra("Práctica libre")+pra)

# ---------- 2. En marcha ----------
def caja(et,v,u='',w=0,h=0,x=0,y=0,tam=110,extra=''):
    return f'''<div class="t" style="position:absolute;left:{x}px;top:{y}px;width:{w}px;height:{h}px;gap:0">
 <div style="font:800 italic 22px B;letter-spacing:3px;opacity:.85">{et}</div>
 <div style="font:800 italic {tam}px B;line-height:1;color:#f4f1e6">{v}<span style="font-size:{tam*0.36:.0f}px;opacity:.8"> {u}</span></div>{extra}</div>'''
arriba='''<div style="position:absolute;left:24px;top:22px;font:600 italic 22px B;letter-spacing:3px;opacity:.85">RECORRIDO LIBRE · LARGÓ 14:05</div>
<div style="position:absolute;right:24px;top:6px;display:flex;gap:10px">
 <div class="pill" style="height:50px;font-size:23px"><div class="dot" style="background:#ff3b30;width:16px;height:16px"></div>REC</div>
 <div class="pill" style="height:50px;font-size:23px"><div class="dot" style="background:#5ce68a"></div>GPS 10 Hz · 2 m</div>
 <div class="pill" style="height:50px;font-size:23px">Terminar</div></div>'''
marcha=arriba+caja("TIEMPO","0:23:41",'',760,300,22,72,150, '<div style="font:600 24px B;color:#f4c531;margin-top:6px">parado 1:12</div>') \
 + caja("DISTANCIA","18,42","km",476,300,782,72,120) \
 + caja("VELOCIDAD KM/H","56",'',360,300,22,390,130,'<div style="width:280px;height:10px;border-radius:5px;margin-top:14px;background:linear-gradient(90deg,#5ce68a,#f4c531,#e0473a);position:relative"><div style="position:absolute;left:44%;top:-6px;width:4px;height:22px;background:#fff"></div></div>') \
 + caja("MÁXIMA","98",'km/h',190,145,400,390,64) + caja("PROMEDIO","46,6",'km/h',190,145,400,545,64) \
 + '''<div class="t y" style="position:absolute;left:608px;top:390px;width:650px;height:300px;gap:6px">
 <div style="font:800 italic 96px B;letter-spacing:8px">MARCAR</div>
 <div style="font:800 italic 26px B;letter-spacing:3px">DEJA LA MARCA 4 · NO CORTA NADA</div></div>'''
page('2-en-marcha',1280,720,marcha)

# ---------- 3. Informe ----------
# recorrido inventado, pintado por velocidad
pts=[]; x,y=60,420; a=-0.3
import random; random.seed(7)
for i in range(240):
    a+=math.sin(i/14)*0.09+random.uniform(-0.04,0.04); x+=math.cos(a)*2.3; y+=math.sin(a)*1.6-0.25
    pts.append((x,y))
# que el recorrido ocupe toda la caja del mapa (576×500, con margen)
mx=[p[0] for p in pts]; my=[p[1] for p in pts]
k=min((576-90)/(max(mx)-min(mx)),(500-110)/(max(my)-min(my)))
ky=(500-130)/(max(my)-min(my))
pts=[(45+(px-min(mx))*k, 70+(py-min(my))*ky) for px,py in pts]
def col(v):
    return '#3b82f6' if v<35 else '#5ce68a' if v<55 else '#f4c531' if v<75 else '#e0473a'
vel=[45+35*math.sin(i/19)+10*math.sin(i/5) for i in range(len(pts))]
seg=''.join(f'<line x1="{pts[i][0]:.1f}" y1="{pts[i][1]:.1f}" x2="{pts[i+1][0]:.1f}" y2="{pts[i+1][1]:.1f}" stroke="{col(vel[i])}" stroke-width="9" stroke-linecap="round"/>' for i in range(len(pts)-1))
marcas=[(62,'1'),(110,'2'),(160,'3'),(205,'4')]
mk=''.join(f'<circle cx="{pts[i][0]:.0f}" cy="{pts[i][1]:.0f}" r="15" fill="#f4c531" stroke="#111" stroke-width="3"/><text x="{pts[i][0]:.0f}" y="{pts[i][1]+7:.0f}" text-anchor="middle" font-family="B" font-weight="800" font-size="20" fill="#111">{n}</text>' for i,n in marcas)
ini=f'<circle cx="{pts[0][0]:.0f}" cy="{pts[0][1]:.0f}" r="12" fill="#fff"/><text x="{pts[0][0]+18:.0f}" y="{pts[0][1]+28:.0f}" font-family="B" font-weight="800" font-style="italic" font-size="18" fill="#fff">LARGADA</text>'
fin=f'<rect x="{pts[-1][0]-10:.0f}" y="{pts[-1][1]-10:.0f}" width="20" height="20" fill="#111" stroke="#fff" stroke-width="3"/><text x="{pts[-1][0]-20:.0f}" y="{pts[-1][1]-18:.0f}" text-anchor="end" font-family="B" font-weight="800" font-style="italic" font-size="18" fill="#fff">LLEGADA</text>'
gx=[i*560/len(vel) for i in range(len(vel))]
gy=[150-v*1.25 for v in vel]
graf=' '.join(f'{gx[i]:.1f},{gy[i]:.1f}' for i in range(len(vel)))
gm=''.join(f'<line x1="{gx[i]:.0f}" y1="8" x2="{gx[i]:.0f}" y2="150" stroke="#f4c531" stroke-dasharray="4 4" stroke-width="2"/><text x="{gx[i]+4:.0f}" y="22" font-family="B" font-weight="800" font-size="16" fill="#f4c531">{n}</text>' for i,n in marcas)
def d(et,v):
    return f'<div style="display:flex;flex-direction:column"><span style="font:800 italic 15px B;letter-spacing:2px;opacity:.75">{et}</span><span style="font:700 38px C">{v}</span></div>'
inf=f'''<div class="bar" style="width:1280px"><div class="atras">{ico("atras",s=40)}</div>
 <div><div class="tit" style="font-size:32px">Recorrido libre</div><div style="font:600 italic 18px B;letter-spacing:2px;opacity:.8">07/11 · 14:05 · RECORRIDO</div></div>
 <div class="esp"></div>
 <div class="seg" style="border-width:2px"><div class="on" style="height:50px;font-size:22px">Informe</div><div style="height:50px;font-size:22px">Video</div></div>
 <div class="pill" style="background:linear-gradient(#ffe07a,#f4c531);color:#111;border-color:#fff2b0">Compartir</div></div>
<div class="t" style="position:absolute;left:22px;top:96px;width:620px;height:606px;justify-content:flex-start;align-items:stretch;padding:18px 22px">
 <div style="display:flex;justify-content:space-between;font:800 italic 17px B;letter-spacing:2px;opacity:.85"><span>RECORRIDO · COLOR = VELOCIDAD</span>
 <span style="display:flex;gap:6px"><b style="background:#f4c531;color:#111;border-radius:12px;padding:2px 10px">Vel.</b><b style="border:2px solid #c9ced3;border-radius:12px;padding:0 10px">Acel.</b></span></div>
 <svg width="576" height="520" viewBox="0 0 576 520">{seg}{ini}{fin}{mk}</svg></div>
<div class="t" style="position:absolute;left:660px;top:96px;width:598px;height:200px;justify-content:flex-start;align-items:stretch;padding:16px 18px;gap:4px">
 <div style="font:800 italic 17px B;letter-spacing:2px;opacity:.85">VELOCIDAD KM/H · A LO LARGO DEL RECORRIDO</div>
 <svg width="560" height="155" viewBox="0 0 560 155"><line x1="0" y1="150" x2="560" y2="150" stroke="#ffffff40"/>{gm}<polyline points="{graf}" fill="none" stroke="#f4f1e6" stroke-width="3"/></svg></div>
<div class="t" style="position:absolute;left:660px;top:312px;width:598px;height:180px;display:grid;grid-template-columns:repeat(3,1fr);gap:8px 14px;padding:18px 24px;align-items:center;justify-items:start">
 {d("TIEMPO","0:42:18")}{d("DISTANCIA","32,7 km")}{d("PROMEDIO","46,4 km/h")}
 {d("MÁXIMA","98 km/h")}{d("PARADO","3:05")}{d("ACEL. MÁX.","3,1 m/s²")}</div>
<div class="t" style="position:absolute;left:660px;top:508px;width:598px;height:194px;justify-content:flex-start;align-items:stretch;padding:16px 22px;gap:6px">
 <div style="font:800 italic 17px B;letter-spacing:2px;opacity:.85">MARCAS · TOCÁ UNA Y EL VIDEO VA AHÍ</div>
 {''.join(f'<div style="display:flex;gap:18px;align-items:center;font:800 italic 22px B"><b style="width:30px;height:30px;border-radius:50%;background:#f4c531;color:#111;display:grid;place-items:center;font-size:18px">{n}</b><span style="font:700 26px C;width:96px">{t}</span><span style="font:700 26px C;width:110px">{k}</span><span style="color:#f4c531">{v}</span></div>' for n,t,k,v in [("1","0:06:40","5,1 km","38 km/h"),("2","0:16:02","12,4 km","71 km/h"),("3","0:26:15","20,2 km","52 km/h"),("4","0:36:30","28,1 km","44 km/h")])}
</div>'''
page('3-informe',1280,720,inf)
