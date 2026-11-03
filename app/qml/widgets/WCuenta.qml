import QtQuick
import Betometro

// Cronómetro de la vuelta, con dos modos (se elige en el editor de pantalla):
// - «progresiva» (de fábrica): cuenta para arriba desde la marca, 0:00.0 → 4:00.0; al pasar el ideal se pone en rojo.
// - «regresiva»: Falta / Pasado, como la app.
// Los dos con «faltan 2.310 m», la barra de adelantado/atrasado y el tiempo ideal chiquito en la esquina.
Caja {
    id: raiz
    property var est: ({})
    property bool extra: true
    property bool barra: true
    property string modo: "progresiva"
    property real ancho: 1.2
    property real anchoLibre: 100000   // espacio del medio sin widgets encima (lo calcula el tablero)
    readonly property bool progresiva: modo !== "regresiva"
    readonly property real restante: est.restanteMs || 0
    readonly property bool pasado: restante < 0
    readonly property real ideal: est.tiempoIdealMs || 1
    readonly property real trans: Math.max(0, est.transMs || 0)

    // Tiempo ideal de la vuelta: siempre a la vista, chiquito
    Text {
        anchors { top: parent.top; right: parent.right; topMargin: Math.max(8, raiz.height * 0.05); rightMargin: Math.max(12, raiz.height * 0.06) }
        text: "IDEAL " + Formato.tiempo(raiz.ideal)
        color: Tema.tintaSuave
        font { family: Tema.fuente600; letterSpacing: 1; pixelSize: Math.max(12, Math.min(18, raiz.height * 0.06)) }
    }
    Column {
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: raiz.height * 0.02
        Etiq { alto: raiz.height; width: parent.width
               text: raiz.progresiva ? (raiz.pasado ? "Pasado el ideal" : "Vuelta") : (raiz.pasado ? "Pasado" : "Falta") }
        Numero {
            width: parent.width
            pAlto: raiz.barra ? 66 : 82; pAncho: 105; alto: raiz.height - 16; anchoCaja: Math.min(raiz.width, raiz.anchoLibre) - 24; ancho: raiz.ancho
            color: !raiz.pasado ? Tema.numero : raiz.progresiva ? Tema.mal : Tema.warn
            text: raiz.progresiva ? Formato.reloj(raiz.trans, false)
                  : raiz.pasado ? "+" + Formato.reloj(-raiz.restante, false) : Formato.reloj(raiz.restante, !!raiz.est.enCurso)
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
