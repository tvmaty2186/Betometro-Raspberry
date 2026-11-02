// Compara el motor en C++ con el Motor de la app en todos los casos de casos.js.
// Uso: node pruebas/motor/comparar.js <ruta a correr_motor> [carpeta para el informe]
// Aceptación (plan E1): menos de 1 ms en tiempos y menos de 1 cm en distancias. Además se exige
// lo mismo en todo lo demás (velocidades, adelanto, estados, eventos): tiene que dar igual.
'use strict';
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const { casos } = require('./casos');
const { correrApp } = require('./correr-app');

const binario = process.argv[2];
const carpetaInforme = process.argv[3];
if (!binario) { console.error('Falta la ruta a correr_motor'); process.exit(2); }

const TIEMPOS = new Set(['t', 'inicio', 'idealMs', 'realMs', 'difMs', 'transMs', 'restanteMs']);
const DISTANCIAS = new Set(['distM', 'restanteM']);
const TOL_MS = 1, TOL_M = 0.01, TOL_REL = 1e-9;

const campos = linea => linea.split(' ').map(x => { const i = x.indexOf('='); return i < 0 ? ['', x] : [x.slice(0, i), x.slice(i + 1)]; });
const maximos = {};   // campo → mayor diferencia vista
let lineasTotal = 0, estadosTotal = 0, vueltasTotal = 0, interrumpidas = 0;
const fallas = [];

for (const [nombre, caso] of Object.entries(casos())) {
  const app = correrApp(caso).trimEnd().split('\n');
  const cpp = execFileSync(binario, { input: caso, maxBuffer: 1 << 28 }).toString().trimEnd().split('\n');
  // La vuelta interrumpida es propia de la Raspberry: la app no la tiene
  const cppComparable = cpp.filter(l => { if (l.startsWith('ev interrumpida')) { interrumpidas++; return false; } return true; });
  if (app.length !== cppComparable.length) {
    fallas.push(`${nombre}: la app dio ${app.length} líneas y C++ ${cppComparable.length}`);
  }
  const n = Math.min(app.length, cppComparable.length);
  for (let i = 0; i < n; i++) {
    lineasTotal++;
    if (app[i].startsWith('est ')) estadosTotal++;
    if (app[i].startsWith('ev cerrada')) vueltasTotal++;
    const a = campos(app[i]), c = campos(cppComparable[i]);
    if (a.length !== c.length) { fallas.push(`${nombre} línea ${i + 1}:\n  app: ${app[i]}\n  C++: ${cppComparable[i]}`); break; }
    for (let k = 0; k < a.length; k++) {
      const [clave, va] = a[k], [claveC, vc] = c[k];
      if (clave !== claveC) { fallas.push(`${nombre} línea ${i + 1}: campo ${clave} contra ${claveC}`); break; }
      if (va === vc) { maximos[clave || '(texto)'] = Math.max(maximos[clave || '(texto)'] || 0, 0); continue; }
      const na = Number(va), nc = Number(vc);
      if (va === '-' || vc === '-' || isNaN(na) || isNaN(nc)) {
        fallas.push(`${nombre} línea ${i + 1}: ${clave} app=${va} C++=${vc}`); continue;
      }
      const dif = Math.abs(na - nc);
      maximos[clave] = Math.max(maximos[clave] || 0, dif);
      const tol = TIEMPOS.has(clave) ? TOL_MS : DISTANCIAS.has(clave) ? TOL_M : TOL_REL * Math.max(1, Math.abs(na));
      if (!(dif < tol)) fallas.push(`${nombre} línea ${i + 1}: ${clave} app=${va} C++=${vc} (diferencia ${dif})`);
    }
    if (fallas.length > 30) break;
  }
  if (fallas.length > 30) break;
}

const cantCasos = Object.keys(casos()).length;
const resumen = [
  '# Comparación del motor C++ con el Motor de la app 2.1.1',
  '',
  `- Casos: **${cantCasos}**`,
  `- Líneas comparadas: **${lineasTotal}** (estados: ${estadosTotal}, vueltas cerradas: ${vueltasTotal})`,
  `- Vueltas interrumpidas (solo Raspberry, no se comparan): ${interrumpidas}`,
  `- Resultado: **${fallas.length ? 'FALLA' : 'IGUALES'}**`,
  '',
  '| Campo | Mayor diferencia |',
  '|---|---|',
  ...Object.keys(maximos).filter(k => k !== '(texto)').sort().map(k => `| ${k} | ${maximos[k]} |`),
  '',
];
if (fallas.length) resumen.push('## Fallas', '', ...fallas.map(f => '- ' + f), '');
const texto = resumen.join('\n');
console.log(texto);
if (carpetaInforme) fs.writeFileSync(path.join(carpetaInforme, 'comparacion-motor.md'), texto);
process.exit(fallas.length ? 1 : 0);
