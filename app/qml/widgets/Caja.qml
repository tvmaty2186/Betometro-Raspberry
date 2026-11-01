import QtQuick
import Betometro

// Fondo de un widget del Avispón: verde oscuro con doble anillo cromo (.w del tema).
// `redondo`: círculo.
Item {
    id: raiz
    property bool redondo: false
    property real radio: redondo ? width / 2 : Tema.radio + 4
    Rectangle { anchors.fill: parent; anchors.topMargin: 10; anchors.bottomMargin: -10; radius: raiz.radio; color: Qt.rgba(0, 0, 0, 0.35) }
    Rectangle { anchors.fill: parent; anchors.margins: -3; radius: raiz.radio + 3; color: Tema.bordeCromoOscuro; visible: !Tema.sol }
    Rectangle { anchors.fill: parent; anchors.margins: -Tema.anchoBorde; radius: raiz.radio + 2; color: Tema.bordeCromo }
    Rectangle {
        anchors.fill: parent; radius: raiz.radio
        gradient: Gradient {
            GradientStop { position: 0; color: Tema.caja1 }
            GradientStop { position: 1; color: Tema.caja2 }
        }
        Rectangle { visible: Tema.avispon; x: parent.radius * 0.7; width: parent.width - 1.4 * parent.radius; height: 1; color: Qt.rgba(1, 1, 1, 0.3) }
    }
}
