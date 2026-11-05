import QtQuick
import Betometro

// Opciones en botones grandes (una sola elegida). opciones: [{texto, valor}]
Rectangle {
    id: raiz
    property var opciones: []
    property var valor
    signal elegido(var v)
    height: 66; radius: 20
    width: fila.width + 6
    color: Qt.rgba(0, 0, 0, 0.4)
    border { width: 3; color: Tema.bordeCromo }
    clip: true
    Row {
        id: fila; x: 3; y: 3
        Repeater {
            model: raiz.opciones
            Rectangle {
                required property var modelData
                readonly property bool sel: raiz.valor === modelData.valor
                width: Math.max(96, t.implicitWidth + 40); height: 60; radius: 17
                color: sel ? Tema.acento : "transparent"
                Text { id: t; anchors.centerIn: parent; text: parent.modelData.texto
                       color: parent.sel ? Tema.acentoTinta : Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 25 } }
                MouseArea { anchors.fill: parent; onClicked: raiz.elegido(parent.modelData.valor) }
            }
        }
    }
}
