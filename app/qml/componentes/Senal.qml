import QtQuick
import Betometro

// Rayitas de señal del wifi (0 a 4), de menor a mayor, como en los celulares.
// Cada rayita con su posición fija (antes estaban en un Row con anchors y quedaban una encima de otra).
Item {
    id: raiz
    property int nivel: 0
    property real grosor: 6
    property real paso: 5            // cuánto crece cada rayita
    width: 4 * grosor + 3 * 3
    height: grosor + 3 * paso
    Repeater {
        model: 4
        Rectangle {
            required property int index
            x: index * (raiz.grosor + 3)
            width: raiz.grosor; height: raiz.grosor + index * raiz.paso; radius: 2
            y: raiz.height - height
            color: index < raiz.nivel ? Tema.tinta : Qt.rgba(1, 1, 1, 0.25)
        }
    }
}
