# Cámara: vista previa grande + botones grandes
cam=f'''<div style="position:absolute;left:22px;top:96px;width:740px;height:606px;border-radius:30px;border:3px solid #c9ced3;overflow:hidden;
 background:linear-gradient(#5d8fb0,#a9c6d6 45%,#5a6b4a 46%,#3d4a32)">
 <div style="position:absolute;left:0;right:0;bottom:0;height:46%;background:linear-gradient(80deg,transparent 40%,#555 41%,#666 59%,transparent 60%)"></div>
 <div style="position:absolute;left:18px;top:16px" class="pill">{ico("camara",s=30)} Vista previa</div>
 <div style="position:absolute;right:18px;top:16px" class="pill"><div class="dot" style="background:#5ce68a"></div>MJPEG · lista</div></div>
<div class="grid" style="left:780px;top:96px;width:478px;height:606px;grid-template-columns:1fr 1fr;grid-template-rows:repeat(3,1fr);gap:16px">
 {tile("Grabar",ico("rec"),"Prendida","y")}
 {tile("Probar 10 s",ico("crono"),"")}
 {tile("Girar 180°",ico("girar"),"")}
 {tile("Calidad",ico("pantalla",s=60),"720p")}
 {tile("Sonido",ico("mic"),"De la cámara")}
 {tile("Lugar",ico("tarjeta"),"Entran 14 h")}
</div>'''
page('camara-h',1280,720,barra("Cámara")+cam)
# Práctica libre: tres números grandes y LARGAR enorme
def dato(n,v,u=''):
    return f'''<div class="fila" style="height:132px;flex-direction:column;align-items:flex-start;justify-content:center;gap:4px">
 <div style="font:800 italic 22px B;letter-spacing:2px;opacity:.85">{n}</div>
 <div style="display:flex;align-items:center;width:100%;justify-content:space-between">
 <b style="width:84px;height:72px;border-radius:22px;border:3px solid #c9ced3;display:grid;place-items:center;font:800 44px B;background:#0b2a17">−</b>
 <span style="font:700 70px C">{v}<span style="font-size:34px"> {u}</span></span>
 <b style="width:84px;height:72px;border-radius:22px;border:3px solid #c9ced3;display:grid;place-items:center;font:800 44px B;background:#0b2a17">+</b></div></div>'''
pra=f'''<div class="grid" style="left:22px;top:96px;width:560px;grid-template-columns:1fr;gap:16px">
 {dato("TIEMPO IDEAL","4:00,0")}{dato("DISTANCIA","3.200","m")}
 <div class="fila" style="height:132px"><div class="e">Medidas</div><div class="seg"><div class="on">Impares</div><div>Pares</div><div>Todas</div></div></div>
 <div style="font:800 italic 26px B;color:#f4c531;padding-left:8px">Promedio 48,0 km/h</div></div>
<div class="t y" style="position:absolute;left:604px;top:96px;width:654px;height:606px">{ico("bandera","#111",120)}
 <div style="font:800 italic 120px B;letter-spacing:4px">LARGAR</div></div>'''
page('practica-h',1280,720,barra("Práctica libre")+pra)
# Clima: grande y simple
dias=[("Hoy","clima",11,20,0),("Vie","lluvia",10,19,80),("Sáb","clima",12,21,20),("Dom","sol",15,24,30),("Lun","sol",18,27,0)]
dd="".join(f'''<div class="t" style="gap:6px"><div class="n" style="font-size:28px">{d}</div>{ico(i,s=70)}
 <div style="font:700 40px C">{b}°</div><div style="font:600 22px B;opacity:.8">{a}°</div>
 <div style="font:600 22px B;color:#5fb4ff">{l}%</div></div>''' for d,i,a,b,l in dias)
cli=f'''<div class="fila" style="position:absolute;left:22px;top:96px;width:1236px;height:250px;justify-content:flex-start;gap:30px">
 {ico("clima",s=170)}<div style="font:700 150px C;line-height:1">21°</div>
 <div style="display:flex;flex-direction:column;gap:10px;font:800 italic 30px B">
 <div>Santa Clara del Mar</div><div style="color:#5fb4ff">Lluvia 10 %</div><div>Viento 18 · ráfagas 32</div></div>
 <div class="esp"></div><div style="display:flex;flex-direction:column;gap:14px">
 <div class="pill" style="height:64px;background:#f4c531;color:#111;border-color:#f4c531">{ico("gps","#111",32)} GPS</div>
 <div class="pill" style="height:64px">Buscar…</div></div></div>
<div class="grid" style="left:22px;top:364px;width:1236px;height:338px;grid-template-columns:repeat(5,1fr)">{dd}</div>'''
page('clima-h',1280,720,barra("Clima")+cli)
