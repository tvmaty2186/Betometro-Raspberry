import QtQuick
import Betometro

// Botón amarillo con letras negras (.btn-ama): LARGAR, MARCAR, + Nueva.
Item {
    id: raiz
    property string texto: ""
    property string subtexto: ""   // línea chica debajo (por ejemplo, por qué no se puede largar)
    property real tamTexto: 22
    property bool activo: true
    property real radio: 16
    signal tocado()
    opacity: activo ? 1 : 0.45
    Rectangle { anchors.fill: parent; anchors.topMargin: 8; anchors.bottomMargin: -8; radius: raiz.radio; color: Qt.rgba(0, 0, 0, 0.4) }
    Rectangle { anchors.fill: parent; anchors.margins: -3; radius: raiz.radio + 3; color: "#6b5200" }
    Rectangle { anchors.fill: parent; anchors.margins: -2; radius: raiz.radio + 2; color: "#fff6d0" }
    Rectangle {
        anchors.fill: parent; radius: raiz.radio
        gradient: Gradient {
            GradientStop { position: 0; color: zona.pressed ? Tema.amarillo2 : Tema.amarilloClaro }
            GradientStop { position: 0.45; color: Tema.amarillo }
            GradientStop { position: 1; color: Tema.amarillo2 }
        }
    }
    Text {
        visible: raiz.subtexto !== ""
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 14 }
        text: raiz.subtexto; color: Tema.negro
        font { family: Tema.fuente600; italic: true; pixelSize: 17; letterSpacing: 17 * 0.08; capitalization: Font.AllUppercase }
    }
    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: raiz.subtexto !== "" ? -10 : 0
        width: parent.width - 16
        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.HorizontalFit; minimumPixelSize: 12
        text: raiz.texto
        color: Tema.negro
        font { family: Tema.fuente800; italic: true; weight: Font.Black; pixelSize: raiz.tamTexto; letterSpacing: raiz.tamTexto * 0.04 }
    }
    MouseArea { id: zona; anchors.fill: parent; enabled: raiz.activo; onClicked: raiz.tocado() }
}
