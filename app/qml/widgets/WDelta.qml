import QtQuick
import Betometro

// Adelantado / atrasado, en número: EN TIEMPO, ADELANTADO 1,2 s, ATRASADO 1,2 s (en color de alerta).
Caja {
    id: raiz
    property var est: ({})
    property real ancho: 1
    readonly property var d: est.deltaS === undefined ? null : est.deltaS
    Column {
        anchors.centerIn: parent
        width: parent.width - 24
        Etiq { alto: raiz.height; width: parent.width; text: "Adelantado / atrasado" }
        Numero {
            width: parent.width
            pAlto: 70; pAncho: 95; alto: raiz.height - 16; anchoCaja: raiz.width - 24; ancho: raiz.ancho
            color: raiz.d !== null && raiz.d <= -0.05 ? Tema.warn : Tema.numero
            text: raiz.d === null ? (raiz.est.enCurso ? "SIN DATOS" : "EN TIEMPO")
                : Math.abs(raiz.d) < 0.05 ? "EN TIEMPO"
                : (raiz.d > 0 ? "ADELANTADO " : "ATRASADO ") + Math.abs(raiz.d).toFixed(1).replace(".", ",") + " s"
        }
    }
}
