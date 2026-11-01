// Genera los recorridos de prueba del motor. Cada caso es una lista de órdenes en texto:
//   carrera T D patron vueltasTotal antirreboteMs umbralPrecisionM calibrar congelarMs factorGps
//   m t velMs precisionM     muestra del GPS
//   k t fuente               marca
//   e t                      pedir estado
//   x t                      terminar (a mano)
//   ca ms | cu m | cc 0/1    cambiar en vivo antirrebote / umbral / calibrar
// Los dos motores (app y C++) leen exactamente el mismo texto.
'use strict';

// Azar repetible (mulberry32): mismos casos en cada corrida
function azar(semilla) {
  let a = semilla >>> 0;
  return () => {
    a = (a + 0x6D2B79F5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

const kmh = v => v / 3.6;

// Arma un recorrido: muestras del GPS, marcas y estados cada 100 ms, ordenados por tiempo.
function recorrido(o) {
  const r = azar(o.semilla || 1);
  const ord = [];
  const T = o.T, D = o.D;
  ord.push(['carrera', T, D, o.patron || 'alt1', o.vueltasTotal || 0, o.antirrebote ?? 3000,
            o.umbral ?? 20, o.calibrar === false ? 0 : 1, o.congelar ?? 20000, o.factorGps ?? 1]);
  const t0 = 1000 + r() * 500;                 // performance.now() no arranca en 0
  const hz = o.hz || 10;
  const vueltas = o.vueltas || 4;
  const sesgo = o.sesgo || 1;                  // el GPS mide de más o de menos
  const largada = t0 + (o.antesMs ?? 5000);
  const fin = largada + vueltas * T * 1.02 + (o.despuesMs ?? 2000);
  const eventos = [];

  // Muestras
  if (!o.sinGps) {
    let t = t0;
    while (t < fin) {
      const enHueco = (o.huecos || []).some(([a, b]) => t >= largada + a && t < largada + b);
      if (!enHueco) {
        const base = D > 0 ? D / (T / 1000) : kmh(50);
        let v = base * sesgo * (1 + (o.variacion ?? 0.15) * Math.sin((t - largada) / 7000) + (r() - 0.5) * (o.ruido ?? 0.04));
        if (t < largada) v = r() * 0.5;
        v = Math.max(0, v);
        let prec = 2 + r() * 6;
        if (r() < (o.malaPrecision || 0)) prec = 20 + r() * 60;
        if (o.precisionBorde && r() < 0.1) prec = o.umbral ?? 20;   // justo en el umbral: no se descarta
        eventos.push([t, 'm', v, prec]);
      }
      const paso = 1000 / hz;
      t += paso + (o.jitter ?? 3) * (r() - 0.5) * 2;
      if (o.desordenadas && r() < 0.03) eventos.push([t - paso * 1.5, 'm', kmh(40), 3]);
    }
  }

  // Marcas
  eventos.push([largada, 'k', 'pulsador']);
  let tl = largada;
  for (let n = 1; n <= vueltas; n++) {
    let dur = T * (1 + (r() - 0.5) * (o.errorMarca ?? 0.02));
    if (o.vueltaCorta === n) dur = T * 0.7;    // se cortó camino: no calibra
    tl += dur;
    eventos.push([tl, 'k', ['pantalla', 'pulsador', 'teclado'][n % 3]]);
    if (o.dobles && r() < 0.6) eventos.push([tl + 50 + r() * (o.antirrebote ?? 3000) * 0.9, 'k', 'pulsador']);
    if (o.dobles && r() < 0.2) eventos.push([tl + (o.antirrebote ?? 3000) + 0.0, 'k', 'pantalla']);  // justo en el antirrebote
  }
  if (o.terminarEn) eventos.push([largada + o.terminarEn, 'x']);

  // Cambios en vivo
  for (const [dt, que, val] of (o.cambios || [])) eventos.push([largada + dt, que, val]);

  // Estados cada 100 ms (como lee la pantalla) y además en momentos al azar
  for (let t = t0; t < fin; t += 100) eventos.push([t, 'e']);
  for (let i = 0; i < 200; i++) eventos.push([t0 + r() * (fin - t0), 'e']);

  eventos.sort((a, b) => a[0] - b[0]);
  for (const ev of eventos) {
    const [t, tipo, ...resto] = ev;
    if (tipo === 'm') ord.push(['m', t, resto[0], resto[1]]);
    else if (tipo === 'k') ord.push(['k', t, resto[0]]);
    else if (tipo === 'x') ord.push(['x', t]);
    else if (tipo === 'e') ord.push(['e', t]);
    else ord.push([tipo, resto[0]]);
  }
  return ord.map(f => f.map(String).join(' ')).join('\n') + '\n';
}

function casos() {
  const lista = {
    'basico-10hz': { T: 240000, D: 3200, vueltas: 5, semilla: 1 },
    'basico-1hz': { T: 240000, D: 3200, vueltas: 4, hz: 1, jitter: 80, semilla: 2 },
    'patron-todas': { T: 90000, D: 1500, patron: 'todas', vueltas: 4, semilla: 3 },
    'patron-alt2': { T: 90000, D: 1500, patron: 'alt2', vueltas: 4, semilla: 4 },
    'huecos-gps': { T: 120000, D: 2000, vueltas: 3, huecos: [[10000, 14000], [40000, 52000], [200000, 203500]], semilla: 5 },
    'mala-precision': { T: 120000, D: 2000, vueltas: 3, malaPrecision: 0.3, precisionBorde: true, semilla: 6 },
    'marcas-dobles': { T: 60000, D: 1000, vueltas: 6, dobles: true, semilla: 7 },
    'calibracion-gps-de-mas': { T: 180000, D: 3000, vueltas: 5, sesgo: 1.06, semilla: 8 },
    'calibracion-gps-de-menos': { T: 180000, D: 3000, vueltas: 5, sesgo: 0.93, semilla: 9 },
    'calibracion-vuelta-corta': { T: 180000, D: 3000, vueltas: 5, sesgo: 1.03, vueltaCorta: 2, semilla: 10 },
    'sin-calibrar': { T: 180000, D: 3000, vueltas: 3, sesgo: 1.05, calibrar: false, semilla: 11 },
    'gps-fuera-de-rango': { T: 180000, D: 3000, vueltas: 3, sesgo: 1.3, semilla: 12 },
    'sin-gps': { T: 100000, D: 1800, vueltas: 3, sinGps: true, semilla: 13 },
    'sin-distancia': { T: 100000, D: 0, vueltas: 3, semilla: 14 },
    'congelar-3s': { T: 60000, D: 900, vueltas: 4, congelar: 3000, semilla: 15 },
    'congelar-60s': { T: 90000, D: 1200, vueltas: 3, congelar: 60000, semilla: 16 },
    'vuelta-pasada': { T: 60000, D: 900, vueltas: 3, errorMarca: 0.4, semilla: 17 },
    'llega-antes': { T: 60000, D: 900, vueltas: 3, sesgo: 1.12, calibrar: false, semilla: 18 },
    'vueltas-total': { T: 60000, D: 900, vueltas: 5, vueltasTotal: 3, semilla: 19 },
    'muestras-desordenadas': { T: 90000, D: 1500, vueltas: 3, desordenadas: true, semilla: 20 },
    'sin-antirrebote': { T: 60000, D: 1000, vueltas: 4, antirrebote: 0, dobles: true, semilla: 21 },
    'cambios-en-vivo': { T: 120000, D: 2000, vueltas: 4, malaPrecision: 0.2, dobles: true,
                         cambios: [[30000, 'cu', 5], [90000, 'cu', 60], [150000, 'cc', 0], [250000, 'cc', 1], [300000, 'ca', 10000]],
                         semilla: 22 },
    'factor-inicial': { T: 120000, D: 2000, vueltas: 3, factorGps: 1.02, semilla: 23 },
    'terminar-a-mano': { T: 120000, D: 2000, vueltas: 3, terminarEn: 300000, semilla: 24 },
    'detenido-largo': { T: 120000, D: 2000, vueltas: 2, variacion: 1.2, semilla: 25 },
  };
  for (let s = 0; s < 40; s++) {   // al azar: distintas vueltas, velocidades, frecuencias y errores
    const r = azar(1000 + s);
    lista['azar-' + String(s).padStart(2, '0')] = {
      T: 20000 + Math.round(r() * 400000), D: Math.round(r() * 8000), patron: ['todas', 'alt1', 'alt2'][s % 3],
      vueltas: 1 + Math.floor(r() * 5), hz: [1, 5, 10][Math.floor(r() * 3)], jitter: r() * 40,
      sesgo: 0.9 + r() * 0.2, variacion: r() * 0.5, ruido: r() * 0.2, malaPrecision: r() * 0.3,
      errorMarca: r() * 0.1, dobles: r() < 0.5, congelar: 3000 + Math.round(r() * 30) * 1000,
      antirrebote: [0, 1500, 3000, 5000][Math.floor(r() * 4)], umbral: 5 + Math.round(r() * 30),
      calibrar: r() < 0.8, vueltasTotal: r() < 0.3 ? 2 : 0, desordenadas: r() < 0.2,
      huecos: r() < 0.4 ? [[5000, 9000 + r() * 8000]] : [], semilla: 5000 + s,
    };
  }
  const out = {};
  for (const [nombre, o] of Object.entries(lista)) out[nombre] = recorrido(o);
  return out;
}

module.exports = { casos };
