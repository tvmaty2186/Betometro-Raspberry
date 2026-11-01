import QtQuick
import Betometro

// Botón oscuro con borde cromo (.btn-cromo): Informe, Guardar, Borrar, Olvidar.
Item {
    id: raiz
    property string texto: ""
    property real tamTexto: 16
    property real radio: 14
    property bool activo: true
    signal tocado()
    opacity: activo ? 1 : 0.45
    Rectangle { anchors.fill: parent; anchors.margins: -3; radius: raiz.radio + 3; color: Tema.bordeCromoOscuro }
    Rectangle { anchors.fill: parent; anchors.margins: -2; radius: raiz.radio + 2; color: Tema.bordeCromo }
    Rectangle {
        anchors.fill: parent; radius: raiz.radio
        gradient: Gradient {
            GradientStop { position: 0; color: zona.pressed ? "#3a5a45" : "#2f4a39" }
            GradientStop { position: 1; color: "#0c1f13" }
        }
    }
    Text {
        anchors.centerIn: parent
        text: raiz.texto
        color: Tema.blanco
        font { family: Tema.fuente600; italic: true; weight: Font.Bold; pixelSize: raiz.tamTexto }
    }
    MouseArea { id: zona; anchors.fill: parent; enabled: raiz.activo; onClicked: raiz.tocado() }
}
