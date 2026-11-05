import QtQuick
import Betometro

// Interruptor grande (sí / no)
Rectangle {
    id: raiz
    property bool prendida: false
    signal cambiada(bool v)
    width: 124; height: 66; radius: 33
    color: prendida ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
    border { width: 3; color: Tema.bordeCromo }
    Rectangle {
        width: 50; height: 50; radius: 25; y: 8
        x: raiz.prendida ? raiz.width - width - 8 : 8
        Behavior on x { NumberAnimation { duration: 120 } }
        gradient: Gradient { GradientStop { position: 0; color: "#ffffff" } GradientStop { position: 1; color: "#b9bfc5" } }
    }
    MouseArea { anchors.fill: parent; anchors.margins: -8; onClicked: raiz.cambiada(!raiz.prendida) }
}
