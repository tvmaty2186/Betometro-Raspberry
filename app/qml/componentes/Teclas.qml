import QtQuick
import Betometro

// Las teclas del teclado en pantalla (se usan en el teclado que se abre encima y en la pantalla de la clave del wifi).
Column {
    id: raiz
    property var filas: []
    property bool mayuscula: false
    property bool grande: false     // números: teclas con letra más grande
    signal tecla(string k)
    spacing: 10
    Repeater {
        model: raiz.filas
        Row {
            id: filaT
            required property string modelData
            readonly property var teclas: modelData.indexOf(" ") >= 0 ? modelData.split(" ") : Array.from(modelData)
            readonly property real unidad: (raiz.width - (teclas.length - 1) * 10) / (teclas.length + (modelData.indexOf("espacio") >= 0 ? 4 : 0))
            width: raiz.width
            height: (raiz.height - (raiz.filas.length - 1) * 10) / raiz.filas.length
            spacing: 10
            Repeater {
                model: filaT.teclas
                Rectangle {
                    required property string modelData
                    readonly property bool especial: modelData === "✓"
                    width: modelData === "espacio" ? filaT.unidad * 5 : filaT.unidad
                    height: filaT.height
                    radius: 12
                    gradient: Gradient {
                        GradientStop { position: 0; color: especial ? Tema.acentoClaro : (zona.pressed ? Qt.lighter(Tema.caja1, 1.6) : Qt.lighter(Tema.caja1, 1.25)) }
                        GradientStop { position: 1; color: especial ? Tema.acentoOscuro : Tema.caja2 }
                    }
                    border { width: Tema.sol ? 3 : 2; color: Tema.bordeCromo }
                    Text {
                        anchors.centerIn: parent
                        text: modelData === "espacio" ? "espacio" : (raiz.mayuscula && modelData.length === 1 ? modelData.toUpperCase() : modelData)
                        color: especial ? Tema.acentoTinta : Tema.tinta
                        font { family: Tema.fuente600; pixelSize: raiz.grande ? 34 : 24 }
                    }
                    MouseArea { id: zona; anchors.fill: parent; onClicked: raiz.tecla(modelData) }
                }
            }
        }
    }
}
