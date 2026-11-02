// Saca el `Motor` de la copia de la app (referencia/app-2.1.1/index.html) y lo devuelve listo para usar.
// No se copia a mano: se lee del archivo, así se prueba contra el código real de la app.
'use strict';
const fs = require('fs');
const path = require('path');

function cargarMotorApp() {
  const archivo = path.join(__dirname, '..', '..', 'referencia', 'app-2.1.1', 'index.html');
  const html = fs.readFileSync(archivo, 'utf8');
  const inicio = html.indexOf('const Motor = (() => {');
  if (inicio < 0) throw new Error('No encontré el Motor en ' + archivo);
  const fin = html.indexOf('\n})();', inicio);
  if (fin < 0) throw new Error('No encontré el final del Motor');
  const codigo = html.slice(inicio, fin + '\n})();'.length);
  return new Function(codigo + '\nreturn Motor;')();
}

module.exports = { cargarMotorApp };
