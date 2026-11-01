import QtQuick
import Betometro

// Cuenta regresiva: Falta / Pasado, m:ss.d, «faltan 2.310 m» y la barra de adelantado/atrasado.
Caja {
    id: raiz
    property var est: ({})
    property bool extra: true
    property bool barra: true
    readonly property real restante: est.restanteMs || 0
    readonly property bool pasado: restante < 0
    readonly property real ideal: est.tiempoIdealMs || 1
    Column {
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: raiz.height * 0.02
        Etiq { alto: raiz.height; width: parent.width; text: raiz.pasado ? "Pasado" : "Falta" }
        Numero {
            width: parent.width
            height: raiz.height * (raiz.barra ? 0.52 : 0.62)
            maximo: raiz.height * (raiz.barra ? 0.6 : 0.7)
            color: raiz.pasado ? Tema.alerta : Tema.numero
            text: raiz.pasado ? "+" + Formato.reloj(-raiz.restante, false) : Formato.reloj(raiz.restante, !!raiz.est.enCurso)
        }
        Text {
            visible: raiz.extra
            width: parent.width; horizontalAlignment: Text.AlignHCenter
            text: "faltan " + (raiz.est.distanciaM > 0 && !raiz.est.sinGps ? Formato.miles(Math.max(0, raiz.est.restanteM)) : "–") + " m"
            color: Tema.tinta
            font { family: Tema.fuente600; pixelSize: Math.max(14, Math.min(24, raiz.height * 0.075)) }
        }
        Item { width: 1; height: raiz.height * 0.02; visible: raiz.barra }
        Barra {
            visible: raiz.barra
            anchors.horizontalCenter: parent.horizontalCenter
            width: raiz.width * 0.46; height: Math.max(8, raiz.height * 0.038)
            rango: Math.max(1, raiz.ideal / 1000 * 0.10)
            valor: raiz.est.deltaS === undefined ? null : raiz.est.deltaS
        }
    }
}
