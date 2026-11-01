import QtQuick
import Betometro

// Adelantado / atrasado, en número: EN TIEMPO, ADELANTADO 1,2 s, ATRASADO 1,2 s (en color de alerta).
Caja {
    id: raiz
    property var est: ({})
    readonly property var d: est.deltaS === undefined ? null : est.deltaS
    Column {
        anchors.centerIn: parent
        width: parent.width - 24
        Etiq { alto: raiz.height; width: parent.width; text: "Adelantado / atrasado" }
        Numero {
            width: parent.width; height: raiz.height * 0.55; maximo: raiz.height * 0.4
            color: raiz.d !== null && raiz.d <= -0.05 ? "#ff8a7a" : Tema.crema
            text: raiz.d === null ? (raiz.est.enCurso ? "SIN DATOS" : "EN TIEMPO")
                : Math.abs(raiz.d) < 0.05 ? "EN TIEMPO"
                : (raiz.d > 0 ? "ADELANTADO " : "ATRASADO ") + Math.abs(raiz.d).toFixed(1).replace(".", ",") + " s"
        }
    }
}
