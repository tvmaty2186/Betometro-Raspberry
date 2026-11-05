import sys, os
R='/home/user/Betometro-Raspberry/recursos'
S=sys.argv[1]
CSS=f"""
@font-face{{font-family:B;src:url(file://{R}/fuentes/barlow-latin-600-normal.woff2);font-weight:600}}
@font-face{{font-family:B;src:url(file://{R}/fuentes/barlow-latin-600-italic.woff2);font-weight:600;font-style:italic}}
@font-face{{font-family:B;src:url(file://{R}/fuentes/barlow-latin-800-italic.woff2);font-weight:800;font-style:italic}}
@font-face{{font-family:C;src:url(file://{R}/fuentes/barlow-condensed-latin-700-normal.woff2);font-weight:700}}
*{{box-sizing:border-box;margin:0}}
body{{font-family:B;color:#f4f1e6;overflow:hidden;position:relative;
 background:radial-gradient(circle at 20% -10%,#2f7a4b,#0f3d22 50%,#07200f)}}
.bar{{position:absolute;left:0;right:0;top:0;height:76px;display:flex;align-items:center;gap:16px;padding:0 22px;
 background:linear-gradient(#123d24,#081a0f);border-bottom:4px solid #f4c531}}
.bar img{{height:52px}}
.marca{{font:800 italic 34px B;background:linear-gradient(#fff,#9aa3ad 55%,#e8ebee);-webkit-background-clip:text;color:transparent}}
.esp{{flex:1}}
.pill{{display:flex;align-items:center;gap:10px;height:50px;padding:0 18px;border-radius:25px;background:#0007;border:2px solid #c9ced3;font:800 italic 24px B}}
.dot{{width:16px;height:16px;border-radius:50%}}
.hora{{font:700 40px C;letter-spacing:1px}}
.atras{{width:64px;height:64px;border-radius:20px;border:3px solid #c9ced3;display:grid;place-items:center;background:#0006}}
.tit{{font:800 italic 38px B;text-transform:uppercase;letter-spacing:1px}}
.grid{{position:absolute;display:grid;gap:18px}}
.t{{position:relative;border-radius:30px;border:3px solid #c9ced3;
 background:linear-gradient(160deg,#2f7a4b 0%,#14492b 55%,#0b2a17);box-shadow:0 10px 24px #0009, inset 0 2px 0 #ffffff40;
 display:flex;flex-direction:column;align-items:center;justify-content:center;gap:14px;overflow:hidden}}
.t::after{{content:"";position:absolute;left:8%;right:8%;top:6px;height:34%;border-radius:50%;background:linear-gradient(#ffffff26,#fff0)}}
.t .n{{font:800 italic 30px B;text-align:center;line-height:1.05}}
.t .s{{font:600 21px B;color:#f4c531;text-align:center}}
.t.y{{background:linear-gradient(#ffe07a,#f4c531 55%,#d9a20f);color:#111;border-color:#fff2b0}}
.t.y .s{{color:#3a2c00}}
.t.r{{background:linear-gradient(#e0473a,#a3170e);border-color:#ffb3aa}}
svg{{display:block}}
.fila{{display:flex;align-items:center;justify-content:space-between;height:96px;padding:0 26px;border-radius:24px;
 border:3px solid #c9ced3;background:linear-gradient(#24603b,#0e331d)}}
.fila .e{{font:800 italic 30px B}}
.seg{{display:flex;border-radius:22px;overflow:hidden;border:3px solid #c9ced3}}
.seg div{{padding:0 26px;height:64px;display:grid;place-items:center;font:800 italic 26px B;background:#0b2a17}}
.seg div.on{{background:linear-gradient(#ffe07a,#f4c531);color:#111}}
.sw{{width:120px;height:64px;border-radius:32px;border:3px solid #c9ced3;background:#0b2a17;position:relative}}
.sw::after{{content:"";position:absolute;top:6px;left:6px;width:46px;height:46px;border-radius:50%;background:linear-gradient(#fff,#b9bfc5)}}
.sw.on{{background:#f4c531}} .sw.on::after{{left:62px}}
.pm{{display:flex;align-items:center;gap:12px}}
.pm b{{width:72px;height:64px;border-radius:20px;border:3px solid #c9ced3;display:grid;place-items:center;font:800 40px B;background:#0b2a17}}
.pm span{{min-width:120px;text-align:center;font:700 44px C}}
"""
def ico(n,c="#fff",s=72):
    P={
    'bandera':'<path d="M14 58V10M14 12h30l-4 9 4 9H14" />',
    'crono':'<circle cx="32" cy="36" r="20"/><path d="M32 36V24M26 8h12M32 8v8"/>',
    'check':'<rect x="14" y="10" width="36" height="46" rx="6"/><path d="M22 34l7 7 13-14"/>',
    'pantalla':'<rect x="8" y="12" width="20" height="18" rx="4"/><rect x="34" y="12" width="22" height="18" rx="4"/><rect x="8" y="36" width="48" height="16" rx="4"/>',
    'camara':'<rect x="6" y="18" width="38" height="28" rx="6"/><path d="M44 28l14-8v24l-14-8z"/>',
    'red':'<path d="M8 26a34 34 0 0 1 48 0M16 34a22 22 0 0 1 32 0M24 42a10 10 0 0 1 16 0"/><circle cx="32" cy="50" r="3" fill="'+c+'"/>',
    'ajustes':'<circle cx="32" cy="32" r="9"/><path d="M32 6v9M32 49v9M6 32h9M49 32h9M13.6 13.6l6.4 6.4M44 44l6.4 6.4M13.6 50.4l6.4-6.4M44 20l6.4-6.4"/>',
    'voz':'<path d="M10 26h10l14-12v36L20 38H10z"/><path d="M42 22a14 14 0 0 1 0 20M48 16a22 22 0 0 1 0 32"/>',
    'tema':'<circle cx="32" cy="32" r="24"/><circle cx="22" cy="24" r="4"/><circle cx="36" cy="18" r="4"/><circle cx="44" cy="30" r="4"/>',
    'auto':'<path d="M8 40v-8l6-12h36l6 12v8z"/><circle cx="18" cy="42" r="5"/><circle cx="46" cy="42" r="5"/>',
    'gps':'<circle cx="32" cy="28" r="8"/><path d="M32 58S14 40 14 28a18 18 0 0 1 36 0c0 12-18 30-18 30z"/>',
    'velocidad':'<path d="M10 44a22 22 0 1 1 44 0"/><path d="M32 44l12-14"/>',
    'actualizar':'<path d="M50 30A18 18 0 1 0 46 44"/><path d="M50 18v12H38"/>',
    'apagar':'<path d="M32 8v22"/><path d="M20 16a20 20 0 1 0 24 0"/>',
    'mic':'<rect x="24" y="8" width="16" height="30" rx="8"/><path d="M16 30a16 16 0 0 0 32 0M32 46v10"/>',
    'girar':'<path d="M48 22A18 18 0 1 0 50 36"/><path d="M50 10v12H38"/>',
    'rec':'<circle cx="32" cy="32" r="16" fill="#ff3b30" stroke="none"/>',
    'atras':'<path d="M38 14L20 32l18 18"/>',
    'tarjeta':'<path d="M14 8h26l10 10v38H14z"/><path d="M22 18v8M30 18v8M38 18v8"/>',
    'clima':'<circle cx="24" cy="24" r="9" stroke="#f4c531" fill="#f4c531"/><path d="M24 6v4M10 10l3 3M6 24h4M38 10l-3 3" stroke="#f4c531"/><path d="M22 52h24a10 10 0 0 0 0-20 13 13 0 0 0-24 4 8 8 0 0 0 0 16z" fill="#eef1f3" stroke="#eef1f3"/>',
    'lluvia':'<path d="M18 38h28a10 10 0 0 0 0-20 13 13 0 0 0-24 4 8 8 0 0 0-4 16z" fill="#b9c0c6" stroke="#b9c0c6"/><path d="M22 46l-3 8M32 46l-3 8M42 46l-3 8" stroke="#5fb4ff"/>',
    'sol':'<circle cx="32" cy="32" r="11" fill="#f4c531" stroke="#f4c531"/><path d="M32 8v6M32 50v6M8 32h6M50 32h6M15 15l4 4M45 45l4 4M15 49l4-4M45 19l4-4" stroke="#f4c531"/>',
    }
    return f'<svg width="{s}" height="{s}" viewBox="0 0 64 64" fill="none" stroke="{c}" stroke-width="5" stroke-linecap="round" stroke-linejoin="round">{P[n]}</svg>'
def barra(titulo=None, w=1280):
    izq = f'<div class="atras">{ico("atras",s=40)}</div><div class="tit">{titulo}</div>' if titulo else f'<img src="file://{R}/img/leon.png"><div class="marca">BETÓMETRO</div>'
    return f'''<div class="bar" style="width:{w}px">{izq}<div class="esp"></div>
 <div class="pill"><div class="dot" style="background:#5ce68a"></div>GPS 10</div>
 <div class="pill">{ico("red",s=34)}</div><div class="hora">14:05</div></div>'''
def page(nombre,w,h,body):
    open(f'{S}/{nombre}.html','w').write(f'<!doctype html><meta charset=utf-8><style>{CSS} body{{width:{w}px;height:{h}px}}</style>{body}')
def tile(n,icon,sub='',cls='',extra=''):
    return f'<div class="t {cls}" {extra}>{icon}<div class="n">{n}</div>{f"<div class=s>{sub}</div>" if sub else ""}</div>'
exec(open(sys.argv[2]).read())
