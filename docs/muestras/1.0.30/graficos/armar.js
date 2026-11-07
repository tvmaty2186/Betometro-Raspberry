// Muestras 1.0.30 «gráficos a pantalla completa» (para que Matías apruebe antes de programar).
// Arma los HTML y les saca foto. Uso: node docs/muestras/1.0.30/graficos/armar.js   (necesita playwright)
//
// Datos como los arma datosInforme (app/nucleo/informe.cpp): cada punto de la traza es
//   [t (s desde que empezó la vuelta), d (m), km/h, acel m/s², adelanto s (+ adelantado), x, y, ok]
// y cada vuelta trae n, medida, realMs, difMs, velPromKmh, velMaxKmh, L.
const fs = require('fs'), path = require('path');
const D = __dirname;

// ---------- utilidades ----------
let semilla = 7;
const rnd = () => { semilla |= 0; semilla = semilla + 0x6D2B79F5 | 0; let t = Math.imul(semilla ^ semilla >>> 15, 1 | semilla); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; };
const nf = (x, d = 1) => x.toFixed(d).replace('.', ',');
const km = (m, d = 1) => nf(m / 1000, d);
const tfmt = (s) => { const m = Math.floor(s / 60), r = (s % 60).toFixed(1).padStart(4, '0').replace('.', ','); return m + ':' + r; };
const tlargo = (s) => { const h = Math.floor(s / 3600), m = Math.floor(s % 3600 / 60), r = Math.floor(s % 60); return h + ':' + String(m).padStart(2, '0') + ':' + String(r).padStart(2, '0'); };
const dif = (s) => (s > 0 ? '+' : s < 0 ? '−' : '±') + nf(Math.abs(s)) + ' s';
const esc = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;');

// ---------- carrera con vueltas: «Paraná», 8 vueltas de 3.200 m, ideal 2:30,0 (sugerida 76,8), medidas impares ----------
const DIST = 3200, IDEAL = 150, SUG = DIST / IDEAL * 3.6;
function armarCarrera() {
  const difs = [1.2, -2.5, 1.5, -0.2, 2.2, -1.1, 1.3, 1.1];   // s contra el ideal, por vuelta
  const vueltas = [];
  for (let n = 1; n <= 8; n++) {
    // forma del circuito (curvas lentas, rectas rápidas) + variación propia de la vuelta
    const f1 = 0.6 + rnd() * 0.8, f2 = rnd() * 6.28, f3 = rnd() * 6.28;
    const forma = s => SUG + 11 * Math.sin(s / DIST * 6.28 * 2 + 0.4) + 6 * Math.sin(s / DIST * 6.28 * 5 + f2) + 3 * Math.sin(s / DIST * 6.28 * 11 + f3) * f1
      + (n === 4 ? 9 * Math.exp(-Math.pow((s - 2800) / 140, 2)) : 0);   // V4: el pico de 87 km/h cerca del km 12,4
    // tiempo de la vuelta con esa forma → se escala para que dé el tiempo real
    let T = 0; for (let s = 0; s < DIST; s += 10) T += 10 / (forma(s + 5) / 3.6);
    const real = IDEAL + difs[n - 1], k = T / real;
    const m = []; let t = 0, d = 0, prevV = null;
    while (d < DIST) {
      const v = Math.max(5, forma(d) * k + (rnd() - 0.5) * 2.2);   // ruido del GPS a 5 lecturas por s
      const a = prevV === null ? 0 : (v - prevV) / 3.6 / 0.2;
      m.push([+t.toFixed(1), +d.toFixed(1), +v.toFixed(1), +Math.max(-9, Math.min(9, a)).toFixed(2), +(d / (DIST / IDEAL) - t).toFixed(1), 0, 0, 1]);
      prevV = v; t += 0.2; d += v / 3.6 * 0.2;
    }
    if (n === 4) {   // V4: en el km 12,4 de la carrera (2.800 m de la vuelta) la lectura da 87 km/h
      let c = m[0]; for (const p of m) if (Math.abs(p[1] - 2800) < Math.abs(c[1] - 2800)) c = p;
      const base = c[2];
      for (const p of m) p[2] = +(p[2] + (87 - base) * Math.exp(-Math.pow((p[1] - 2800) / 110, 2))).toFixed(1);
    }
    const vs = m.map(p => p[2]);
    vueltas.push({ n, medida: n % 2 === 1, realMs: Math.round(real * 1000), difMs: Math.round(difs[n - 1] * 1000),
      velPromKmh: DIST / real * 3.6, velMaxKmh: Math.max(...vs), m, L: DIST });
  }
  return { nombre: 'Paraná', vueltas, distanciaM: DIST, idealMs: IDEAL * 1000, largadas: 1 };
}

