import QtQuick
import Betometro

// Renglón de una tarjeta: etiqueta (o algo propio) a la izquierda y control a la derecha (.fila).
Item {
    id: raiz
    property string etiqueta: ""
    property bool ultima: false
    property bool resaltada: false   // fondo amarillito (la red conectada o elegida)
    property bool tocable: false     // toda la fila responde al toque
    property color colorEtiqueta: Tema.blanco
    property real altoMinimo: 74
    default property alias derecha: lado.data
    property alias izquierda: ladoIzq.data
    signal tocada()
    width: parent ? parent.width : 400
    implicitHeight: Math.max(altoMinimo, lado.childrenRect.height + 28)
    Rectangle { anchors.fill: parent; visible: raiz.resaltada; color: Qt.rgba(244 / 255, 197 / 255, 49 / 255, 0.18) }
    MouseArea { anchors.fill: parent; enabled: raiz.tocable; onClicked: raiz.tocada() }
    Text {
        visible: raiz.etiqueta !== ""
        anchors { left: parent.left; leftMargin: 22; verticalCenter: parent.verticalCenter }
        text: raiz.etiqueta; color: raiz.colorEtiqueta
        font { family: Tema.fuente600; pixelSize: 20 }
    }
    Item {
        id: ladoIzq
        anchors { left: parent.left; leftMargin: 22; verticalCenter: parent.verticalCenter }
        width: childrenRect.width; height: childrenRect.height
    }
    Item {
        id: lado
        anchors { right: parent.right; rightMargin: 22; verticalCenter: parent.verticalCenter }
        width: childrenRect.width; height: childrenRect.height
    }
    Rectangle { visible: !raiz.ultima; anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 1; color: Qt.rgba(1, 1, 1, 0.12) }
}
