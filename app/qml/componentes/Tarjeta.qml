import QtQuick
import Betometro

// Tarjeta verde translúcida con borde cromo doble (.card). `oscura`: .card.oscuro
Item {
    id: raiz
    property bool oscura: false
    // Gráficos, mapas y tablas de datos: con el tema propio, siempre el gris oscuro del Deportivo, para que se lean
    // con cualquier color elegido (Matías, 08/11/2026: «con el fondo rojo no contrastan bien los datos»)
    property bool datos: false
    readonly property bool fijo: datos && Tema.propio
    property real radio: Tema.radio
    default property alias contenido: interior.data
    // Fondo verde translúcido, como la app: se ve el fondo a través
    Rectangle {
        anchors.fill: parent; radius: raiz.radio
        gradient: Gradient {
            GradientStop { position: 0; color: raiz.fijo ? Tema.datos1 : raiz.oscura ? Tema.caja1 : Tema.tarjeta1 }
            GradientStop { position: 1; color: raiz.fijo ? Tema.datos2 : raiz.oscura ? Tema.caja2 : Tema.tarjeta2 }
        }
        // brillo de arriba (inset 0 1px 0 blanco)
        Rectangle { visible: Tema.avispon; x: parent.radius; width: parent.width - 2 * parent.radius; height: 1; color: Qt.rgba(1, 1, 1, 0.35) }
    }
    // Borde cromo doble: 2 px claro y 1 px oscuro por fuera
    Rectangle { anchors.fill: parent; anchors.margins: -3; radius: raiz.radio + 3; color: "transparent"; border { width: 1; color: Tema.bordeCromoOscuro } }
    Rectangle { anchors.fill: parent; anchors.margins: -2; radius: raiz.radio + 2; color: "transparent"; border { width: Tema.anchoBorde; color: Tema.bordeCromo } }
    Item { id: interior; anchors.fill: parent }
}
