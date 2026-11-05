t=[tile("Mi auto",ico("auto"),"101 · Avispón Verde"),
 tile("Pantalla y tema",ico("tema"),"Avispón · brillo 100%"),
 tile("Voz y sonido",ico("voz"),"Elena · 20 s"),
 tile("Velocidad y GPS",ico("velocidad"),"Réplica NEO-M8N"),
 tile("Conectividad",ico("red"),"Maty-WIFI 5 GHz"),
 tile("Cámara",ico("camara"),"MJPEG · lista"),
 tile("Actualizar",ico("actualizar"),"Estás al día"),
 tile("Apagar",ico("apagar"),"Tocá 2 veces","r")]
page('ajustes-h',1280,720,barra("Ajustes")+f'<div class="grid" style="left:22px;top:96px;width:1236px;height:606px;grid-template-columns:repeat(4,1fr);grid-template-rows:1fr 1fr">{"".join(t)}</div>')
# Una sección por dentro: Voz y sonido, filas grandes
filas=f'''
<div class="fila"><div class="e">Volumen</div><div class="pm"><b>−</b><span>80%</span><b>+</b>
 <div class="pill" style="height:64px;border-radius:20px">{ico("voz",s=30)} Probar</div></div></div>
<div class="fila"><div class="e">Voz</div><div class="seg"><div class="on">Elena</div><div>Tomás</div></div></div>
<div class="fila"><div class="e">Cuenta en voz</div><div class="pm"><b>−</b><span>20 s</span><b>+</b></div></div>
<div class="fila"><div class="e">Anunciar cada vuelta</div><div class="sw on"></div></div>
<div class="fila"><div class="e">Clic al marcar</div><div class="sw on"></div></div>'''
page('voz-h',1280,720,barra("Voz y sonido")+f'<div class="grid" style="left:22px;top:96px;width:1236px;grid-template-columns:1fr;gap:14px">{filas}</div>')
