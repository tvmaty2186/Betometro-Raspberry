import QtQuick
import Betometro

// Botón redondo «‹» para volver (.volver).
Rectangle {
    id: raiz
    signal tocado()
    width: 42; height: 42; radius: 21
    color: zona.pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.4)
    border { width: 2; color: Tema.bordeCromo }
    Text { anchors.centerIn: parent; anchors.verticalCenterOffset: -2; text: "‹"; color: Tema.tinta; font { family: Tema.fuente600; weight: Font.Bold; pixelSize: 26 } }
    MouseArea { id: zona; anchors.fill: parent; anchors.margins: -14; onClicked: raiz.tocado() }
}
