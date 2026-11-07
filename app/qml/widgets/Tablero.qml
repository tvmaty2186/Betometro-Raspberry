import QtQuick
import Betometro

// El tablero de widgets de la pantalla de carrera. Lo usan la carrera y el editor.
// En el editor (`editando`): tocar elige, mantener apretado 350 ms y arrastrar mueve (con imán a 8 px
// y líneas guía fucsia), las asas cambian el tamaño (4 en los cuadrados, 1 en los redondos). Como el
// editor de la app (6.8 y 2.4.1): el elegido NO sube de capa (los chicos siguen adelante y se pueden
// tocar); solo sube el que se está moviendo. El borde punteado y las asas van en una capa arriba de todo.
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
    property Item celdaSel: null     // la celda elegida (para dibujar borde y asas arriba de todo)

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

    // Ancho libre en el medio del widget i: sin lo que tapan los widgets que tiene encima (en el armado de
    // fábrica, el V1 y la sugerida van adentro de la cuenta). Así el número no se mete debajo de ellos.
    function anchoLibre(i) {
        const a = px(armado[i]), cx = a.x + a.w / 2;
        let izq = a.x, der = a.x + a.w;
        armado.forEach((o, k) => {
            if (k === i) return;
            const b = px(o);
            if (b.w * b.h >= a.w * a.h) return;                                  // solo los que van adelante (más chicos)
            if (b.y >= a.y + a.h || b.y + b.h <= a.y || b.x >= a.x + a.w || b.x + b.w <= a.x) return;   // no se tocan
            if (b.x + b.w / 2 < cx) izq = Math.max(izq, b.x + b.w); else der = Math.min(der, b.x);
        });
        return Math.max(60, 2 * Math.min(cx - izq, der - cx));
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
            z: moviendo ? 20 : 0
            opacity: moviendo && zona.arrastre ? 0.6 : 1
            readonly property bool redondo: def.forma === "redondo"
            readonly property bool sel: raiz.editando && raiz.elegido === i
            onSelChanged: if (sel) raiz.celdaSel = celda; else if (raiz.celdaSel === celda) raiz.celdaSel = null
            Component.onCompleted: if (sel) raiz.celdaSel = celda
            Component.onDestruction: if (raiz.celdaSel === celda) raiz.celdaSel = null

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
            MouseArea {
                id: zona
                enabled: raiz.editando
                anchors.fill: parent
                pressAndHoldInterval: 350
                preventStealing: true
                property bool arrastre: false
                property real px0: 0
                property real py0: 0
                onPressed: m => {
                    // En los redondos, el toque cuenta solo dentro del círculo (golpe() de la app)
                    if (celda.redondo && Math.hypot(m.x - width / 2, m.y - height / 2) > width / 2) { m.accepted = false; return; }
                    px0 = m.x; py0 = m.y; arrastre = false;
                }
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
        }
    }
    // Borde punteado y asas del elegido: arriba de todos los widgets
    Item {
        id: capaSel
        z: 25
        visible: raiz.celdaSel !== null
        readonly property Item c: raiz.celdaSel
        x: c ? c.x : 0; y: c ? c.y : 0; width: c ? c.width : 0; height: c ? c.height : 0
        Canvas {   // borde punteado amarillo (#ffd23f, 3 px, a 7 px), como la app
            id: borde
            anchors.fill: parent; anchors.margins: -10
            onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
            Connections { target: capaSel; function onCChanged() { borde.requestPaint(); } }
            onPaint: {
                const g = getContext("2d"); g.reset();
                g.strokeStyle = "#ffd23f"; g.lineWidth = 3; g.setLineDash([4, 3]);
                const r = capaSel.c && capaSel.c.redondo ? Math.min(width, height) / 2 - 2 : 28;
                g.beginPath(); g.roundedRect(1.5, 1.5, width - 3, height - 3, r, r); g.stroke();
            }
        }
        // Pellizco con dos dedos sobre el elegido: lo agranda o achica desde el centro (mínimo 60×44), como la app
        PinchHandler {
            target: null
            property real w0: 0
            property real h0: 0
            property real cx: 0
            property real cy: 0
            onActiveChanged: {
                const c = capaSel.c; if (!c) return;
                if (active) {
                    w0 = c.p.w; h0 = c.p.h; cx = c.p.x + c.p.w / 2; cy = c.p.y + c.p.h / 2;
                    c.mx = c.p.x; c.my = c.p.y; c.mw = c.p.w; c.mh = c.p.h; c.moviendo = true;
                } else if (c.moviendo) {
                    raiz.cambio(raiz.aFrac(c.i, Math.max(0, c.mx), Math.max(0, c.my), c.mw, c.mh));
                    c.moviendo = false;
                }
            }
            onActiveScaleChanged: {
                const c = capaSel.c; if (!c || !active) return;
                const tope = c.redondo ? Math.min(raiz.width, raiz.height) : raiz.width;
                const w = Math.max(c.redondo ? 70 : 60, Math.min(tope, w0 * activeScale));
                const h = c.redondo ? w : Math.max(44, Math.min(raiz.height, h0 * activeScale));
                c.mw = w; c.mh = h;
                c.mx = Math.max(0, Math.min(raiz.width - w, cx - w / 2)); c.my = Math.max(0, Math.min(raiz.height - h, cy - h / 2));
            }
        }
        Repeater {
            // Cuadrados: 4 asas (la esquina opuesta queda quieta). Redondos: 1 asa, crece desde el centro.
            model: capaSel.c && capaSel.c.redondo ? ["re"] : ["nw", "ne", "sw", "se"]
            Rectangle {
                required property string modelData
                readonly property bool der: modelData === "ne" || modelData === "se" || modelData === "re"
                readonly property bool abajo: modelData === "sw" || modelData === "se" || modelData === "re"
                width: 36; height: 36; radius: 18
                x: modelData === "re" ? capaSel.width / 2 + capaSel.width / 2 * 0.707 - 18 : (der ? capaSel.width - 18 : -18)
                y: modelData === "re" ? capaSel.height / 2 + capaSel.height / 2 * 0.707 - 18 : (abajo ? capaSel.height - 18 : -18)
                color: Tema.acento; border { width: 2; color: "#000" }
                Text { anchors.centerIn: parent; text: "⇲"   // apunta hacia su esquina
                       rotation: parent.modelData === "nw" ? 180 : parent.modelData === "ne" ? -90 : parent.modelData === "sw" ? 90 : 0
                       color: "#000"; font { pixelSize: 20; bold: true } }
                MouseArea {
                    anchors.fill: parent; anchors.margins: -12
                    preventStealing: true
                    property real ax: 0
                    property real ay: 0
                    onPressed: m => {
                        const c = capaSel.c; if (!c) return;
                        const g = mapToItem(raiz, m.x, m.y); ax = g.x; ay = g.y;
                        c.mx = c.p.x; c.my = c.p.y; c.mw = c.p.w; c.mh = c.p.h;
                        c.moviendo = true;
                    }
                    onPositionChanged: m => {
                        const c = capaSel.c; if (!c) return;
                        const g = mapToItem(raiz, m.x, m.y);
                        if (c.redondo) {   // crece desde el centro: diámetro = 2 × distancia al centro
                            const cx = c.p.x + c.p.w / 2, cy = c.p.y + c.p.h / 2;
                            const d = Math.max(70, Math.min(2 * Math.hypot(g.x - cx, g.y - cy), Math.min(raiz.width, raiz.height)));
                            c.mw = d; c.mh = d; c.mx = cx - d / 2; c.my = cy - d / 2;
                            return;
                        }
                        const dx = g.x - ax, dy = g.y - ay;
                        if (parent.der) c.mw = Math.max(60, Math.min(raiz.width - c.p.x, c.p.w + dx));
                        else { const nx = Math.max(0, Math.min(c.p.x + c.p.w - 60, c.p.x + dx)); c.mw = c.p.x + c.p.w - nx; c.mx = nx; }
                        if (parent.abajo) c.mh = Math.max(44, Math.min(raiz.height - c.p.y, c.p.h + dy));
                        else { const ny = Math.max(0, Math.min(c.p.y + c.p.h - 44, c.p.y + dy)); c.mh = c.p.y + c.p.h - ny; c.my = ny; }
                    }
                    onReleased: {
                        const c = capaSel.c; if (!c) return;
                        raiz.cambio(raiz.aFrac(c.i, Math.max(0, c.mx), Math.max(0, c.my), c.mw, c.mh));
                        c.moviendo = false;
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

    Component { id: cCuenta; WCuenta { est: raiz.est; extra: !!parent.def.extra; barra: parent.def.barra !== false; modo: parent.def.modo || "progresiva"; ancho: parent.def.ancho || 1.2
                                    estilo: parent.def.e || "numeros"; redondo: parent.def.forma === "redondo"
                                    anchoLibre: raiz.anchoLibre(parent.parent.i) } }
    Component { id: cTipo; WTipo { est: raiz.est } }
    Component {
        id: cNecesaria
        WNumero {
            // Si pasa el máximo del velocímetro (160), queda trabada en 160 y en rojo, sin el ■ (Matías, 04/11/2026)
            readonly property bool pasada: raiz.est.vNecKmh > 160
            // Sin GPS en carrera: que diga qué hacer, no un «–» (07-2.3)
            readonly property bool sinGps: !!raiz.est.enCurso && !!raiz.est.sinGps && Datos.ajustes.velActiva !== false
            ancho: parent.def.ancho || 1; redondo: parent.def.forma === "redondo"
            etiqueta: "Sugerida"
            colorNumero: pasada || sinGps ? "#d23b2c" : Tema.numero
            numero: sinGps ? "SIN GPS" : raiz.est.vNecKmh === undefined || raiz.est.vNecKmh === null ? "–" : pasada ? "160" : Math.round(raiz.est.vNecKmh) + (raiz.est.congelada ? "■" : "")
            extra: sinGps ? "seguí por tiempo" : parent.def.extra ? "base " + Math.round(raiz.est.vBaseKmh || 0) : ""
        }
    }
    Component { id: cVelocidad; WVelocidad { est: raiz.est; barra: parent.def.barra !== false; extra: !!parent.def.extra; ancho: parent.def.ancho || 1 } }
    Component { id: cVelocimetro; WVelocimetro { est: raiz.est; extra: !!parent.def.extra } }
    Component {
        id: cUltima
        WNumero {
            ancho: parent.def.ancho || 1; redondo: parent.def.forma === "redondo"
            etiqueta: fila ? "Última vuelta" + (Crono.ultima.n ? " · V" + Crono.ultima.n : "")
                           : "Última vuelta" + (Crono.ultima.n ? "\nV" + Crono.ultima.n : "")
            numero: Crono.ultima.realMs ? Formato.reloj(Crono.ultima.realMs, false) : "–"
            extra: parent.def.extra && Crono.ultima.realMs ? Formato.dif(Crono.ultima.difMs) + " del ideal" : ""
        }
    }
    Component { id: cMarcar; WMarcar { id: wm; est: raiz.est; onMarcar: if (!raiz.editando) raiz.marcar(); Component.onCompleted: raiz.botonMarcar = wm } }
    Component { id: cFaltan; WNumero { ancho: parent.def.ancho || 1; redondo: parent.def.forma === "redondo"; etiqueta: "Faltan"; numero: Formato.miles(Math.max(0, raiz.est.restanteM || 0)) + " m"; extra: parent.def.extra ? "de " + Formato.miles(raiz.est.distanciaM || 0) + " m" : "" } }
    Component { id: cTranscurrido; WNumero { ancho: parent.def.ancho || 1; redondo: parent.def.forma === "redondo"; etiqueta: "Transcurrido"; numero: Formato.reloj(raiz.est.transMs || 0, false) } }
    Component { id: cIdeal; WNumero { ancho: parent.def.ancho || 1; redondo: parent.def.forma === "redondo"; etiqueta: "Ideal"; numero: Formato.reloj(raiz.est.tiempoIdealMs || 0, false) } }
    Component { id: cProxima; WNumero { ancho: parent.def.ancho || 1; redondo: parent.def.forma === "redondo"; etiqueta: "Próxima vuelta"; numero: raiz.est.proxMedida ? "MEDIDA" : "LIBRE" } }
    Component { id: cDelta; WDelta { est: raiz.est; ancho: parent.def.ancho || 1; estilo: parent.def.e || "numero"; redondo: parent.def.forma === "redondo" } }
    property var botonMarcar: null
    function efectoMarcar() { if (botonMarcar) botonMarcar.efecto(); }
}
