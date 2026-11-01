import QtQuick
import Betometro

// Velocímetro redondo crema con bisel cromado (.gauge de la app). Hasta `maximo` km/h.
Item {
    id: raiz
    property real maximo: 160
    property real valor: 0
    property bool hayValor: true
    property real sugerida: -1      // km/h; −1 = sin marca
    width: 320; height: 320
    onValorChanged: lienzo.requestPaint()
    onSugeridaChanged: lienzo.requestPaint()
    onHayValorChanged: lienzo.requestPaint()

    Canvas {
        id: lienzo
        x: -40; y: -40; width: raiz.width + 80; height: raiz.height + 90
        onPaint: {
            const c = getContext("2d"), W = raiz.width, k = W / 320, cx = 40 + W / 2, cy = 40 + W / 2, R = W / 2;
            c.reset();
            for (let i = 0; i < 10; i++) {   // sombra 0 18px 40px
                c.beginPath(); c.arc(cx, cy + 18, R + 15 + 22 - i * 2.2, 0, Math.PI * 2);
                c.fillStyle = "rgba(0,0,0,0.05)"; c.fill();
            }
            const disco = (rr, col) => { c.beginPath(); c.arc(cx, cy, rr, 0, Math.PI * 2); c.fillStyle = col; c.fill(); };
            disco(R + 15, "#b9bfc5"); disco(R + 11, "#5d646b"); disco(R + 9, "#e8ebee");
            const g = c.createRadialGradient(cx - R * 0.1, cy - R * 0.24, 0, cx - R * 0.1, cy - R * 0.24, R * 1.2);
            g.addColorStop(0, "#fffbef"); g.addColorStop(0.55, "#efe6cf"); g.addColorStop(1, "#d6caa9");
            disco(R, g);
            const r = R - 14 * k;
            c.textAlign = "center";
            const ang = v => (225 - Math.max(0, Math.min(raiz.maximo, v)) / raiz.maximo * 270) * Math.PI / 180;
            if (raiz.sugerida >= 0) {   // franja verde ±2 km/h y marca amarilla de la sugerida
                c.beginPath();
                c.arc(cx, cy, r - 5 * k, -ang(raiz.sugerida - 2), -ang(raiz.sugerida + 2), false);
                c.strokeStyle = "#2f7a49"; c.lineWidth = 10 * k; c.lineCap = "butt"; c.stroke();
                const a = ang(raiz.sugerida);
                c.beginPath();
                c.moveTo(cx + Math.cos(a) * (r + 4 * k), cy - Math.sin(a) * (r + 4 * k));
                c.lineTo(cx + Math.cos(a) * (r - 26 * k), cy - Math.sin(a) * (r - 26 * k));
                c.strokeStyle = "#d9a20f"; c.lineWidth = 7 * k; c.stroke();
            }
            for (let v = 0; v <= raiz.maximo; v += 10) {
                const a = (225 - v / raiz.maximo * 270) * Math.PI / 180, L = (v % 20 ? 10 : 20) * k;
                c.beginPath();
                c.moveTo(cx + Math.cos(a) * r, cy - Math.sin(a) * r);
                c.lineTo(cx + Math.cos(a) * (r - L), cy - Math.sin(a) * (r - L));
                c.strokeStyle = "#1d2a20"; c.lineWidth = (v % 20 ? 2 : 5) * k; c.stroke();
                if (v % 20 === 0) {
                    c.font = "700 " + (26 * k) + "px '" + Tema.fuenteNum + "'";
                    c.fillStyle = "#1d5233";
                    c.fillText(String(v), cx + Math.cos(a) * (r - 42 * k), cy - Math.sin(a) * (r - 42 * k) + 9 * k);
                }
            }
            if (raiz.hayValor) {
                const v = Math.max(0, Math.min(raiz.maximo, raiz.valor));
                const a = (225 - v / raiz.maximo * 270) * Math.PI / 180;
                c.beginPath(); c.moveTo(cx, cy);
                c.lineTo(cx + Math.cos(a) * (r - 30 * k), cy - Math.sin(a) * (r - 30 * k));
                c.strokeStyle = "#111"; c.lineWidth = 7 * k; c.lineCap = "round"; c.stroke();
            }
            disco(14 * k, "#111");
            c.font = "italic 800 " + (56 * k) + "px '" + Tema.fuenteNum + "'";
            c.fillStyle = "#1d5233";
            if (raiz.hayValor) c.fillText(String(Math.round(raiz.valor)), cx, cy + 80 * k);
            else { c.font = "italic 800 " + (30 * k) + "px '" + Tema.fuenteNum + "'"; c.fillText("SIN GPS", cx, cy + 74 * k); }
            c.font = (14 * k) + "px '" + Tema.fuente + "'";
            c.fillStyle = "#333";
            c.fillText("K M / H", cx, cy + 104 * k);
        }
        Component.onCompleted: requestPaint()
    }
}
