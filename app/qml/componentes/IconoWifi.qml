import QtQuick
import Betometro

// Ícono de wifi de abanico (punto y tres arcos), como el que eligió Matías (04/11/2026).
// nivel 0 = sin wifi (todo apagado); 1 = punto; 2, 3, 4 = punto y uno, dos o tres arcos.
Canvas {
    id: raiz
    property int nivel: 0
    property color prendido: Tema.tinta
    property color apagado: Qt.rgba(1, 1, 1, 0.25)
    width: 40; height: 32
    onNivelChanged: requestPaint()
    onPrendidoChanged: requestPaint()
    onPaint: {
        const c = getContext("2d");
        c.reset();
        // Todo sale del alto: el arco de afuera tiene que entrar entero
        const g = height * 0.12, base = height - g * 0.9, cx = width / 2;   // g: grosor de los arcos
        c.lineCap = "round";
        // punto
        c.beginPath(); c.arc(cx, base - g * 0.5, g * 0.85, 0, Math.PI * 2);
        c.fillStyle = String(nivel >= 1 ? prendido : apagado); c.fill();
        // arcos de adentro hacia afuera
        for (let i = 1; i <= 3; i++) {
            const r = g * 1.3 + i * g * 1.95;
            c.beginPath();
            c.arc(cx, base, r, Math.PI * 1.25, Math.PI * 1.75, false);
            c.lineWidth = g;
            c.strokeStyle = String(nivel >= i + 1 ? prendido : apagado);
            c.stroke();
        }
    }
}
