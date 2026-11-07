// Muestras de la 1.0.29 (para que Matías apruebe antes de programar). Genera los HTML y les saca foto.
// Uso: node docs/muestras/1.0.29/armar.js   (necesita playwright; el QR usa el paquete «qrcode» si está, si no dibuja uno fijo)
const fs = require('fs'), path = require('path');
const D = __dirname;
const LEON = '../../../recursos/img/leon.png';

// ---------- íconos (TIcono.qml pasado a SVG; viewBox 64, trazo 5) ----------
const PATHS = {
  bandera: '<path d="M14 58V10M14 12h32l-5 9 5 9H14"/>',
  crono: '<circle cx="32" cy="36" r="20"/><path d="M32 36V24M26 8h12M32 8v8"/>',
  check: '<rect x="14" y="10" width="36" height="46" rx="6"/><path d="M22 34l7 7 13-14"/>',
  pantalla: '<rect x="8" y="12" width="20" height="18" rx="4"/><rect x="34" y="12" width="22" height="18" rx="4"/><rect x="8" y="36" width="48" height="16" rx="4"/>',
  camara: '<rect x="6" y="18" width="38" height="28" rx="6"/><path d="M44 28l14-8v24l-14-8z"/>',
  red: '<path d="M9 31a30 30 0 0 1 46 0M17 38a19 19 0 0 1 30 0M26 45a8 8 0 0 1 12 0"/><circle cx="32" cy="50" r="3" fill="currentColor"/>',
  ajustes: '<circle cx="32" cy="32" r="9"/>' + [0, 1, 2, 3, 4, 5, 6, 7].map(i => { const a = i * Math.PI / 4; return `<path d="M${(32 + Math.cos(a) * 16).toFixed(1)} ${(32 + Math.sin(a) * 16).toFixed(1)}L${(32 + Math.cos(a) * 25).toFixed(1)} ${(32 + Math.sin(a) * 25).toFixed(1)}"/>`; }).join(''),
  voz: '<path d="M10 26h10l14-12v36L20 38H10z"/><path d="M42.5 23.5a12 12 0 0 1 0 17M48 18a20 20 0 0 1 0 28"/>',
  tema: '<circle cx="32" cy="32" r="24"/><circle cx="22" cy="24" r="4"/><circle cx="36" cy="18" r="4"/><circle cx="44" cy="30" r="4"/>',
  gps: '<circle cx="32" cy="28" r="8"/><path d="M32 58C20 44 14 36 14 28a18 18 0 0 1 36 0c0 8-6 16-18 30z"/>',
  velocidad: '<path d="M10 44a22 22 0 0 1 44 0"/><path d="M32 44l12-14"/>',
  apagar: '<path d="M32 8v22"/><path d="M20.5 15.6a20 20 0 1 0 23 0"/>',
  atras: '<path d="M38 14L20 32l18 18"/>',
  informe: '<path d="M12 54V10M12 54h44M18 44l12-14 8 8 14-18"/>',
  ruta: '<circle cx="12" cy="50" r="5"/><circle cx="54" cy="12" r="4" fill="currentColor"/><path d="M17 48c14-4 4-22 18-26s14 10 19-6"/>',
  qr: '<rect x="8" y="8" width="20" height="20" rx="3"/><rect x="36" y="8" width="20" height="20" rx="3"/><rect x="8" y="36" width="20" height="20" rx="3"/><path d="M38 38h16v16M38 48h8"/>',
  simulador: '<path d="M49 22a20 20 0 1 0 4 16"/><path d="M50 10v12H38"/><path d="M27 24v18l15-9z"/>',
  alerta: '<path d="M32 8L58 54H6z"/><path d="M32 26v14"/><circle cx="32" cy="47" r="1.5" fill="currentColor"/>',
};
// Engranajes de verdad: rueda con dientes y agujero en el medio
function gear(n, ro, ri, ht, hb, hole, extra = '') {
  const step = 2 * Math.PI / n, P = (r, a) => `${(32 + r * Math.cos(a)).toFixed(2)} ${(32 + r * Math.sin(a)).toFixed(2)}`;
  let d = '';
  for (let i = 0; i < n; i++) {
    const a = i * step - Math.PI / 2;
    const p0 = P(ri, a - step * hb), p1 = P(ro, a - step * ht), p2 = P(ro, a + step * ht), p3 = P(ri, a + step * hb);
    d += (i === 0 ? 'M' + p0 : 'A' + ri + ' ' + ri + ' 0 0 1 ' + p0) + 'L' + p1 + 'L' + p2 + 'L' + p3;
  }
  d += 'A' + ri + ' ' + ri + ' 0 0 1 ' + P(ri, -Math.PI / 2 - step * hb) + 'Z';
  return `<path d="${d}"/><circle cx="32" cy="32" r="${hole}"/>` + extra;
}
PATHS.engranajeA = gear(8, 27, 20, 0.17, 0.30, 8);
PATHS.engranajeB = gear(6, 27, 19, 0.21, 0.35, 9);
PATHS.engranajeC = gear(10, 27, 21, 0.15, 0.27, 7);
function ico(n, s = 72, c = 'currentColor') {
  return `<svg width="${s}" height="${s}" viewBox="0 0 64 64" fill="none" stroke="${c}" stroke-width="5" stroke-linecap="round" stroke-linejoin="round" style="color:${c}">${PATHS[n]}</svg>`;
}
// IconoClima.qml: sol y sol con nube
function climaIco(tipo, s = 79) {
  const sol = (x, y, r) => [0, 1, 2, 3, 4, 5, 6, 7].map(i => { const a = i * Math.PI / 4; return `<path d="M${(x + Math.cos(a) * r * 1.35).toFixed(1)} ${(y + Math.sin(a) * r * 1.35).toFixed(1)}L${(x + Math.cos(a) * r * 1.75).toFixed(1)} ${(y + Math.sin(a) * r * 1.75).toFixed(1)}" stroke="#f4c531" stroke-width="2.6" stroke-linecap="round"/>`; }).join('') + `<circle cx="${x}" cy="${y}" r="${r}" fill="#f4c531"/>`;
  const nube = (x, y, w, col) => { const h = w * 0.42; return `<path d="M${x + h / 2} ${y}A${h / 2} ${h / 2} 0 0 1 ${x + h / 2} ${y - h}A${h * .62} ${h * .62} 0 0 1 ${x + w - h / 2} ${y - h}A${h / 2} ${h / 2} 0 0 1 ${x + w - h / 2} ${y}Z" fill="${col}" stroke="rgba(0,0,0,.25)"/>`; };
  const body = tipo === 'sol' ? sol(24, 24, 9) : sol(18, 17, 7) + nube(12, 38, 30, '#eef1f3');
  return `<svg width="${s}" height="${s}" viewBox="0 0 48 48" fill="none">${body}</svg>`;
}
const wifiIco = `<svg width="38" height="30" viewBox="0 0 38 30" fill="none" stroke="#f7f7f2" stroke-width="3" stroke-linecap="round"><path d="M3 11a23 23 0 0 1 32 0M9 17a15 15 0 0 1 20 0M14.5 23a7 7 0 0 1 9 0"/><circle cx="19" cy="27" r="2.5" fill="#f7f7f2" stroke="none"/></svg>`;

