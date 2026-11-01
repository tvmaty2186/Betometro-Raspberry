import QtQuick
import Betometro

// Redondel cromado con el número del auto (.num101 de la app).
Item {
    id: raiz
    property string numero: "101"
    property real d: 112
    width: d; height: d
    Canvas {
        x: -8; y: -8; width: raiz.d + 16; height: raiz.d + 34
        onPaint: {
            const c = getContext("2d"), r = raiz.d / 2, cx = 8 + r, cy = 8 + r;
            c.reset();
            for (let i = 0; i < 6; i++) { c.beginPath(); c.arc(cx, cy + 10, r + 5 + 8 - i * 1.6, 0, Math.PI * 2); c.fillStyle = "rgba(0,0,0,0.08)"; c.fill(); }
            const disco = (rr, col) => { c.beginPath(); c.arc(cx, cy, rr, 0, Math.PI * 2); c.fillStyle = col; c.fill(); };
            disco(r + 5, "#4b5258"); disco(r + 3, "#f5f6f7");
            const g = c.createRadialGradient(cx - r * 0.2, cy - r * 0.4, 0, cx - r * 0.2, cy - r * 0.4, r * 1.3);
            g.addColorStop(0, "#ffffff"); g.addColorStop(0.45, "#d8dde1"); g.addColorStop(0.75, "#9aa2a9"); g.addColorStop(1, "#e3e6e9");
            disco(r, g);
        }
        Component.onCompleted: requestPaint()
    }
    Text {
        anchors.centerIn: parent
        text: raiz.numero
        color: Tema.numeroVerde
        font { family: Tema.fuenteNum; italic: true; weight: Font.Bold; pixelSize: raiz.d * 46 / 112 }
    }
}
