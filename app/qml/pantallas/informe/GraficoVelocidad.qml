import QtQuick
import Betometro

// Gráfico del informe: la velocidad de la vuelta elegida a lo largo de la distancia, con la sugerida punteada.
// En recorrido libre, las marcas como rayas amarillas con su número.
Tarjeta { datos: true;
    id: caja
    property bool h: true
    property bool vv: false          // se está viendo el video: el gráfico no se muestra
    property bool rec: false         // recorrido libre
    property var inf: ({})           // Datos.informe(id): largadas
    property var vu: null            // la vuelta elegida (inf.vueltas[lap])
    property int lap: -1
    property var marcas: []
    property real sug: 0             // velocidad sugerida (km/h), 0 si no hay distancia
    property string modo: "vel"      // «comp»: la vuelta contra la modelo, superpuestas
    property var vm: null            // la vuelta modelo
    property string nombreModelo: ""
    property bool esModelo: false     // la vuelta elegida es la modelo
    readonly property bool comp: modo === "comp" && !rec && vm !== null
    onCompChanged: repintar()
    signal abrir()                   // doble toque: el gráfico a pantalla completa (1.0.30)
    function nombreV(v) { return "V" + v.n + (inf.largadas > 1 ? " L" + v.largada : ""); }
    function repintar() { grafico.requestPaint(); }
    onLapChanged: repintar()

    oscura: true
    visible: !caja.vv
    x: caja.h ? 648 : 20; y: caja.h ? 110 : 670
    width: caja.h ? 608 : parent.width - 40; height: caja.h ? 230 : 210
    Sub { x: 18; y: 16; text: caja.comp ? "Velocidad · " + (caja.vu ? caja.nombreV(caja.vu) : "") + (caja.esModelo ? " (modelo)" : " contra " + caja.nombreModelo + " (modelo, punteada)") : caja.rec ? "Velocidad km/h · a lo largo del recorrido" : "Velocidad km/h" + (caja.vu ? " · " + caja.nombreV(caja.vu) : "") + (caja.sug > 0 ? " · sugerida " + caja.sug.toFixed(1).replace(".", ",") : ""); tam: 12 }
    MouseArea {
        anchors.fill: parent
        onClicked: { if (unToque.running) { unToque.stop(); caja.abrir(); } else unToque.restart(); }
        Timer { id: unToque; interval: 280 }
    }
    Canvas {
        id: grafico
        anchors { fill: parent; topMargin: 40; margins: 16 }
        onWidthChanged: requestPaint()
        onPaint: {
            const g = getContext("2d");
            g.reset();
            const M = caja.vu && caja.vu.m ? caja.vu.m : [];
            if (!M.length) return;
            const L = caja.vu.L || 1;
            let vmax = caja.sug + 10;
            for (const p of M) if (p[7]) vmax = Math.max(vmax, p[2]);
            if (caja.comp && caja.vm.m) for (const p of caja.vm.m) if (p[7]) vmax = Math.max(vmax, p[2]);
            const esc = Math.max(20, Math.ceil(vmax / 10) * 10);
            const X = d => 30 + (width - 34) * Math.min(1, d / L), Y = v => height - 6 - (height - 12) * v / esc;
            g.strokeStyle = "rgba(255,255,255,0.12)"; g.lineWidth = 1;
            g.fillStyle = Tema.tintaSuave; g.font = "11px '" + Tema.fuente + "'"; g.textAlign = "right";
            for (let i = 0; i <= 4; i++) { const v = esc * i / 4; g.beginPath(); g.moveTo(30, Y(v)); g.lineTo(width - 4, Y(v)); g.stroke(); g.fillText(String(Math.round(v)), 24, Y(v) + 4); }
            if (caja.sug > 0 && !caja.comp) {
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
            if (caja.comp && !caja.esModelo && caja.vm.m) {   // la modelo, punteada (cada una a su largo)
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
                if (!abierto) { g.moveTo(X(p[1]), Y(p[2])); abierto = true; } else g.lineTo(X(p[1]), Y(p[2]));
            }
            g.stroke();
        }
    }
}
