import QtQuick
import Betometro

// Etiqueta chica de estado (.chip-c): «● GPS 9 satélites».
Rectangle {
    id: raiz
    property string texto: ""
    property color punto: Tema.tinta
    property int tam: 16             // tamaño de la letra (el del inicio es más grande: 20)
    radius: height / 2
    color: Tema.sol ? "#ffffff" : Qt.rgba(0, 0, 0, 0.55)
    border { width: 1.5; color: Tema.bordeCromo }
    implicitHeight: Math.round(tam * 2)
    implicitWidth: fila.implicitWidth + Math.round(tam * 1.75)
    Row {
        id: fila
        anchors.centerIn: parent
        spacing: 6
        Text { text: "●"; color: raiz.punto; font { pixelSize: raiz.tam } anchors.verticalCenter: parent.verticalCenter }
        Text { text: raiz.texto; color: Tema.tinta; font { family: Tema.fuente600; italic: Tema.cursiva; weight: Font.Bold; pixelSize: raiz.tam } }
    }
}