// ---------- SVG ----------
const E = (n, a = {}, inner = '') => `<${n}${Object.entries(a).map(([k, v]) => ` ${k}="${v}"`).join('')}>${inner}</${n}>`;
const T = (x, y, txt, a = {}) => E('text', { x: +x.toFixed(1), y: +y.toFixed(1), ...a }, esc(txt));

// Gráfico de velocidad con zoom: todas las lecturas (sin suavizar), cuadrícula cada 100 m y 5 km/h,
// vuelta por vuelta su tiempo, error y MEDIDA/LIBRE, la sugerida, y el punto tocado con todo su dato.
// est: 'pi' o 'celu' (tamaños y colores de cada lado)
function graficoZoom({ C, de, a, punto, box, est }) {
  const pi = est === 'pi';
  const { x0, x1, y0, y1 } = box;
  const X = d => x0 + (x1 - x0) * (d - de) / (a - de);
  // puntos visibles (distancia acumulada en toda la carrera)
  const pts = [];
  let acum = 0;
  for (const v of C.vueltas) {
    for (const p of v.m) { const d = acum + p[1]; if (d >= de - 50 && d <= a + 50) pts.push({ d, v: p[2], p, vu: v }); }
    acum += v.L;
  }
  const vis = pts.filter(q => q.d >= de && q.d <= a);
  const vmin = Math.min(...vis.map(q => q.v)), vmax = Math.max(...vis.map(q => q.v)), vprom = vis.reduce((s, q) => s + q.v, 0) / vis.length;
  const yA = Math.floor((vmin - 4) / 5) * 5, yB = Math.ceil((vmax + 4) / 5) * 5;
  const Y = v => y1 - (y1 - y0) * (v - yA) / (yB - yA);
  const colVel = v => { const d = v - SUG; return d > 6 ? 'var(--rojo)' : d > 2 ? 'var(--ama)' : d < -6 ? 'var(--azul)' : d < -2 ? 'var(--celeste)' : 'var(--verde)'; };
  const fino = pi ? 'rgba(255,255,255,0.07)' : '#ffffff0d', medio = pi ? 'rgba(255,255,255,0.16)' : '#363b41';
  const ejeTxt = pi ? { fill: '#d6e6d2', 'font-family': 'B', 'font-size': 14 } : { class: 'eje' };
  let s = '';
  // cuadrícula: cada 100 m (fina) y cada 500 m (media); cada 5 km/h (fina) y cada 10 (media)
  for (let d = Math.ceil(de / 100) * 100; d <= a; d += 100) {
    const f = d % 500 === 0; s += E('line', { x1: X(d), x2: X(d), y1: y0, y2: y1, stroke: f ? medio : fino, 'stroke-width': 1 });
    if (f) s += T(X(d), y1 + (pi ? 20 : 12), d % 1000 === 0 ? km(d, 0) + ' km' : km(d, 1), { ...ejeTxt, 'text-anchor': 'middle' });
  }
  for (let v = yA; v <= yB; v += 5) {
    const f = v % 10 === 0; s += E('line', { x1: x0, x2: x1, y1: Y(v), y2: Y(v), stroke: f ? medio : fino, 'stroke-width': 1 });
    if (f) s += T(x0 - (pi ? 10 : 5), Y(v) + 4, String(v), { ...ejeTxt, 'text-anchor': 'end' });
  }
  // sugerida punteada (como hoy: amarilla en la Pi, blanca en la página)
  if (SUG > yA && SUG < yB) {
    s += E('line', { x1: x0, x2: x1, y1: Y(SUG), y2: Y(SUG), stroke: pi ? '#f4c531' : '#fff', 'stroke-dasharray': pi ? '8 6' : '4 4', 'stroke-width': pi ? 2 : 1 });
    s += T(x0 + 8, Y(SUG) + (pi ? 18 : 12), 'sugerida ' + nf(SUG), pi ? { fill: '#f4c531', 'font-family': 'B', 'font-weight': 600, 'font-style': 'italic', 'font-size': 15 } : { fill: '#fff', 'font-size': 10, 'font-weight': 700 });
  }
  // cada vuelta: raya al empezar y arriba su nombre, tipo, tiempo y error
  acum = 0;
  for (const v of C.vueltas) {
    const d0 = acum; acum += v.L;
    if (d0 < de - 1 || d0 > a + 1) continue;
    const x = X(d0);
    s += E('line', { x1: x, x2: x, y1: y0, y2: y1, stroke: '#f4c531', 'stroke-dasharray': pi ? '5 5' : '3 3', 'stroke-width': pi ? 1.5 : 1 });
    if (d0 >= a - 1) continue;
    const err = v.difMs / 1000, cErr = Math.abs(err) <= 1 ? (pi ? '#7fd36b' : 'var(--verde)') : Math.abs(err) <= 3 ? (pi ? '#f4c531' : 'var(--ama)') : (pi ? '#ff6b5a' : 'var(--rojo)');
    if (pi) {
      s += T(x + 8, y0 + 22, 'V' + v.n, { fill: '#f4c531', 'font-family': 'B', 'font-weight': 800, 'font-style': 'italic', 'font-size': 20 });
      const tipo = v.medida ? 'MEDIDA' : 'LIBRE', cT = v.medida ? '#ff6b5a' : '#7fd36b';
      s += E('rect', { x: x + 48, y: y0 + 7, width: tipo.length * 9 + 14, height: 20, rx: 10, fill: 'none', stroke: cT, 'stroke-width': 1.5 });
      s += T(x + 55 + tipo.length * 4.5, y0 + 21, tipo, { fill: cT, 'font-family': 'B', 'font-weight': 800, 'font-style': 'italic', 'font-size': 12, 'letter-spacing': 1, 'text-anchor': 'middle' });
      s += T(x + 70 + tipo.length * 9 + 8, y0 + 23, tfmt(v.realMs / 1000), { fill: '#f3ecd8', 'font-family': 'C', 'font-weight': 700, 'font-style': 'italic', 'font-size': 22 });
      s += T(x + 70 + tipo.length * 9 + 70, y0 + 23, dif(err), { fill: cErr, 'font-family': 'C', 'font-weight': 700, 'font-style': 'italic', 'font-size': 22 });
    } else {
      s += E('text', { x: +(x + 4).toFixed(1), y: y0 + 11, fill: '#f4c531', 'font-size': 10, 'font-weight': 800, 'font-style': 'italic' },
        esc('V' + v.n + ' · ' + (v.medida ? 'medida' : 'libre') + ' · ' + tfmt(v.realMs / 1000) + ' · ') + E('tspan', { fill: cErr }, esc(dif(err))));
    }
  }
  // la traza completa, lectura por lectura (sin suavizar)
  if (pi) {
    s += E('polyline', { points: pts.map(q => X(q.d).toFixed(1) + ',' + Y(q.v).toFixed(1)).join(' '), fill: 'none', stroke: '#f3ecd8', 'stroke-width': 2.5, 'stroke-linejoin': 'round' });
  } else {
    for (let i = 1; i < pts.length; i++) s += E('line', { x1: X(pts[i - 1].d).toFixed(1), y1: Y(pts[i - 1].v).toFixed(1), x2: X(pts[i].d).toFixed(1), y2: Y(pts[i].v).toFixed(1), stroke: colVel(pts[i].v), 'stroke-width': 2, 'stroke-linecap': 'round' });
  }
  // el punto tocado
  let q = vis[0]; for (const w of vis) if (Math.abs(w.d - punto) < Math.abs(q.d - punto)) q = w;
  const px = X(q.d), py = Y(q.v);
  s += E('line', { x1: px, x2: px, y1: y0, y2: y1, stroke: '#fff', 'stroke-width': 1.5, opacity: 0.8 });
  s += E('circle', { cx: px, cy: py, r: pi ? 8 : 5, fill: '#fff', stroke: '#111', 'stroke-width': pi ? 3 : 2 });
  return { svg: s, tramo: { vmin, vmax, vprom, yA, yB }, punto: { x: px, y: py, q } };
}

