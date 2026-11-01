import QtQuick
import Betometro

// Pantalla 10: Informe de una carrera. Muestras #s10 y #v10.
// Los datos salen del mismo cálculo que la app (datosInforme): recorrido común, velocidad, aceleración y adelanto.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property string id_: Navegacion.parametros.id || ""
    readonly property var c: Datos.carrera(id_)
    readonly property var inf: Datos.informe(id_)
    readonly property var vueltas: inf.vueltas || []
    readonly property var conTraza: vueltas.map((v, i) => v.m ? i : -1).filter(i => i >= 0)
    property int lap: conTraza.length ? conTraza[0] : -1
    property string modo: "vel"      // vel, acel, ade
    readonly property var vu: lap >= 0 ? vueltas[lap] : null
    readonly property real dist: inf.distanciaM || 0
    readonly property real sug: dist > 0 ? dist / (inf.idealMs / 1000) * 3.6 : 0

    function nombreV(v) { return "V" + v.n + (inf.largadas > 1 ? " L" + v.largada : ""); }
    function colorPunto(p) {
        if (!p[7]) return "#555555";
        if (modo === "acel") { const a = p[3]; return a > 0.8 ? Tema.bien : a > 0.25 ? "#8fe3b0" : a < -0.8 ? "#d23b2c" : a < -0.25 ? "#f08a96" : "#9aa3ad"; }
        if (modo === "ade") { const s = Math.abs(p[4]); return s < 1 ? Tema.bien : s < 3 ? Tema.acento : "#d23b2c"; }
        if (!(sug > 0)) return Tema.bien;
        const d = p[2] - sug;
        return d > 6 ? "#d23b2c" : d > 2 ? Tema.acento : d >= -2 ? Tema.bien : d >= -6 ? "#7fd3ff" : "#3a8de0";
    }
    function colorDif(ms) { const s = Math.abs(ms / 1000); return s <= 0.2 ? Tema.bien : s <= 0.5 ? Tema.acento : Tema.alerta; }
    onLapChanged: { mapa.requestPaint(); grafico.requestPaint(); }
    onModoChanged: mapa.requestPaint()

    Fondo { anchors.fill: parent }

    // Cabecera: volver, león, nombre y datos; 101 y Compartir
    Volver { x: raiz.h ? 24 : 20; y: raiz.h ? 30 : 26; onTocado: Navegacion.ir("ficha", { id: raiz.id_ }) }
    Row {
        x: raiz.h ? 80 : 24; y: raiz.h ? 14 : 96; spacing: 14
        Image { source: "qrc:/recursos/img/leon.png"; height: raiz.h ? 46 : 50; fillMode: Image.PreserveAspectFit; anchors.verticalCenter: parent.verticalCenter; mipmap: true }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            TextoCromo { texto: (raiz.c.nombre || "").toUpperCase(); tam: raiz.h ? 36 : 38 }
            Sub { text: raiz.c.fechaCorta + " · ideal " + raiz.c.ideal + (raiz.dist > 0 ? " · " + raiz.c.distancia + " m" : "") + " · " + raiz.vueltas.length + " vueltas" }
        }
    }
    Row {
        anchors { right: parent.right; rightMargin: raiz.h ? 24 : 20 }
        y: raiz.h ? 20 : 20; spacing: 16
        Numero101 { d: raiz.h ? 62 : 58; numero: Datos.ajustes.numeroAuto || "101"; anchors.verticalCenter: parent.verticalCenter }
        BotonAmarillo { width: raiz.h ? 190 : 170; height: raiz.h ? 56 : 54; texto: "Compartir"; anchors.verticalCenter: parent.verticalCenter
                        onTocado: Navegacion.ir("compartir", { id: raiz.id_ }) }
    }

    // Mapa
    Tarjeta {
        id: cajaMapa
        oscura: true
        x: raiz.h ? 24 : 20; y: raiz.h ? 110 : 180
        width: raiz.h ? 600 : parent.width - 40; height: raiz.h ? 590 : 470
        Sub { x: 18; y: 16; text: raiz.modo === "vel" ? "Recorrido de la vuelta · color = velocidad" : raiz.modo === "acel" ? "Recorrido · color = aceleración" : "Recorrido · color = adelanto"; tam: 12 }
        Row {
            anchors { right: parent.right; rightMargin: 14; top: parent.top; topMargin: 10 }
            spacing: 6
            Repeater {
                model: [{ t: "Vel.", m: "vel" }, { t: "Acel.", m: "acel" }].concat(raiz.dist > 0 ? [{ t: "Adel.", m: "ade" }] : [])
                Rectangle {
                    required property var modelData
                    width: tm.implicitWidth + 18; height: 26; radius: 13
                    color: raiz.modo === modelData.m ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
                    border { width: 1; color: Tema.bordeCromo }
                    Text { id: tm; anchors.centerIn: parent; text: modelData.t; color: raiz.modo === modelData.m ? Tema.acentoTinta : Tema.tinta
                           font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 13 } }
                    MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: raiz.modo = modelData.m }
                }
            }
        }
        Canvas {
            id: mapa
            anchors { fill: parent; topMargin: 44; margins: 14 }
            onWidthChanged: requestPaint()
            onPaint: {
                const g = getContext("2d");
                g.reset();
                const P = raiz.inf.pista || [];
                if (!P.length) {
                    g.fillStyle = Tema.tintaSuave; g.font = "italic 18px '" + Tema.fuente600 + "'"; g.textAlign = "center";
                    g.fillText("Esta carrera no tiene el recorrido grabado.", width / 2, height / 2);
                    return;
                }
                const k = Math.min(width / 364, height / 330), ox = (width - 364 * k) / 2, oy = (height - 330 * k) / 2;
                const X = p => ox + p[0] * k, Y = p => oy + p[1] * k;
                g.lineJoin = "round"; g.lineCap = "round";
                g.beginPath(); g.moveTo(X(P[0]), Y(P[0]));
                for (const p of P) g.lineTo(X(p), Y(p));
                if (raiz.inf.cerrada) g.closePath();
                g.strokeStyle = "rgba(0,0,0,0.45)"; g.lineWidth = 20 * k; g.stroke();
                g.strokeStyle = "#3a3f45"; g.lineWidth = 11 * k; g.stroke();
                const M = raiz.vu && raiz.vu.m ? raiz.vu.m : [];
                g.lineWidth = 8 * k;
                for (let i = 1; i < M.length; i++) {
                    if (!M[i][7] || !M[i - 1][7]) continue;
                    g.beginPath(); g.moveTo(ox + M[i - 1][5] * k, oy + M[i - 1][6] * k); g.lineTo(ox + M[i][5] * k, oy + M[i][6] * k);
                    g.strokeStyle = raiz.colorPunto(M[i]); g.stroke();
                }
                g.beginPath(); g.arc(X(P[0]), Y(P[0]), 9 * k, 0, Math.PI * 2); g.fillStyle = "#ffffff"; g.fill();
                const izq = X(P[0]) > width * 0.7;   // cerca del borde derecho: el texto va a la izquierda
                g.fillStyle = Tema.tinta; g.font = "italic " + Math.round(11 * k) + "px '" + Tema.fuente800 + "'"; g.textAlign = izq ? "right" : "left";
                g.fillText("LARGADA", X(P[0]) + (izq ? -14 : 14) * k, Y(P[0]) + 4 * k);
            }
        }
    }

    // Velocidad de la vuelta elegida, a lo largo de la distancia
    Tarjeta {
        id: cajaVel
        oscura: true
        x: raiz.h ? 648 : 20; y: raiz.h ? 110 : 670
        width: raiz.h ? 608 : parent.width - 40; height: raiz.h ? 230 : 210
        Sub { x: 18; y: 16; text: "Velocidad km/h" + (raiz.vu ? " · " + raiz.nombreV(raiz.vu) : "") + (raiz.sug > 0 ? " · sugerida " + raiz.sug.toFixed(1).replace(".", ",") : ""); tam: 12 }
        Canvas {
            id: grafico
            anchors { fill: parent; topMargin: 40; margins: 16 }
            onWidthChanged: requestPaint()
            onPaint: {
                const g = getContext("2d");
                g.reset();
                const M = raiz.vu && raiz.vu.m ? raiz.vu.m : [];
                if (!M.length) return;
                const L = raiz.vu.L || 1;
                let vmax = raiz.sug + 10;
                for (const p of M) if (p[7]) vmax = Math.max(vmax, p[2]);
                const esc = Math.max(20, Math.ceil(vmax / 10) * 10);
                const X = d => 30 + (width - 34) * Math.min(1, d / L), Y = v => height - 6 - (height - 12) * v / esc;
                g.strokeStyle = "rgba(255,255,255,0.12)"; g.lineWidth = 1;
                g.fillStyle = Tema.tintaSuave; g.font = "11px '" + Tema.fuente + "'"; g.textAlign = "right";
                for (let i = 0; i <= 4; i++) { const v = esc * i / 4; g.beginPath(); g.moveTo(30, Y(v)); g.lineTo(width - 4, Y(v)); g.stroke(); g.fillText(String(Math.round(v)), 24, Y(v) + 4); }
                if (raiz.sug > 0) {
                    g.strokeStyle = Tema.acento; g.lineWidth = 2; g.setLineDash([8, 6]);
                    g.beginPath(); g.moveTo(30, Y(raiz.sug)); g.lineTo(width - 4, Y(raiz.sug)); g.stroke(); g.setLineDash([]);
                }
                g.strokeStyle = Tema.numero; g.lineWidth = 3.5; g.lineJoin = "round";
                let abierto = false;
                g.beginPath();
                for (const p of M) {
                    if (!p[7]) { abierto = false; continue; }
                    if (!abierto) { g.moveTo(X(p[1]), Y(p[2])); abierto = true; } else g.lineTo(X(p[1]), Y(p[2]));
                }
                g.stroke();
            }
        }
    }

    // Todas las vueltas
    Tarjeta {
        oscura: true
        x: cajaVel.x; y: raiz.h ? 362 : 900
        width: cajaVel.width; height: raiz.h ? 338 : 360
        Sub { x: 22; y: 16; text: "Todas las vueltas · tocá una para verla"; tam: 12 }
        Flickable {
            id: tabla
            anchors { fill: parent; topMargin: 40; leftMargin: 22; rightMargin: 22; bottomMargin: 12 }
            contentHeight: colT.height; clip: true; boundsBehavior: Flickable.StopAtBounds
            Column {
                id: colT; width: tabla.width
                Repeater {
                    model: raiz.vueltas
                    Item {
                        required property var modelData
                        required property int index
                        width: colT.width; height: 40
                        Rectangle { anchors.fill: parent; radius: 8; color: Qt.rgba(244 / 255, 197 / 255, 49 / 255, 0.18); visible: index === raiz.lap }
                        Row {
                            anchors.verticalCenter: parent.verticalCenter; x: 6
                            readonly property real u: (colT.width - 12) / 10
                            Text { width: parent.u * 1.6; text: raiz.nombreV(modelData); color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 24 } }
                            Row { width: parent.u * 3; spacing: 8
                                Rectangle { width: 10; height: 10; radius: 5; anchors.verticalCenter: parent.verticalCenter
                                            color: modelData.interrumpida ? "#8a929b" : modelData.medida ? "#d23b2c" : Tema.bien }
                                Sub { anchors.verticalCenter: parent.verticalCenter; tam: 12; text: modelData.interrumpida ? "interrumpida" : modelData.medida ? "medida" : "práctica" } }
                            Text { width: parent.u * 2.2; text: Formato.tiempo(modelData.realMs); color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 24 } }
                            Text { width: parent.u * 2; text: modelData.difMs === undefined ? "–" : Formato.dif(modelData.difMs)
                                   color: modelData.difMs === undefined ? Tema.tintaSuave : raiz.colorDif(modelData.difMs)
                                   font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 24 } }
                            Text { width: parent.u * 1.2; horizontalAlignment: Text.AlignRight
                                   text: modelData.velPromKmh !== null && modelData.velPromKmh !== undefined ? String(modelData.velPromKmh).replace(".", ",") : "–"
                                   color: Tema.tintaSuave; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 20 } }
                        }
                        MouseArea { anchors.fill: parent; enabled: !!modelData.m; onClicked: raiz.lap = index }
                    }
                }
                Item {
                    width: colT.width; height: 46
                    readonly property var med: raiz.vueltas.filter(v => v.medida && v.difMs !== undefined)
                    visible: med.length > 0
                    Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.2) }
                    Sub { x: 6; anchors.verticalCenter: parent.verticalCenter; tam: 13; text: "Error total · " + parent.med.length + (parent.med.length === 1 ? " medida" : " medidas") }
                    Text { anchors { right: parent.right; rightMargin: 6; verticalCenter: parent.verticalCenter }
                           text: raiz.c.errorTotal || ""; color: Tema.acento; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 26 } }
                }
            }
        }
    }
}
