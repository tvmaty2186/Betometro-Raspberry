import QtQuick
import Betometro

// Al lado del video del informe: Mapa | Gráfico (Matías, 08/11/2026: «poder conmutar entre el mapa y los gráficos»)
Row {
    id: fila
    property string lado: "mapa"     // mapa, grafico
    signal elegir(string l)
    spacing: 0
    Repeater {
        model: [{ t: "Mapa", l: "mapa" }, { t: "Gráfico", l: "grafico" }]
        Rectangle {
            required property var modelData
            required property int index
            readonly property bool on: fila.lado === modelData.l
            width: tl.implicitWidth + 24; height: 28; radius: 14
            color: on ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
            border { width: 1; color: Tema.bordeCromo }
            Text { id: tl; anchors.centerIn: parent; text: modelData.t; color: parent.on ? Tema.acentoTinta : Tema.tinta
                   font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 15 } }
            MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: fila.elegir(parent.modelData.l) }
        }
    }
}
