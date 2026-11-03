import QtQuick
import Betometro

// Etiqueta de vuelta medida / práctica (.tag).
Rectangle {
    property string texto: ""
    property bool medida: true
    property real tam: 11
    radius: height / 2
    color: medida ? Tema.acento : Tema.numero
    implicitWidth: t.implicitWidth + tam * 1.8; implicitHeight: t.implicitHeight + tam * 0.55
    Text { id: t; anchors.centerIn: parent; text: parent.texto; color: parent.medida ? "#000" : Tema.verde2
           font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: tam; letterSpacing: tam * 0.08 } }
}
