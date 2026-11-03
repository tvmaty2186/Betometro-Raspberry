import QtQuick
import Betometro

// Fondo de un widget del Avispón: verde oscuro con doble anillo cromo (.w del tema).
// `redondo`: círculo.
Item {
    id: raiz
    property bool redondo: false
    property real radio: redondo ? width / 2 : Tema.radio + 4
    // Brillo de Rally, Deportivo y Cabina: capas cada vez más anchas y suaves (20 px)
    Repeater {
        model: Tema.glow.a > 0 ? 5 : 0
        Rectangle {
            required property int index
            readonly property real m: 3 + index * 4
            anchors.fill: parent; anchors.margins: -m
            radius: raiz.radio + m; color: "transparent"
            border { width: 4; color: Qt.rgba(Tema.glow.r, Tema.glow.g, Tema.glow.b, Tema.glow.a * [0.55, 0.35, 0.2, 0.1, 0.05][index]) }
        }
    }
    // Sombra difuminada abajo (0 10px 18px rgba(0,0,0,.5) de la app): capas cada vez más anchas y suaves
    Repeater {
        model: Tema.sol ? 0 : 4
        Rectangle {
            required property int index
            anchors.fill: parent; anchors.topMargin: 10 - index * 3; anchors.bottomMargin: -10 - index * 3
            anchors.leftMargin: -index * 3; anchors.rightMargin: -index * 3
            radius: raiz.radio + index * 3; color: Qt.rgba(0, 0, 0, [0.22, 0.12, 0.07, 0.04][index])
        }
    }
    Rectangle { anchors.fill: parent; anchors.margins: -3; radius: raiz.radio + 3; color: Tema.bordeCromoOscuro; visible: !Tema.sol }
    Rectangle { anchors.fill: parent; anchors.margins: -Tema.anchoBorde; radius: raiz.radio + 2; color: Tema.bordeCromo }
    Rectangle {
        visible: !Tema.avispon
        anchors.fill: parent; radius: raiz.radio
        gradient: Gradient {
            GradientStop { position: 0; color: Tema.caja1 }
            GradientStop { position: 1; color: Tema.caja2 }
        }
    }
    // Avispón (app, línea 651): cara de instrumento. Fondo radial desde arriba, luz en el borde de arriba,
    // sombra adentro abajo y reflejo con forma de lente arriba. Se pinta una sola vez (no cambia en carrera).
    Canvas {
        id: cara
        visible: Tema.avispon
        anchors.fill: parent
        onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
        onPaint: {
            const g = getContext("2d"); g.reset();
            const r = Math.min(raiz.radio, width / 2, height / 2);
            g.beginPath(); g.roundedRect(0, 0, width, height, r, r); g.closePath();
            const fondo = g.createRadialGradient(width / 2, 0, 0, width / 2, 0, Math.max(width * 0.6, height * 0.9));
            fondo.addColorStop(0, "#1f4a2f"); fondo.addColorStop(1, "#08190e");
            g.fillStyle = fondo; g.fill();
            const sombra = g.createLinearGradient(0, height * 0.55, 0, height);   // inset 0 -18px 30px rgba(0,0,0,.45)
            sombra.addColorStop(0, "rgba(0,0,0,0)"); sombra.addColorStop(1, "rgba(0,0,0,0.45)");
            g.fillStyle = sombra; g.fill();
        }
    }
    Rectangle {   // luz fina en el borde de arriba (inset 0 1px 0 rgba(255,255,255,.3))
        visible: Tema.avispon; x: raiz.radio * 0.7; width: parent.width - 1.4 * raiz.radio; height: 1; color: Qt.rgba(1, 1, 1, 0.3)
    }
    Canvas {   // reflejo (::after de la app): elipse de 8 % a 92 % del ancho, arriba, 36 % del alto, blanco .12 → 0
        visible: Tema.avispon
        x: parent.width * (raiz.redondo ? 0.18 : 0.08); y: parent.height * 0.03
        width: parent.width * (raiz.redondo ? 0.64 : 0.84); height: parent.height * 0.36
        onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
        onPaint: {
            const g = getContext("2d"); g.reset();
            const brillo = g.createLinearGradient(0, 0, 0, height);
            brillo.addColorStop(0, "rgba(255,255,255,0.12)"); brillo.addColorStop(1, "rgba(255,255,255,0)");
            g.fillStyle = brillo;
            g.beginPath(); g.ellipse(0, 0, width, height * 1.2); g.fill();   // la parte de abajo de la elipse queda afuera
        }
    }

}
