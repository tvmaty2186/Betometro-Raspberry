import QtQuick
import Betometro

// El tablero de widgets de la pantalla de carrera. Lo usan la carrera y el editor.
// En el editor (`editando`): tocar elige, mantener apretado 350 ms y arrastrar mueve (con imán a 8 px
// y líneas guía fucsia), el asa de abajo a la derecha cambia el tamaño. Como el editor de la app (6.8).
Item {
    id: raiz
    property var armado: []          // en fracciones
    property var est: ({})
    property bool editando: false
    property int elegido: -1
    property bool velActiva: true
    signal cambio(var armado)        // el editor guarda la lista nueva
    signal marcar()
    property var guias: []           // líneas del imán: [{v: true/false, p: px}]

    function px(w) {
        const W = width, H = height, r = w.forma === "redondo";
        return { x: w.x * W, y: w.y * H, w: w.w * W, h: r ? w.w * W : w.h * H };
    }
    function aFrac(i, x, y, w, h) {
        const W = width, H = height, a = armado.slice(), o = Object.assign({}, a[i]);
        o.x = x / W; o.y = y / H; o.w = w / W; o.h = (o.forma === "redondo" ? w : h) / H;
        a[i] = o;
        return a;
    }
    // Imán: se pega a bordes y centros de los otros widgets y del tablero
    function iman(i, x, y, w, h) {
        const lx = [0, width / 2, width], ly = [0, height / 2, height];
        armado.forEach((o, k) => { if (k === i) return; const p = px(o); lx.push(p.x, p.x + p.w / 2, p.x + p.w); ly.push(p.y, p.y + p.h / 2, p.y + p.h); });
        let nx = x, ny = y, g = [], mejorX = 9, mejorY = 9;
        for (const L of lx) for (const off of [0, w / 2, w]) { const d = Math.abs(x + off - L); if (d < mejorX) { mejorX = d; nx = L - off; g = g.filter(q => !q.v); g.push({ v: true, p: L }); } }
        for (const L of ly) for (const off of [0, h / 2, h]) { const d = Math.abs(y + off - L); if (d < mejorY) { mejorY = d; ny = L - off; g = g.filter(q => q.v); g.push({ v: false, p: L }); } }
        guias = g;
        return { x: nx, y: ny };
    }

    // Los más grandes primero, así los chicos quedan adelante
    readonly property var orden: armado.map((w, i) => i).sort((a, b) => armado[b].w * armado[b].h - armado[a].w * armado[a].h)

    Repeater {
        model: raiz.orden
        Item {
            id: celda
            required property int modelData
            readonly property int i: modelData
            readonly property var def: raiz.armado[i] || {}
            readonly property var p: raiz.px(def)
            readonly property bool oculto: !raiz.editando && !raiz.velActiva && Armado.necesitaGps(def.t)
            property real mx: 0
            property real my: 0
            property real mw: 0
            property real mh: 0
            property bool moviendo: false
            visible: !oculto
            x: moviendo ? mx : p.x; y: moviendo ? my : p.y
            width: moviendo ? mw : p.w; height: moviendo ? mh : p.h
            z: raiz.elegido === i ? 10 : 0
            opacity: moviendo && zona.arrastre ? 0.6 : 1

            Loader {
                anchors.fill: parent
                property var def: celda.def
                sourceComponent: {
                    switch (celda.def.t) {
                    case "cuenta": return cCuenta;
                    case "tipo": return cTipo;
                    case "necesaria": return cNecesaria;
                    case "velocimetro": return celda.def.forma === "redondo" ? cVelocimetro : cVelocidad;
                    case "ultima": return cUltima;
                    case "marcar": return cMarcar;
                    case "faltan": return cFaltan;
                    case "transcurrido": return cTranscurrido;
                    case "ideal": return cIdeal;
                    case "proxima": return cProxima;
                    case "delta": return cDelta;
                    }
                    return null;
                }
            }

            // ---- Edición ----
            Rectangle {   // borde punteado amarillo del elegido
                visible: raiz.editando && raiz.elegido === celda.i
                anchors.fill: parent; anchors.margins: -6
                radius: celda.def.forma === "redondo" ? width / 2 : 26
                color: "transparent"; border { width: 3; color: Tema.acento }
            }
            MouseArea {
                id: zona
                enabled: raiz.editando
                anchors.fill: parent
                pressAndHoldInterval: 350
                preventStealing: true
                property bool arrastre: false
                property real px0: 0
                property real py0: 0
                onPressed: m => { px0 = m.x; py0 = m.y; arrastre = false; }
                onPressAndHold: {
                    arrastre = true;
                    celda.mx = celda.p.x; celda.my = celda.p.y; celda.mw = celda.p.w; celda.mh = celda.p.h;
                    celda.moviendo = true;
                    raiz.elegido = celda.i;
                }
                onPositionChanged: m => {
                    if (!arrastre) return;
                    const pos = raiz.iman(celda.i, celda.mx + m.x - px0, celda.my + m.y - py0, celda.mw, celda.mh);
                    celda.mx = Math.max(0, Math.min(raiz.width - celda.mw, pos.x));
                    celda.my = Math.max(0, Math.min(raiz.height - celda.mh, pos.y));
                }
                onReleased: m => {
                    if (arrastre) {
                        raiz.cambio(raiz.aFrac(celda.i, celda.mx, celda.my, celda.mw, celda.mh));
                        celda.moviendo = false; arrastre = false; raiz.guias = [];
                    } else if (Math.hypot(m.x - px0, m.y - py0) < 12) {
                        raiz.elegido = celda.i;
                    }
                }
            }
            Rectangle {   // asa para cambiar el tamaño
                visible: raiz.editando && raiz.elegido === celda.i
                width: 36; height: 36; radius: 18
                x: celda.def.forma === "redondo" ? parent.width / 2 + parent.width / 2 * 0.707 - 18 : parent.width - 18
                y: celda.def.forma === "redondo" ? parent.height / 2 + parent.height / 2 * 0.707 - 18 : parent.height - 18
                color: Tema.acento; border { width: 2; color: "#000" }
                Text { anchors.centerIn: parent; text: "⇲"; color: "#000"; font { pixelSize: 20; bold: true } }
                MouseArea {
                    anchors.fill: parent; anchors.margins: -12
                    preventStealing: true
                    property real ax: 0
                    property real ay: 0
                    onPressed: m => {
                        const g = mapToItem(raiz, m.x, m.y); ax = g.x; ay = g.y;
                        celda.mx = celda.p.x; celda.my = celda.p.y; celda.mw = celda.p.w; celda.mh = celda.p.h;
                        celda.moviendo = true;
                    }
                    onPositionChanged: m => {
                        const g = mapToItem(raiz, m.x, m.y);
                        const r = celda.def.forma === "redondo";
                        if (r) {   // crece desde el centro
                            const cx = celda.p.x + celda.p.w / 2, cy = celda.p.y + celda.p.h / 2;
                            const d = Math.max(70, Math.min(Math.hypot(g.x - cx, g.y - cy) * 2 / 1.414 * 1.0, Math.min(raiz.width, raiz.height)));
                            celda.mw = d; celda.mh = d; celda.mx = cx - d / 2; celda.my = cy - d / 2;
                        } else {
                            celda.mw = Math.max(60, Math.min(raiz.width - celda.p.x, celda.p.w + g.x - ax));
                            celda.mh = Math.max(44, Math.min(raiz.height - celda.p.y, celda.p.h + g.y - ay));
                        }
                    }
                    onReleased: {
                        raiz.cambio(raiz.aFrac(celda.i, Math.max(0, celda.mx), Math.max(0, celda.my), celda.mw, celda.mh));
                        celda.moviendo = false;
                    }
                }
            }
        }
    }
    // Líneas guía del imán (fucsia)
    Repeater {
        model: raiz.guias
        Rectangle {
            required property var modelData
            z: 20
            color: "#ff3df0"
            x: modelData.v ? modelData.p - 1 : 0; y: modelData.v ? 0 : modelData.p - 1
            width: modelData.v ? 2 : raiz.width; height: modelData.v ? raiz.height : 2
        }
    }
    MouseArea {   // tocar el vacío deselecciona
        enabled: raiz.editando
        anchors.fill: parent; z: -1
        onClicked: raiz.elegido = -1
    }

    Component { id: cCuenta; WCuenta { est: raiz.est; extra: parent.def.extra !== false; barra: parent.def.barra !== false; modo: parent.def.modo || "progresiva" } }
    Component { id: cTipo; WTipo { est: raiz.est } }
    Component {
        id: cNecesaria
        WNumero {
            etiqueta: "Sugerida"
            numero: raiz.est.vNecKmh === undefined || raiz.est.vNecKmh === null ? "–" : Math.round(raiz.est.vNecKmh) + (raiz.est.congelada ? "■" : "")
            extra: parent.def.extra ? "base " + Math.round(raiz.est.vBaseKmh || 0) : ""
        }
    }
    Component { id: cVelocidad; WVelocidad { est: raiz.est; barra: parent.def.barra !== false; extra: !!parent.def.extra } }
    Component { id: cVelocimetro; WVelocimetro { est: raiz.est } }
    Component {
        id: cUltima
        WNumero {
            etiqueta: fila ? "Última vuelta" + (Crono.ultima.n ? " · V" + Crono.ultima.n : "")
                           : "Última vuelta" + (Crono.ultima.n ? "\nV" + Crono.ultima.n : "")
            numero: Crono.ultima.realMs ? Formato.reloj(Crono.ultima.realMs, false) : "–"
            extra: parent.def.extra && Crono.ultima.realMs ? Formato.dif(Crono.ultima.difMs) + " del ideal" : ""
        }
    }
    Component { id: cMarcar; WMarcar { id: wm; est: raiz.est; onMarcar: if (!raiz.editando) raiz.marcar(); Component.onCompleted: raiz.botonMarcar = wm } }
    Component { id: cFaltan; WNumero { etiqueta: "Faltan"; numero: Formato.miles(Math.max(0, raiz.est.restanteM || 0)) + " m"; extra: parent.def.extra !== false ? "de " + Formato.miles(raiz.est.distanciaM || 0) + " m" : "" } }
    Component { id: cTranscurrido; WNumero { etiqueta: "Transcurrido"; numero: Formato.reloj(raiz.est.transMs || 0, false) } }
    Component { id: cIdeal; WNumero { etiqueta: "Ideal"; numero: Formato.reloj(raiz.est.tiempoIdealMs || 0, false) } }
    Component { id: cProxima; WNumero { etiqueta: "Próxima vuelta"; numero: raiz.est.proxMedida ? "MEDIDA" : "LIBRE" } }
    Component { id: cDelta; WDelta { est: raiz.est } }
    property var botonMarcar: null
    function efectoMarcar() { if (botonMarcar) botonMarcar.efecto(); }
}
