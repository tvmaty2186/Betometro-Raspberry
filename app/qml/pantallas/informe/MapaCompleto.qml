import QtQuick
import Betometro

// Mapa del informe a pantalla completa (1.0.30, muestra docs/muestras/1.0.30/graficos/2-mapa-pi.png): la pista y
// el recorrido de la vuelta elegida con color por velocidad, aceleración o adelanto, con zoom y arrastre.
// Rayitas cada 1 km con el número, LARGADA, marcas numeradas con km y hora (recorrido libre), la escala de colores,
// el mapita con todo el recorrido y el recuadro de lo visible, el punto tocado con su globito, Vel./Acel./Adel., X.
// Sin sugerida (recorrido libre) el color de la velocidad es contra el promedio (Matías, 07/11/2026).
// Se dibuja con Canvas solo lo visible y se repinta solo al cambiar el zoom, la posición, el modo o el punto.
Tarjeta {
    id: caja
    property var inf: ({})           // Datos.informe(id): pista, cerrada
    property var vu: null            // la vuelta elegida (inf.vueltas[lap])
    property bool rec: false         // recorrido libre
    property var marcas: []
    property real dist: 0
    property real sug: 0
    property string modo: "vel"      // vel, acel, ade
    signal cerrar()
    signal elegirModo(string m)

    oscura: true
    readonly property var pista: inf.pista || []
    readonly property var lecturas: vu && vu.m ? vu.m : []
    readonly property real largo: vu && vu.L ? vu.L : 1
    readonly property real prom: {   // promedio de la vuelta: el color sin sugerida
        let s = 0, n = 0;
        for (const p of lecturas) if (p[7]) { s += p[2]; n++; }
        return n ? s / n : 0;
    }
    readonly property real ref: sug > 0 ? sug : prom
    // Vista: k píxeles por unidad del dibujo (364 × 330), con el origen en (ox, oy)
    readonly property real ax0: 60
    readonly property real ay0: 100
    readonly property real ax1: width - 60
    readonly property real ay1: height - 96
    readonly property real kBase: Math.min((ax1 - ax0) / 364, (ay1 - ay0) / 330)
    property real k: 1
    property real ox: 0
    property real oy: 0
    property var sel: null           // la lectura tocada
    function ex(x) { return ox + x * k; }
    function ey(y) { return oy + y * k; }
    function color(p) {
        if (!p[7]) return "#555555";
        if (modo === "acel") { const ac = p[3]; return ac > 0.8 ? Tema.bien : ac > 0.25 ? "#8fe3b0" : ac < -0.8 ? "#d23b2c" : ac < -0.25 ? "#f08a96" : "#9aa3ad"; }
        if (modo === "ade") { const s = Math.abs(p[4]); return s < 1 ? Tema.bien : s < 3 ? Tema.acento : "#d23b2c"; }
        const d = p[2] - ref;
        return d > 6 ? "#d23b2c" : d > 2 ? Tema.acento : d >= -2 ? Tema.bien : d >= -6 ? "#7fd3ff" : "#3a8de0";
    }
    function kmTxt(m, dec) { return (m / 1000).toFixed(dec).replace(".", ","); }
    function fmt(v, dec) { return v.toFixed(dec).replace(".", ","); }
    function fmtSigno(v, dec) { return (v > 0 ? "+" : v < 0 ? "−" : "±") + Math.abs(v).toFixed(dec).replace(".", ","); }
    function nombreV(v) { return "V" + v.n + (inf.largadas > 1 ? " L" + v.largada : ""); }

    function abrir() {   // todo el recorrido a la vista
        sel = null;
        k = kBase;
        ox = (ax0 + ax1) / 2 - 182 * k; oy = (ay0 + ay1) / 2 - 165 * k;
        repintar();
    }
    function poner(nk, nox, noy) {   // con límites: del todo a la vista hasta 40 veces, y sin perder el dibujo
        k = Math.max(kBase, Math.min(kBase * 40, nk));
        ox = Math.max(ax0 + 40 - 364 * k, Math.min(ax1 - 40, nox));
        oy = Math.max(ay0 + 40 - 330 * k, Math.min(ay1 - 40, noy));
        repintar();
    }
    function verTramo(d0, d1) {   // acercar hasta que se vea justo el tramo entre esas distancias (m) de la vuelta
        let x0 = Infinity, x1 = -Infinity, y0 = Infinity, y1 = -Infinity;
        for (const p of lecturas) if (p[7] && p[1] >= d0 && p[1] <= d1) { x0 = Math.min(x0, p[5]); x1 = Math.max(x1, p[5]); y0 = Math.min(y0, p[6]); y1 = Math.max(y1, p[6]); }
        if (!(x1 >= x0)) return;
        const nk = Math.min((ax1 - ax0) / Math.max(4, x1 - x0), (ay1 - ay0) / Math.max(4, y1 - y0));
        poner(nk, (ax0 + ax1) / 2 - (x0 + x1) / 2 * Math.min(kBase * 40, Math.max(kBase, nk)), (ay0 + ay1) / 2 - (y0 + y1) / 2 * Math.min(kBase * 40, Math.max(kBase, nk)));
    }
    function acercar(px, py, factor) {
        const nk = Math.max(kBase, Math.min(kBase * 40, k * factor));
        poner(nk, px - (px - ox) * nk / k, py - (py - oy) * nk / k);
    }
    function repintar() { mapa.requestPaint(); mapita.requestPaint(); }
    onModoChanged: repintar()
    function elegir(px, py) {   // la lectura más cercana al toque (hasta 40 px)
        let mejor = null, dm = 40 * 40;
        for (const p of lecturas) {
            if (!p[7]) continue;
            const dx = caja.ex(p[5]) - px, dy = caja.ey(p[6]) - py, dd = dx * dx + dy * dy;
            if (dd < dm) { dm = dd; mejor = p; }
        }
        sel = mejor;
        mapa.requestPaint();
    }
    function seVe(x, y, m) { return x >= -m && x <= width + m && y >= -m && y <= height + m; }
    // Dónde está la lectura respecto de las marcas (recorrido libre)
    function entreMarcas(p) {
        const t = p[0] * 1000;
        let n = 0;
        for (const mk of marcas) if (mk.ms <= t) n++;
        if (!marcas.length) return "";
        if (n === 0) return " · antes de la marca 1";
        if (n === marcas.length) return " · después de la marca " + n;
        return " · entre la marca " + n + " y la " + (n + 1);
    }
    property string txtViendo: ""

    Sub { x: 22; y: 18; width: botones.x - 40; elide: Text.ElideRight; tam: 12
          text: (caja.rec ? "Recorrido" : caja.vu ? "Recorrido de la " + caja.nombreV(caja.vu).replace("V", "vuelta ") : "Recorrido")
                + " · color = " + (caja.modo === "vel" ? "velocidad" : caja.modo === "acel" ? "aceleración" : "adelanto")
                + " · " + caja.kmTxt(caja.largo, 1) + " km" + (caja.rec && caja.marcas.length ? " · " + caja.marcas.length + (caja.marcas.length === 1 ? " marca" : " marcas") : "") }
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
                MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: caja.elegirModo(parent.modelData.m) }
            }
        }
    }
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
        id: mapa
        anchors.fill: parent
        onPaint: {
            const g = getContext("2d");
            g.reset();
            const P = caja.pista, M = caja.lecturas, k = caja.k;
            if (!P.length) {
                g.fillStyle = Tema.tintaSuave; g.font = "italic 18px '" + Tema.fuente600 + "'"; g.textAlign = "center";
                g.fillText("Esta carrera no tiene el recorrido grabado.", width / 2, height / 2);
                return;
            }
            const X = x => caja.ex(x), Y = y => caja.ey(y);
            g.save();
            g.beginPath(); g.rect(3, 3, width - 6, height - 6); g.clip();
            // la pista (sombra y gris), más ancha al acercar pero no más de 30 px
            const kk = Math.max(1, Math.min(1.6, k / caja.kBase * 0.4 + 1));
            g.lineJoin = "round"; g.lineCap = "round";
            g.beginPath(); g.moveTo(X(P[0][0]), Y(P[0][1]));
            for (const p of P) g.lineTo(X(p[0]), Y(p[1]));
            if (caja.inf.cerrada) g.closePath();
            g.strokeStyle = "rgba(0,0,0,0.45)"; g.lineWidth = 19 * kk; g.stroke();
            g.strokeStyle = "#3a3f45"; g.lineWidth = 11 * kk; g.stroke();
            // la vuelta pintada, tramo por tramo, solo lo visible
            g.lineWidth = 8 * kk;
            let col = null, abierto = false;
            for (let i = 1; i < M.length; i++) {
                const a = M[i - 1], b = M[i];
                if (!a[7] || !b[7] || !caja.seVe(X(b[5]), Y(b[6]), 40)) { if (abierto) g.stroke(); abierto = false; col = null; continue; }
                const c = caja.color(b);
                if (c !== col) {
                    if (abierto) g.stroke();
                    g.strokeStyle = c; g.beginPath(); g.moveTo(X(a[5]), Y(a[6])); col = c; abierto = true;
                }
                g.lineTo(X(b[5]), Y(b[6]));
            }
            if (abierto) g.stroke();
            // rayitas cada 1 km (cada 5 si están muy juntas), con el número
            let largo = 0;
            for (let i = 1; i < M.length; i++) if (M[i][7] && M[i - 1][7]) largo += Math.hypot(M[i][5] - M[i - 1][5], M[i][6] - M[i - 1][6]);
            const pxKm = largo * k / Math.max(1, caja.largo / 1000);   // píxeles que mide 1 km en pantalla
            const pasoKm = pxKm >= 60 ? 1000 : pxKm >= 12 ? 5000 : 0;
            g.font = "13px '" + Tema.fuente600 + "'"; g.textAlign = "center";
            let dmin = 0, dmax = 0, hay = false;
            if (pasoKm > 0) {
                let j = 1;
                for (let d = pasoKm; d < caja.largo; d += pasoKm) {
                    while (j < M.length && M[j][1] < d) j++;
                    if (j >= M.length) break;
                    const a = M[j - 1], b = M[j];
                    if (!a[7] || !b[7]) continue;
                    const px = X(a[5] + (b[5] - a[5]) * 0.5), py = Y(a[6] + (b[6] - a[6]) * 0.5);
                    if (!caja.seVe(px, py, 0)) continue;
                    let nx = -(b[6] - a[6]), ny = b[5] - a[5]; const n = Math.hypot(nx, ny) || 1; nx /= n; ny /= n;
                    g.strokeStyle = "rgba(255,255,255,0.8)"; g.lineWidth = 2;
                    g.beginPath(); g.moveTo(px + nx * 11, py + ny * 11); g.lineTo(px - nx * 11, py - ny * 11); g.stroke();
                    g.fillStyle = Tema.tintaSuave; g.fillText(caja.kmTxt(d, 0), px + nx * 24, py + ny * 24 + 5);
                }
            }
            // qué parte se ve (para el texto de abajo)
            for (const p of M) if (p[7] && caja.seVe(X(p[5]), Y(p[6]), 0)) { if (!hay) { dmin = p[1]; hay = true; } dmax = p[1]; }
            caja.txtViendo = !hay ? "" : k <= caja.kBase * 1.01 ? (caja.rec ? "Recorrido completo" : "Vuelta completa") : "Viendo del km " + caja.kmTxt(dmin, 1) + " al " + caja.kmTxt(dmax, 1);
            // largada
            const p0 = P[0];
            g.beginPath(); g.arc(X(p0[0]), Y(p0[1]), 14, 0, Math.PI * 2); g.fillStyle = "#ffffff"; g.fill();
            const izq = X(p0[0]) > width * 0.7;
            g.fillStyle = Tema.tinta; g.font = "italic 18px '" + Tema.fuente800 + "'"; g.textAlign = izq ? "right" : "left";
            g.fillText("LARGADA", X(p0[0]) + (izq ? -22 : 22), Y(p0[1]) + 7);
            if (caja.rec) {   // recorrido libre: la llegada y las marcas con km y hora
                const F = P[P.length - 1];
                if (!caja.inf.cerrada) {
                    g.fillStyle = "#111111"; g.fillRect(X(F[0]) - 12, Y(F[1]) - 12, 24, 24);
                    g.strokeStyle = "#ffffff"; g.lineWidth = 3; g.strokeRect(X(F[0]) - 12, Y(F[1]) - 12, 24, 24);
                    const izqF = X(F[0]) > width * 0.7;
                    g.fillStyle = Tema.tinta; g.textAlign = izqF ? "right" : "left";
                    g.fillText("LLEGADA", X(F[0]) + (izqF ? -22 : 22), Y(F[1]) + 7);
                }
                for (const mk of caja.marcas) {
                    if (mk.x === undefined || !caja.seVe(X(mk.x), Y(mk.y), 60)) continue;
                    g.beginPath(); g.arc(X(mk.x), Y(mk.y), 16, 0, Math.PI * 2);
                    g.fillStyle = Tema.amarillo; g.fill(); g.strokeStyle = "#111111"; g.lineWidth = 3; g.stroke();
                    g.fillStyle = "#111111"; g.font = "italic 20px '" + Tema.fuente800 + "'"; g.textAlign = "center";
                    g.fillText(String(mk.n), X(mk.x), Y(mk.y) + 7);
                    // la etiqueta del lado de afuera de la curva, para no pisar la ruta
                    let i = 0; for (let q = 1; q < M.length; q++) if (Math.abs(M[q][0] - mk.ms / 1000) < Math.abs(M[i][0] - mk.ms / 1000)) i = q;
                    const a = M[Math.max(0, i - 2)], b = M[Math.min(M.length - 1, i + 2)], c = M[Math.min(M.length - 1, i + 12)];
                    let nx = -(b[6] - a[6]), ny = b[5] - a[5]; const n = Math.hypot(nx, ny) || 1; nx /= n; ny /= n;
                    if ((c[5] - M[i][5]) * nx + (c[6] - M[i][6]) * ny > 0) { nx = -nx; ny = -ny; }
                    g.fillStyle = Tema.acento; g.font = "italic 15px '" + Tema.fuente600 + "'"; g.textAlign = nx < 0 ? "right" : "left";
                    g.fillText("km " + caja.kmTxt(mk.m || 0, 1) + " · " + Formato.hms(mk.ms, true), X(mk.x) + nx * 30, Y(mk.y) + ny * 30 + 5);
                }
            }
            // el punto tocado
            const s = caja.sel;
            if (s) {
                g.beginPath(); g.arc(X(s[5]), Y(s[6]), 10, 0, Math.PI * 2); g.fillStyle = "#ffffff"; g.fill();
                g.strokeStyle = "#111111"; g.lineWidth = 3; g.stroke();
                globito.x = Math.min(width - globito.width - 20, Math.max(20, X(s[5]) + 24));
                globito.y = Math.min(height - 150, Math.max(76, Y(s[6]) - 88));
            }
            g.restore();
        }
    }
    // Globito del punto tocado
    Rectangle {
        id: globito
        visible: !!caja.sel
        width: filaG.implicitWidth + 28; height: 74; radius: 16
        color: Qt.rgba(8 / 255, 25 / 255, 14 / 255, 0.94); border { width: 2; color: Tema.bordeCromo }
        readonly property var p: caja.sel || [0, 0, 0, 0, 0, 0, 0, 0]
        Row {
            x: 14; y: 6; spacing: 4
            Text { id: numG; text: caja.modo === "vel" ? String(Math.round(globito.p[2])) : caja.modo === "acel" ? caja.fmtSigno(globito.p[3], 1) : caja.fmtSigno(globito.p[4], 1)
                   color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 32 } }
            Text { anchors.baseline: numG.baseline; text: caja.modo === "vel" ? "km/h" : caja.modo === "acel" ? "m/s²" : "s"
                   color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 18 } }
        }
        Text { id: filaG; x: 14; y: 46; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 16 }
               text: "km " + caja.kmTxt(globito.p[1], 2) + " · " + (caja.rec ? Formato.hms(globito.p[0] * 1000, true) + caja.entreMarcas(globito.p)
                     : Formato.tiempo(globito.p[0] * 1000) + " de vuelta" + (caja.dist > 0 ? " · " + caja.fmtSigno(globito.p[4], 1) + " s " + (globito.p[4] >= 0 ? "adelantado" : "atrasado") : ""))
                     + (caja.modo !== "vel" ? " · " + Math.round(globito.p[2]) + " km/h" : "") }
    }

    // Gestos: pellizco = zoom; arrastrar = mover; toque = punto; doble toque = acercar
    PinchArea {
        anchors.fill: parent
        anchors.topMargin: 80
        property real k0: 1
        property real ox0: 0
        property real oy0: 0
        onPinchStarted: { k0 = caja.k; ox0 = caja.ox; oy0 = caja.oy; }
        onPinchUpdated: (pinch) => {
            const nk = Math.max(caja.kBase, Math.min(caja.kBase * 40, k0 * pinch.scale));
            const sx = pinch.startCenter.x, sy = pinch.startCenter.y + 80, cx = pinch.center.x, cy = pinch.center.y + 80;
            caja.poner(nk, cx - (sx - ox0) * nk / k0, cy - (sy - oy0) * nk / k0);
        }
        MouseArea {
            id: zona
            anchors.fill: parent
            property real xPres: 0
            property real yPres: 0
            property real oxPres: 0
            property real oyPres: 0
            property bool arrastro: false
            property real xToque: -1
            property real yToque: -1
            onPressed: (m) => { xPres = m.x; yPres = m.y; oxPres = caja.ox; oyPres = caja.oy; arrastro = false; }
            onPositionChanged: (m) => {
                if (!pressed) return;
                if (!arrastro && Math.hypot(m.x - xPres, m.y - yPres) < 8) return;
                arrastro = true;
                caja.poner(caja.k, oxPres + m.x - xPres, oyPres + m.y - yPres);
            }
            onClicked: (m) => {
                if (arrastro) return;
                if (unToque.running && Math.hypot(m.x - xToque, m.y - yToque) < 40) { unToque.stop(); xToque = -1; caja.acercar(m.x, m.y + 80, 2); return; }
                xToque = m.x; yToque = m.y; unToque.restart();
            }
            Timer { id: unToque; interval: 280; onTriggered: caja.elegir(zona.xToque, zona.yToque + 80) }
        }
    }

    // Escala de colores
    Rectangle {
        x: 22; y: parent.height - 50 - height
        width: 232; height: colLey.height + 20; radius: 14
        color: Qt.rgba(8 / 255, 25 / 255, 14 / 255, 0.86); border { width: 1; color: Qt.rgba(207 / 255, 212 / 255, 216 / 255, 0.6) }
        Column {
            id: colLey
            x: 14; y: 10; spacing: 6
            Repeater {
                model: caja.modo === "vel" ? [["#3a8de0", "muy lento", ""], ["#7fd3ff", "lento", ""], [Tema.bien, caja.sug > 0 ? "en la sugerida" : "en el promedio", caja.fmt(caja.ref, 0) + " km/h"], [Tema.acento, "rápido", ""], ["#d23b2c", "muy rápido", ""], ["#555555", "sin señal", ""]]
                     : caja.modo === "acel" ? [["#d23b2c", "frena fuerte", ""], ["#f08a96", "frena", ""], ["#9aa3ad", "parejo", ""], ["#8fe3b0", "acelera", ""], [Tema.bien, "acelera fuerte", ""], ["#555555", "sin señal", ""]]
                     : [[Tema.bien, "±1 s", ""], [Tema.acento, "1 a 3 s", ""], ["#d23b2c", "más de 3 s", ""], ["#555555", "sin señal", ""]]
                Row {
                    required property var modelData
                    spacing: 10
                    Rectangle { width: 26; height: 8; radius: 4; color: parent.modelData[0]; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: tLey; text: parent.modelData[1]; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 15 } }
                    Text { visible: !!parent.modelData[2]; text: parent.modelData[2]; color: Tema.tintaSuave; anchors.baseline: tLey.baseline; font { family: Tema.fuente600; pixelSize: 13 } }
                }
            }
        }
    }
    // Mapita: todo el recorrido y el recuadro de lo que se ve
    Rectangle {
        x: parent.width - 22 - width; y: parent.height - 50 - height
        width: 236; height: 160; radius: 14; clip: true
        color: Qt.rgba(8 / 255, 25 / 255, 14 / 255, 0.86); border { width: 1; color: Qt.rgba(207 / 255, 212 / 255, 216 / 255, 0.6) }
        Canvas {
            id: mapita
            anchors.fill: parent
            onPaint: {
                const g = getContext("2d");
                g.reset();
                const P = caja.pista;
                if (!P.length) return;
                const mk = Math.min((width - 24) / 364, (height - 24) / 330), mx = (width - 364 * mk) / 2, my = (height - 330 * mk) / 2;
                const X = x => mx + x * mk, Y = y => my + y * mk;
                g.lineJoin = "round"; g.lineCap = "round";
                g.beginPath(); g.moveTo(X(P[0][0]), Y(P[0][1]));
                for (const p of P) g.lineTo(X(p[0]), Y(p[1]));
                if (caja.inf.cerrada) g.closePath();
                g.strokeStyle = "#9aa3ad"; g.lineWidth = 3; g.stroke();
                g.fillStyle = Tema.amarillo;
                for (const m of caja.marcas) if (m.x !== undefined) { g.beginPath(); g.arc(X(m.x), Y(m.y), 4, 0, Math.PI * 2); g.fill(); }
                g.fillStyle = "#ffffff"; g.beginPath(); g.arc(X(P[0][0]), Y(P[0][1]), 4, 0, Math.PI * 2); g.fill();
                // lo visible en pantalla, pasado al mapita
                const vx0 = (0 - caja.ox) / caja.k, vx1 = (caja.width - caja.ox) / caja.k, vy0 = (0 - caja.oy) / caja.k, vy1 = (caja.height - caja.oy) / caja.k;
                g.fillStyle = "rgba(244,197,49,0.14)"; g.strokeStyle = Tema.acento; g.lineWidth = 2;
                g.beginPath(); g.roundedRect(X(vx0), Y(vy0), (vx1 - vx0) * mk, (vy1 - vy0) * mk, 3, 3); g.fill(); g.stroke();
            }
        }
    }
    Sub { x: 22; y: parent.height - 36; tam: 10; text: caja.txtViendo }
    Sub { anchors.right: parent.right; anchors.rightMargin: 22; y: parent.height - 36; tam: 10; color: Tema.apagadoTinta
          text: "Doble toque: acercar · arrastrá para recorrer · X para cerrar" }
}
