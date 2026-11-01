import QtQuick
import Betometro

// Etiqueta chica de estado (.chip-c): «● GPS 9 satélites».
Rectangle {
    id: raiz
    property string texto: ""
    property color punto: Tema.blanco
    radius: height / 2
    color: Qt.rgba(0, 0, 0, 0.55)
    border { width: 1.5; color: "#c9ced3" }
    implicitHeight: 32
    implicitWidth: fila.implicitWidth + 28
    Row {
        id: fila
        anchors.centerIn: parent
        spacing: 6
        Text { text: "●"; color: raiz.punto; font { pixelSize: 13 } anchors.verticalCenter: parent.verticalCenter }
        Text { text: raiz.texto; color: Tema.blanco; font { family: Tema.fuente; italic: true; weight: Font.Bold; pixelSize: 13 } }
    }
}
