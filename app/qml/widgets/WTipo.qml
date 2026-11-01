import QtQuick
import Betometro

// Vuelta y tipo: redondel V3 / MEDIDA (amarillo) o PRÁCTICA (crema). Antes de largar: 10 VUELTAS / LISTO.
Item {
    id: raiz
    property var est: ({})
    readonly property bool enCurso: !!est.enCurso
    readonly property bool medida: est.medida !== false
    readonly property int total: est.vueltasTotal || 0
    Item {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height); height: width
        Rectangle { anchors.fill: parent; anchors.margins: -5; radius: width / 2; color: "#4b5258" }
        Rectangle { anchors.fill: parent; anchors.margins: -3; radius: width / 2; color: "#f5f6f7" }
        Rectangle {
            anchors.fill: parent; radius: width / 2
            gradient: Gradient {
                GradientStop { position: 0; color: raiz.medida ? "#ffe9a0" : "#fffbef" }
                GradientStop { position: 0.5; color: raiz.medida ? Tema.amarillo : Tema.crema }
                GradientStop { position: 1; color: raiz.medida ? Tema.amarillo2 : "#d6caa9" }
            }
        }
        Column {
            anchors.centerIn: parent
            width: parent.width * 0.8
            Numero {
                width: parent.width; height: parent.parent.height * 0.42
                color: Tema.numeroVerde; style: Text.Normal
                maximo: parent.parent.height * 0.43
                text: raiz.enCurso ? "V" + raiz.est.n : (raiz.total ? raiz.total : "∞")
            }
            Text {
                width: parent.width; horizontalAlignment: Text.AlignHCenter
                fontSizeMode: Text.HorizontalFit; minimumPixelSize: 8
                text: raiz.enCurso ? (raiz.medida ? "MEDIDA" : "PRÁCTICA") : (raiz.total ? "VUELTAS" : "LISTO")
                color: "#1d3a24"
                font { family: Tema.fuente800; italic: true; pixelSize: parent.parent.height * 0.105; letterSpacing: parent.parent.height * 0.01 }
            }
        }
    }
}
