// Saca armarRecorrido y datosInforme de la copia de la app y los deja listos para usar.
'use strict';
const fs = require('fs');
const path = require('path');
function cargarInformeApp() {
  const html = fs.readFileSync(path.join(__dirname, '..', '..', 'referencia', 'app-2.1.1', 'index.html'), 'utf8');
  const ini = html.indexOf('function armarRecorrido(c)');
  const fin = html.indexOf('function informeHTML(c)');
  if (ini < 0 || fin < 0) throw new Error('No encontré armarRecorrido / datosInforme en la app');
  const codigo = html.slice(ini, fin);
  return new Function('cfg', codigo + '\nreturn { armarRecorrido, datosInforme };');
}
module.exports = { cargarInformeApp };
