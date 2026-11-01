import QtQuick
import Betometro

// Título de una tarjeta con la banderita blanca y amarilla (.h3).
Row {
    property string texto: ""
    leftPadding: 22; topPadding: 16; bottomPadding: 6
    spacing: 12
    Banda { width: 30; height: 14; anchors.verticalCenter: parent.verticalCenter }
    Text { text: parent.texto; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 24 } }
}
