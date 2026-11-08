import QtQuick
import Betometro

// Velocímetro del inicio a elegir (Matías, 08/11/2026: «5 o 6 modelos de velocímetros decorativos»).
// estilo: "" = el del tema (Velocimetro.qml) · clasico · deportivo · digital · rally · aviacion · anillo
// Muestras aprobadas: docs/muestras/personalizar/velocimetros.png. En capas, como Velocimetro.qml: la cara se
// pinta una sola vez; el arco de luces (digital y anillo) solo cuando cambia el número entero; la aguja solo gira.
Item {
    id: raiz
    property string estilo: ""
    property real valor: 0
    property real sugerida: -1
    property real maximo: 160
    width: 320; height: 320
    readonly property real k: width / 220   // las muestras se dibujaron en un cuadro de 220
    readonly property bool conAguja: estilo === "clasico" || estilo === "deportivo" || estilo === "rally" || estilo === "aviacion"
    readonly property int entero: Math.round(Math.max(0, Math.min(maximo, valor)))
    function ang(v) { return (-225 + Math.max(0, Math.min(maximo, v)) / maximo * 270) * Math.PI / 180; }
    // Paleta de cada estilo (fijos: son decorativos; el anillo usa el acento del tema)
    readonly property var p: ({
        clasico:   { bisel: "#cfd4d8", cara: "#f3ecd8", cara2: "#d6caa9", rayas: "#1d2a20", nums: "#1d5233", aguja: "#111111", numero: "#1d5233", fNum: "Barlow Condensed" },
        deportivo: { bisel: "#9aa3ad", aro: "#e9ecef", cara: "#141414", cara2: "#0a0a0a", rayas: "#ffffff", nums: "#ffffff", aguja: "#e10600", numero: "#ffffff", zona: "#e10600", fNum: "Barlow ExtraBold" },
        digital:   { bisel: "#0c0d0f", cara: "#0c0d0f", cara2: "#0c0d0f", numero: "#ffffff", fNum: "Barlow Condensed" },
        rally:     { bisel: "#2a2219", cara: "#120e0a", cara2: "#120e0a", rayas: "#f3e7c8", nums: "#ffb000", aguja: "#ffb000", numero: "#ffb000", pista: "#3a2f22", zona: "#c0162c", fNum: "Barlow Condensed" },
        aviacion:  { bisel: "#2b3640", cara: "#05080a", cara2: "#05080a", rayas: "#e8eef2", nums: "#3cf06e", aguja: "#e8eef2", numero: "#3cf06e", tornillo: "#4fc3f7", fNum: "B612 Mono" },
        anillo:    { bisel: "transparent", cara: "#0c0e10", cara2: "#0c0e10", numero: "#ffffff", fNum: "Barlow Condensed" }
    })
    readonly property var c: p[estilo] || p.clasico

    // El del tema (lo de siempre)
    Velocimetro {
        visible: raiz.estilo === ""
        anchors.fill: parent
        valor: raiz.valor; sugerida: raiz.sugerida; maximo: raiz.maximo
    }

    // Cara fija
    Canvas {
        id: cara
        visible: raiz.estilo !== ""
        anchors.fill: parent
        onWidthChanged: requestPaint()
        Connections { target: raiz; function onEstiloChanged() { cara.requestPaint(); } }
        Connections { target: Tema; function onAcentoChanged() { cara.requestPaint(); } }
        onPaint: {
            const g = getContext("2d"); g.reset();
            if (raiz.estilo === "") return;
            const K = raiz.k, C = raiz.c, e = raiz.estilo;
            g.translate(width / 2, height / 2); g.scale(K, K);
            const circ = (r, col) => { g.beginPath(); g.arc(0, 0, r, 0, Math.PI * 2); g.fillStyle = col; g.fill(); };
            const arco = (r, v0, v1, col, w) => { g.beginPath(); g.arc(0, 0, r, raiz.ang(v0), raiz.ang(v1), false); g.strokeStyle = col; g.lineWidth = w; g.lineCap = "round"; g.stroke(); };
            const rayas = (r1, r2, col, w) => {
                for (let v = 0; v <= 160; v += 10) {
                    const a = raiz.ang(v), largo = v % 20 === 0;
                    g.beginPath(); g.moveTo(Math.cos(a) * r1, Math.sin(a) * r1); g.lineTo(Math.cos(a) * (largo ? r2 : (r1 + r2) / 2 + 2), Math.sin(a) * (largo ? r2 : (r1 + r2) / 2 + 2));
                    g.strokeStyle = col; g.lineWidth = largo ? w : w / 2; g.stroke();
                }
            };
            const nums = (r, col, fam, fs) => {
                g.fillStyle = col; g.font = "bold " + fs + "px '" + fam + "'"; g.textAlign = "center"; g.textBaseline = "middle";
                for (let v = 0; v <= 160; v += 20) { const a = raiz.ang(v); g.fillText(String(v), Math.cos(a) * r, Math.sin(a) * r); }
            };
            if (e === "clasico") {
                circ(104, C.bisel);
                const gr = g.createRadialGradient(0, -30, 10, 0, 0, 96); gr.addColorStop(0, "#fffbef"); gr.addColorStop(1, C.cara2);
                circ(96, gr); rayas(90, 78, C.rayas, 4); nums(64, C.nums, C.fNum, 17);
            } else if (e === "deportivo") {
                circ(104, C.bisel); circ(98, C.aro); circ(94, C.cara);
                arco(88, 120, 160, C.zona, 6); rayas(90, 76, C.rayas, 4); nums(62, C.nums, C.fNum, 15);
            } else if (e === "digital") {
                circ(104, C.cara);
            } else if (e === "rally") {
                circ(104, C.bisel); circ(96, C.cara);
                arco(88, 0, 160, C.pista, 10); arco(88, 130, 160, C.zona, 10); rayas(82, 70, C.rayas, 3); nums(55, C.nums, C.fNum, 18);
            } else if (e === "aviacion") {
                circ(104, C.bisel); circ(96, C.cara); rayas(90, 76, C.rayas, 3); nums(62, C.nums, C.fNum, 14);
                for (const t of [[-1, -1], [1, -1], [-1, 1], [1, 1]]) { g.beginPath(); g.arc(t[0] * 74, t[1] * 74, 4, 0, Math.PI * 2); g.fillStyle = C.tornillo; g.fill(); }
                g.fillStyle = C.tornillo; g.font = "10px 'B612 Mono'"; g.textAlign = "center"; g.fillText("AIRSPEED", 0, -30);
            } else if (e === "anillo") {
                circ(104, C.cara); arco(84, 0, 160, "rgba(255,255,255,0.13)", 16);
            }
        }
    }

    // Lo que cambia con el número: el arco de luces (digital) o el anillo de color (anillo)
    Canvas {
        id: luces
        visible: raiz.estilo === "digital" || raiz.estilo === "anillo"
        anchors.fill: parent
        property int v: raiz.entero
        onVChanged: requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        onWidthChanged: requestPaint()
        Connections { target: Tema; function onAcentoChanged() { luces.requestPaint(); } }
        onPaint: {
            const g = getContext("2d"); g.reset();
            if (!visible) return;
            g.translate(width / 2, height / 2); g.scale(raiz.k, raiz.k);
            if (raiz.estilo === "digital") {
                for (let x = 0; x <= 160; x += 5) {
                    const a = raiz.ang(x);
                    g.beginPath(); g.moveTo(Math.cos(a) * 92, Math.sin(a) * 92); g.lineTo(Math.cos(a) * 80, Math.sin(a) * 80);
                    g.strokeStyle = x <= v ? (x < 110 ? "#2fd162" : "#ffd400") : "#2a2e33"; g.lineWidth = 7; g.stroke();
                }
            } else {
                if (v > 0) { g.beginPath(); g.arc(0, 0, 84, raiz.ang(0), raiz.ang(v), false); g.strokeStyle = String(Tema.acento); g.lineWidth = 16; g.lineCap = "round"; g.stroke(); }
                const a = raiz.ang(v); g.beginPath(); g.arc(Math.cos(a) * 84, Math.sin(a) * 84, 6, 0, Math.PI * 2); g.fillStyle = "#ffffff"; g.fill();
            }
        }
    }

    // Número
    Text {
        visible: raiz.estilo !== ""
        anchors.horizontalCenter: parent.horizontalCenter
        readonly property bool grande: raiz.estilo === "digital" || raiz.estilo === "anillo"
        y: grande ? raiz.height / 2 - height * 0.62 : raiz.height / 2 + 42 * raiz.k
        text: String(raiz.entero)
        color: raiz.c.numero
        font { family: raiz.c.fNum; weight: Font.Bold; pixelSize: (grande ? 74 : raiz.estilo === "deportivo" ? 34 : 32) * raiz.k }
    }
    Text {
        visible: raiz.estilo === "digital" || raiz.estilo === "anillo"
        anchors.horizontalCenter: parent.horizontalCenter
        y: raiz.height / 2 + 30 * raiz.k
        text: "KM/H"; color: "#9aa0a6"
        font { family: "Barlow SemiBold"; pixelSize: 13 * raiz.k; letterSpacing: 2 }
    }
    Text {
        visible: raiz.estilo === "digital" && raiz.sugerida >= 0
        anchors.horizontalCenter: parent.horizontalCenter
        y: raiz.height / 2 + 50 * raiz.k
        text: "sug " + Math.round(raiz.sugerida); color: "#ffd400"
        font { family: "Barlow ExtraBold"; italic: true; pixelSize: 16 * raiz.k }
    }

    // Aguja (clásico, deportivo, rally, aviación): solo gira
    Rectangle {
        visible: raiz.conAguja
        width: 5 * raiz.k; height: 80 * raiz.k; radius: width / 2
        x: raiz.width / 2 - width / 2; y: raiz.height / 2 - height
        color: raiz.c.aguja || "#111111"
        transformOrigin: Item.Bottom
        rotation: -135 + Math.max(0, Math.min(raiz.maximo, raiz.valor)) / raiz.maximo * 270
        antialiasing: true
    }
    Rectangle {
        visible: raiz.conAguja
        width: 16 * raiz.k; height: width; radius: width / 2
        anchors.centerIn: parent
        color: raiz.c.aguja || "#111111"
    }
}