// ---------- piezas ----------
function barra(titulo, hora = '15:40', gps = 'GPS 11') {
  const izq = titulo ? `<div class="atras">${ico('atras', 38)}</div><div class="tit">${titulo}</div>`
    : `<img class="leon" src="${LEON}"><div class="tit" style="font-size:32px">BETÓMETRO</div><div class="pill ver">v1.0.29</div>`;
  return `<div class="bar"><div class="izq">${izq}</div><div class="der"><div class="pill"><div class="dot"></div>${gps}</div><div class="wifi">${wifiIco}</div><div class="hora">${hora}</div></div></div>`;
}
const st = (o) => Object.entries(o).map(([k, v]) => `${k}:${typeof v === 'number' ? v + 'px' : v}`).join(';');
function boton({ x, y, w, h, cls = '', icono = '', tamIcono = 72, nombre, tamNombre = 29, dato = '', html = '', clima = '' }) {
  const n = nombre ? `<div class="n" style="font-size:${tamNombre}px${tamNombre > 60 ? ';letter-spacing:4px' : ''}">${nombre}</div>` : '';
  return `<div class="b ${cls}" style="${st({ left: x, top: y, width: w, height: h })}">${html}${clima || (icono ? ico(icono, tamIcono) : '')}${n}${dato ? `<div class="d">${dato}</div>` : ''}</div>`;
}
function fila({ x, y, w, h = 96, cls = '', e, det = '', der = '' }) {
  return `<div class="fila ${cls}" style="${st({ left: x, top: y, width: w, height: h })}"><div class="txt"><div class="e">${e}</div>${det ? `<div class="det">${det}</div>` : ''}</div><div class="der">${der}</div></div>`;
}
const masmenos = (v) => `<div class="pm"><div class="k">−</div><div class="v">${v}</div><div class="k">+</div></div>`;
const seg = (ops, on) => `<div class="seg">${ops.map(o => `<div class="${o === on ? 'on' : ''}">${o}</div>`).join('')}</div>`;
function page(nombre, titulo, body) {
  fs.writeFileSync(path.join(D, nombre + '.html'), `<!doctype html><html lang="es"><meta charset="utf-8"><title>${titulo}</title><link rel="stylesheet" href="kit.css"><body>${barra(titulo === 'Inicio' ? '' : titulo)}<div class="area">${body}</div></body></html>`);
  return nombre;
}

