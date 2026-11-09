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
    // Cursor (Matías, 08/11/2026): blanco con borde negro, más grueso. Dos formas para elegir:
    // «raya» (la de siempre, vertical) o «flecha» (triangulito encima de la barra, centrado, apuntando hacia arriba)
    readonly property bool flecha: Datos.ajustes.barraCursor !== "raya"   // de fábrica, la flecha (Matías, 08/11/2026)
    // Siempre blanco intenso (Matías, 09/11/2026): antes, sin dato, iba al 35 % y se veía verde sobre la barra
    readonly property real opCursor: 1
    Rectangle {   // sombra del cursor (0 0 6px rgba(0,0,0,.6))
        visible: !raiz.flecha
        width: cursor.width + 6; height: cursor.height + 6; radius: width / 2
        x: cursor.x - 3; y: cursor.y - 3; color: Qt.rgba(0, 0, 0, 0.3); opacity: cursor.opacity
    }
    Rectangle {
        id: cursor
        visible: !raiz.flecha
        width: Math.max(8, raiz.height * 0.95); height: Math.max(20, raiz.height * 3.2); radius: width / 2
        y: (raiz.height - height) / 2
        x: raiz.pos * raiz.width - width / 2
        color: "#ffffff"; border { width: 4; color: "#000000" }
        opacity: raiz.opCursor
        // Sin animación: el dato llega 10 a 20 veces por segundo y la animación nunca terminaba (la pantalla se
        // redibujaba a 60 cuadros por segundo toda la carrera) (03-1.1)
    }
    Canvas {   // la flecha: se dibuja una vez; al moverse solo cambia x
        id: triangulo
        visible: raiz.flecha
        width: Math.max(38, raiz.height * 3.4); height: Math.max(30, raiz.height * 2.5)
        x: raiz.pos * raiz.width - width / 2
        y: (raiz.height - height) / 2   // centrado sobre la barra (Matías, 08/11/2026)
        opacity: raiz.opCursor
        onWidthChanged: requestPaint(); onHeightChanged: requestPaint(); onVisibleChanged: if (visible) requestPaint()
        Connections { target: Tema; function onSolChanged() { triangulo.requestPaint(); } }
        onPaint: {
            const c = getContext("2d"); c.reset();
            const b = Math.max(4, height * 0.13);   // borde negro grueso, tipo negrita (Matías, 09/11/2026)
            c.beginPath(); c.moveTo(width / 2, b); c.lineTo(width - b, height - b); c.lineTo(b, height - b); c.closePath();
            // Blanca con borde negro; en modo sol, negra con borde blanco (Matías, 08/11/2026)
            c.fillStyle = Tema.sol ? "#000000" : "#ffffff"; c.fill();
            c.lineWidth = b; c.lineJoin = "round"; c.strokeStyle = Tema.sol ? "#ffffff" : "#000000"; c.stroke();
        }
    }
}
