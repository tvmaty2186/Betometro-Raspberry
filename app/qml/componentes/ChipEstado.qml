import QtQuick
import Betometro

// Chip de estado de una carrera: ERROR 9,6 s / HOY / EN 14 DÍAS (color según nivel).
Rectangle {
    property string texto: ""
    property string nivel: "neutro"   // ok, medio, mal, hoy, neutro
    readonly property color tinta: nivel === "ok" ? Tema.bien : nivel === "medio" ? Tema.acento : nivel === "mal" ? Tema.alerta : nivel === "hoy" ? Tema.acento : Tema.tintaSuave
    radius: height / 2
    color: Qt.rgba(0, 0, 0, 0.45)
    border { width: 1.5; color: tinta }
    implicitWidth: t.implicitWidth + 22; implicitHeight: t.implicitHeight + 8
    Text { id: t; anchors.centerIn: parent; text: parent.texto; color: parent.tinta
           font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 13; letterSpacing: 13 * 0.06 } }
}
