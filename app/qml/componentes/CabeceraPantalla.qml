import QtQuick
import Betometro

// Arriba de cada pantalla: Volver, título cromado y rayas (.cab + .rayasT).
Item {
    id: raiz
    property string titulo: ""
    property real derechaRayas: Contexto.horizontal ? 90 : 150
    signal volver()
    anchors.fill: parent
    Rayas { x: parent.width - raiz.derechaRayas - width; y: -30 }
    Row {
        x: Contexto.horizontal ? 28 : 24; y: 24
        spacing: 14
        Volver { anchors.verticalCenter: parent.verticalCenter; onTocado: raiz.volver() }
        TextoCromo { anchors.verticalCenter: parent.verticalCenter; texto: raiz.titulo; tam: 40 }
    }
}