// Barrita de abajo: toda la carrera chiquita y el recuadro amarillo de lo que se ve
function barrita({ C, de, a, box, est }) {
  const pi = est === 'pi';
  const { x0, x1, y0, y1 } = box;
  const total = C.vueltas.reduce((s, v) => s + v.L, 0);
  const X = d => x0 + (x1 - x0) * d / total;
  let vmin = 1e9, vmax = 0; for (const v of C.vueltas) for (const p of v.m) { vmin = Math.min(vmin, p[2]); vmax = Math.max(vmax, p[2]); }
  const Y = v => y1 - 4 - (y1 - y0 - 8) * (v - vmin + 3) / (vmax - vmin + 6);
  let s = E('rect', { x: x0, y: y0, width: x1 - x0, height: y1 - y0, rx: pi ? 10 : 6, fill: 'rgba(0,0,0,0.35)', stroke: 'rgba(207,212,216,0.5)' });
  let acum = 0, pts = [];
  for (const v of C.vueltas) {
    for (let i = 0; i < v.m.length; i += 4) pts.push(X(acum + v.m[i][1]).toFixed(1) + ',' + Y(v.m[i][2]).toFixed(1));
    if (acum > 0) s += E('line', { x1: X(acum), x2: X(acum), y1: y0 + 2, y2: y1 - 2, stroke: '#f4c531', 'stroke-dasharray': '3 3', opacity: 0.8 });
    s += T(X(acum) + 5, y0 + (pi ? 14 : 10), 'V' + v.n, pi ? { fill: '#f4c531', 'font-family': 'B', 'font-weight': 800, 'font-style': 'italic', 'font-size': 12 } : { fill: '#f4c531', 'font-size': 8, 'font-weight': 800, 'font-style': 'italic' });
    acum += v.L;
  }
  s += E('polyline', { points: pts.join(' '), fill: 'none', stroke: pi ? '#f3ecd8' : '#b9cdb8', 'stroke-width': 1, opacity: 0.6 });
  // el recuadro de lo que se ve, con sus manijas
  s += E('rect', { x: X(de), y: y0 - 2, width: X(a) - X(de), height: y1 - y0 + 4, rx: 5, fill: 'rgba(244,197,49,0.16)', stroke: '#f4c531', 'stroke-width': 2 });
  for (const xx of [X(de), X(a)]) s += E('rect', { x: xx - (pi ? 5 : 3), y: (y0 + y1) / 2 - (pi ? 12 : 8), width: pi ? 10 : 6, height: pi ? 24 : 16, rx: 3, fill: '#f4c531', stroke: '#111', 'stroke-width': 1 });
  return s;
}

