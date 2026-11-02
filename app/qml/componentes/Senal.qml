import QtQuick
import Betometro

// Rayitas de señal del wifi (0 a 4).
Row {
    property int nivel: 0
    spacing: 3
    Repeater {
        model: 4
        Rectangle {
            required property int index
            width: 6; height: 6 + index * 5; radius: 2
            anchors.bottom: parent.bottom
            color: index < parent.nivel ? Tema.tinta : Qt.rgba(1, 1, 1, 0.25)
        }
    }
}
