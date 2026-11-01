import QtQuick
import Betometro

// Redondel cromado con día y mes (.fecha).
Item {
    id: raiz
    property string dia: ""
    property string mes: ""
    width: 62; height: 62
    Rectangle { anchors.fill: parent; anchors.margins: -4; radius: width / 2; color: "#4b5258" }
    Rectangle { anchors.fill: parent; anchors.margins: -2; radius: width / 2; color: "#f5f6f7" }
    Rectangle {
        anchors.fill: parent; radius: width / 2
        gradient: Gradient {
            GradientStop { position: 0; color: "#ffffff" }
            GradientStop { position: 0.45; color: "#d8dde1" }
            GradientStop { position: 0.8; color: "#9aa2a9" }
            GradientStop { position: 1; color: "#e3e6e9" }
        }
    }
    Column {
        anchors.centerIn: parent
        spacing: -4
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: raiz.dia; color: Tema.numeroVerde
               font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 26 } }
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: raiz.mes; color: Tema.numeroVerde
               font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 12 } }
    }
}