// Grilla 4×2 del Modo Tablet (295,5 × 294 con 18 de separación)
const AN = (1236 - 54) / 4, AL = (606 - 18) / 2;
const celda = (i, al, o) => boton({ x: (i % 4) * (AN + 18), y: Math.floor(i / 4) * (al + 18), w: AN, h: al, ...o });

const paginas = [];

// ============ 1. Tiempo por casilleros (Práctica) ============
function cajaTiempo({ segOn = false, segTxt = '00', total = '4:00:0' }) {
  const grupo = (sub, val, on, x) => `<div style="position:absolute;left:${x}px;top:44px" class="sub">${sub}</div>
    <div style="position:absolute;left:${x}px;top:72px;display:flex;align-items:center;gap:8px"><div class="tk">−</div><div class="cas ${on ? 'on' : ''}">${val}</div><div class="tk">+</div></div>`;
  return `<div class="dato" style="left:0;top:0;width:560px;height:150px">
    <div class="et">Tiempo ideal</div>
    <div style="position:absolute;right:26px;top:6px;font:700 italic 38px C;color:#f3ecd8">${total}</div>
    ${grupo('Minutos', '4', false, 19)}
    <div style="position:absolute;left:267px;top:66px;width:26px;text-align:center;font:700 italic 60px C;color:#f3ecd8;line-height:1">:</div>
    ${grupo('Segundos', segTxt, segOn, 293)}
  </div>`;
}
function practica(opts, teclado) {
  const col = cajaTiempo(opts)
    + `<div class="dato" style="left:0;top:166px;width:560px"><div class="et">Distancia</div><div class="tecla" style="left:22px">−</div><div class="tecla" style="right:22px">+</div><div class="num">3.200 <small>m</small></div></div>`
    + fila({ x: 0, y: 318, w: 560, h: 110, e: 'Medidas', der: seg(['Impares', 'Pares', 'Todas'], 'Impares') })
    + `<div style="position:absolute;left:8px;top:444px;font:800 italic 27px B;color:#f4c531">Promedio 48,0 km/h</div>`
    + fila({ x: 0, y: 492, w: 560, h: 110, e: 'Recorrido libre', det: 'Sin vueltas, sin tiempo, sin distancia', der: ico('ruta', 64, '#f4c531') });
  const der = teclado
    ? `<div class="panel" style="left:582px;top:0;width:654px;height:606px"><div class="cab"><b>Escribí los segundos</b><span>de 0 a 59 · de izquierda a derecha</span></div>
       <div class="teclas"><div>1</div><div>2</div><div>3</div><div>4</div><div>5</div><div>6</div><div>7</div><div>8</div><div>9</div><div>⌫</div><div>0</div><div class="ok">✓</div></div></div>`
    : boton({ x: 582, y: 0, w: 654, h: 606, cls: 'y', icono: 'bandera', tamIcono: 120, nombre: 'LARGAR', tamNombre: 110 });
  return col + der;
}
paginas.push(page('1-tiempo-casilleros', 'Práctica libre', practica({})));
paginas.push(page('1-tiempo-casilleros-b', 'Práctica libre', practica({ segOn: true, segTxt: '3<span class="cur"></span>', total: '4:3_:0' }, true)));
// La misma idea en la ficha de carrera
function fichaGualeguaychu() {
  const F = (i, e, det, der, cls = '') => fila({ x: 0, y: i * 100, w: 798, h: 90, e, det, der, cls });
  const cas = (v, on) => `<div class="pm" style="gap:10px"><div class="k" style="width:70px">−</div><div class="cas chica ${on ? 'on' : ''}">${v}</div><div class="k" style="width:70px">+</div></div>`;
  return F(0, 'Nombre', 'Gualeguaychú', ico('bandera', 40)) + F(1, 'Localidad', 'Gualeguaychú, Entre Ríos', ico('lugar' in PATHS ? 'lugar' : 'gps', 40))
    + F(2, 'Fecha', '', masmenos('21/11/26'))
    + F(3, 'Tiempo ideal', '4:00:0', `${cas('4')}<div style="font:700 italic 46px C;color:#f3ecd8;margin:0 2px">:</div>${cas('00')}`)
    + F(4, 'Distancia', '', masmenos('3.200 m'))
    + F(5, 'Medidas', '', seg(['Impares', 'Pares', 'Todas'], 'Impares'))
    + boton({ x: 816, y: 0, w: 420, h: 470, cls: 'y viejo', icono: 'bandera', tamIcono: 90, nombre: 'LARGAR', tamNombre: 70, dato: 'Se larga el 21 nov 2026' })
    + boton({ x: 816, y: 486, w: 202, h: 120, nombre: 'Guardar', tamNombre: 27 }) + boton({ x: 1034, y: 486, w: 202, h: 120, nombre: 'Borrar', tamNombre: 27 });
}
PATHS.lugar = PATHS.gps;
paginas.push(page('1-tiempo-casilleros-c', 'Gualeguaychú', fichaGualeguaychu()));

