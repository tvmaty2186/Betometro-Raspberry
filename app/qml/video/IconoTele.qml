import QtQuick

// Televisor chiquito dibujado (el emoji 📺 no está en las letras del equipo)
Canvas {
    property color color: "#ffffff"
    implicitWidth: 30; implicitHeight: 26
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onColorChanged: requestPaint()
    onPaint: {
        const g = getContext("2d"), w = width, h = height, l = Math.max(2, w / 12);
        g.reset();
        g.strokeStyle = color; g.fillStyle = color; g.lineWidth = l; g.lineCap = "round"; g.lineJoin = "round";
        // antenas
        g.beginPath(); g.moveTo(w * 0.32, h * 0.04); g.lineTo(w * 0.5, h * 0.22); g.lineTo(w * 0.68, h * 0.04); g.stroke();
        // pantalla
        const y0 = h * 0.24, r = w * 0.12;
        g.beginPath(); g.roundedRect(l / 2, y0, w - l, h - y0 - l / 2, r, r); g.stroke();
        g.globalAlpha = 0.35;
        g.beginPath(); g.roundedRect(l * 2, y0 + l * 1.5, w - l * 4, h - y0 - l * 3.5, r / 2, r / 2); g.fill();
    }
}