const xIco = (s, c) => `<svg width="${s}" height="${s}" viewBox="0 0 64 64" fill="none" stroke="${c}" stroke-width="6" stroke-linecap="round"><path d="M16 16l32 32M48 16L16 48"/></svg>`;

// ================= 1. Pi: velocidad a pantalla completa (vueltas 3 y 4) =================
function muestraVelocidadPi(C) {
  const de = 6400, a = 12800, punto = 12400;
  const box = { x0: 80, y0: 104, x1: 1216, y1: 530 };
  const g = graficoZoom({ C, de, a, punto, box, est: 'pi' });
  const q = g.punto.q, p = q.p;
  const ade = p[4];
  // globito del punto tocado: km, km/h, tiempo de la vuelta, adelanto, vuelta
  const gx = g.punto.x - 290, gy = g.punto.y - 40;
  const globito = `<div style="position:absolute;left:${gx}px;top:${gy}px;width:262px;height:104px;border-radius:16px;background:rgba(8,25,14,.94);border:2px solid #cfd4d8;padding:8px 14px">
    <div style="display:flex;align-items:baseline;gap:10px"><span style="font:700 italic 36px C;color:#f3ecd8;line-height:1">${Math.round(q.v)} <small style="font-size:20px">km/h</small></span>
      <span style="margin-left:auto;background:#f4c531;color:#0b0d0c;border-radius:12px;padding:1px 10px;font:800 italic 17px B">V${q.vu.n}</span></div>
    <div style="font:600 16px B;color:#f7f7f2;margin-top:4px">km ${km(q.d, 2)} · ${tfmt(p[0])} de vuelta</div>
    <div style="font:800 italic 17px B;color:${Math.abs(ade) <= 1 ? '#7fd36b' : Math.abs(ade) <= 3 ? '#f4c531' : '#ff6b5a'}">${dif(ade)} ${ade >= 0 ? 'adelantado' : 'atrasado'} en ese punto</div>
  </div>
  <svg style="position:absolute;left:0;top:0" width="1256" height="696"><path d="M${gx + 262} ${gy + 52}L${g.punto.x - 10} ${g.punto.y}" stroke="#cfd4d8" stroke-width="2"/></svg>`;
  const body = `<div class="tarjeta">
    <div class="sub" style="left:22px;top:18px">Velocidad km/h · Paraná · 8 vueltas · sugerida ${nf(SUG)}</div>
    <div class="modos" style="right:106px;top:16px"><div class="on">Vel.</div><div>Acel.</div><div>Adel.</div></div>
    <div class="cerrar" style="right:14px;top:10px">${xIco(36, '#f7f7f2')}</div>
    <div class="sub" style="left:22px;top:60px;color:#f4c531">Tramo visible · km ${km(de)} a ${km(a)} · vueltas 3 y 4</div>
    <div style="position:absolute;right:16px;top:52px;display:flex;gap:26px;align-items:baseline">
      ${['Máx', 'Mín', 'Prom'].map((e, i) => `<div style="display:flex;align-items:baseline;gap:8px"><span class="sub" style="position:static">${e}</span><span style="font:700 italic 26px C;color:#f3ecd8">${i === 2 ? nf(g.tramo.vprom) : Math.round([g.tramo.vmax, g.tramo.vmin][i])}</span><span class="sub chica" style="position:static">km/h</span></div>`).join('')}
    </div>
    <svg style="position:absolute;left:0;top:0" width="1256" height="696">${g.svg}</svg>
    ${globito}
    <svg style="position:absolute;left:0;top:0" width="1256" height="696">${barrita({ C, de, a, box: { x0: 80, y0: 584, x1: 1216, y1: 640 }, est: 'pi' })}</svg>
    <div class="sub chica" style="left:22px;top:656px">Viendo km ${km(de)} a ${km(a)} de ${km(C.vueltas.length * DIST)}</div>
    <div class="pista" style="right:22px;top:656px">Doble toque: acercar · arrastrá para recorrer · X para cerrar</div>
  </div>`;
  fs.writeFileSync(path.join(D, '1-velocidad-pi.html'), `<!doctype html><html lang="es"><meta charset="utf-8"><title>Velocidad a pantalla completa (Pi)</title><link rel="stylesheet" href="kit.css"><body class="pi">${body}</body></html>`);
}

