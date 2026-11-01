import QtQuick
import Betometro

// Valor que se toca para cambiar (.campo).
Rectangle {
    id: raiz
    property string texto: ""
    property real anchoMinimo: 96
    property bool activo: true
    property bool editando: false
    signal tocado()
    implicitWidth: Math.max(anchoMinimo, t.implicitWidth + 32)
    implicitHeight: t.implicitHeight + 14
    radius: 14
    color: editando ? Qt.rgba(244 / 255, 197 / 255, 49 / 255, 0.25) : Qt.rgba(0, 0, 0, 0.35)
    border { width: 2; color: editando ? Tema.acento : Tema.bordeCromo }
    opacity: activo ? 1 : 0.5
    Text {
        id: t
        anchors.centerIn: parent
        text: raiz.texto
        color: Tema.tinta
        font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 26 }
    }
    MouseArea { anchors.fill: parent; anchors.margins: -6; enabled: raiz.activo; onClicked: raiz.tocado() }
}
