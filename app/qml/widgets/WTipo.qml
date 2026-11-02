import QtQuick
import Betometro

// Vuelta y tipo: redondel V3 / MEDIDA (amarillo) o PRÁCTICA (crema). Antes de largar: V1 / LISTO (sin cantidad fija de vueltas).
Item {
    id: raiz
    property var est: ({})
    readonly property bool enCurso: !!est.enCurso
    readonly property bool medida: est.medida !== false
    Item {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height); height: width
        Rectangle { anchors.fill: parent; anchors.margins: -5; radius: width / 2; color: Tema.avispon ? "#4b5258" : Tema.bordeCromo }
        Rectangle { anchors.fill: parent; anchors.margins: -3; radius: width / 2; color: Tema.avispon ? "#f5f6f7" : Tema.bordeCromo }
        Rectangle {
            anchors.fill: parent; radius: width / 2
            gradient: Gradient {
                GradientStop { position: 0; color: raiz.medida ? Tema.acentoClaro : (Tema.avispon ? "#fffbef" : Tema.caja1) }
                GradientStop { position: 0.5; color: raiz.medida ? Tema.acento : (Tema.avispon ? Tema.crema : Tema.caja1) }
                GradientStop { position: 1; color: raiz.medida ? Tema.acentoOscuro : (Tema.avispon ? "#d6caa9" : Tema.caja2) }
            }
        }
        Column {
            anchors.centerIn: parent
            width: parent.width * 0.8
            Numero {
                width: parent.width; height: parent.parent.height * 0.42
                color: raiz.medida ? (Tema.avispon ? Tema.numeroVerde : Tema.acentoTinta) : (Tema.avispon ? Tema.numeroVerde : Tema.numero); style: Text.Normal
                maximo: parent.parent.height * 0.43
                text: "V" + (raiz.enCurso ? raiz.est.n : 1)
            }
            Text {
                width: parent.width; horizontalAlignment: Text.AlignHCenter
                fontSizeMode: Text.HorizontalFit; minimumPixelSize: 8
                text: raiz.enCurso ? (raiz.medida ? "MEDIDA" : "PRÁCTICA") : "LISTO"
                color: raiz.medida ? (Tema.avispon ? "#1d3a24" : Tema.acentoTinta) : (Tema.avispon ? "#1d3a24" : Tema.tinta)
                font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: parent.parent.height * 0.105; letterSpacing: parent.parent.height * 0.01 }
            }
        }
    }
}