// ================= 2. Pi: mapa del recorrido libre a pantalla completa, con zoom =================
// Recorrido libre de 40,3 km con 7 marcas. Sin distancia no hay sugerida y hoy el mapa se pinta todo verde:
// acá el color es contra el PROMEDIO del recorrido (decisión para Matías).
function armarRecorrido() {
  const LARGO = 40300, PASO = 40;   // un punto cada 40 m (la traza real tiene más; para dibujar alcanza)
  // una ruta: el rumbo va y viene suave (sin rulos), con curvas más cerradas cada tanto
  const rumbo = d => 0.15 + 0.9 * Math.sin(d / 5200) + 0.55 * Math.sin(d / 1700 + 1) + 0.3 * Math.sin(d / 620 + 2) + 0.12 * Math.sin(d / 230 + 0.5);
  const pts = []; let x = 0, y = 0;
  for (let d = 0; d <= LARGO; d += PASO) {
    const h = rumbo(d), curv = (rumbo(d + 20) - rumbo(d - 20)) / 40;   // rad por metro
    x += Math.cos(h); y += Math.sin(h);
    // velocidad: baja en curva, sube en recta, con ruido del GPS
    const v = Math.max(8, Math.min(98, 74 - 26000 * Math.abs(curv) + 7 * Math.sin(d / 2100) + (rnd() - 0.5) * 3));
    pts.push({ d, x, y, v, curv });
  }
  const prom = pts.reduce((s, p) => s + p.v, 0) / pts.length;
  // tiempo acumulado
  let t = 0; for (const p of pts) { t += PASO / (p.v / 3.6); p.t = t; }
  const marcasKm = [5.1, 10.3, 14.8, 20.2, 25.0, 30.6, 35.9];
  const marcas = marcasKm.map((k, i) => { const p = pts[Math.round(k * 1000 / PASO)]; return { n: i + 1, m: k * 1000, x: p.x, y: p.y, t: p.t, v: p.v }; });
  return { pts, prom, marcas, total: pts[pts.length - 1].t };
}
function muestraMapaPi(R) {
  const W = 1256, H = 696;
  const colV = v => { const d = v - R.prom; return d > 6 ? '#d23b2c' : d > 2 ? '#f4c531' : d >= -2 ? '#7fd36b' : d >= -6 ? '#7fd3ff' : '#3a8de0'; };
  // zoom: se ve del km 0 al 15 (largada y marcas 1, 2 y 3), bien grande
  const vis = R.pts.filter(p => p.d <= 15000);
  const bx0 = Math.min(...vis.map(p => p.x)), bx1 = Math.max(...vis.map(p => p.x)), by0 = Math.min(...vis.map(p => p.y)), by1 = Math.max(...vis.map(p => p.y));
  const area = { x0: 60, y0: 100, x1: W - 60, y1: H - 96 };
  const k = Math.min((area.x1 - area.x0) / (bx1 - bx0), (area.y1 - area.y0) / (by1 - by0));
  const ox = (area.x0 + area.x1) / 2 - (bx0 + bx1) / 2 * k, oy = (area.y0 + area.y1) / 2 - (by0 + by1) / 2 * k;
  const X = p => ox + p.x * k, Y = p => oy + p.y * k;
  const road = R.pts.map(p => X(p).toFixed(1) + ',' + Y(p).toFixed(1)).join(' ');
  let s = E('polyline', { points: road, fill: 'none', stroke: 'rgba(0,0,0,0.45)', 'stroke-width': 30, 'stroke-linejoin': 'round', 'stroke-linecap': 'round' });
  s += E('polyline', { points: road, fill: 'none', stroke: '#3a3f45', 'stroke-width': 18, 'stroke-linejoin': 'round', 'stroke-linecap': 'round' });
  for (let i = 1; i < R.pts.length; i++) {
    const a = R.pts[i - 1], b = R.pts[i];
    if (X(b) < -40 || X(b) > W + 40 || Y(b) < -40 || Y(b) > H + 40) continue;
    s += E('line', { x1: X(a).toFixed(1), y1: Y(a).toFixed(1), x2: X(b).toFixed(1), y2: Y(b).toFixed(1), stroke: colV(b.v), 'stroke-width': 13, 'stroke-linecap': 'round' });
  }
  // cada 1 km, una rayita en la ruta con el km (detalle del zoom)
  for (let d = 1000; d <= 15000; d += 1000) {
    const p = R.pts[d / 40], q = R.pts[d / 40 + 1], nx = -(q.y - p.y), ny = q.x - p.x, n = Math.hypot(nx, ny);
    s += E('line', { x1: X(p) + nx / n * 11, y1: Y(p) + ny / n * 11, x2: X(p) - nx / n * 11, y2: Y(p) - ny / n * 11, stroke: '#fff', 'stroke-width': 2, opacity: 0.8 });
    if (d % 5000 !== 0) s += T(X(p) + nx / n * 24, Y(p) + ny / n * 24 + 5, km(d, 0), { fill: '#d6e6d2', 'font-family': 'B', 'font-weight': 600, 'font-size': 13, 'text-anchor': 'middle' });
  }
  // largada
  const p0 = R.pts[0];
  s += E('circle', { cx: X(p0), cy: Y(p0), r: 14, fill: '#fff' });
  s += T(X(p0) + 22, Y(p0) + 7, 'LARGADA', { fill: '#f7f7f2', 'font-family': 'B', 'font-weight': 800, 'font-style': 'italic', 'font-size': 18 });
  // marcas numeradas
  for (const mk of R.marcas) {
    if (mk.m > 15000) continue;
    s += E('circle', { cx: X(mk), cy: Y(mk), r: 16, fill: '#f4c531', stroke: '#111', 'stroke-width': 3 });
    s += T(X(mk), Y(mk) + 7, String(mk.n), { fill: '#111', 'font-family': 'B', 'font-weight': 800, 'font-style': 'italic', 'font-size': 20, 'text-anchor': 'middle' });
    // la etiqueta del lado de afuera de la curva (para no pisar la ruta)
    const i = Math.round(mk.m / 40), pa = R.pts[i - 1], pb = R.pts[i + 1];
    let nx = -(pb.y - pa.y), ny = pb.x - pa.x; const n = Math.hypot(nx, ny); nx /= n; ny /= n;
    if (R.pts[i].curv > 0) { nx = -nx; ny = -ny; }
    const lx = X(mk) + nx * 30, ly = Y(mk) + ny * 30;
    s += T(lx, ly + 5, 'km ' + km(mk.m) + ' · ' + tlargo(mk.t), { fill: '#f4c531', 'font-family': 'B', 'font-weight': 600, 'font-style': 'italic', 'font-size': 15, 'text-anchor': nx < 0 ? 'end' : 'start' });
  }
  // punto tocado: km 8,2
  const pt = R.pts[8200 / 40];
  s += E('circle', { cx: X(pt), cy: Y(pt), r: 10, fill: '#fff', stroke: '#111', 'stroke-width': 3 });
  const gl = Math.min(W - 420, Math.max(20, X(pt) + 24)), gt = Math.min(H - 150, Math.max(76, Y(pt) - 88));
  const globito = `<div style="position:absolute;left:${gl}px;top:${gt}px;border-radius:16px;background:rgba(8,25,14,.94);border:2px solid #cfd4d8;padding:8px 14px;white-space:nowrap">
    <div style="font:700 italic 32px C;color:#f3ecd8;line-height:1">${Math.round(pt.v)} <small style="font-size:18px">km/h</small></div>
    <div style="font:600 16px B;color:#f7f7f2;margin-top:4px">km ${km(pt.d, 2)} · ${tlargo(pt.t)} · entre la marca 1 y la 2</div></div>`;
  // mapita: todo el recorrido y el recuadro de lo que se ve
  const MX = 236, MY = 160, mx0 = Math.min(...R.pts.map(p => p.x)), mx1 = Math.max(...R.pts.map(p => p.x)), my0 = Math.min(...R.pts.map(p => p.y)), my1 = Math.max(...R.pts.map(p => p.y));
  const mk = Math.min((MX - 24) / (mx1 - mx0), (MY - 24) / (my1 - my0));
  const MXf = p => 12 + (p.x - mx0) * mk + ((MX - 24) - (mx1 - mx0) * mk) / 2, MYf = p => 12 + (p.y - my0) * mk + ((MY - 24) - (my1 - my0) * mk) / 2;
  let mini = E('polyline', { points: R.pts.filter((p, i) => i % 3 === 0).map(p => MXf(p).toFixed(1) + ',' + MYf(p).toFixed(1)).join(' '), fill: 'none', stroke: '#9aa3ad', 'stroke-width': 3, 'stroke-linejoin': 'round' });
  for (const m of R.marcas) mini += E('circle', { cx: MXf(m), cy: MYf(m), r: 4, fill: '#f4c531' });
  mini += E('circle', { cx: MXf(p0), cy: MYf(p0), r: 4, fill: '#fff' });
  // lo visible en pantalla, pasado al mapita
  const vx0 = (area.x0 - 40 - ox) / k, vx1 = (W - ox) / k, vy0 = (0 - oy) / k, vy1 = (H - oy) / k;
  const rx = MXf({ x: vx0 }), ry = MYf({ y: vy0 });
  mini += E('rect', { x: rx, y: ry, width: (vx1 - vx0) * mk, height: (vy1 - vy0) * mk, fill: 'rgba(244,197,49,0.14)', stroke: '#f4c531', 'stroke-width': 2, rx: 3 });
  const ley = [['#3a8de0', 'muy lento'], ['#7fd3ff', 'lento'], ['#7fd36b', 'en el promedio', nf(R.prom, 0) + ' km/h'], ['#f4c531', 'rápido'], ['#d23b2c', 'muy rápido'], ['#555555', 'sin señal']];
  const body = `<div class="tarjeta">
    <svg style="position:absolute;left:0;top:0" width="${W}" height="${H}">${s}</svg>
    ${globito}
    <div class="sub" style="left:22px;top:18px">Recorrido · color = velocidad · ${km(40300)} km · 7 marcas</div>
    <div class="modos" style="right:106px;top:16px"><div class="on">Vel.</div><div>Acel.</div></div>
    <div class="cerrar" style="right:14px;top:10px">${xIco(36, '#f7f7f2')}</div>
    <div class="ley" style="left:22px;bottom:50px">${ley.map(([c, t, d]) => `<div class="f"><i style="background:${c}"></i>${t}${d ? ` <small>${d}</small>` : ''}</div>`).join('')}</div>
    <div class="mapita" style="right:22px;bottom:50px;width:${MX}px;height:${MY}px"><svg width="${MX}" height="${MY}">${mini}</svg></div>
    <div class="sub chica" style="left:22px;top:${H - 36}px">Viendo del km 0 al 15 · largada y marcas 1 a 3</div>
    <div class="pista" style="right:22px;top:${H - 36}px">Doble toque: acercar · arrastrá para recorrer · X para cerrar</div>
  </div>`;
  fs.writeFileSync(path.join(D, '2-mapa-pi.html'), `<!doctype html><html lang="es"><meta charset="utf-8"><title>Mapa a pantalla completa (Pi)</title><link rel="stylesheet" href="kit.css"><body class="pi">${body}</body></html>`);
}

