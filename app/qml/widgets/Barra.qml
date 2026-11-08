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
            // los colores de la app (.w-barra)
            GradientStop { position: 0; color: "#d7263d" }
            GradientStop { position: 0.25; color: "#f5c518" }
            GradientStop { position: 0.45; color: "#2ecc71" }
            GradientStop { position: 0.55; color: "#2ecc71" }
            GradientStop { position: 0.75; color: "#f5c518" }
            GradientStop { position: 1; color: "#d7263d" }
        }
    }
    Rectangle {   // sombra del cursor (0 0 6px rgba(0,0,0,.6))
        width: cursor.width + 6; height: cursor.height + 6; radius: width / 2
        x: cursor.x - 3; y: cursor.y - 3; color: Qt.rgba(0, 0, 0, 0.3); opacity: cursor.opacity
    }
    Rectangle {
        id: cursor
        // Más gruesa y de un color que resalta en cada tema (Matías, 08/11/2026), con borde oscuro
        width: Math.max(8, raiz.height * 0.95); height: Math.max(20, raiz.height * 3.2); radius: width / 2
        y: (raiz.height - height) / 2
        x: raiz.pos * raiz.width - width / 2
        color: Tema.cursorBarra; border { width: 2; color: "#0b0d0c" }
        opacity: raiz.valor === null || raiz.valor === undefined ? 0.35 : 1
        // Sin animación: el dato llega 10 a 20 veces por segundo y la animación nunca terminaba (la pantalla se
        // redibujaba a 60 cuadros por segundo toda la carrera) (03-1.1)
    }
}
