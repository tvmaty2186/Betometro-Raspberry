import QtQuick
import Betometro

// Botón redondo del inicio: disco verde con triple aro cromado (.disco de la app).
Item {
    id: raiz
    property real d: 96
    property string icono: ""
    property string texto: ""
    property bool etiquetaIzquierda: true   // horizontal: a la izquierda; vertical: abajo
    signal tocado()
    width: d; height: d

    Canvas {
        id: lienzo
        x: -12; y: -12; width: raiz.d + 24; height: raiz.d + 34
        onPaint: {
            const c = getContext("2d"), r = raiz.d / 2, cx = 12 + r, cy = 12 + r;
            c.reset();
            // sombra 0 8px 14px
            for (let i = 0; i < 6; i++) {
                c.beginPath(); c.arc(cx, cy + 8, r + 7 - i * 1.2 + 6, 0, Math.PI * 2);
                c.fillStyle = "rgba(0,0,0,0.07)"; c.fill();
            }
            const aro = (rr, col) => { c.beginPath(); c.arc(cx, cy, rr, 0, Math.PI * 2); c.fillStyle = col; c.fill(); };
            aro(r + 7, "#b9bfc5"); aro(r + 5, "#5d646b"); aro(r + 4, "#e9ecee");
            const g = c.createRadialGradient(cx - r * 0.1, cy - r * 0.4, 0, cx - r * 0.1, cy - r * 0.4, r * 1.25);
            g.addColorStop(0, "#3c8a57"); g.addColorStop(0.55, "#14432a"); g.addColorStop(1, "#0a2414");
            aro(r, g);
            // brillo arriba y sombra adentro abajo
            const b = c.createLinearGradient(0, cy - r, 0, cy + r);
            b.addColorStop(0, "rgba(255,255,255,0.18)"); b.addColorStop(0.3, "rgba(255,255,255,0)");
            b.addColorStop(0.75, "rgba(0,0,0,0)"); b.addColorStop(1, "rgba(0,0,0,0.45)");
            aro(r, b);
        }
        Component.onCompleted: requestPaint()
    }
    Image {
        anchors.centerIn: parent
        width: raiz.d * 0.46; height: width
        sourceSize: Qt.size(width * 2, height * 2)
        source: raiz.icono ? "qrc:/qt/qml/Betometro/iconos/" + raiz.icono + ".svg" : ""
    }
    Text {
        text: raiz.texto
        color: Tema.blanco
        font { family: Tema.fuente; italic: true; weight: Font.Bold; pixelSize: 16 * raiz.d / 96 }
        style: Text.Raised; styleColor: "#000"
        horizontalAlignment: raiz.etiquetaIzquierda ? Text.AlignRight : Text.AlignHCenter
        width: 200
        x: raiz.etiquetaIzquierda ? -width - 12 : (raiz.d - width) / 2
        y: raiz.etiquetaIzquierda ? (raiz.d - height) / 2 : raiz.d + 8
    }
    MouseArea { anchors.fill: parent; anchors.margins: -8; onClicked: raiz.tocado() }
}
