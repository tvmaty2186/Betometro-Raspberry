// Compara datosInforme de la app con el de C++ en carreras con recorridos de prueba.
// Uso: node pruebas/informe/comparar.js <ruta a correr_informe>
'use strict';
const { execFileSync } = require('child_process');
const { cargarInformeApp } = require('./informe-app');
const bin = process.argv[2];

function azar(semilla) { let a = semilla >>> 0; return () => { a = (a + 0x6D2B79F5) >>> 0; let t = a; t = Math.imul(t ^ (t >>> 15), t | 1); t ^= t + Math.imul(t ^ (t >>> 7), t | 61); return ((t ^ (t >>> 14)) >>> 0) / 4294967296; }; }
const fix = (x, n) => +x.toFixed(n);

// Circuito: óvalo deformado de `largo` metros alrededor de un punto
function carrera(o) {
  const r = azar(o.semilla), L = o.largo, T = o.idealMs;
  const lat0 = -34.6 + r() * 0.1, lon0 = -58.4 + r() * 0.1;
  const punto = (d, ruidoM) => {
    const a = 2 * Math.PI * (d / L), R = L / (2 * Math.PI);
    const x = R * 1.4 * Math.cos(a) + R * 0.2 * Math.cos(3 * a) + (r() - 0.5) * ruidoM;
    const y = R * 0.7 * Math.sin(a) + (r() - 0.5) * ruidoM;
    return [lat0 + y / 110540, lon0 + x / (111320 * Math.cos(lat0 * Math.PI / 180))];
  };
  const vueltas = [];
  for (let n = 1; n <= o.vueltas; n++) {
    const traza = []; let d = 0, t = 0; const hz = o.hz || 1, base = L / (T / 1000);
    const sesgo = 1 + (r() - 0.5) * 0.06;
    while (d < L * sesgo) {
      const v = base * (1 + 0.25 * Math.sin(d / L * 6.28 * 3) + (r() - 0.5) * 0.1);
      const enHueco = (o.huecos || []).some(([a, b]) => n === o.vueltaHueco && d >= a && d < b);
      let prec = 2 + r() * 5; if (r() < (o.mala || 0)) prec = 25 + r() * 40;
      let [la, lo] = punto(d, prec);
      if (r() < (o.saltos || 0)) { la += 0.002; }
      if (!enHueco) traza.push([fix(t, 1), Math.round(Math.max(0, d) * 10) / 10, fix(v * 3.6, 1), fix(la, 6), fix(lo, 6), Math.round(prec)]);
      t += 1 / hz; d += v / hz;
    }
    const real = Math.round(t * 1000);
    const vu = { largada: n > (o.corte || 99) ? 2 : 1, n, medida: n % 2 === 1, idealMs: T, realMs: real, difMs: real - T, fuente: 'pantalla', fin: 0,
                 distM: o.sinDist ? undefined : Math.round(d), velPromKmh: fix(d / t * 3.6, 1), velMaxKmh: fix(Math.max(...traza.map(p => p[2]), 0), 1) };
    if (!o.sinTraza || n > 1) vu.traza = o.cortaTraza && n === 2 ? traza.slice(0, 3) : traza;
    vueltas.push(vu);
  }
  return { id: 'c' + o.semilla, nombre: o.nombre || 'Prueba <"&> ' + o.semilla, fecha: '2026-11-01', tiempoIdealMs: T, distanciaM: o.sinDist ? 0 : L,
           patron: 'alt1', practica: false, creada: '', vueltasTotal: o.vueltas, vueltas };
}

