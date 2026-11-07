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
    property string estilo: "numeros"  // «Estilo» del editor: numeros o anillo (app 1.3)
    readonly property bool anillo: estilo === "anillo"
    // Interior: sin el relleno (8×12 px; en los redondos, 16 % de cada lado como la app)
    readonly property real interiorA: redondo ? width * 0.68 : Math.min(width, anchoLibre) - 24
    readonly property real interiorH: redondo ? height * 0.68 : height - 16
    readonly property real avance: progresiva ? trans / ideal : restante / ideal   // 0 a 1, para el anillo
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
        visible: !raiz.anillo
        anchors.centerIn: parent
        width: raiz.redondo ? parent.width * 0.68 : parent.width - 40
        spacing: raiz.height * 0.02
        Etiq { alto: raiz.height; width: parent.width
               text: raiz.progresiva ? (raiz.pasado ? "Pasado el ideal" : "Vuelta") : (raiz.pasado ? "Pasado" : "Falta") }
        Numero {
            width: parent.width
            pAlto: raiz.barra ? 66 : 82; pAncho: 105; alto: raiz.interiorH; anchoCaja: raiz.interiorA; ancho: raiz.ancho
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
    // Anillo: aro con el avance de la vuelta y el número en el medio (app: r=86 en 200×200, trazo 12).
    // Solo se crea si se usa el estilo anillo: antes existía siempre y se recalculaba (03-3)
    Loader {
        active: raiz.anillo
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.88; height: width
        sourceComponent: anilloComp
    }
    Component {
        id: anilloComp
    Item {
        Canvas {
            id: aro
            anchors.fill: parent
            property real p: Math.round(Math.max(0, Math.min(1, raiz.avance)) * 400) / 400   // repinta solo si se mueve
            onPChanged: requestPaint(); onWidthChanged: requestPaint()
            onPaint: {
                const g = getContext("2d"); g.reset();
                const e = width / 200, cx = width / 2, R = 86 * e;
                g.lineWidth = 12 * e;
                g.beginPath(); g.arc(cx, cx, R, 0, Math.PI * 2); g.strokeStyle = String(Tema.cara2); g.globalAlpha = 0.35; g.stroke();
                g.globalAlpha = 1;
                if (p > 0) {
                    g.beginPath(); g.arc(cx, cx, R, -Math.PI / 2, -Math.PI / 2 + p * Math.PI * 2, false);
                    g.strokeStyle = String(raiz.pasado ? (raiz.progresiva ? Tema.mal : Tema.warn) : Tema.sugerida); g.lineCap = "round"; g.stroke();
                }
            }
        }
        Column {
            anchors.centerIn: parent
            width: parent.width * 0.7
            Etiq { alto: raiz.height; width: parent.width
                   text: raiz.progresiva ? (raiz.pasado ? "Pasado el ideal" : "Vuelta") : (raiz.pasado ? "Pasado" : "Falta") }
            Numero {
                width: parent.width
                pAlto: 26; pAncho: 45; alto: raiz.interiorH; anchoCaja: raiz.interiorA; ancho: 1
                color: !raiz.pasado ? Tema.numero : raiz.progresiva ? Tema.mal : Tema.warn
                text: raiz.progresiva ? Formato.reloj(raiz.trans, false)
                      : raiz.pasado ? "+" + Formato.reloj(-raiz.restante, false) : Formato.reloj(raiz.restante, !!raiz.est.enCurso)
            }
            Text {
                visible: raiz.extra
                width: parent.width; horizontalAlignment: Text.AlignHCenter
                text: "faltan " + (raiz.est.distanciaM > 0 && !raiz.est.sinGps ? Formato.miles(Math.max(0, raiz.est.restanteM)) : "–") + " m"
                color: Tema.tinta
                font { family: Tema.fuente600; pixelSize: Math.max(13, Math.min(20, raiz.height * 0.06)) }
            }
        }
    }
    }
}
