import QtQuick
import Betometro

// Velocímetro redondo crema con bisel cromado (.gauge de la app). Hasta `maximo` km/h.
// En capas, para no repintar todo con cada dato (Pi 3): la cara (bisel, rayitas y números) se pinta
// una sola vez; la franja de la sugerida, solo si cambia la sugerida; la aguja solo gira.
// Sin GPS la aguja queda en 0, apenas visible (opacidad .25), como la app.
Item {
    id: raiz
    property real maximo: 160
    property real valor: 0
    property bool hayValor: true
    property real sugerida: -1      // km/h; −1 = sin marca
    property string extra: ""       // dato extra arriba del centro («máx 81»)
    width: 320; height: 320
    readonly property real k: width / 320
    readonly property real r: width / 2 - 14 * k                     // radio de las rayitas
    function grados(v) { return -135 + Math.max(0, Math.min(maximo, v)) / maximo * 270; }   // 0 = arriba, a la derecha +

    // Cara: sombra, bisel, fondo, rayitas, números y «KM/H»
    Canvas {
        id: cara
        x: -40; y: -40; width: raiz.width + 80; height: raiz.height + 90
        onWidthChanged: requestPaint()
        Connections { target: Tema; function onNombreChanged() { cara.requestPaint(); } function onSolChanged() { cara.requestPaint(); } }
        onPaint: {
            const c = getContext("2d"), W = raiz.width, k = raiz.k, cx = 40 + W / 2, cy = 40 + W / 2, R = W / 2;
            c.reset();
            for (let i = 0; i < 10; i++) {   // sombra 0 18px 40px
                c.beginPath(); c.arc(cx, cy + 18, R + 15 + 22 - i * 2.2, 0, Math.PI * 2);
                c.fillStyle = "rgba(0,0,0,0.05)"; c.fill();
            }
            const disco = (rr, col) => { c.beginPath(); c.arc(cx, cy, rr, 0, Math.PI * 2); c.fillStyle = col; c.fill(); };
            if (Tema.avispon) { disco(R + 15, "#b9bfc5"); disco(R + 11, "#5d646b"); disco(R + 9, "#e8ebee"); }
            else disco(R + 4, String(Tema.bordeCromo));
            const g = c.createRadialGradient(cx - R * 0.1, cy - R * 0.24, 0, cx - R * 0.1, cy - R * 0.24, R * 1.2);
            g.addColorStop(0, String(Tema.cara1)); g.addColorStop(1, String(Tema.cara2));
            disco(R, g);
            const r = raiz.r;
            c.textAlign = "center";
            for (let v = 0; v <= raiz.maximo; v += 10) {
                const a = (225 - v / raiz.maximo * 270) * Math.PI / 180, L = (v % 20 ? 10 : 20) * k;
                c.beginPath();
                c.moveTo(cx + Math.cos(a) * r, cy - Math.sin(a) * r);
                c.lineTo(cx + Math.cos(a) * (r - L), cy - Math.sin(a) * (r - L));
                c.strokeStyle = String(Tema.rayitas); c.lineWidth = (v % 20 ? 2 : 5) * k; c.stroke();
                if (v % 20 === 0) {
                    c.font = "700 " + (26 * k) + "px '" + Tema.fuenteNum + "'";
                    c.fillStyle = String(Tema.numerosDial);
                    c.fillText(String(v), cx + Math.cos(a) * (r - 42 * k), cy - Math.sin(a) * (r - 42 * k) + 9 * k);
                }
            }
            c.font = (14 * k) + "px '" + Tema.fuente + "'";
            c.fillStyle = String(Tema.rayitas);
            c.fillText("K M / H", cx, cy + 104 * k);
        }
    }

    // Franja verde ±2 km/h y marca amarilla de la sugerida (se repinta solo si cambia medio km/h)
    Canvas {
        id: zona
        anchors.fill: parent
        property real s: raiz.sugerida < 0 ? -1 : Math.round(raiz.sugerida * 2) / 2
        onSChanged: requestPaint()
        onWidthChanged: requestPaint()
        Connections { target: Tema; function onNombreChanged() { zona.requestPaint(); } function onSolChanged() { zona.requestPaint(); } }
        onPaint: {
            const c = getContext("2d"); c.reset();
            if (s < 0) return;
            const k = raiz.k, cx = width / 2, cy = height / 2, r = raiz.r;
            const ang = v => (225 - Math.max(0, Math.min(raiz.maximo, v)) / raiz.maximo * 270) * Math.PI / 180;
            c.beginPath();
            c.arc(cx, cy, r - 5 * k, -ang(s - 2), -ang(s + 2), false);
            c.strokeStyle = String(Tema.zona); c.lineWidth = 10 * k; c.lineCap = "butt"; c.stroke();
            // Si la sugerida pasa el máximo del dial, la marca queda en el máximo y se pone roja (Matías, 04/11/2026)
            const pasada = s > raiz.maximo;
            const a = ang(s);
            c.beginPath();
            c.moveTo(cx + Math.cos(a) * (r + 4 * k), cy - Math.sin(a) * (r + 4 * k));
            c.lineTo(cx + Math.cos(a) * (r - 26 * k), cy - Math.sin(a) * (r - 26 * k));
            c.strokeStyle = pasada ? "#d23b2c" : String(Tema.sugerida); c.lineWidth = 7 * k; c.stroke();
        }
    }

    // Dato extra arriba del centro y número digital abajo
    Text {
        visible: raiz.extra !== ""
        anchors.horizontalCenter: parent.horizontalCenter
        y: raiz.height / 2 - 62 * raiz.k
        text: raiz.extra; color: Tema.numerosDial; opacity: 0.85
        font { family: Tema.fuente600; pixelSize: 18 * raiz.k }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: raiz.height / 2 + (raiz.hayValor ? 34 : 50) * raiz.k
        text: raiz.hayValor ? String(Math.round(raiz.valor)) : "SIN GPS"
        color: Tema.numerosDial
        font { family: Tema.fuenteNum; italic: true; weight: Font.ExtraBold; pixelSize: (raiz.hayValor ? 56 : 30) * raiz.k; features: { "tnum": 1 } }
    }

    // Aguja: solo gira
    Rectangle {
        id: aguja
        width: 7 * raiz.k; height: raiz.r - 30 * raiz.k
        radius: width / 2
        x: raiz.width / 2 - width / 2; y: raiz.height / 2 - height   // el pie queda en el centro (lo tapa el disco)
        color: Tema.aguja
        opacity: raiz.hayValor ? 1 : 0.25
        transformOrigin: Item.Bottom
        rotation: raiz.grados(raiz.hayValor ? raiz.valor : 0)
        antialiasing: true
    }
    Rectangle {   // centro
        width: 28 * raiz.k; height: width; radius: width / 2
        anchors.centerIn: parent
        color: Tema.aguja
    }
}