// ============ 2. Engranajes ============
function inicio(fila2) {
  return celda(0, AL, { cls: 'y', icono: 'bandera', nombre: 'Carreras', dato: 'Próxima: Gualeguaychú 21/11' })
    + celda(1, AL, { icono: 'crono', nombre: 'Práctica libre', dato: '4:00 · 3,2 km' })
    + celda(2, AL, { icono: 'check', nombre: 'Chequeo previo', dato: 'GPS listo' })
    + celda(3, AL, { clima: climaIco('solnube'), nombre: 'Clima', dato: '21° · Santa Clara del Mar' })
    + fila2;
}
const badge = (l) => `<div class="badge">${l}</div>`;
paginas.push(page('2-engranajes', 'Inicio', inicio(
  celda(4, AL, { icono: 'engranajeA', nombre: 'Ajustes', dato: 'v1.0.29', html: badge('A') })
  + celda(5, AL, { icono: 'engranajeB', nombre: 'Ajustes', dato: 'v1.0.29', html: badge('B') })
  + celda(6, AL, { icono: 'engranajeC', nombre: 'Ajustes', dato: 'v1.0.29', html: badge('C') })
  + celda(7, AL, { clima: climaIco('sol'), nombre: 'Clima', dato: '24° · despejado' }))));

// ============ 3. Botones apagados ============
function carreras(cls, motivo) {
  const largar = (y, m) => cls === 'viejo'
    ? boton({ x: 996, y, w: 220, h: 96, cls: 'y viejo', nombre: 'LARGAR', tamNombre: 34 })
    : boton({ x: 996, y, w: 220, h: 96, cls, nombre: 'LARGAR', tamNombre: 28, dato: m });
  const prox = (y, dia, mes, nom, lugar, clima, m) => `<div class="car" style="left:0;top:${y}px;width:1236px;height:130px">
    <div class="fecha"><b>${dia}</b><small>${mes}</small></div><div class="nom"><b>${nom}</b><span>${lugar}</span></div>
    ${clima}${largar(17, m)}</div>`;
  const climaOk = `<div class="clima" style="left:782px;top:17px">${climaIco('sol', 56)}<div><div class="t">19°</div><div class="l">20%</div></div></div>`;
  const climaSin = `<div class="clima sin" style="left:782px;top:17px"><b>Clima</b><span>desde 20/11</span></div>`;
  return fila({ x: 0, y: 0, w: 1236, h: 90, e: '+&nbsp; Nueva carrera', der: ico('bandera', 44, '#f4c531') })
    + `<div class="sec" style="left:0;top:112px">PRÓXIMAS</div>`
    + prox(154, '21', 'NOV', 'Gualeguaychú', 'Gualeguaychú', climaOk, motivo ? 'Se larga el 21 nov' : '')
    + prox(298, '05', 'DIC', 'Concepción del Uruguay', 'Concepción del Uruguay', climaSin, motivo ? 'Se larga el 5 dic' : '')
    + `<div class="sec" style="left:0;top:450px">CORRIDAS</div>`
    + `<div class="car" style="left:0;top:492px;width:1236px;height:116px"><div class="fecha" style="width:74px;height:74px"><b>06</b><small>NOV</small></div>
       <div class="nom" style="left:120px"><b style="font-size:32px">Santa Clara</b><span style="font-size:21px">6 vueltas · error total 9,6 s</span></div>
       ${boton({ x: 998, y: 16, w: 220, h: 84, nombre: 'Informe', tamNombre: 30 })}</div>`;
}
paginas.push(page('3-botones-apagados-antes', 'Carreras', carreras('viejo')));
paginas.push(page('3-botones-apagados', 'Carreras', carreras('off', true)));
function fichaSantaClara(nuevo) {
  const off = nuevo ? 'off' : 'viejo';
  const F = (i, e, det, der, cls = '') => fila({ x: 0, y: i * 100, w: 798, h: 90, e, det, der, cls });
  return F(0, 'Nombre', 'Santa Clara', ico('bandera', 40)) + F(1, 'Localidad', 'Tocá para elegir', ico('gps', 40))
    + F(2, 'Fecha', '', masmenos('06/11/26'))
    + F(3, 'Tiempo ideal', '', masmenos('3:00,0'), off)
    + F(4, 'Distancia', '', masmenos('2.500 m'), off)
    + F(5, 'Medidas', '', seg(['Impares', 'Pares', 'Todas'], 'Todas'), off)
    + (nuevo
      ? boton({ x: 816, y: 0, w: 420, h: 470, cls: 'y', icono: 'informe', tamIcono: 90, nombre: 'VER INFORME', tamNombre: 62, dato: 'Finalizada · 6 vueltas · error total 9,6 s' })
        + boton({ x: 816, y: 486, w: 202, h: 120, cls: 'off', nombre: 'Guardar', tamNombre: 27, dato: 'Sin cambios' })
      : boton({ x: 816, y: 0, w: 420, h: 470, cls: 'y viejo', icono: 'bandera', tamIcono: 90, nombre: 'FINALIZADA', tamNombre: 70 })
        + boton({ x: 816, y: 486, w: 202, h: 120, nombre: 'Ver informe', tamNombre: 27 }))
    + boton({ x: 1034, y: 486, w: 202, h: 120, nombre: 'Borrar', tamNombre: 27 });
}
paginas.push(page('3-botones-apagados-b-antes', 'Santa Clara', fichaSantaClara(false)));
paginas.push(page('3-botones-apagados-b', 'Santa Clara', fichaSantaClara(true)));

