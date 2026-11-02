import QtQuick
import Betometro

// Llave de prender y apagar (.toggle).
Rectangle {
    id: raiz
    property bool prendida: false
    signal cambiada(bool valor)
    width: 58; height: 32; radius: 16
    color: prendida ? Tema.acento : Qt.lighter(Tema.caja1, 1.2)
    border { width: 1; color: Qt.rgba(0, 0, 0, 0.3) }
    Rectangle {
        width: 26; height: 26; radius: 13; y: 3
        x: raiz.prendida ? raiz.width - width - 3 : 3
        gradient: Gradient { GradientStop { position: 0; color: "#ffffff" } GradientStop { position: 0.5; color: "#9aa1a8" } GradientStop { position: 1; color: "#e8ebee" } }
        Behavior on x { NumberAnimation { duration: 120 } }
    }
    MouseArea { anchors.fill: parent; anchors.margins: -10; onClicked: raiz.cambiada(!raiz.prendida) }
}
