import QtQuick
import Betometro

// Barra de colores con cursor (6.4): `valor` entre −rango y +rango; derecha = adelantado / más rápido.
Item {
    id: raiz
    property var valor: null        // null = sin dato
    property real rango: 1
    readonly property real pos: valor === null || valor === undefined ? 0.5 : 0.5 + Math.max(-rango, Math.min(rango, valor)) / rango * 0.5
    height: 12
    Rectangle {
        anchors.fill: parent; radius: height / 2
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "#d23b2c" }
            GradientStop { position: 0.25; color: Tema.amarillo }
            GradientStop { position: 0.45; color: "#7fd36b" }
            GradientStop { position: 0.55; color: "#7fd36b" }
            GradientStop { position: 0.75; color: Tema.amarillo }
            GradientStop { position: 1; color: "#d23b2c" }
        }
    }
    Rectangle {
        width: 7; height: raiz.height * 2.2; radius: 3
        y: (raiz.height - height) / 2
        x: raiz.pos * raiz.width - width / 2
        color: "#ffffff"; border { width: 1.5; color: "#000" }
        opacity: raiz.valor === null || raiz.valor === undefined ? 0.35 : 1
        Behavior on x { NumberAnimation { duration: 250 } }
    }
}
