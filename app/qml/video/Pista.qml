import QtQuick
import Betometro

// La pista chiquita del video con un punto amarillo donde va el auto. La pista se dibuja una sola vez;
// el punto es un círculo que se mueve (no se vuelve a pintar nada en cada cuadro).
// Mismas coordenadas que el mapa del informe (pista en un cuadro de 364×330).
Item {
    id: raiz
    property var inf: ({})
    property var punto: null      // {lap, x, y}
    property real grosor: 1       // ancho de la línea, relativo al mapa del informe
    readonly property real k: Math.min(width / 364, height / 330)
    readonly property real ox: (width - 364 * k) / 2
    readonly property real oy: (height - 330 * k) / 2
    onInfChanged: lienzo.requestPaint()
    Canvas {
        id: lienzo
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const g = getContext("2d");
            g.reset();
            const P = raiz.inf.pista || [];
            if (!P.length) return;
            const k = raiz.k, X = p => raiz.ox + p[0] * k, Y = p => raiz.oy + p[1] * k;
            g.lineJoin = "round"; g.lineCap = "round";
            g.beginPath(); g.moveTo(X(P[0]), Y(P[0]));
            for (const p of P) g.lineTo(X(p), Y(p));
            if (raiz.inf.cerrada) g.closePath();
            g.strokeStyle = "#3a3f45"; g.lineWidth = Math.max(6, 22 * k * raiz.grosor); g.stroke();
            g.strokeStyle = "#cfd4d8"; g.lineWidth = Math.max(2, 7 * k * raiz.grosor); g.stroke();
            // la largada: rayita blanca
            g.fillStyle = "#ffffff"; g.beginPath(); g.arc(X(P[0]), Y(P[0]), Math.max(3, 8 * k), 0, Math.PI * 2); g.fill();
        }
    }
    Rectangle {
        visible: !!raiz.punto && raiz.punto.lap >= 0
        readonly property real r: Math.max(7, Math.min(raiz.width, raiz.height) * 0.075)
        width: 2 * r; height: 2 * r; radius: r
        x: raiz.punto ? raiz.ox + raiz.punto.x * raiz.k - r : 0
        y: raiz.punto ? raiz.oy + raiz.punto.y * raiz.k - r : 0
        color: Tema.amarillo; border { width: Math.max(2, r * 0.27); color: "#111111" }
    }
}
