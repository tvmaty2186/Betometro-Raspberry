import QtQuick
import Betometro

// Elegir el color de la vuelta (Matías, 09/11/2026): 5 círculos, el elegido con aro y ✓.
// Rojo y verde son los de fábrica (medida y libre).
Row {
    id: raiz
    property string valor
    signal elegido(string v)
    readonly property var colores: [
        { c: "#b3132b", n: "Rojo" }, { c: "#0f6e3a", n: "Verde" }, { c: "#1f4fa8", n: "Azul" },
        { c: "#d4a600", n: "Amarillo" }, { c: "#d2600c", n: "Naranja" }
    ]
    spacing: 12
    Repeater {
        model: raiz.colores
        Rectangle {
            required property var modelData
            readonly property bool sel: String(raiz.valor).toLowerCase() === modelData.c
            width: 62; height: 62; radius: 31
            color: modelData.c
            border { width: sel ? 5 : 2; color: sel ? Tema.tinta : Qt.rgba(1, 1, 1, 0.35) }
            Text { anchors.centerIn: parent; visible: parent.sel; text: "✓"; color: "#ffffff"
                   font { family: Tema.fuente800; weight: Font.ExtraBold; pixelSize: 30 } }
            MouseArea { anchors.fill: parent; anchors.margins: -4; onClicked: raiz.elegido(parent.modelData.c) }
        }
    }
}
