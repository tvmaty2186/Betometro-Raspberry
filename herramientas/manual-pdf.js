// Arma recursos/manual.pdf (lo baja el celu en /manual.pdf) desde recursos/web/ayuda.html, la misma página de /ayuda.
// Correr cada vez que se cambia el manual:  node herramientas/manual-pdf.js
// Usa el Chromium de Playwright (require('playwright')). Las fuentes y el león salen de recursos/.
const { chromium } = require('playwright');
const fs = require('fs'), path = require('path');
const R = path.join(__dirname, '..', 'recursos');
(async () => {
  const b = await chromium.launch();
  const ctx = await b.newContext();
  await ctx.route('**/r/*', r => {
    const n = path.basename(new URL(r.request().url()).pathname);
    const f = n.endsWith('.woff2') ? `${R}/fuentes/${n}` : `${R}/img/${n}`;
    fs.existsSync(f) ? r.fulfill({ path: f }) : r.fulfill({ status: 404 });
  });
  const p = await ctx.newPage();
  const html = fs.readFileSync(`${R}/web/ayuda.html`, 'utf8');
  await p.route('http://betometro/ayuda', r => r.fulfill({ body: html, contentType: 'text/html; charset=utf-8' }));
  await p.goto('http://betometro/ayuda');
  await p.evaluate(() => document.fonts.ready);
  await p.pdf({
    path: `${R}/manual.pdf`, format: 'A4', printBackground: true, displayHeaderFooter: true, headerTemplate: '<span></span>',
    footerTemplate: '<div style="font-size:8pt;width:100%;text-align:center;color:#777">Betómetro · Manual del navegante · página <span class=pageNumber></span> de <span class=totalPages></span></div>',
    margin: { top: '14mm', bottom: '16mm', left: '14mm', right: '14mm' },
  });
  await b.close();
  console.log('Listo: recursos/manual.pdf');
})();
