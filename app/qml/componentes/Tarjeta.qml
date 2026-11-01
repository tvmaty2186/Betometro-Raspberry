import QtQuick
import Betometro

// Tarjeta verde translúcida con borde cromo doble (.card). `oscura`: .card.oscuro
Item {
    id: raiz
    property bool oscura: false
    property real radio: 18
    default property alias contenido: interior.data
    // Fondo verde translúcido, como la app: se ve el fondo a través
    Rectangle {
        anchors.fill: parent; radius: raiz.radio
        gradient: Gradient {
            GradientStop { position: 0; color: raiz.oscura ? "#1f4a2f" : Qt.rgba(64 / 255, 140 / 255, 90 / 255, 0.55) }
            GradientStop { position: 1; color: raiz.oscura ? "#08190e" : Qt.rgba(10 / 255, 38 / 255, 22 / 255, 0.92) }
        }
        // brillo de arriba (inset 0 1px 0 blanco)
        Rectangle { x: parent.radius; width: parent.width - 2 * parent.radius; height: 1; color: Qt.rgba(1, 1, 1, 0.35) }
    }
    // Borde cromo doble: 2 px claro y 1 px oscuro por fuera
    Rectangle { anchors.fill: parent; anchors.margins: -3; radius: raiz.radio + 3; color: "transparent"; border { width: 1; color: Tema.bordeCromoOscuro } }
    Rectangle { anchors.fill: parent; anchors.margins: -2; radius: raiz.radio + 2; color: "transparent"; border { width: 2; color: Tema.bordeCromo } }
    Item { id: interior; anchors.fill: parent }
}
