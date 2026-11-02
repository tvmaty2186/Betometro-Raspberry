// Corre un caso (texto de órdenes, ver casos.js) con el Motor de la app y devuelve lo que pasó, en texto.
// correr_motor.cpp hace exactamente lo mismo con el motor en C++.
'use strict';
const { cargarMotorApp } = require('./motor-app');
const Motor = cargarMotorApp();

const txt = x => (x === null || x === undefined) ? '-' : String(x);

function correrApp(caso) {
  const out = [];
  let carrera = null, motor = null, ahora = 0, terminarPendiente = false;
  const oyente = (tipo, d) => {
    if (tipo === 'vuelta') out.push(`ev vuelta n=${d.n} medida=${d.medida} inicio=${txt(d.inicio)}`);
    else if (tipo === 'cerrada') {
      out.push(`ev cerrada n=${d.n} medida=${d.medida} idealMs=${txt(d.idealMs)} realMs=${txt(d.realMs)} difMs=${txt(d.difMs)}` +
               ` fuente=${d.fuente} distM=${txt(d.distM)} velPromKmh=${txt(d.velPromKmh)} velMaxKmh=${txt(d.velMaxKmh)}` +
               ` factorVuelta=${txt(d.factorVuelta)}`);
      // Lo que hace la app (alEvento): si llegó al total, termina la carrera
      if (carrera.vueltasTotal && d.n >= carrera.vueltasTotal) terminarPendiente = true;
    }
    else if (tipo === 'ignorada') out.push('ev ignorada');
    else if (tipo === 'descartada') out.push(`ev descartada t=${txt(d.t)}`);
    else if (tipo === 'terminada') out.push('ev terminada');
  };
  for (const linea of caso.split('\n')) {
    if (!linea.trim()) continue;
    const p = linea.split(' ');
    const op = p[0];
    if (op === 'carrera') {
      carrera = { tiempoIdealMs: +p[1], distanciaM: +p[2], patron: p[3], vueltasTotal: +p[4], antirreboteMs: +p[5],
                  umbralPrecisionM: +p[6], calibrar: p[7] === '1', congelarMs: +p[8], factorGps: +p[9], vueltas: [] };
      motor = Motor.crear(carrera, oyente);
    } else if (op === 'm') {
      ahora = +p[1];
      const ok = motor.muestra({ t: +p[1], velMs: +p[2], precisionM: +p[3] });
      out.push(`r muestra ${ok}`);
    } else if (op === 'k') {
      ahora = +p[1];
      const r = motor.marcar(+p[1], p[2]);
      out.push(`r marcar ${r} factor=${txt(motor.factor)}`);
      if (terminarPendiente) { terminarPendiente = false; if (motor.enCurso) motor.terminar(); }
    } else if (op === 'x') {
      ahora = +p[1];
      if (motor.enCurso) motor.terminar();
    } else if (op === 'e') {
      ahora = +p[1];
      const e = motor.estado(+p[1]);
      out.push(`est t=${p[1]} enCurso=${e.enCurso} n=${e.n} medida=${e.medida} transMs=${txt(e.transMs)}` +
               ` restanteMs=${txt(e.restanteMs)} proxMedida=${e.proxMedida} sinGps=${e.sinGps} vActKmh=${txt(e.vActKmh)}` +
               ` vBaseKmh=${txt(e.vBaseKmh)} vNecKmh=${txt(e.vNecKmh)} congelada=${!!e.congelada}` +
               ` restanteM=${txt(e.restanteM)} deltaS=${txt(e.deltaS)}`);
    } else if (op === 'ca') carrera.antirreboteMs = +p[1];
    else if (op === 'cu') carrera.umbralPrecisionM = +p[1];
    else if (op === 'cc') carrera.calibrar = p[1] === '1';
    else throw new Error('Orden desconocida: ' + linea);
  }
  return out.join('\n') + '\n';
}

module.exports = { correrApp };
