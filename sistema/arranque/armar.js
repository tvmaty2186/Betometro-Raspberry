// Arma la imagen de la pantalla de arranque (psplash) desde arranque.html.
// Uso: node sistema/arranque/armar.js   → sistema/board/rpi3/arranque.png (720×1280, de pie)
// La pantalla real está de pie; la interfaz va girada 90° (como el programa con --fisica).
const { chromium } = require('playwright');
const path = require('path');
(async () => {
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: 1280, height: 720 } });
  await p.goto('file://' + path.join(__dirname, 'arranque.html'));
  await p.waitForTimeout(400);
  await p.locator('#a').screenshot({ path: path.join(__dirname, 'arranque-horizontal.png') });
  await b.close();
})();
