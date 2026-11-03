import QtQuick
import Betometro

// Adelantado / atrasado, en número: EN TIEMPO, ADELANTADO 1,2 s, ATRASADO 1,2 s (en color de alerta).
Caja {
    id: raiz
    property var est: ({})
    property real ancho: 1
    property string estilo: "numero"   // «Estilo» del editor: numero o barra (app 1.3)
    readonly property real interiorA: redondo ? width * 0.68 : width - 24
    readonly property real interiorH: redondo ? height * 0.68 : height - 16
    readonly property var d: est.deltaS === undefined ? null : est.deltaS
    Column {
        visible: raiz.estilo !== "barra"
        anchors.centerIn: parent
        width: raiz.interiorA
        Etiq { alto: raiz.height; width: parent.width; text: "Adelantado / atrasado" }
        Numero {
            width: parent.width
            pAlto: 70; pAncho: 95; alto: raiz.interiorH; anchoCaja: raiz.interiorA; ancho: raiz.ancho
            color: raiz.d !== null && raiz.d <= -0.05 ? Tema.warn : Tema.numero
            text: raiz.d === null ? (raiz.est.enCurso ? "SIN DATOS" : "EN TIEMPO")
                : Math.abs(raiz.d) < 0.05 ? "EN TIEMPO"
                : (raiz.d > 0 ? "ADELANTADO " : "ATRASADO ") + Math.abs(raiz.d).toFixed(1).replace(".", ",") + " s"
        }
    }
    // Barra: cero en el medio; a la derecha adelantado (verde), a la izquierda atrasado (amarillo), ±5 s
    Column {
        visible: raiz.estilo === "barra"
        anchors.centerIn: parent
        width: raiz.interiorA
        spacing: raiz.height * 0.06
        Etiq { alto: raiz.height; width: parent.width; text: "Adelantado / atrasado" }
        Item {
            width: parent.width; height: Math.max(24, raiz.height * 0.28)
            readonly property real e: width / 300
            Rectangle {   // carril
                x: 10 * parent.e; width: 280 * parent.e; anchors.verticalCenter: parent.verticalCenter
                height: parent.height * 0.45; radius: 6 * parent.e; color: Qt.rgba(1, 1, 1, 0.12)
            }
            Rectangle {   // lo que va adelantado o atrasado: 28 por segundo, hasta 5 s
                readonly property real dd: raiz.d === null ? 0 : Math.max(-5, Math.min(5, raiz.d))
                height: parent.height * 0.75; anchors.verticalCenter: parent.verticalCenter; radius: 4 * parent.e
                width: Math.abs(dd) * 28 * parent.e
                x: dd >= 0 ? 150 * parent.e : 150 * parent.e - width
                color: dd >= 0 ? Tema.zona : Tema.warn
            }
            Rectangle { x: 150 * parent.e - 1.5; width: 3; height: parent.height; color: Tema.tinta }   // el cero
        }
        Text {
            width: parent.width; horizontalAlignment: Text.AlignHCenter
            text: raiz.d === null ? (raiz.est.enCurso ? "sin datos de velocidad" : "en tiempo al largar")
                : Math.abs(raiz.d) < 0.05 ? "En tiempo"
                : (raiz.d > 0 ? "Adelantado " : "Atrasado ") + Math.abs(raiz.d).toFixed(1).replace(".", ",") + " s"
            color: Tema.tinta; opacity: 0.85
            font { family: Tema.fuente600; pixelSize: Math.max(14, Math.min(26, raiz.height * 0.1)) }
        }
    }
}
