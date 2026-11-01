import QtQuick
import Betometro

// Renglón de una tarjeta: etiqueta a la izquierda y control a la derecha (.fila).
Item {
    id: raiz
    property string etiqueta: ""
    property bool ultima: false
    property color colorEtiqueta: Tema.blanco
    default property alias derecha: lado.data
    width: parent ? parent.width : 400
    implicitHeight: Math.max(74, lado.childrenRect.height + 28)
    Text {
        anchors { left: parent.left; leftMargin: 22; verticalCenter: parent.verticalCenter }
        text: raiz.etiqueta; color: raiz.colorEtiqueta
        font { family: Tema.fuente600; weight: Font.DemiBold; pixelSize: 20 }
    }
    Item {
        id: lado
        anchors { right: parent.right; rightMargin: 22; verticalCenter: parent.verticalCenter }
        width: childrenRect.width; height: childrenRect.height
    }
    Rectangle { visible: !raiz.ultima; anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 1; color: Qt.rgba(1, 1, 1, 0.12) }
}
