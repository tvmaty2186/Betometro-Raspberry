tiles=[tile("Carreras",ico("bandera","#111",80),"Próxima: Gualeguaychú 19/11","y"),
 tile("Práctica libre",ico("crono"),"4:00 · 3,2 km"),
 tile("Chequeo previo",ico("check"),"Todo listo"),
 tile("Clima",ico("clima",s=80),"21° · Santa Clara"),
 tile("Pantalla de carrera",ico("pantalla"),""),
 tile("Cámara",ico("camara"),"Lista"),
 tile("Informes en la red",ico("red"),"192.168.1.83"),
 tile("Ajustes",ico("ajustes"),"v1.0.15")]
page('inicio-h',1280,720,barra()+f'<div class="grid" style="left:22px;top:96px;width:1236px;height:606px;grid-template-columns:repeat(4,1fr);grid-template-rows:1fr 1fr">{"".join(tiles)}</div>')
page('inicio-v',720,1280,barra(w=720).replace('<div class="marca">BETÓMETRO</div>','<div class="marca" style="font-size:28px">BETÓMETRO</div>')+f'<div class="grid" style="left:18px;top:96px;width:684px;height:1166px;grid-template-columns:1fr 1fr;grid-template-rows:repeat(4,1fr)">{"".join(tiles)}</div>')
