import QtQuick
import Betometro

// Opciones en fila, una elegida en amarillo (.seg).
Item {
    id: raiz
    property var opciones: []      // [{texto, valor}]
    property var valor
    property bool activo: true
    signal elegido(var valor)
    implicitWidth: fila.implicitWidth
    implicitHeight: 40
    opacity: activo ? 1 : 0.5
    Rectangle { id: fondo; anchors.fill: parent; radius: 14; color: "#0c1f13"; clip: true
        Row {
            id: fila
            Repeater {
                model: raiz.opciones
                Rectangle {
                    required property var modelData
                    readonly property bool on: modelData.valor === raiz.valor
                    width: txt.implicitWidth + 36; height: raiz.height
                    radius: on ? 14 : 0
                    gradient: Gradient {
                        GradientStop { position: 0; color: on ? Tema.amarilloClaro : "#1c3325" }
                        GradientStop { position: 0.45; color: on ? Tema.amarillo : "#132a1c" }
                        GradientStop { position: 1; color: on ? Tema.amarillo2 : "#0c1f13" }
                    }
                    Text { id: txt; anchors.centerIn: parent; text: modelData.texto; color: on ? Tema.negro : Tema.blanco
                           font { family: Tema.fuente600; italic: true; weight: Font.Bold; pixelSize: 18 } }
                    MouseArea { anchors.fill: parent; enabled: raiz.activo; onClicked: raiz.elegido(modelData.valor) }
                }
            }
        }
    }
    Rectangle { anchors.fill: parent; anchors.margins: -2; radius: 16; color: "transparent"; border { width: 2; color: Tema.bordeCromo } }
}
