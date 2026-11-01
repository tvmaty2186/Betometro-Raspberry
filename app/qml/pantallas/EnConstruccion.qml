import QtQuick
import Betometro

Item {
    property string pantallaNombre: ""
    Fondo { anchors.fill: parent }
    TextoCromo { anchors.centerIn: parent; texto: pantallaNombre.toUpperCase(); tam: 44 }
    Text {
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 40 }
        text: "Pantalla en construcción · tocá para volver"
        color: Tema.tintaSuave; font { family: Tema.fuente; italic: true; pixelSize: 20 }
    }
    MouseArea { anchors.fill: parent; onClicked: Navegacion.volver() }
}
