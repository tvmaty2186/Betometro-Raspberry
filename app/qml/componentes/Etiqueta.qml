import QtQuick
import Betometro

// Etiqueta de vuelta medida / práctica (.tag).
Rectangle {
    property string texto: ""
    property bool medida: true
    radius: 10
    color: medida ? Tema.acento : Tema.numero
    implicitWidth: t.implicitWidth + 20; implicitHeight: t.implicitHeight + 6
    Text { id: t; anchors.centerIn: parent; text: parent.texto; color: parent.medida ? "#000" : Tema.verde2
           font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 11; letterSpacing: 11 * 0.08 } }
}
