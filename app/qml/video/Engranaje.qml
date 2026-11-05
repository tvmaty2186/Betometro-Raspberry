import QtQuick

// Engranaje dibujado (no depende de que la letra tenga «⚙»)
Canvas {
    property color color: "#111111"
    onWidthChanged: requestPaint()
    onColorChanged: requestPaint()
    onPaint: {
        const g = getContext("2d"); g.reset(); g.scale(width / 64, height / 64);
        g.strokeStyle = color; g.lineWidth = 6; g.lineCap = "round";
        g.beginPath(); g.arc(32, 32, 9, 0, Math.PI * 2); g.stroke();
        for (let i = 0; i < 8; i++) {
            const a = i * Math.PI / 4;
            g.beginPath(); g.moveTo(32 + 17 * Math.cos(a), 32 + 17 * Math.sin(a)); g.lineTo(32 + 26 * Math.cos(a), 32 + 26 * Math.sin(a)); g.stroke();
        }
    }
}
