import QtQuick
import Betometro

// Botones Vel. · Acel. · Adel. · Comp. del informe (el mapa y el gráfico al lado del video los comparten)
Row {
    id: fila
    property string modo: "vel"
    property real dist: 0            // sin distancia no hay adelanto
    property bool conComp: false     // hay vuelta modelo (y no es recorrido libre)
    signal elegirModo(string m)
    spacing: 6
    Repeater {
        model: [{ t: "Vel.", m: "vel" }, { t: "Acel.", m: "acel" }].concat(fila.dist > 0 ? [{ t: "Adel.", m: "ade" }] : [])
               .concat(fila.conComp ? [{ t: "Comp.", m: "comp" }] : [])
        Rectangle {
            required property var modelData
            width: tm.implicitWidth + 18; height: 26; radius: 13
            color: fila.modo === modelData.m ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
            border { width: 1; color: Tema.bordeCromo }
            Text { id: tm; anchors.centerIn: parent; text: modelData.t; color: fila.modo === modelData.m ? Tema.acentoTinta : Tema.tinta
                   font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 16 } }
            MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: fila.elegirModo(modelData.m) }
        }
    }
}
