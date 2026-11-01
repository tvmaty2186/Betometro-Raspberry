import QtQuick
import Betometro

// Separador de sección: PRÓXIMAS / CORRIDAS con banda (.bandaSec).
Item {
    property string texto: ""
    width: parent ? parent.width : 400
    height: 44
    Text {
        id: t
        anchors.verticalCenter: parent.verticalCenter
        text: parent.texto; color: Tema.blanco
        font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 18; letterSpacing: 18 * 0.14 }
    }
    Banda { anchors { left: t.right; leftMargin: 12; right: parent.right; verticalCenter: parent.verticalCenter } height: 12 }
}
