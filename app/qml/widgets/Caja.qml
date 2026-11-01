import QtQuick
import Betometro

// Fondo de un widget del Avispón: verde oscuro con doble anillo cromo (.w del tema).
// `redondo`: círculo.
Item {
    id: raiz
    property bool redondo: false
    property real radio: redondo ? width / 2 : 22
    Rectangle { anchors.fill: parent; anchors.topMargin: 10; anchors.bottomMargin: -10; radius: raiz.radio; color: Qt.rgba(0, 0, 0, 0.35) }
    Rectangle { anchors.fill: parent; anchors.margins: -3; radius: raiz.radio + 3; color: Tema.bordeCromoOscuro }
    Rectangle { anchors.fill: parent; anchors.margins: -2; radius: raiz.radio + 2; color: Tema.bordeCromo }
    Rectangle {
        anchors.fill: parent; radius: raiz.radio
        gradient: Gradient {
            GradientStop { position: 0; color: "#1f4a2f" }
            GradientStop { position: 0.7; color: "#0c2215" }
            GradientStop { position: 1; color: "#06140b" }
        }
        Rectangle { x: parent.radius * 0.7; width: parent.width - 1.4 * parent.radius; height: 1; color: Qt.rgba(1, 1, 1, 0.3) }
    }
}