const casos = [
  { semilla: 1, largo: 3200, idealMs: 240000, vueltas: 4 },
  { semilla: 2, largo: 1500, idealMs: 90000, vueltas: 3, hz: 10 },
  { semilla: 3, largo: 2800, idealMs: 210000, vueltas: 5, mala: 0.2 },
  { semilla: 4, largo: 2000, idealMs: 120000, vueltas: 3, saltos: 0.05 },
  { semilla: 5, largo: 3000, idealMs: 180000, vueltas: 4, huecos: [[500, 1200]], vueltaHueco: 2 },
  { semilla: 6, largo: 2500, idealMs: 150000, vueltas: 3, sinDist: true },
  { semilla: 7, largo: 900, idealMs: 60000, vueltas: 6, corte: 3 },
  { semilla: 8, largo: 30000, idealMs: 1200000, vueltas: 2 },
  { semilla: 9, largo: 200, idealMs: 30000, vueltas: 3, hz: 5 },
  { semilla: 10, largo: 3200, idealMs: 240000, vueltas: 3, sinTraza: true, cortaTraza: true },
];
for (let s = 0; s < 20; s++) { const r = azar(100 + s); casos.push({ semilla: 200 + s, largo: 500 + Math.round(r() * 6000), idealMs: 30000 + Math.round(r() * 400000),
  vueltas: 1 + Math.floor(r() * 5), hz: [1, 5, 10][s % 3], mala: r() * 0.3, saltos: r() * 0.04, huecos: [[100, 100 + r() * 800]], vueltaHueco: 1 + (s % 3), corte: 1 + (s % 4) }); }

const fallas = []; let numeros = 0, maxCoord = 0, maxOtro = 0, hzActual = 1, acelSalteadas = 0;
function comparar(a, b, ruta) {
  if (typeof a === 'number' && typeof b === 'number') {
    // Diferencia a propósito (04/11/2026): con más de 1 lectura por segundo la Raspberry calcula la aceleración
    // con las lecturas de ~1 s antes y después (la app usa las vecinas: con el celular a 1 Hz es lo mismo).
    if (hzActual > 1 && /\.m\[\d+\]\[3\]$/.test(ruta)) { acelSalteadas++; return; }
    numeros++;
    const d = Math.abs(a - b);
    const coord = /\.pista\[|\.m\[\d+\]\[[56]\]/.test(ruta);
    if (coord) maxCoord = Math.max(maxCoord, d); else maxOtro = Math.max(maxOtro, d);
    if (d > (coord ? 0.11 : /\.m\[\d+\]\[[34]\]/.test(ruta) ? 0.011 : 1e-9 * Math.max(1, Math.abs(a)))) fallas.push(`${ruta}: app=${a} C++=${b}`);
    return;
  }
  if (Array.isArray(a) && Array.isArray(b)) {
    if (a.length !== b.length) { fallas.push(`${ruta}: largo app=${a.length} C++=${b.length}`); return; }
    a.forEach((x, i) => comparar(x, b[i], `${ruta}[${i}]`)); return;
  }
  if (a && b && typeof a === 'object' && typeof b === 'object') {
    for (const k of Object.keys(a)) comparar(a[k], b[k], `${ruta}.${k}`);
    for (const k of Object.keys(b)) if (!(k in a)) fallas.push(`${ruta}.${k}: sobra en C++`);
    return;
  }
  if (a !== b) fallas.push(`${ruta}: app=${JSON.stringify(a)} C++=${JSON.stringify(b)}`);
}

const umbral = 20;
const { datosInforme } = cargarInformeApp()({ umbralPrecisionM: umbral });
for (const o of casos) {
  const c = carrera(o);
  hzActual = o.hz || 1;
  const app = JSON.parse(JSON.stringify(datosInforme(JSON.parse(JSON.stringify(c)))));
  const cpp = JSON.parse(execFileSync(bin, [String(umbral)], { input: JSON.stringify(c), maxBuffer: 1 << 28 }).toString());
  comparar(app, cpp, 'caso' + o.semilla);
  if (fallas.length > 20) break;
}
console.log(`(aceleración con ventana de 1 s, sin comparar en 5/10 Hz: ${acelSalteadas} lecturas)`);
console.log(`Informe: ${casos.length} carreras, ${numeros} números comparados. Mayor diferencia en el dibujo: ${maxCoord.toFixed(3)} px; en lo demás: ${maxOtro}`);
if (fallas.length) { console.log('FALLAS:\n' + fallas.slice(0, 20).join('\n')); process.exit(1); }
console.log('IGUALES');
