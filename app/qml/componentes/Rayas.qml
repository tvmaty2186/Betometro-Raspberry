import QtQuick
import Betometro

// Rayas diagonales blanca y amarilla (.rayasT): skewX(−28°) sobre una caja de width × height.
// El lienzo es más ancho que la caja para que la inclinación no se corte.
Item {
    id: raiz
    width: 80; height: 130
    readonly property real s: Math.tan(28 * Math.PI / 180) * height / 2
    Canvas {
        x: -raiz.s; y: 0; width: raiz.width + 2 * raiz.s; height: raiz.height
        onPaint: {
            const c = getContext("2d"), W = raiz.width, H = raiz.height, s = raiz.s, o = raiz.s;
            c.reset();
            const franja = (x0, x1, col) => {
                c.beginPath();
                c.moveTo(o + x0 + s, 0); c.lineTo(o + x1 + s, 0); c.lineTo(o + x1 - s, H); c.lineTo(o + x0 - s, H); c.closePath();
                c.fillStyle = col; c.fill();
            };
            franja(0, W * 0.40, Tema.blanco);
            franja(W * 0.52, W, Tema.amarillo);
        }
        Component.onCompleted: requestPaint()
    }
}
