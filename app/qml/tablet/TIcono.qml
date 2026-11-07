import QtQuick
import Betometro

// Íconos de línea del Modo Tablet (dibujados: se pintan del color que se pida, en cualquier tema).
// nombre: bandera, crono, check, pantalla, camara, red, ajustes, voz, tema, auto, gps, velocidad,
//         actualizar, apagar, mic, girar, espejo, rec, atras, tarjeta, buscar, lista, qr, informe, lugar, nube, ruta
Canvas {
    id: raiz
    property string nombre: ""
    property color color: Tema.tinta
    width: 72; height: 72
    onNombreChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onPaint: {
        const c = getContext("2d"), k = width / 64;
        c.reset();
        c.scale(k, k);
        c.strokeStyle = String(color); c.fillStyle = String(color);
        c.lineWidth = 5; c.lineCap = "round"; c.lineJoin = "round";
        const L = (pts) => { c.beginPath(); c.moveTo(pts[0], pts[1]); for (let i = 2; i < pts.length; i += 2) c.lineTo(pts[i], pts[i + 1]); c.stroke(); };
        const C = (x, y, r, relleno) => { c.beginPath(); c.arc(x, y, r, 0, Math.PI * 2); if (relleno) c.fill(); else c.stroke(); };
        const A = (x, y, r, a0, a1) => { c.beginPath(); c.arc(x, y, r, a0 * Math.PI / 180, a1 * Math.PI / 180, false); c.stroke(); };
        const R = (x, y, w, h, r) => { c.beginPath(); c.roundedRect(x, y, w, h, r, r); c.stroke(); };
        switch (nombre) {
        case "bandera": L([14, 58, 14, 10]); L([14, 12, 46, 12, 41, 21, 46, 30, 14, 30]); break;
        case "crono": C(32, 36, 20); L([32, 36, 32, 24]); L([26, 8, 38, 8]); L([32, 8, 32, 16]); break;
        case "check": R(14, 10, 36, 46, 6); L([22, 34, 29, 41, 42, 27]); break;
        case "tilde": L([14, 33, 26, 45, 50, 20]); break;
        case "borrar": L([22, 16, 54, 16, 54, 48, 22, 48, 8, 32, 22, 16]); L([31, 24, 45, 40]); L([45, 24, 31, 40]); break;
        case "pantalla": R(8, 12, 20, 18, 4); R(34, 12, 22, 18, 4); R(8, 36, 48, 16, 4); break;
        case "camara": R(6, 18, 38, 28, 6); L([44, 28, 58, 20, 58, 44, 44, 36]); break;
        case "red": A(32, 50, 30, 220, 320); A(32, 50, 19, 220, 320); A(32, 50, 8, 220, 320); C(32, 50, 3, true); break;
        case "ajustes": C(32, 32, 9);
            for (let i = 0; i < 8; i++) { const a = i * Math.PI / 4; L([32 + Math.cos(a) * 16, 32 + Math.sin(a) * 16, 32 + Math.cos(a) * 25, 32 + Math.sin(a) * 25]); }
            break;
        case "voz": L([10, 26, 20, 26, 34, 14, 34, 50, 20, 38, 10, 38, 10, 26]); A(34, 32, 12, -45, 45); A(34, 32, 20, -45, 45); break;
        case "tema": C(32, 32, 24); C(22, 24, 4); C(36, 18, 4); C(44, 30, 4); break;
        case "auto": L([8, 42, 8, 32, 14, 20, 50, 20, 56, 32, 56, 42, 8, 42]); C(18, 42, 5); C(46, 42, 5); break;
        case "gps": C(32, 26, 8);
            c.beginPath(); c.moveTo(32, 58); c.bezierCurveTo(20, 44, 14, 36, 14, 26); c.arc(32, 26, 18, Math.PI, 0, false);
            c.bezierCurveTo(50, 36, 44, 44, 32, 58); c.stroke(); break;
        case "velocidad": A(32, 44, 22, 180, 360); L([32, 44, 44, 30]); break;
        case "actualizar": A(32, 32, 18, 20, 330); L([50, 12, 50, 24, 38, 24]); L([32, 22, 32, 40]); L([25, 33, 32, 40, 39, 33]); break;
        case "apagar": L([32, 8, 32, 30]); A(32, 34, 20, -55, 235); break;
        case "mic": R(24, 8, 16, 30, 8); A(32, 30, 16, 0, 180); L([32, 46, 32, 56]); break;
        case "girar": A(32, 32, 18, 20, 330); L([50, 12, 50, 24, 38, 24]); break;
        case "espejo":   // dos triángulos espejados a los lados de una línea punteada
            L([26, 16, 26, 48, 8, 48, 26, 16]); L([38, 16, 38, 48, 56, 48, 38, 16]);
            c.lineWidth = 3; c.setLineDash([4, 4]); L([32, 6, 32, 58]); c.setLineDash([]); break;
        case "rec": c.fillStyle = "#ff3b30"; C(32, 32, 16, true); break;
        case "atras": L([38, 14, 20, 32, 38, 50]); break;
        case "ruta":   // recorrido libre: un camino con curvas, de un punto a otro
            C(12, 50, 5); C(54, 12, 4, true);
            c.beginPath(); c.moveTo(17, 48); c.bezierCurveTo(31, 44, 21, 26, 35, 22); c.bezierCurveTo(49, 18, 49, 32, 54, 16); c.stroke(); break;
        case "tarjeta": L([14, 8, 40, 8, 50, 18, 50, 56, 14, 56, 14, 8]); L([22, 18, 22, 26]); L([30, 18, 30, 26]); L([38, 18, 38, 26]); break;
        case "buscar": C(28, 28, 15); L([39, 39, 54, 54]); break;
        case "lista": L([20, 18, 54, 18]); L([20, 32, 54, 32]); L([20, 46, 54, 46]); C(10, 18, 2, true); C(10, 32, 2, true); C(10, 46, 2, true); break;
        case "qr": R(8, 8, 20, 20, 3); R(36, 8, 20, 20, 3); R(8, 36, 20, 20, 3); L([38, 38, 54, 38, 54, 54]); L([38, 48, 46, 48]); break;
        case "informe": L([12, 54, 12, 10]); L([12, 54, 56, 54]); L([18, 44, 30, 30, 38, 38, 52, 20]); break;
        case "nube":   // nube con flecha para arriba (Google Drive)
            c.beginPath(); c.moveTo(18, 48); c.arc(18, 38, 10, Math.PI / 2, Math.PI * 1.5, false); c.arc(32, 26, 14, Math.PI * 1.1, Math.PI * 1.95, false);
            c.arc(46, 38, 10, Math.PI * 1.5, Math.PI / 2, false); c.stroke();
            L([32, 56, 32, 34]); L([24, 42, 32, 34, 40, 42]); break;
        case "tele": L([22, 6, 32, 15, 42, 6]); R(6, 15, 52, 36, 7); L([20, 58, 44, 58]); L([32, 51, 32, 58]); break;
        case "lugar": C(32, 26, 8);
            c.beginPath(); c.moveTo(32, 58); c.bezierCurveTo(20, 44, 14, 36, 14, 26); c.arc(32, 26, 18, Math.PI, 0, false);
            c.bezierCurveTo(50, 36, 44, 44, 32, 58); c.stroke(); break;
        }
    }
}
