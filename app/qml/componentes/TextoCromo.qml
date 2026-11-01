import QtQuick
import Betometro

// Título relleno con el degradé cromo y sombra abajo (como .cromo-txt de la app).
// Se dibuja una sola vez en un Canvas: no gasta nada mientras no cambie.
Item {
    id: raiz
    property string texto: ""
    property real tam: 44
    property int peso: Font.ExtraBold

    readonly property string css: "italic 800 " + tam + "px '" + Tema.fuente + "'"

    TextMetrics {
        id: medida
        font.family: Tema.fuente
        font.italic: true
        font.weight: raiz.peso
        font.pixelSize: raiz.tam
        font.letterSpacing: raiz.tam * 0.02
        text: raiz.texto
    }
    implicitWidth: Math.ceil(medida.advanceWidth + tam * 0.25)
    implicitHeight: Math.ceil(tam * 1.25)

    Canvas {
        id: lienzo
        anchors.fill: parent
        onPaint: {
            const c = getContext("2d");
            c.reset();
            c.font = raiz.css;
            c.textBaseline = "alphabetic";
            const y = raiz.tam * 0.98;
            // sombra
            c.fillStyle = "rgba(0,0,0,0.55)";
            c.fillText(raiz.texto, 1, y + 3);
            // cromo
            const g = c.createLinearGradient(0, y - raiz.tam * 0.98, 0, y + raiz.tam * 0.27);   // como el alto de línea en CSS
            for (const p of Tema.cromo) g.addColorStop(p[0], p[1]);
            c.fillStyle = g;
            c.fillText(raiz.texto, 0, y);
        }
    }
    onTextoChanged: lienzo.requestPaint()
    onTamChanged: lienzo.requestPaint()
    Component.onCompleted: lienzo.requestPaint()
}
