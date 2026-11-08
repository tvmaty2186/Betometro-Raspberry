import QtQuick
import Betometro

// Comparar una vuelta contra la vuelta modelo (la medida más cerca del ideal), a pantalla completa (Matías,
// 08/11/2026, muestra docs/muestras/informe-gps/b.png): las dos velocidades superpuestas a lo largo de la vuelta,
// el tiempo ganado o perdido punto a punto, el mapa pintado donde se ganó (verde) o se perdió (rojo) y una tabla
// con 4 sectores iguales. Arriba se elige la vuelta. Se usa el recorrido grabado de cada vuelta (inf.vueltas[i].m:
// [t, d, kmh, acel, adel, x, y, ok]); cada vuelta se lleva a su largo (L) para que las dos midan lo mismo.
Item {
    id: raiz
    property var inf: ({})
    property string nombre: ""
    property int modelo: -1
    property int elegida: -1
    signal cerrar()
    readonly property var vueltas: inf.vueltas || []
    readonly property var candidatas: { const r = []; for (let i = 0; i < vueltas.length; ++i) if (i !== modelo && vueltas[i].m && vueltas[i].m.length > 4) r.push(i); return r; }
    function abrir(lap) { elegida = lap >= 0 && lap !== modelo && candidatas.indexOf(lap) >= 0 ? lap : (candidatas.length ? candidatas[0] : -1); }
    function nombreV(i) { const v = vueltas[i]; return v ? "V" + v.n + ((inf.largadas || 1) > 1 ? " L" + v.largada : "") : ""; }

    // Serie de una vuelta: fracción de la vuelta (0..1) → segundos, velocidad, punto del mapa. Empieza en 0 s y
    // termina en el tiempo del pulsador, así la diferencia total es la de la vuelta.
    function serie(i) {
        const v = vueltas[i];
        if (!v || !v.m) return null;
        const L = v.L || 1, f = [0], t = [0], k = [], x = [], y = [];
        for (const p of v.m) {
            if (!p[7]) continue;
            const fr = Math.min(1, Math.max(0, p[1] / L));
            if (fr <= f[f.length - 1] || p[0] <= t[t.length - 1]) continue;
            f.push(fr); t.push(p[0]); k.push([fr, p[2]]); x.push([fr, p[5], p[6]]);
        }
        f.push(1); t.push(Math.max(t[t.length - 1], v.realMs / 1000));
        return { f: f, t: t, k: k, xy: x, real: v.realMs };
    }
    function tEn(s, fr) {
        let a = 0, b = s.f.length - 1;
        while (b - a > 1) { const m = (a + b) >> 1; if (s.f[m] <= fr) a = m; else b = m; }
        const df = s.f[b] - s.f[a];
        return s.t[a] + (df > 0 ? (fr - s.f[a]) / df * (s.t[b] - s.t[a]) : 0);
    }
    readonly property var sA: elegida >= 0 ? serie(elegida) : null
    readonly property var sM: modelo >= 0 ? serie(modelo) : null
    function delta(fr) { return tEn(sA, fr) - tEn(sM, fr); }   // + = la elegida va atrasada contra la modelo
    readonly property var sectores: {
        if (!sA || !sM) return [];
        const r = [], L = vueltas[modelo].L || 0;
        for (let s = 0; s < 4; ++s) r.push({ de: Math.round(L * s / 4), a: Math.round(L * (s + 1) / 4), dt: delta((s + 1) / 4) - delta(s / 4) });
        return r;
    }
    function seg(x) { return (x >= 0 ? "+" : "−") + Math.abs(x).toFixed(1).replace(".", ",") + " s"; }
    onSAChanged: { graf.requestPaint(); mapa.requestPaint(); }
    onSMChanged: { graf.requestPaint(); mapa.requestPaint(); }

    Tarjeta { anchors.fill: parent; oscura: true }
    Sub { x: 24; y: 18; tam: 14; text: "Comparar · " + raiz.nombre + " · contra " + raiz.nombreV(raiz.modelo) + ", la vuelta modelo (la más cerca del ideal)" }
    // Elegir la vuelta
    Row {
        x: 24; y: 48; spacing: 8
        Text { anchors.verticalCenter: parent.verticalCenter; text: "Vuelta:"; color: Tema.tintaSuave; font { family: Tema.fuente600; italic: true; pixelSize: 16 } }
        Repeater {
            model: raiz.candidatas
            Rectangle {
                required property int modelData
                readonly property bool on: modelData === raiz.elegida
                width: tCh.implicitWidth + 26; height: 34; radius: 17
                color: on ? Tema.acento : "transparent"; border { width: 2; color: on ? Tema.acentoClaro : Tema.bordeCromo }
                Text { id: tCh; anchors.centerIn: parent; text: raiz.nombreV(modelData); color: parent.on ? Tema.acentoTinta : Tema.tinta
                       font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 17 } }
                MouseArea { anchors.fill: parent; anchors.margins: -4; onClicked: raiz.elegida = parent.modelData }
            }
        }
    }
    // Cerrar
    Rectangle {
        anchors { right: parent.right; rightMargin: 20; top: parent.top; topMargin: 14 }
        width: 62; height: 62; radius: 16; color: "transparent"; border { width: 3; color: Tema.bordeCromo }
        Text { anchors.centerIn: parent; text: "✕"; color: Tema.tinta; font { pixelSize: 34; weight: Font.Bold } }
        MouseArea { anchors.fill: parent; onClicked: raiz.cerrar() }
    }
    // Leyenda
    Text {
        x: 24; y: 92; textFormat: Text.StyledText
        readonly property var a: raiz.vueltas[raiz.elegida]
        readonly property var m: raiz.vueltas[raiz.modelo]
        text: !a || !m ? "" : "<font color='" + Tema.numero + "'>━ " + raiz.nombreV(raiz.elegida) + " · " + Formato.tiempo(a.realMs) + (a.difMs !== undefined ? " · " + Formato.dif(a.difMs) : "") + "</font>"
              + "&nbsp;&nbsp;&nbsp;<font color='" + Tema.acento + "'>┅ " + raiz.nombreV(raiz.modelo) + " modelo · " + Formato.tiempo(m.realMs) + (m.difMs !== undefined ? " · " + Formato.dif(m.difMs) : "") + "</font>"
        font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 17 }
    }

    // Gráficos: arriba las velocidades, abajo el tiempo ganado / perdido
    Canvas {
        id: graf
        x: 10; y: 118; width: 820; height: parent.height - 130
        Connections { target: Tema; function onNombreChanged() { graf.requestPaint(); } function onSolChanged() { graf.requestPaint(); } }
        onPaint: {
            const g = getContext("2d"); g.reset();
            const A = raiz.sA, M = raiz.sM;
            if (!A || !M) return;
            const x0 = 60, w = width - x0 - 20, X = fr => x0 + fr * w;
            const yv0 = 40, yv1 = height * 0.56, yd0 = height * 0.66, yd1 = height - 30;
            // escala de velocidad
            let vmin = 1e9, vmax = 0;
            for (const s of [A, M]) for (const p of s.k) { vmin = Math.min(vmin, p[1]); vmax = Math.max(vmax, p[1]); }
            vmin = Math.floor(vmin / 10) * 10; vmax = Math.ceil(vmax / 10) * 10; if (vmax <= vmin) vmax = vmin + 10;
            const YV = v => yv1 - (v - vmin) / (vmax - vmin) * (yv1 - yv0);
            g.font = "12px '" + Tema.fuente600 + "'"; g.fillStyle = String(Tema.tintaSuave); g.strokeStyle = "rgba(255,255,255,0.12)"; g.lineWidth = 1;
            for (let v = vmin; v <= vmax; v += 10) { g.beginPath(); g.moveTo(x0, YV(v)); g.lineTo(x0 + w, YV(v)); g.stroke(); g.fillText(String(v), 22, YV(v) + 4); }
            // sectores
            g.setLineDash([6, 6]); g.strokeStyle = String(Tema.acento); g.globalAlpha = 0.5;
            for (let s = 1; s < 4; ++s) { g.beginPath(); g.moveTo(X(s / 4), 22); g.lineTo(X(s / 4), yd1); g.stroke(); }
            g.globalAlpha = 1; g.setLineDash([]);
            g.font = "bold italic 17px '" + Tema.fuente800 + "'"; g.fillStyle = String(Tema.acento); g.textAlign = "center";
            for (let s = 0; s < 4; ++s) g.fillText("S" + (s + 1), X((s + 0.5) / 4), 22);
            g.textAlign = "left";
            const linea = (s, col, punteada) => {
                g.beginPath(); g.setLineDash(punteada ? [9, 6] : []);
                s.k.forEach((p, i) => i ? g.lineTo(X(p[0]), YV(p[1])) : g.moveTo(X(p[0]), YV(p[1])));
                g.strokeStyle = col; g.lineWidth = 3; g.stroke(); g.setLineDash([]);
            };
            linea(M, String(Tema.acento), true);
            linea(A, String(Tema.numero), false);
            // tiempo ganado / perdido (arriba = adelantado contra la modelo)
            const N = 300, D = [];
            let dmax = 0.5;
            for (let i = 0; i <= N; ++i) { const d = raiz.delta(i / N); D.push(d); dmax = Math.max(dmax, Math.abs(d)); }
            dmax = Math.ceil(dmax * 2) / 2;
            const ym = (yd0 + yd1) / 2, YD = d => ym + d / dmax * (yd1 - yd0) / 2;   // d > 0 (atrasado) va para abajo
            g.font = "italic 15px '" + Tema.fuente600 + "'"; g.fillStyle = String(Tema.tinta);
            g.fillText("TIEMPO GANADO / PERDIDO CONTRA " + raiz.nombreV(raiz.modelo) + " (SEGUNDOS)", x0, yd0 - 14);
            g.fillStyle = "rgba(0,0,0,0.2)"; g.fillRect(x0, yd0, w, yd1 - yd0);
            g.strokeStyle = "rgba(255,255,255,0.5)"; g.beginPath(); g.moveTo(x0, ym); g.lineTo(x0 + w, ym); g.stroke();
            g.font = "12px '" + Tema.fuente600 + "'"; g.fillStyle = String(Tema.tintaSuave);
            g.fillText("+" + String(dmax).replace(".", ","), 14, YD(-dmax) + 10); g.fillText("0", 30, ym + 4); g.fillText("−" + String(dmax).replace(".", ","), 14, YD(dmax) - 2);
            g.font = "italic 13px '" + Tema.fuente600 + "'";
            g.fillStyle = String(Tema.bien); g.fillText("arriba: adelantado", x0 + 8, yd0 + 16);
            g.fillStyle = String(Tema.rojo); g.fillText("abajo: atrasado", x0 + 8, yd1 - 8);
            g.beginPath(); D.forEach((d, i) => i ? g.lineTo(X(i / N), YD(d)) : g.moveTo(X(0), YD(d)));
            g.strokeStyle = String(D[N] > 0 ? Tema.rojo : Tema.bien); g.lineWidth = 3.5; g.stroke();
            g.font = "bold italic 18px '" + Tema.fuente800 + "'"; g.fillStyle = g.strokeStyle; g.textAlign = "right";
            g.fillText(raiz.seg(-D[N]), x0 + w - 4, YD(D[N]) + (D[N] > 0 ? -10 : 22));
        }
    }

    // Mapa: dónde se fue la diferencia
    Tarjeta {
        x: parent.width - 410; y: 96; width: 390; height: 250
        Sub { x: 14; y: 10; tam: 12; text: "Dónde se fue la diferencia" }
        Canvas {
            id: mapa
            anchors { fill: parent; topMargin: 30; bottomMargin: 26; leftMargin: 10; rightMargin: 10 }
            onPaint: {
                const g = getContext("2d"); g.reset();
                const P = raiz.inf.pista || [], A = raiz.sA;
                if (!P.length || !A || !raiz.sM) return;
                const k = Math.min(width / 364, height / 330), ox = (width - 364 * k) / 2, oy = (height - 330 * k) / 2;
                const X = x => ox + x * k, Y = y => oy + y * k;
                g.lineJoin = "round"; g.lineCap = "round";
                g.beginPath(); g.moveTo(X(P[0][0]), Y(P[0][1])); for (const p of P) g.lineTo(X(p[0]), Y(p[1]));
                if (raiz.inf.cerrada) g.closePath();
                g.strokeStyle = "rgba(0,0,0,0.45)"; g.lineWidth = 18 * k; g.stroke();
                g.lineWidth = 8 * k;
                for (let i = 1; i < A.xy.length; ++i) {
                    const a = A.xy[i - 1], b = A.xy[i];
                    const dd = raiz.delta(b[0]) - raiz.delta(a[0]), v = dd / Math.max(0.001, b[0] - a[0]) / 100;   // s cada 1 % de vuelta
                    g.strokeStyle = v > 0.01 ? String(Tema.rojo) : v < -0.01 ? String(Tema.bien) : "rgba(255,255,255,0.55)";
                    g.beginPath(); g.moveTo(X(a[1]), Y(a[2])); g.lineTo(X(b[1]), Y(b[2])); g.stroke();
                }
                g.fillStyle = "#fff"; g.beginPath(); g.arc(X(P[0][0]), Y(P[0][1]), 6, 0, 2 * Math.PI); g.fill();
            }
        }
        Text { x: 14; anchors.bottom: parent.bottom; anchors.bottomMargin: 6; textFormat: Text.StyledText
               text: "<font color='" + Tema.rojo + "'>rojo</font> perdió · <font color='" + Tema.bien + "'>verde</font> ganó contra " + raiz.nombreV(raiz.modelo)
               color: Tema.tintaSuave; font { family: Tema.fuente600; italic: true; pixelSize: 13 } }
    }
    // Sectores
    Tarjeta {
        x: parent.width - 410; y: 360; width: 390; height: parent.height - 380
        Sub { x: 18; y: 12; tam: 12; text: "Por sector · " + raiz.nombreV(raiz.elegida) + " contra " + raiz.nombreV(raiz.modelo) }
        Column {
            x: 18; y: 44; width: parent.width - 36; spacing: 8
            Repeater {
                model: raiz.sectores
                Item {
                    required property var modelData
                    required property int index
                    width: parent.width; height: 34
                    readonly property bool lento: modelData.dt > 0.05
                    readonly property bool rapido: modelData.dt < -0.05
                    Text { anchors.verticalCenter: parent.verticalCenter; text: "S" + (index + 1); color: Tema.acento
                           font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 22 } }
                    Text { x: 46; anchors.verticalCenter: parent.verticalCenter; text: modelData.de + " a " + modelData.a + " m"; color: Tema.tintaSuave
                           font { family: Tema.fuente600; pixelSize: 15 } }
                    Text { anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                           text: raiz.seg(modelData.dt) + (parent.lento ? " más lento" : parent.rapido ? " más rápido" : " igual")
                           color: parent.lento ? Tema.rojo : parent.rapido ? Tema.bien : Tema.tinta
                           font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 21 } }
                }
            }
            Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.25) }
            Item {
                width: parent.width; height: 40
                readonly property real dt: raiz.sA && raiz.sM ? (raiz.sA.real - raiz.sM.real) / 1000 : 0
                Text { anchors.verticalCenter: parent.verticalCenter; text: "Vuelta"; color: Tema.tinta
                       font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 23 } }
                Text { anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                       text: Math.abs(parent.dt).toFixed(1).replace(".", ",") + " s " + (parent.dt > 0.05 ? "más lenta" : parent.dt < -0.05 ? "más rápida" : "igual")
                       color: parent.dt > 0.05 ? Tema.rojo : parent.dt < -0.05 ? Tema.bien : Tema.tinta
                       font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 23 } }
            }
        }
    }
}