// ================= 3. Celu acostado (844×390): velocidad a pantalla completa con el estilo de la página =================
function muestraVelocidadCelu(C) {
  const de = 6400, a = 12800, punto = 12400;
  const box = { x0: 40, y0: 54, x1: 820, y1: 282 };
  const g = graficoZoom({ C, de, a, punto, box, est: 'celu' });
  const q = g.punto.q, p = q.p, ade = p[4];
  const colAde = Math.abs(ade) < 1 ? 'var(--verde)' : Math.abs(ade) < 3 ? 'var(--ama)' : 'var(--rojo)';
  const gx = g.punto.x - 196, gy = g.punto.y - 26;
  const globito = `<div style="position:absolute;left:${gx}px;top:${gy}px;width:176px;border-radius:10px;background:#08190eee;border:1.5px solid var(--borde);padding:5px 9px;box-shadow:0 4px 10px #0008">
    <div style="display:flex;align-items:baseline;gap:6px"><b style="font:700 italic 22px/1 var(--num);color:var(--crema)">${Math.round(q.v)} <small style="font-size:12px">km/h</small></b><span class="chip on" style="margin-left:auto;padding:1px 7px;font-size:10px">V${q.vu.n}</span></div>
    <div style="font:600 10px var(--ui);color:var(--ink);margin-top:3px">km ${km(q.d, 2)} · ${tfmt(p[0])} de vuelta</div>
    <div style="font:800 italic 11px var(--ui);color:${colAde}">${dif(ade)} ${ade >= 0 ? 'adelantado' : 'atrasado'}</div></div>
    <svg style="position:absolute;left:0;top:0" width="832" height="378"><path d="M${gx + 176} ${gy + 30}L${g.punto.x - 6} ${g.punto.y}" stroke="#cfd4d8" stroke-width="1.5"/></svg>`;
  const body = `<div class="card">
    <div class="t">Velocidad km/h <b>sugerida ${nf(SUG)}</b></div>
    <div class="chips" style="left:50%;top:9px;transform:translateX(-50%)"><div class="chip on">Velocidad</div><div class="chip">Aceleración</div><div class="chip">Adelanto</div></div>
    <div class="x">${xIco(18, '#f7f7f2')}</div>
    <div class="t" style="top:32px;color:var(--ama)">Tramo visible · km ${km(de)} a ${km(a)} · V3 y V4 <b>máx ${Math.round(g.tramo.vmax)} · mín ${Math.round(g.tramo.vmin)} · prom ${nf(g.tramo.vprom)}</b></div>
    <svg style="position:absolute;left:0;top:0" width="832" height="378">${g.svg}</svg>
    ${globito}
    <svg style="position:absolute;left:0;top:0" width="832" height="378">${barrita({ C, de, a, box: { x0: 40, y0: 308, x1: 820, y1: 342 }, est: 'celu' })}</svg>
    <div class="pie" style="top:352px">doble toque: acercar · arrastrá para recorrer · X para cerrar</div>
  </div>`;
  fs.writeFileSync(path.join(D, '3-velocidad-celu.html'), `<!doctype html><html lang="es"><meta charset="utf-8"><title>Velocidad a pantalla completa (celu)</title><link rel="stylesheet" href="kit.css"><body class="celu">${body}</body></html>`);
}

(async () => {
  const C = armarCarrera();
  muestraVelocidadPi(C);
  const R = armarRecorrido();
  muestraMapaPi(R);
  muestraVelocidadCelu(C);
  const { chromium } = require('playwright');
  const b = await chromium.launch();
  for (const [n, w, h, dsf] of [['1-velocidad-pi', 1280, 720, 1], ['2-mapa-pi', 1280, 720, 1], ['3-velocidad-celu', 844, 390, 2]]) {
    const pg = await b.newPage({ viewport: { width: w, height: h }, deviceScaleFactor: dsf });
    await pg.goto('file://' + path.join(D, n + '.html'));
    await pg.evaluate(() => document.fonts.ready);
    await pg.screenshot({ path: path.join(D, n + '.png') });
    console.log(n + '.png');
    await pg.close();
  }
  await b.close();
})();