// ============ 4. Simulador en el Chequeo previo ============
// Grilla 4×2 más baja (230) y TODO LISTO como barra ancha abajo: entra la casilla nueva sin achicar íconos ni letras.
const AL2 = (606 - 110 - 36) / 2;
function chequeo(sim) {
  return celda(0, AL2, { icono: 'gps', nombre: 'GPS', dato: 'GPS listo · 2,8 m' })
    + celda(1, AL2, { icono: 'velocidad', nombre: 'Módulo', dato: 'NEO-M8N · 10 por s' })
    + celda(2, AL2, { icono: 'bandera', nombre: 'Pulsador', dato: 'Tocá para probar' })
    + celda(3, AL2, { icono: 'voz', nombre: 'Probar la voz', dato: 'Volumen 100%' })
    + celda(4, AL2, { icono: 'apagar', nombre: 'Tensión', dato: 'Bien' })
    + celda(5, AL2, { icono: 'tema', nombre: 'Temperatura', dato: '48 °C' })
    + celda(6, AL2, { icono: 'camara', nombre: 'Cámara y lugar', dato: 'Apagada · 27,7 GB' })
    + celda(7, AL2, sim)
    + `<div class="b y" style="left:0;top:${2 * AL2 + 36}px;width:1236px;height:110px;flex-direction:row;gap:22px">${ico('check', 64)}<div class="n" style="font-size:46px;letter-spacing:3px">TODO LISTO</div></div>`;
}
paginas.push(page('4-simulador-chequeo', 'Chequeo previo', chequeo({ icono: 'simulador', nombre: 'Simulador', dato: '<span style="color:#7fd36b">Simulador OK · 12 pruebas</span>' })));
paginas.push(page('4-simulador-chequeo-b', 'Chequeo previo', chequeo({ cls: 'o', icono: 'simulador', nombre: 'Simulador', dato: 'Revisar · 2 avisos · tocá para ver' })));
paginas.push(page('4-simulador-chequeo-c', 'Chequeo previo', chequeo({ cls: 'r', icono: 'simulador', nombre: 'Simulador', dato: 'Hay errores: comunicate<br>con el desarrollador' })));

