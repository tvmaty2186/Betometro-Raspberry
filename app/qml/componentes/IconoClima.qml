import QtQuick
import Betometro

// Ícono del clima según el código del pronóstico (WMO, el de Open-Meteo). Dibujado: la Raspberry no tiene emojis.
//   0 sol · 1-2 sol con nube · 3 nublado · 45/48 niebla · 51-67 y 80-82 lluvia · 71-77 y 85-86 nieve · 95-99 tormenta
Canvas {
    id: raiz
    property int codigo: 0
    width: 48; height: 48
    onCodigoChanged: requestPaint()
    onWidthChanged: requestPaint()
    readonly property string tipo: codigo === 0 ? "sol" : codigo <= 2 ? "solnube" : codigo === 3 ? "nube"
                                   : codigo === 45 || codigo === 48 ? "niebla" : codigo >= 95 ? "tormenta"
                                   : (codigo >= 71 && codigo <= 77) || codigo === 85 || codigo === 86 ? "nieve" : "lluvia"
    onPaint: {
        const c = getContext("2d"), k = width / 48;
        c.reset();
        c.lineCap = "round"; c.lineJoin = "round";
        function sol(x, y, r) {
            c.strokeStyle = "#f4c531"; c.lineWidth = 2.6 * k;
            for (let i = 0; i < 8; i++) {
                const a = i * Math.PI / 4;
                c.beginPath(); c.moveTo(x + Math.cos(a) * r * 1.35, y + Math.sin(a) * r * 1.35);
                c.lineTo(x + Math.cos(a) * r * 1.75, y + Math.sin(a) * r * 1.75); c.stroke();
            }
            c.beginPath(); c.arc(x, y, r, 0, Math.PI * 2); c.fillStyle = "#f4c531"; c.fill();
        }
        function nube(x, y, w, color) {   // x,y: abajo a la izquierda
            const h = w * 0.42;
            c.beginPath();
            c.moveTo(x + h * 0.5, y);
            c.arc(x + h * 0.5, y - h * 0.5, h * 0.5, Math.PI / 2, Math.PI * 1.5);
            c.arc(x + w * 0.42, y - h * 0.75, h * 0.62, Math.PI * 1.1, Math.PI * 1.9);
            c.arc(x + w - h * 0.5, y - h * 0.5, h * 0.5, Math.PI * 1.5, Math.PI / 2);
            c.closePath();
            c.fillStyle = color; c.fill();
            c.strokeStyle = "rgba(0,0,0,0.25)"; c.lineWidth = 1 * k; c.stroke();
        }
        const t = tipo;
        if (t === "sol") { sol(24 * k, 24 * k, 9 * k); return; }
        if (t === "solnube") { sol(18 * k, 17 * k, 7 * k); nube(12 * k, 38 * k, 30 * k, "#eef1f3"); return; }
        if (t === "niebla") {
            nube(8 * k, 28 * k, 32 * k, "#c8cdd2");
            c.strokeStyle = "#c8cdd2"; c.lineWidth = 2.6 * k;
            for (let i = 0; i < 2; i++) { c.beginPath(); c.moveTo(9 * k, (34 + i * 6) * k); c.lineTo(39 * k, (34 + i * 6) * k); c.stroke(); }
            return;
        }
        const gris = t === "nube" ? "#e2e6e9" : "#b9c0c6";
        nube(7 * k, t === "nube" ? 36 * k : 30 * k, 34 * k, gris);
        if (t === "lluvia") {
            c.strokeStyle = "#5fb4ff"; c.lineWidth = 2.6 * k;
            for (let i = 0; i < 3; i++) { c.beginPath(); c.moveTo((15 + i * 8) * k, 35 * k); c.lineTo((12 + i * 8) * k, 43 * k); c.stroke(); }
        } else if (t === "nieve") {
            c.fillStyle = "#ffffff";
            for (let i = 0; i < 3; i++) { c.beginPath(); c.arc((15 + i * 8) * k, (38 + (i % 2) * 4) * k, 2.2 * k, 0, Math.PI * 2); c.fill(); }
        } else if (t === "tormenta") {
            c.beginPath(); c.moveTo(26 * k, 31 * k); c.lineTo(19 * k, 40 * k); c.lineTo(24 * k, 40 * k); c.lineTo(20 * k, 47 * k);
            c.lineTo(30 * k, 37 * k); c.lineTo(25 * k, 37 * k); c.closePath(); c.fillStyle = "#f4c531"; c.fill();
        }
    }
}
