import QtQuick
import Betometro

// Gráfico del informe: la velocidad de la vuelta elegida a lo largo de la distancia, con la sugerida punteada.
// En recorrido libre, las marcas como rayas amarillas con su número. En «Comp.», la vuelta y la modelo superpuestas.
// Con el video (Matías, 08/11/2026): va al lado del video en lugar del mapa (Mapa | Gráfico), muestra lo que digan
// los botones (velocidad, aceleración, adelanto o Comp.), una raya amarilla marca dónde va el video y un toque
// lleva el video a ese punto.
Tarjeta { datos: true;
    id: caja
    property bool h: true
    property bool vv: false          // se está viendo el video
    property string lado: "mapa"     // con el video: el gráfico se ve solo si se eligió «Gráfico»
    property bool rec: false         // recorrido libre
    property var inf: ({})           // Datos.informe(id): largadas
    property var vu: null            // la vuelta elegida (inf.vueltas[lap])
    property int lap: -1
    property var marcas: []
    property real sug: 0             // velocidad sugerida (km/h), 0 si no hay distancia
    property real dist: 0
    property string modo: "vel"      // vel, acel, ade, comp
    property var vm: null            // la vuelta modelo
    property string nombreModelo: ""
    property bool esModelo: false     // la vuelta elegida es la modelo
    property var auto: ({ lap: -1 })  // reproductor.e: dónde va el video
    readonly property bool comp: modo === "comp" && !rec && vm !== null
    // Sin el video el gráfico chico es siempre de velocidad (o Comp.); con el video, lo que digan los botones
    readonly property string que: comp ? "comp" : vv && (modo === "acel" || modo === "ade") ? modo : "vel"
    onQueChanged: repintar()
    signal abrir()                   // doble toque: el gráfico a pantalla completa (1.0.30)
    signal elegirModo(string m)
    signal elegirLado(string l)
    signal irVideo(int lap, real t)
    function nombreV(v) { return "V" + v.n + (inf.largadas > 1 ? " L" + v.largada : ""); }
    function repintar() { grafico.requestPaint(); }
    onLapChanged: repintar()
    // Posición en el gráfico (la misma cuenta que el dibujo)
    function xDe(d) { const L = vu && vu.L ? vu.L : 1; return grafico.x + 30 + (grafico.width - 34) * Math.max(0, Math.min(1, d / L)); }

    oscura: true
    visible: !caja.vv || caja.lado === "grafico"
    x: !caja.vv ? (caja.h ? 648 : 20) : (caja.h ? 648 : 20)
    y: !caja.vv ? (caja.h ? 110 : 670) : (caja.h ? 110 : 600)
    width: !caja.vv ? (caja.h ? 608 : parent.width - 40) : (caja.h ? 608 : 330)
    height: !caja.vv ? (caja.h ? 230 : 210) : (caja.h ? 230 : 280)
    Sub {
        x: caja.vv ? (caja.h ? lado.x + lado.width + 12 : 18) : 18; y: 16; tam: 12
        visible: !caja.vv || caja.h
        width: (caja.vv ? botonesModo.x : caja.width - 18) - x - 10; elide: Text.ElideRight
        text: caja.que === "comp" ? "Velocidad · " + (caja.vu ? caja.nombreV(caja.vu) : "") + (caja.esModelo ? " (modelo)" : " contra " + caja.nombreModelo + " (modelo, punteada)")
              : caja.que === "acel" ? "Aceleración m/s²" + (caja.vu ? " · " + caja.nombreV(caja.vu) : "")
              : caja.que === "ade" ? "Adelanto en segundos" + (caja.vu ? " · " + caja.nombreV(caja.vu) : "")
              : caja.rec ? "Velocidad km/h · a lo largo del recorrido"
              : "Velocidad km/h" + (caja.vu ? " · " + caja.nombreV(caja.vu) : "") + (caja.sug > 0 ? " · sugerida " + caja.sug.toFixed(1).replace(".", ",") : "")
    }
    MouseArea {
        anchors.fill: parent
        property real xToque: 0
        onClicked: (m) => { if (unToque.running) { unToque.stop(); caja.abrir(); } else { xToque = m.x; unToque.restart(); } }
        // Con el video, un toque lleva el video a ese punto de la vuelta
        Timer {
            id: unToque; interval: 280
            onTriggered: {
                if (!caja.vv || !caja.vu || !caja.vu.m) return;
                let mejor = null, dm = 1e9;
                for (const p of caja.vu.m) { const dd = Math.abs(caja.xDe(p[1]) - parent.xToque); if (dd < dm) { dm = dd; mejor = p; } }
                if (mejor && dm < 30) caja.irVideo(caja.lap, mejor[0]);
            }
        }
    }
    ModosInforme {
        id: botonesModo
        visible: caja.vv
        anchors { right: parent.right; rightMargin: 14; top: parent.top; topMargin: 10 }
        modo: caja.modo; dist: caja.dist; conComp: !!caja.vm && !caja.rec
        onElegirModo: (m) => caja.elegirModo(m)
    }
    LadoVideo {
        id: lado
        visible: caja.vv
        x: 14; y: caja.h ? 9 : 42
        lado: "grafico"
        onElegir: (l) => caja.elegirLado(l)
    }
    Canvas {
        id: grafico
        anchors { fill: parent; topMargin: caja.vv && !caja.h ? 76 : 40; margins: 16 }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const g = getContext("2d");
            g.reset();
            const M = caja.vu && caja.vu.m ? caja.vu.m : [];
            if (!M.length) return;
            const L = caja.vu.L || 1;
            const X = d => 30 + (width - 34) * Math.min(1, d / L);
            // Qué se dibuja: velocidad (de 0 para arriba) o aceleración / adelanto (para los dos lados del cero)
            const col = caja.que === "acel" ? 3 : caja.que === "ade" ? 4 : 2;
            const doble = col !== 2;
            let vmax = doble ? 1 : caja.sug + 10;
            for (const p of M) if (p[7]) vmax = Math.max(vmax, doble ? Math.abs(p[col]) : p[col]);
            if (caja.que === "comp" && caja.vm.m) for (const p of caja.vm.m) if (p[7]) vmax = Math.max(vmax, p[2]);
            const esc = doble ? Math.ceil(vmax) : Math.max(20, Math.ceil(vmax / 10) * 10);
            const Y = doble ? v => height / 2 - (height / 2 - 6) * v / esc : v => height - 6 - (height - 12) * v / esc;
            g.strokeStyle = (Tema.datosClaros ? "rgba(0,0,0,0.12)" : "rgba(255,255,255,0.12)"); g.lineWidth = 1;
            g.fillStyle = Tema.tintaSuave; g.font = "11px '" + Tema.fuente + "'"; g.textAlign = "right";
            for (let i = 0; i <= 4; i++) {
                const v = doble ? esc * (2 - i) / 2 : esc * i / 4;
                g.beginPath(); g.moveTo(30, Y(v)); g.lineTo(width - 4, Y(v)); g.stroke();
                g.fillText(doble ? (v > 0 ? "+" : v < 0 ? "−" : "") + String(Math.abs(v)).replace(".", ",") : String(Math.round(v)), 24, Y(v) + 4);
            }
            if (doble) {   // el cero, más marcado
                g.strokeStyle = (Tema.datosClaros ? "rgba(0,0,0,0.45)" : "rgba(255,255,255,0.45)"); g.beginPath(); g.moveTo(30, Y(0)); g.lineTo(width - 4, Y(0)); g.stroke();
            } else if (caja.sug > 0 && caja.que !== "comp") {
                g.strokeStyle = Tema.acento; g.lineWidth = 2; g.setLineDash([8, 6]);
                g.beginPath(); g.moveTo(30, Y(caja.sug)); g.lineTo(width - 4, Y(caja.sug)); g.stroke(); g.setLineDash([]);
            }
            if (caja.rec) {   // las marcas, como rayas amarillas con su número
                g.strokeStyle = Tema.acento; g.lineWidth = 1.5; g.setLineDash([4, 4]);
                g.fillStyle = Tema.acento; g.textAlign = "left"; g.font = "bold 12px '" + Tema.fuente800 + "'";
                for (const mk of caja.marcas) {
                    const x = X(mk.m || 0);
                    g.beginPath(); g.moveTo(x, 4); g.lineTo(x, height - 6); g.stroke();
                    g.fillText(String(mk.n), x + 4, 14);
                }
                g.setLineDash([]);
            }
            if (caja.que === "comp" && !caja.esModelo && caja.vm.m) {   // la modelo, punteada (cada una a su largo)
                const Lm = caja.vm.L || 1, Xm = d => 30 + (width - 34) * Math.min(1, d / Lm);
                g.strokeStyle = Tema.acento; g.lineWidth = 2.5; g.setLineDash([9, 6]);
                let ab = false;
                g.beginPath();
                for (const p of caja.vm.m) {
                    if (!p[7]) { ab = false; continue; }
                    if (!ab) { g.moveTo(Xm(p[1]), Y(p[2])); ab = true; } else g.lineTo(Xm(p[1]), Y(p[2]));
                }
                g.stroke(); g.setLineDash([]);
            }
            g.strokeStyle = Tema.numero; g.lineWidth = 3.5; g.lineJoin = "round";
            let abierto = false;
            g.beginPath();
            for (const p of M) {
                if (!p[7]) { abierto = false; continue; }
                if (!abierto) { g.moveTo(X(p[1]), Y(p[col])); abierto = true; } else g.lineTo(X(p[1]), Y(p[col]));
            }
            g.stroke();
        }
    }
    // Dónde va el video: una raya que se mueve sola (no se redibuja el gráfico)
    Rectangle {
        visible: caja.vv && caja.auto.lap === caja.lap && caja.auto.d !== undefined
        x: caja.xDe(caja.auto.d || 0) - 1.5; y: grafico.y; width: 3; height: grafico.height
        color: Tema.acento
    }
}
