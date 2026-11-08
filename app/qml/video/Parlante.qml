import QtQuick

// Parlante del volumen del video: con ondas según el nivel (0: tachado = silencio)
Canvas {
    id: raiz
    property real nivel: 1
    width: 32; height: 32
    onNivelChanged: requestPaint()
    onWidthChanged: requestPaint()
    onPaint: {
        const c = getContext("2d"), k = width / 32;
        c.reset(); c.scale(k, k);
        c.strokeStyle = "#ffffff"; c.fillStyle = "#ffffff"; c.lineWidth = 2.6; c.lineCap = "round"; c.lineJoin = "round";
        c.beginPath(); c.moveTo(4, 12); c.lineTo(9, 12); c.lineTo(16, 6); c.lineTo(16, 26); c.lineTo(9, 20); c.lineTo(4, 20); c.closePath(); c.fill();
        if (nivel <= 0) { c.beginPath(); c.moveTo(21, 11); c.lineTo(29, 21); c.moveTo(29, 11); c.lineTo(21, 21); c.stroke(); return; }
        c.beginPath(); c.arc(16, 16, 6, -0.9, 0.9); c.stroke();
        if (nivel > 0.34) { c.beginPath(); c.arc(16, 16, 10.5, -0.9, 0.9); c.stroke(); }
        if (nivel > 0.67) { c.beginPath(); c.arc(16, 16, 15, -0.9, 0.9); c.stroke(); }
    }
}
