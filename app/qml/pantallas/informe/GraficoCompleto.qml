import QtQuick
import Betometro

// Gráfico del informe a pantalla completa (1.0.30, muestra docs/muestras/1.0.30/graficos/1-velocidad-pi.png):
// toda la carrera a lo largo de la distancia, con zoom. Cuadrícula fina cada 100 m y cada 5 km/h (más marcada
// cada 500 m y cada 10), todas las lecturas sin suavizar con color por tramo, la sugerida punteada, una raya en
// cada vuelta con «V3 · MEDIDA · 2:31:5 · +1,5 s», MÁX/MÍN/PROM del tramo visible, Vel./Acel./Adel., X.
// Toque = elige el punto (globito); doble toque = acerca ahí; pellizco = zoom; arrastrar = recorrer; abajo la
// barrita con toda la carrera y el recuadro de lo visible. Se dibuja con Canvas solo lo visible y se repinta
// solo al cambiar el zoom, la posición, el modo o el punto elegido (Pi 3).
Tarjeta { datos: true;
    id: caja
    property var inf: ({})           // Datos.informe(id)
    property string nombre: ""
    property bool rec: false         // recorrido libre
    property var marcas: []
    property real dist: 0
    property real sug: 0             // velocidad sugerida (km/h), 0 si no hay distancia
    property string modo: "vel"      // vel, acel, ade
    signal cerrar()

    oscura: true
    readonly property var vueltas: inf.vueltas || []
    // Tramos: cada vuelta con traza, con su distancia acumulada en la carrera
    readonly property var tramos: {
        const t = []; let acum = 0;
        for (let i = 0; i < vueltas.length; i++) {
            const v = vueltas[i];
            if (!v.m || !v.m.length) continue;
            t.push({ i: i, v: v, d0: acum, L: v.L || 1 });
            acum += v.L || 1;
        }
        return t;
    }
    readonly property real total: tramos.length ? tramos[tramos.length - 1].d0 + tramos[tramos.length - 1].L : 1
    // Promedio de toda la carrera: el color de la velocidad cuando no hay sugerida (recorrido libre)
    readonly property real prom: {
        let s = 0, n = 0;
        for (const t of tramos) for (const p of t.v.m) if (p[7]) { s += p[2]; n++; }
        return n ? s / n : 0;
    }
    readonly property real ref: sug > 0 ? sug : prom
    property real de: 0              // tramo visible, en metros de la carrera
    property real a: 1
    readonly property real span: a - de
    property var sel: null           // punto elegido: { t (tramo), p (lectura), d (m de la carrera) }
    // lo que se calcula al cambiar el tramo visible (recalcular): escala y máximos
    property var esc: ({ yA: 0, yB: 1, fino: 5, medio: 10 })
    property string txtMax: "–"
    property string txtMin: "–"
    property string txtProm: "–"
    property string txtVueltas: ""

    // Caja del gráfico
    readonly property real x0: 80
    readonly property real y0: 104
    readonly property real x1: width - 40
    readonly property real y1: height - 166
    readonly property real bx0: 80
    readonly property real by0: height - 112
    readonly property real bx1: width - 40
    readonly property real by1: height - 56
    function ex(d) { return x0 + (x1 - x0) * (d - de) / span; }
    function dEn(x) { return de + (x - x0) / (x1 - x0) * span; }
    function valor(p) { return modo === "vel" ? p[2] : modo === "acel" ? p[3] : p[4]; }
    function ey(v) { return y1 - (y1 - y0) * (v - esc.yA) / (esc.yB - esc.yA); }
    function color(p) {
        if (!p[7]) return "#555555";
        if (modo === "acel") { const ac = p[3]; return ac > 0.8 ? Tema.bien : ac > 0.25 ? "#8fe3b0" : ac < -0.8 ? "#d23b2c" : ac < -0.25 ? "#f08a96" : "#9aa3ad"; }
        if (modo === "ade") { const s = Math.abs(p[4]); return s < 1 ? Tema.bien : s < 3 ? Tema.acento : "#d23b2c"; }
        const d = p[2] - ref;
        return d > 6 ? "#d23b2c" : d > 2 ? Tema.acento : d >= -2 ? Tema.bien : d >= -6 ? "#7fd3ff" : "#3a8de0";
    }
    function nombreV(v) { return "V" + v.n + (inf.largadas > 1 ? " L" + v.largada : ""); }
    function kmTxt(m, dec) { return (m / 1000).toFixed(dec).replace(".", ","); }
    function fmt(v, dec) { return v.toFixed(dec).replace(".", ","); }
    function fmtSigno(v, dec) { return (v > 0 ? "+" : v < 0 ? "−" : "±") + Math.abs(v).toFixed(dec).replace(".", ","); }

    // Abrir en una vuelta (lap < 0: toda la carrera)
    function abrir(lap) {
        sel = null;
        const t = tramos.find(x => x.i === lap);
        if (t && tramos.length > 1) { de = t.d0; a = t.d0 + t.L; } else { de = 0; a = total; }
        recalcular();
    }
    function ver(nDe, nSpan) {   // mover/zoom con límites: nunca más chico que 200 m ni más grande que todo
        const s = Math.max(Math.min(200, total), Math.min(total, nSpan));
        de = Math.max(0, Math.min(total - s, nDe));
        a = de + s;
        recalcular();
    }
    function acercar(x, factor) {   // zoom alrededor del punto x de la pantalla
        const dC = dEn(x), s = span / factor;
        ver(dC - (x - x0) / (x1 - x0) * s, s);
    }
    // Lecturas del tramo visible (con una de más a cada lado para que la línea llegue al borde)
    function visibles(margen) {
        const r = [];
        for (const t of tramos) {
            if (t.d0 + t.L < de - margen || t.d0 > a + margen) continue;
            const M = t.v.m;
            for (let k = 0; k < M.length; k++) {
                const d = t.d0 + M[k][1];
                if (d < de - margen) continue;
                if (d > a + margen) break;
                r.push({ t: t, p: M[k], d: d });
            }
        }
        return r;
    }
    onModoChanged: { recalcular(); }
    function recalcular() {
        const vis = visibles(0).filter(q => q.p[7]);
        let vmin = Infinity, vmax = -Infinity, s = 0;
        for (const q of vis) { const v = valor(q.p); vmin = Math.min(vmin, v); vmax = Math.max(vmax, v); s += v; }
        if (!vis.length) { vmin = 0; vmax = modo === "vel" ? 100 : 1; }
        const pr = vis.length ? s / vis.length : 0;
        let yA, yB, fino, medio;
        if (modo === "vel") {
            yA = Math.max(0, Math.floor((vmin - 4) / 5) * 5); yB = Math.ceil((vmax + 4) / 5) * 5;
            if (yB - yA < 20) yB = yA + 20;
            fino = 5; medio = 10;
            if (yB - yA > 80) { fino = 10; medio = 20; }
            txtMax = String(Math.round(vmax)); txtMin = String(Math.round(vmin)); txtProm = fmt(pr, 1);
        } else if (modo === "acel") {
            const am = Math.max(0.5, Math.ceil(Math.max(Math.abs(vmin), Math.abs(vmax)) * 2) / 2);
            yA = -am; yB = am; fino = 0.5; medio = 1;
            if (am > 4) { fino = 1; medio = 2; }
            txtMax = fmtSigno(vmax, 1); txtMin = fmtSigno(vmin, 1); txtProm = fmtSigno(pr, 2);
        } else {
            yA = Math.floor(vmin - 1); yB = Math.ceil(vmax + 1);
            if (yB - yA < 4) { yA -= 1; yB += 1; }
            fino = 0.5; medio = 1;
            if (yB - yA > 12) { fino = 1; medio = 5; }
            if (yB - yA > 60) { fino = 5; medio = 10; }
            txtMax = fmtSigno(vmax, 1); txtMin = fmtSigno(vmin, 1); txtProm = fmtSigno(pr, 1);
        }
        esc = { yA: yA, yB: yB, fino: fino, medio: medio };
        // qué vueltas se ven
        const tol = span * 0.02, vs = tramos.filter(t => t.d0 + t.L > de + tol && t.d0 < a - tol).map(t => t.v.n);
        txtVueltas = rec ? "" : vs.length === 1 ? "vuelta " + vs[0] : vs.length === 2 ? "vueltas " + vs[0] + " y " + vs[1] : vs.length ? "vueltas " + vs[0] + " a " + vs[vs.length - 1] : "";
        grafico.requestPaint();
        barrita.requestPaint();
    }
    function elegir(x) {   // el punto más cercano en distancia al toque
        const d = dEn(x), vis = visibles(0);
        let q = null;
        for (const w of vis) if (!q || Math.abs(w.d - d) < Math.abs(q.d - d)) q = w;
        sel = q;
        grafico.requestPaint();
    }
    readonly property string titulo: (modo === "vel" ? "Velocidad km/h" : modo === "acel" ? "Aceleración m/s²" : "Adelanto s")
        + " · " + nombre + (rec ? " · recorrido libre" : " · " + vueltas.length + (vueltas.length === 1 ? " vuelta" : " vueltas"))
        + (modo === "vel" && sug > 0 ? " · sugerida " + fmt(sug, 1) : modo === "vel" && prom > 0 ? " · promedio " + fmt(prom, 1) : "")

    Sub { x: 22; y: 18; width: botones.x - 40; elide: Text.ElideRight; text: caja.titulo; tam: 12 }
    Sub { x: 22; y: 60; color: Tema.acento; tam: 12
          text: "Tramo visible · km " + caja.kmTxt(caja.de, 1) + " a " + caja.kmTxt(caja.a, 1) + (caja.txtVueltas ? " · " + caja.txtVueltas : "") }
    // MÁX / MÍN / PROM del tramo visible, terminando antes de la X (en la muestra PROM quedaba debajo de la X)
    Row {
        anchors { right: cerrar.left; rightMargin: 20; top: parent.top; topMargin: 52 }
        spacing: 26
        Repeater {
            model: [{ e: "Máx", v: caja.txtMax }, { e: caja.modo === "acel" ? "Frenada" : "Mín", v: caja.txtMin }, { e: "Prom", v: caja.txtProm }]
            Row {
                required property var modelData
                spacing: 8
                Sub { text: parent.modelData.e; tam: 12; anchors.baseline: num.baseline }
                Text { id: num; text: parent.modelData.v; color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 26 } }
                Sub { text: caja.modo === "vel" ? "km/h" : caja.modo === "acel" ? "m/s²" : "s"; tam: 10; anchors.baseline: num.baseline }
            }
        }
    }
    // Vel. / Acel. / Adel. (más grandes que en la tarjeta: ahora se tocan a pantalla completa)
    Row {
        id: botones
        anchors { right: cerrar.left; rightMargin: 14; top: parent.top; topMargin: 16 }
        spacing: 8
        Repeater {
            model: [{ t: "Vel.", m: "vel" }, { t: "Acel.", m: "acel" }].concat(caja.dist > 0 ? [{ t: "Adel.", m: "ade" }] : [])
            Rectangle {
                required property var modelData
                width: tm.implicitWidth + 40; height: 40; radius: 20
                color: caja.modo === modelData.m ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
                border { width: 1; color: Tema.bordeCromo }
                Text { id: tm; anchors.centerIn: parent; text: parent.modelData.t; color: caja.modo === parent.modelData.m ? Tema.acentoTinta : Tema.tinta
                       font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 20 } }
                MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: caja.modo = parent.modelData.m }
            }
        }
    }
    // X para cerrar, como el Volver de la cabecera (64×60, radio 18)
    Rectangle {
        id: cerrar
        anchors { right: parent.right; rightMargin: 14; top: parent.top; topMargin: 10 }
        width: 64; height: 60; radius: 18
        color: Qt.rgba(0, 0, 0, 0.35); border { width: 3; color: Tema.bordeCromo }
        Rectangle { anchors.centerIn: parent; width: 30; height: 4; radius: 2; rotation: 45; color: Tema.tinta }
        Rectangle { anchors.centerIn: parent; width: 30; height: 4; radius: 2; rotation: -45; color: Tema.tinta }
        MouseArea { anchors.fill: parent; anchors.margins: -10; onClicked: caja.cerrar() }
    }

    Canvas {
        id: grafico
        anchors.fill: parent
        onPaint: {
            const g = getContext("2d");
            g.reset();
            const x0 = caja.x0, x1 = caja.x1, y0 = caja.y0, y1 = caja.y1, e = caja.esc;
            const X = d => caja.ex(d), Y = v => caja.ey(v);
            // cuadrícula: cada 100 m (fina) y 500 m (media); más espaciada si se ve mucho
            const pasos = [100, 200, 500, 1000, 2000, 5000, 10000];
            let paso = pasos[0];
            for (const p of pasos) { paso = p; if (caja.span / p <= 80) break; }
            const fino = "rgba(255,255,255,0.07)", medio = (Tema.datosClaros ? "rgba(0,0,0,0.16)" : "rgba(255,255,255,0.16)");
            g.lineWidth = 1;
            g.fillStyle = Tema.tintaSuave; g.font = "14px '" + Tema.fuente + "'"; g.textAlign = "center";
            for (let d = Math.ceil(caja.de / paso) * paso; d <= caja.a; d += paso) {
                const f = d % (paso * 5) === 0, x = X(d);
                g.strokeStyle = f ? medio : fino;
                g.beginPath(); g.moveTo(x, y0); g.lineTo(x, y1); g.stroke();
                if (f) g.fillText(d % 1000 === 0 ? caja.kmTxt(d, 0) + " km" : caja.kmTxt(d, 1), x, y1 + 20);
            }
            g.textAlign = "right";
            const dec = e.fino < 1 ? 1 : 0;
            for (let v = e.yA; v <= e.yB + 1e-6; v += e.fino) {
                const f = Math.abs(v / e.medio - Math.round(v / e.medio)) < 1e-6, y = caja.ey(v);
                g.strokeStyle = f ? medio : fino;
                g.beginPath(); g.moveTo(x0, y); g.lineTo(x1, y); g.stroke();
                if (f) g.fillText(caja.modo === "vel" ? String(Math.round(v)) : caja.fmtSigno(v, dec), x0 - 10, y + 4);
            }
            // la sugerida punteada (velocidad) o el cero (aceleración, adelanto)
            if (caja.modo === "vel" && caja.ref > e.yA && caja.ref < e.yB) {
                g.strokeStyle = Tema.acento; g.lineWidth = 2; g.setLineDash([8, 6]);
                g.beginPath(); g.moveTo(x0, caja.ey(caja.ref)); g.lineTo(x1, caja.ey(caja.ref)); g.stroke(); g.setLineDash([]);
                g.fillStyle = Tema.acento; g.font = "italic 15px '" + Tema.fuente600 + "'"; g.textAlign = "left";
                g.fillText((caja.sug > 0 ? "sugerida " : "promedio ") + caja.fmt(caja.ref, 1), x0 + 8, caja.ey(caja.ref) + 18);
            } else if (caja.modo !== "vel" && 0 > e.yA && 0 < e.yB) {
                g.strokeStyle = caja.modo === "ade" ? Tema.acento : (Tema.datosClaros ? "rgba(0,0,0,0.45)" : "rgba(255,255,255,0.45)"); g.lineWidth = 2;
                if (caja.modo === "ade") g.setLineDash([8, 6]);
                g.beginPath(); g.moveTo(x0, caja.ey(0)); g.lineTo(x1, caja.ey(0)); g.stroke(); g.setLineDash([]);
                if (caja.modo === "ade") { g.fillStyle = Tema.acento; g.font = "italic 15px '" + Tema.fuente600 + "'"; g.textAlign = "left"; g.fillText("en hora", x0 + 8, caja.ey(0) + 18); }
            }
            // cada vuelta: la raya al empezar y arriba su nombre, tipo, tiempo y error (recorrido libre: las marcas)
            g.lineWidth = 1.5;
            if (caja.rec) {
                g.strokeStyle = Tema.acento; g.setLineDash([5, 5]);
                g.fillStyle = Tema.acento; g.font = "italic 16px '" + Tema.fuente800 + "'"; g.textAlign = "left";
                for (const mk of caja.marcas) {
                    const d = mk.m || 0;
                    if (d < caja.de || d > caja.a) continue;
                    g.beginPath(); g.moveTo(X(d), y0); g.lineTo(X(d), y1); g.stroke();
                    const txt = "Marca " + mk.n + " · " + Formato.hms(mk.ms, true), cabe = X(d) + 8 + g.measureText(txt).width <= x1;
                    g.textAlign = cabe ? "left" : "right";   // pegada al borde derecho: del otro lado de la raya
                    g.fillText(txt, X(d) + (cabe ? 8 : -8), y0 + 22);
                }
                g.setLineDash([]);
            } else {
                for (const t of caja.tramos) {
                    if (t.d0 < caja.de - 1 || t.d0 > caja.a + 1) continue;
                    const x = X(t.d0);
                    g.strokeStyle = Tema.acento; g.setLineDash([5, 5]);
                    g.beginPath(); g.moveTo(x, y0); g.lineTo(x, y1); g.stroke(); g.setLineDash([]);
                    if (t.d0 >= caja.a - 1) continue;
                    const v = t.v;
                    g.fillStyle = Tema.acento; g.font = "italic 20px '" + Tema.fuente800 + "'"; g.textAlign = "left";
                    g.fillText(caja.nombreV(v), x + 8, y0 + 22);
                    let xx = x + 8 + g.measureText(caja.nombreV(v)).width + 12;
                    const tipo = v.interrumpida ? "INTERR." : v.medida ? "MEDIDA" : "LIBRE", cT = v.interrumpida ? "#b5bcc3" : v.medida ? Tema.mal : Tema.bien;
                    g.font = "italic 12px '" + Tema.fuente800 + "'";
                    const wT = g.measureText(tipo).width + 18;
                    g.strokeStyle = cT; g.lineWidth = 1.5;
                    g.beginPath(); g.roundedRect(xx, y0 + 7, wT, 20, 10, 10); g.stroke();
                    g.fillStyle = cT; g.textAlign = "center"; g.fillText(tipo, xx + wT / 2, y0 + 21);
                    xx += wT + 12;
                    g.font = "italic bold 22px '" + Tema.fuenteNum + "'"; g.textAlign = "left";
                    g.fillStyle = Tema.numero; g.fillText(Formato.tiempo(v.realMs), xx, y0 + 23);
                    xx += g.measureText(Formato.tiempo(v.realMs)).width + 12;
                    if (v.difMs !== undefined) {
                        const err = Math.abs(v.difMs) / 1000;
                        g.fillStyle = err <= 1 ? Tema.bien : err <= 3 ? Tema.acento : Tema.mal;
                        g.fillText(Formato.dif(v.difMs), xx, y0 + 23);
                    }
                }
            }
            // la traza, lectura por lectura (sin suavizar), con el color de cada tramo; solo lo visible
            g.save();
            g.beginPath(); g.rect(x0, y0, x1 - x0, y1 - y0); g.clip();
            const pts = caja.visibles(caja.span * 0.01 + 20);
            const salto = Math.max(1, Math.floor(pts.length / ((x1 - x0) * 2)));   // más lecturas que píxeles: se saltean
            g.lineWidth = 2.5; g.lineJoin = "round"; g.lineCap = "round";
            let col = null, abierto = false;
            for (let i = 0; i < pts.length; i += salto) {
                const q = pts[i];
                if (!q.p[7]) { if (abierto) g.stroke(); abierto = false; col = null; continue; }
                const c = caja.color(q.p), px = X(q.d), py = caja.ey(caja.valor(q.p));
                if (c !== col) {
                    if (abierto) { g.lineTo(px, py); g.stroke(); }
                    g.strokeStyle = c; g.beginPath(); g.moveTo(px, py); col = c; abierto = true;
                } else g.lineTo(px, py);
            }
            if (abierto) g.stroke();
            g.restore();
            // el punto elegido
            const s = caja.sel;
            if (s) {
                const px = X(s.d), py = caja.ey(caja.valor(s.p));
                g.strokeStyle = "rgba(255,255,255,0.8)"; g.lineWidth = 1.5;
                g.beginPath(); g.moveTo(px, y0); g.lineTo(px, y1); g.stroke();
                const gl = caja.posGlobito(px, py);
                g.strokeStyle = Tema.bordeCromo; g.lineWidth = 2;
                g.beginPath(); g.moveTo(gl.izq ? gl.x + gl.w : gl.x, gl.y + 52); g.lineTo(px + (gl.izq ? -10 : 10), py); g.stroke();
                g.beginPath(); g.arc(px, py, 8, 0, Math.PI * 2); g.fillStyle = "#ffffff"; g.fill();
                g.strokeStyle = "#111111"; g.lineWidth = 3; g.stroke();
                globito.x = gl.x; globito.y = gl.y;
            }
        }
    }
    // Dónde va el globito: a la izquierda del punto; si no entra, a la derecha
    function posGlobito(px, py) {
        const w = 262, h = 104;
        const izq = px - 290 >= x0 - 30;
        return { x: izq ? px - 290 : px + 28, y: Math.max(y0 - 10, Math.min(y1 - h, py - 40)), w: w, h: h, izq: izq };
    }
    // Globito del punto tocado: valor, vuelta, km y tiempo de la vuelta, adelanto en ese punto
    Rectangle {
        id: globito
        visible: !!caja.sel
        width: 262; height: 104; radius: 16
        color: Qt.rgba(8 / 255, 25 / 255, 14 / 255, 0.94); border { width: 2; color: Tema.bordeCromo }
        readonly property var p: caja.sel ? caja.sel.p : [0, 0, 0, 0, 0, 0, 0, 0]
        readonly property var t: caja.sel ? caja.sel.t : null
        readonly property real ade: p[4]
        Row {
            x: 14; y: 8; spacing: 4
            Text { id: numG; text: caja.modo === "vel" ? String(Math.round(globito.p[2])) : caja.modo === "acel" ? caja.fmtSigno(globito.p[3], 1) : caja.fmtSigno(globito.p[4], 1)
                   color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 36 } }
            Text { anchors.baseline: numG.baseline; text: caja.modo === "vel" ? "km/h" : caja.modo === "acel" ? "m/s²" : "s"
                   color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 20 } }
        }
        Rectangle {
            visible: !caja.rec && !!globito.t
            anchors { right: parent.right; rightMargin: 14; top: parent.top; topMargin: 12 }
            width: tV.implicitWidth + 20; height: 24; radius: 12; color: Tema.acento
            Text { id: tV; anchors.centerIn: parent; text: globito.t ? caja.nombreV(globito.t.v) : ""; color: Tema.acentoTinta
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 17 } }
        }
        Text { x: 14; y: 52; text: "km " + caja.kmTxt(caja.sel ? caja.sel.d : 0, 2) + " · " + (caja.rec ? Formato.hms(globito.p[0] * 1000, true) : Formato.tiempo(globito.p[0] * 1000) + " de vuelta")
               color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 16 } }
        Text { x: 14; y: 74
               text: caja.modo !== "vel" ? Math.round(globito.p[2]) + " km/h en ese punto"
                   : caja.dist > 0 ? caja.fmtSigno(globito.ade, 1) + " s " + (globito.ade >= 0 ? "adelantado" : "atrasado") + " en ese punto"
                   : caja.fmtSigno(globito.p[3], 1) + " m/s² en ese punto"
               color: caja.modo !== "vel" || !(caja.dist > 0) ? Tema.tinta : Math.abs(globito.ade) <= 1 ? Tema.bien : Math.abs(globito.ade) <= 3 ? Tema.acento : Tema.mal
               font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 17 } }
    }

    // Gestos sobre el gráfico: pellizco = zoom; arrastrar = recorrer; toque = punto; doble toque = acercar
    PinchArea {
        x: caja.x0; y: caja.y0 - 20; width: caja.x1 - caja.x0; height: caja.y1 - caja.y0 + 40
        property real de0: 0
        property real span0: 1
        onPinchStarted: { de0 = caja.de; span0 = caja.span; }
        onPinchUpdated: (pinch) => {
            const s = span0 / Math.max(0.05, pinch.scale);
            const dC = de0 + (pinch.startCenter.x) / width * span0;
            caja.ver(dC - pinch.center.x / width * s, s);
        }
        MouseArea {
            id: zona
            anchors.fill: parent
            property real xPres: 0
            property real dePres: 0
            property bool arrastro: false
            property real xToque: -1
            onPressed: (m) => { xPres = m.x; dePres = caja.de; arrastro = false; }
            onPositionChanged: (m) => {
                if (!pressed) return;
                if (!arrastro && Math.abs(m.x - xPres) < 8) return;
                arrastro = true;
                caja.ver(dePres - (m.x - xPres) / width * caja.span, caja.span);
            }
            onClicked: (m) => {
                if (arrastro) return;
                if (unToque.running && Math.abs(m.x - xToque) < 40) { unToque.stop(); xToque = -1; caja.acercar(m.x + caja.x0, 2); return; }
                xToque = m.x; unToque.restart();
            }
            Timer { id: unToque; interval: 280; onTriggered: caja.elegir(zona.xToque + caja.x0) }
        }
    }

    // Barrita de abajo: toda la carrera chiquita y el recuadro amarillo de lo que se ve (arrastrable)
    Canvas {
        id: barrita
        anchors.fill: parent
        onPaint: {
            const g = getContext("2d");
            g.reset();
            const x0 = caja.bx0, x1 = caja.bx1, y0 = caja.by0, y1 = caja.by1;
            const X = d => x0 + (x1 - x0) * d / caja.total;
            g.fillStyle = "rgba(0,0,0,0.35)"; g.strokeStyle = "rgba(207,212,216,0.5)"; g.lineWidth = 1;
            g.beginPath(); g.roundedRect(x0, y0, x1 - x0, y1 - y0, 10, 10); g.fill(); g.stroke();
            let vmin = Infinity, vmax = -Infinity;
            for (const t of caja.tramos) for (const p of t.v.m) if (p[7]) { vmin = Math.min(vmin, p[2]); vmax = Math.max(vmax, p[2]); }
            if (!(vmax > vmin)) { vmin = 0; vmax = 100; }
            const Y = v => y1 - 4 - (y1 - y0 - 8) * (v - vmin + 3) / (vmax - vmin + 6);
            g.fillStyle = Tema.acento; g.font = "italic 12px '" + Tema.fuente800 + "'"; g.textAlign = "left";
            g.strokeStyle = Tema.acento; g.setLineDash([3, 3]);
            for (const t of caja.tramos) {
                if (t.d0 > 0) { g.beginPath(); g.moveTo(X(t.d0), y0 + 2); g.lineTo(X(t.d0), y1 - 2); g.stroke(); }
                if (!caja.rec && (x1 - x0) / caja.tramos.length > 26) g.fillText(caja.nombreV(t.v), X(t.d0) + 5, y0 + 14);
            }
            if (caja.rec) for (const mk of caja.marcas) { g.beginPath(); g.moveTo(X(mk.m || 0), y0 + 2); g.lineTo(X(mk.m || 0), y1 - 2); g.stroke(); g.fillText(String(mk.n), X(mk.m || 0) + 4, y0 + 14); }
            g.setLineDash([]);
            // la velocidad de toda la carrera, una lectura de cada tantas
            g.strokeStyle = "rgba(243,236,216,0.6)"; g.lineWidth = 1; g.lineJoin = "round";
            const salto = Math.max(1, Math.floor(caja.tramos.reduce((s, t) => s + t.v.m.length, 0) / (x1 - x0)));
            g.beginPath(); let abierto = false;
            for (const t of caja.tramos) for (let k = 0; k < t.v.m.length; k += salto) {
                const p = t.v.m[k];
                if (!p[7]) { abierto = false; continue; }
                if (!abierto) { g.moveTo(X(t.d0 + p[1]), Y(p[2])); abierto = true; } else g.lineTo(X(t.d0 + p[1]), Y(p[2]));
            }
            g.stroke();
            // el recuadro de lo que se ve, con sus manijas
            const rx = X(caja.de), rw = Math.max(6, X(caja.a) - rx);
            g.fillStyle = "rgba(244,197,49,0.16)"; g.strokeStyle = Tema.acento; g.lineWidth = 2;
            g.beginPath(); g.roundedRect(rx, y0 - 2, rw, y1 - y0 + 4, 5, 5); g.fill(); g.stroke();
            g.fillStyle = Tema.acento; g.strokeStyle = "#111111"; g.lineWidth = 1;
            for (const xx of [rx, rx + rw]) { g.beginPath(); g.roundedRect(xx - 5, (y0 + y1) / 2 - 12, 10, 24, 3, 3); g.fill(); g.stroke(); }
        }
        MouseArea {
            x: caja.bx0 - 10; y: caja.by0 - 12; width: caja.bx1 - caja.bx0 + 20; height: caja.by1 - caja.by0 + 24
            property real dX: 0   // distancia (m) entre el toque y el borde izquierdo del recuadro
            onPressed: (m) => {
                const d = (x + m.x - caja.bx0) / (caja.bx1 - caja.bx0) * caja.total;
                if (d >= caja.de - caja.total * 0.01 && d <= caja.a + caja.total * 0.01) dX = d - caja.de;
                else { dX = caja.span / 2; caja.ver(d - dX, caja.span); }
            }
            onPositionChanged: (m) => { if (pressed) caja.ver((x + m.x - caja.bx0) / (caja.bx1 - caja.bx0) * caja.total - dX, caja.span); }
        }
    }
    Sub { x: 22; y: parent.height - 40; tam: 10; text: "Viendo km " + caja.kmTxt(caja.de, 1) + " a " + caja.kmTxt(caja.a, 1) + " de " + caja.kmTxt(caja.total, 1) }
    Sub { anchors.right: parent.right; anchors.rightMargin: 22; y: parent.height - 40; tam: 10; color: Tema.apagadoTinta
          text: "Doble toque: acercar · arrastrá para recorrer · X para cerrar" }
}
