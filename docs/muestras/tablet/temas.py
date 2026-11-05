T={
 'rally':dict(f1='#2a2219',f2='#120e0a',f3='#070504',t1='#3a3125',t2='#16120d',bd='#6d5f48',ac='#ffb000',at='#141008',ti='#f3e7c8',ra=14,fn="B",it='italic',brillo=False),
 'sport':dict(f1='#2b2f34',f2='#1a1c1f',f3='#111214',t1='#33383e',t2='#16181b',bd='#4a4f56',ac='#e10600',at='#ffffff',ti='#ffffff',ra=8,fn="K",it='normal',brillo=False),
 'cabina':dict(f1='#0e1215',f2='#05080a',f3='#000',t1='#15212a',t2='#070b0e',bd='#2b3640',ac='#2fd162',at='#04140a',ti='#e8eef2',ra=10,fn="P",it='normal',brillo=False)}
extra=f"""@font-face{{font-family:K;src:url(file://{R}/fuentes/chakra-petch-latin-700-normal.woff2);font-weight:800}}
@font-face{{font-family:K;src:url(file://{R}/fuentes/chakra-petch-latin-500-normal.woff2);font-weight:600}}
@font-face{{font-family:P;src:url(file://{R}/fuentes/b612-latin-700-normal.woff2);font-weight:800}}
@font-face{{font-family:P;src:url(file://{R}/fuentes/b612-latin-400-normal.woff2);font-weight:600}}"""
for k,v in T.items():
    css=f"""{extra}
body{{background:radial-gradient(circle at 20% -10%,{v['f1']},{v['f2']} 50%,{v['f3']});color:{v['ti']}}}
.bar{{background:linear-gradient({v['t1']},{v['t2']});border-bottom:4px solid {v['ac']}}}
.marca{{background:none;color:{v['ti']};-webkit-text-fill-color:{v['ti']};font-family:{v['fn']};font-style:{v['it']}}}
.pill{{border-color:{v['bd']};font-family:{v['fn']};font-style:{v['it']}}}
.t{{border-radius:{v['ra']*2}px;border:3px solid {v['bd']};background:linear-gradient(160deg,{v['t1']},{v['t2']});box-shadow:0 10px 24px #000a}}
.t::after{{display:none}}
.t .n{{font-family:{v['fn']};font-style:{v['it']}}}
.t .s{{color:{v['ac']};font-family:{v['fn']}}}
.t.y{{background:linear-gradient({v['ac']},{v['ac']});color:{v['at']};border-color:{v['ac']}}}
.t.y .s{{color:{v['at']};opacity:.85}}
.hora{{color:{v['ti']}}}
"""
    tiles=[tile("Carreras",ico("bandera",v['at'],80),"Próxima: Gualeguaychú 19/11","y"),
     tile("Práctica libre",ico("crono",v['ti'])," 4:00 · 3,2 km"),tile("Chequeo previo",ico("check",v['ti']),"Todo listo"),
     tile("Clima",ico("clima",s=80),"21° · Santa Clara"),tile("Pantalla de carrera",ico("pantalla",v['ti']),""),
     tile("Cámara",ico("camara",v['ti']),"Lista"),tile("Informes en la red",ico("red",v['ti']),"192.168.1.83"),tile("Ajustes",ico("ajustes",v['ti']),"v1.0.15")]
    b=barra().replace('<img src','<img style="filter:saturate(0.9)" src')
    open(f'{S}/inicio-{k}.html','w').write(f'<!doctype html><meta charset=utf-8><style>{CSS} body{{width:1280px;height:720px}} {css}</style>{b}<div class="grid" style="left:22px;top:96px;width:1236px;height:606px;grid-template-columns:repeat(4,1fr);grid-template-rows:1fr 1fr">{"".join(tiles)}</div>')
