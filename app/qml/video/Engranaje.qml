import QtQuick

// Engranaje dibujado (no depende de que la letra tenga «⚙»)
Canvas {
    property color color: "#111111"
    onWidthChanged: requestPaint()
    onColorChanged: requestPaint()
    onPaint: {
        const g = getContext("2d"); g.reset(); g.scale(width / 64, height / 64);
        g.strokeStyle = color; g.lineWidth = 6; g.lineCap = "round"; g.lineJoin = "round";
        const c = g;
        // Engranaje de verdad (A, 8 dientes; muestra aprobada 1.0.29): rueda con dientes y agujero al medio
        const n = 8, ro = 27, ri = 20, ht = 0.17, hb = 0.30, paso = 2 * Math.PI / n;
        const P = (r, a) => [32 + r * Math.cos(a), 32 + r * Math.sin(a)];
        c.beginPath();
        for (let i = 0; i < n; i++) {
            const a = i * paso - Math.PI / 2;
            const p0 = P(ri, a - paso * hb), p1 = P(ro, a - paso * ht), p2 = P(ro, a + paso * ht);
            if (i === 0) c.moveTo(p0[0], p0[1]); else c.arc(32, 32, ri, a - paso + paso * hb, a - paso * hb, false);
            c.lineTo(p1[0], p1[1]); c.lineTo(p2[0], p2[1]); const p3 = P(ri, a + paso * hb); c.lineTo(p3[0], p3[1]);
        }
        c.arc(32, 32, ri, -Math.PI / 2 + paso * (n - 1) + paso * hb, -Math.PI / 2 - paso * hb + 2 * Math.PI, false);
        c.closePath(); c.stroke();
        g.beginPath(); g.arc(32, 32, 8, 0, Math.PI * 2); g.stroke();
    }
}
