import QtQuick
import Betometro

// Fondo del Avispón: degradé radial verde y un damero que se desvanece arriba a la izquierda.
Canvas {
    id: lienzo
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    Connections { target: Tema; function onNombreChanged() { lienzo.requestPaint(); } function onSolChanged() { lienzo.requestPaint(); } }

    onPaint: {
        const c = getContext("2d");
        const W = width, H = height;
        c.reset();
        // radial-gradient(120% 80% at 25% 0%, #3d8c57 0%, v2 40%, v3 100%)
        c.save();
        const rx = W * 1.2, ry = H * 0.8;
        c.translate(W * 0.25, 0);
        c.scale(1, ry / rx);
        const g = c.createRadialGradient(0, 0, 0, 0, 0, rx);
        g.addColorStop(0, Tema.fondo1);
        g.addColorStop(0.4, Tema.fondo2);
        g.addColorStop(1, Tema.fondo3);
        c.fillStyle = g;
        c.fillRect(-W, -H * 4, W * 3, H * 9);
        c.restore();
        if (!Tema.damero) return;
        // damero de 46 px, fundido con radial(60% 50% at 0% 20%)
        const L = 23, cx = 0, cy = H * 0.2, ex = W * 0.6, ey = H * 0.5;
        for (let y = 0; y < H * 0.75; y += L) {
            for (let x = 0; x < W * 0.6; x += L) {
                if (((x / L) + (y / L)) % 2 !== 0) continue;
                const dx = (x + L / 2 - cx) / ex, dy = (y + L / 2 - cy) / ey;
                const d = Math.sqrt(dx * dx + dy * dy);
                if (d >= 1) continue;
                c.fillStyle = "rgba(255,255,255," + (0.05 * (1 - d)).toFixed(4) + ")";
                c.fillRect(x, y, L, L);
            }
        }
    }
}
