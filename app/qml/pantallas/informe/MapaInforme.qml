import QtQuick
import Betometro

// Mapa del informe: la pista y el recorrido de la vuelta elegida, con color según velocidad, aceleración o
// adelanto (Vel./Acel./Adel.). En recorrido libre, la llegada y las marcas numeradas. Con video, el punto
// amarillo es donde va el auto y tocar el recorrido lleva el video ahí.
Tarjeta {
    id: caja
    property bool h: true
    property bool vv: false                         // se está viendo el video (la tarjeta se achica)
    property var inf: ({})                          // Datos.informe(id): pista, cerrada
    property var vu: null                           // la vuelta elegida (inf.vueltas[lap])
    property bool rec: false                        // recorrido libre
    property var marcas: []
    property real dist: 0
    property real sug: 0                            // velocidad sugerida (km/h), 0 si no hay distancia
    property int lap: -1
    property string modo: "vel"                     // vel, acel, ade
    property var auto: ({ lap: -1, x: 0, y: 0 })    // reproductor.e: dónde está el auto en el video
    signal elegirModo(string m)
    signal irVideo(int lap, real t)
    function repintar() { mapa.requestPaint(); }
    onLapChanged: repintar()
    onModoChanged: repintar()
    function colorPunto(p) {
        if (!p[7]) return "#555555";
        if (modo === "acel") { const a = p[3]; return a > 0.8 ? Tema.bien : a > 0.25 ? "#8fe3b0" : a < -0.8 ? "#d23b2c" : a < -0.25 ? "#f08a96" : "#9aa3ad"; }
        if (modo === "ade") { const s = Math.abs(p[4]); return s < 1 ? Tema.bien : s < 3 ? Tema.acento : "#d23b2c"; }
        if (!(sug > 0)) return Tema.bien;
        const d = p[2] - sug;
        return d > 6 ? "#d23b2c" : d > 2 ? Tema.acento : d >= -2 ? Tema.bien : d >= -6 ? "#7fd3ff" : "#3a8de0";
    }

    oscura: true
    x: !caja.vv ? (caja.h ? 24 : 20) : (caja.h ? 648 : 20)
    y: !caja.vv ? (caja.h ? 110 : 180) : (caja.h ? 110 : 600)
    width: !caja.vv ? (caja.h ? 600 : parent.width - 40) : (caja.h ? 608 : 330)
    height: !caja.vv ? (caja.h ? 590 : 470) : (caja.h ? 230 : 280)
    // Hasta los botones Vel./Acel./Adel. y con «…» si no entra (Deportivo y Cabina, 08-5)
    Sub { x: 18; y: 16; width: botonesModo.x - 18 - 10; elide: Text.ElideRight
          visible: !caja.vv || caja.h; text: caja.vv ? "Dónde está · tocá y el video va ahí" : caja.modo === "vel" ? (caja.rec ? "Recorrido · color = velocidad" : "Recorrido de la vuelta · color = velocidad") : caja.modo === "acel" ? "Recorrido · color = aceleración" : "Recorrido · color = adelanto"; tam: 12 }
    Row {
        id: botonesModo
        anchors { right: parent.right; rightMargin: 14; top: parent.top; topMargin: 10 }
        spacing: 6
        Repeater {
            model: [{ t: "Vel.", m: "vel" }, { t: "Acel.", m: "acel" }].concat(caja.dist > 0 ? [{ t: "Adel.", m: "ade" }] : [])
            Rectangle {
                required property var modelData
                width: tm.implicitWidth + 18; height: 26; radius: 13
                color: caja.modo === modelData.m ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
                border { width: 1; color: Tema.bordeCromo }
                Text { id: tm; anchors.centerIn: parent; text: modelData.t; color: caja.modo === modelData.m ? Tema.acentoTinta : Tema.tinta
                       font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 16 } }
                MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: caja.elegirModo(modelData.m) }
            }
        }
    }
    Canvas {
        id: mapa
        anchors { fill: parent; topMargin: 44; margins: 14 }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        readonly property real k: Math.min(width / 364, height / 330)
        readonly property real ox: (width - 364 * k) / 2
        readonly property real oy: (height - 330 * k) / 2
        // Con video: el punto amarillo es donde va el auto, y tocar el recorrido lleva el video ahí
        Rectangle {
            visible: caja.vv && caja.auto.lap >= 0
            width: 22; height: 22; radius: 11
            x: mapa.ox + caja.auto.x * mapa.k - 11; y: mapa.oy + caja.auto.y * mapa.k - 11
            color: Tema.amarillo; border { width: 3; color: "#111111" }
        }
        MouseArea {
            anchors.fill: parent
            enabled: caja.vv
            onClicked: (m) => {
                const M = caja.vu && caja.vu.m ? caja.vu.m : [];
                let mejor = null, dm = 1e9;
                for (const p of M) {
                    if (!p[7]) continue;
                    const dx = mapa.ox + p[5] * mapa.k - m.x, dy = mapa.oy + p[6] * mapa.k - m.y, dd = dx * dx + dy * dy;
                    if (dd < dm) { dm = dd; mejor = p; }
                }
                if (mejor && dm < 40 * 40) caja.irVideo(caja.lap, mejor[0]);
            }
        }
        onPaint: {
            const g = getContext("2d");
            g.reset();
            const P = caja.inf.pista || [];
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
            if (caja.inf.cerrada) g.closePath();
            g.strokeStyle = "rgba(0,0,0,0.45)"; g.lineWidth = 20 * k; g.stroke();
            g.strokeStyle = "#3a3f45"; g.lineWidth = 11 * k; g.stroke();
            const M = caja.vu && caja.vu.m ? caja.vu.m : [];
            g.lineWidth = 8 * k;
            for (let i = 1; i < M.length; i++) {
                if (!M[i][7] || !M[i - 1][7]) continue;
                g.beginPath(); g.moveTo(ox + M[i - 1][5] * k, oy + M[i - 1][6] * k); g.lineTo(ox + M[i][5] * k, oy + M[i][6] * k);
                g.strokeStyle = caja.colorPunto(M[i]); g.stroke();
            }
            g.beginPath(); g.arc(X(P[0]), Y(P[0]), 9 * k, 0, Math.PI * 2); g.fillStyle = "#ffffff"; g.fill();
            const izq = X(P[0]) > width * 0.7;   // cerca del borde derecho: el texto va a la izquierda
            g.fillStyle = Tema.tinta; g.font = "italic " + Math.round(11 * k) + "px '" + Tema.fuente800 + "'"; g.textAlign = izq ? "right" : "left";
            g.fillText("LARGADA", X(P[0]) + (izq ? -14 : 14) * k, Y(P[0]) + 4 * k);
            if (caja.rec) {   // recorrido libre: la llegada (cuadradito) y las marcas numeradas
                const F = P[P.length - 1], s = 8 * k;
                if (!caja.inf.cerrada) {
                    g.fillStyle = "#111111"; g.fillRect(X(F) - s, Y(F) - s, 2 * s, 2 * s);
                    g.strokeStyle = "#ffffff"; g.lineWidth = 2.5 * k; g.strokeRect(X(F) - s, Y(F) - s, 2 * s, 2 * s);
                    const izqF = X(F) > width * 0.7;
                    g.fillStyle = Tema.tinta; g.textAlign = izqF ? "right" : "left";
                    g.fillText("LLEGADA", X(F) + (izqF ? -14 : 14) * k, Y(F) + 4 * k);
                }
                g.textAlign = "center";
                g.font = "italic " + Math.round(10 * k) + "px '" + Tema.fuente800 + "'";
                for (const mk of caja.marcas) {
                    if (mk.x === undefined) continue;
                    g.beginPath(); g.arc(ox + mk.x * k, oy + mk.y * k, 9 * k, 0, Math.PI * 2);
                    g.fillStyle = Tema.amarillo; g.fill(); g.strokeStyle = "#111111"; g.lineWidth = 2 * k; g.stroke();
                    g.fillStyle = "#111111"; g.fillText(String(mk.n), ox + mk.x * k, oy + mk.y * k + 3.5 * k);
                }
            }
        }
    }
}
