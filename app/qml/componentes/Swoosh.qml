import QtQuick
import Betometro

// Curva blanca y amarilla de abajo (.swoosh): óvalo con borde amarillo arriba y blanco adentro, girado −4°.
Canvas {
    rotation: -4
    onWidthChanged: requestPaint()
    onPaint: {
        const c = getContext("2d"), W = width, H = height;
        c.reset();
        const ovalo = (dy) => { c.ellipse(0, dy, W, H); };
        c.fillRule = Qt.OddEvenFill;
        c.beginPath(); ovalo(0); ovalo(14); c.fillStyle = Tema.amarillo; c.fill();
        c.beginPath(); ovalo(14); ovalo(30); c.fillStyle = Tema.blanco; c.fill();
    }
    Component.onCompleted: requestPaint()
}