function detalle(qrSvg) {
  const L = (i, ok, b, s) => `<div class="lista" style="left:0;top:${136 + i * 96}px;width:798px;height:86px"><div class="x ${ok ? 'ok' : ''}">${ico(ok ? 'check' : 'alerta', 30, ok ? '#f7f7f2' : '#fff')}</div><div><b>${b}</b>${s ? `<span>${s}</span>` : ''}</div></div>`;
  return boton({ x: 0, y: 0, w: 798, h: 120, cls: 'r', nombre: 'Hay errores: comunicate con el desarrollador', tamNombre: 31, dato: '3 de 12 pruebas fallaron · 07/11/2026 15:40 · v1.0.29' })
    + L(0, false, 'Cuenta regresiva', 'En la vuelta 3 habló 0,4 s tarde')
    + L(1, false, 'Velocidad sugerida', 'Con el GPS quieto marcó 0 km/h en vez de «—»')
    + L(2, false, 'Informe', 'La vuelta 6 no quedó guardada')
    + L(3, true, 'Las otras 9 pruebas, bien', 'Voz, adelantado/atrasado, marcas, historial')
    + `<div class="qrcaja" style="left:816px;top:0;width:420px;height:470px"><div class="q">${qrSvg}</div><p>Escaneá con el celu</p></div>`
    + boton({ x: 816, y: 486, w: 420, h: 120, cls: 'y', nombre: 'Mandar el reporte al desarrollador', tamNombre: 27, html: '' });
}

(async () => {
  let qrSvg;
  try {
    const QR = require('qrcode');
    qrSvg = await QR.toString('https://betometro.local/reporte/sim-20261107-1540', { type: 'svg', margin: 1, errorCorrectionLevel: 'M', color: { dark: '#0d2a18', light: '#fffbef' } });
  } catch (e) {
    qrSvg = `<svg viewBox="0 0 10 10"><text x="1" y="6" font-size="2">QR</text></svg>`;
  }
  paginas.push(page('4-simulador-chequeo-d', 'Simulador', detalle(qrSvg)));

  const { chromium } = require('playwright');
  const b = await chromium.launch();
  const pg = await b.newPage({ viewport: { width: 1280, height: 720 }, deviceScaleFactor: 1 });
  for (const n of paginas) {
    await pg.goto('file://' + path.join(D, n + '.html'));
    await pg.evaluate(() => document.fonts.ready);
    await pg.screenshot({ path: path.join(D, n + '.png') });
    console.log(n + '.png');
  }
  await b.close();
})();
