import QtQuick
import Betometro

// Un día del clima hora por hora (doble toque en un día; Matías, 09/11/2026; muestra aprobada
// docs/muestras/clima-horas/muestra.png): arriba el día y el resumen, una tira de las 24 horas que se desliza (si es
// hoy, la hora actual resaltada) y abajo el día entero: la temperatura como línea y la lluvia como barras.
Item {
    id: raiz
    property bool h: true
    property string fecha: ""        // AAAA-MM-DD; vacío = cerrado
    property string lugar: ""
    property var dia: null           // Clima.dia(): max, min, lluvia, mm, viento, rafagas
    property var horas: []           // Clima.horasDelDia()
    signal cerrar()
    visible: fecha !== ""
    z: 100
    readonly property bool esHoy: fecha === Qt.formatDate(new Date(), "yyyy-MM-dd")
    readonly property int ahora: new Date().getHours()
    function nombreDia() {
        if (!fecha) return "";
        const f = Formato.desdeIso(fecha);
        return (esHoy ? "Hoy" : ["Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"][f.getDay()]) + " " + f.getDate();
    }
    function num(x, dec) { return Number(x || 0).toFixed(dec).replace(".", ","); }
    onHorasChanged: { grafico.requestPaint(); Qt.callLater(ubicar); }
    onVisibleChanged: if (visible) Qt.callLater(ubicar)
    // La tira arranca en la hora actual (hoy) o a las 6
    function ubicar() { tira.contentX = Math.max(0, Math.min(tira.contentWidth - tira.width, ((esHoy ? ahora - 1 : 6)) * 100)); }

    MouseArea { anchors.fill: parent }   // que no se toque lo de abajo
    Fondo { anchors.fill: parent }
    Tarjeta { anchors.fill: parent; anchors.margins: 12; oscura: true }
    Text {
        x: 40; y: 30; width: parent.width - 140; elide: Text.ElideRight; textFormat: Text.StyledText
        text: raiz.nombreDia() + " <font color='" + Tema.acento + "'>· hora por hora</font>"
        color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 34 }
    }
    Sub {
        x: 40; y: 78; width: parent.width - 140; tam: 13; elide: Text.ElideRight
        text: !raiz.dia ? raiz.lugar : raiz.lugar + " · máx " + raiz.dia.max + "° · mín " + raiz.dia.min + "° · lluvia " + (raiz.dia.lluvia || 0)
              + " % · " + raiz.num(raiz.dia.mm, 1) + " mm · viento hasta " + (raiz.dia.rafagas || raiz.dia.viento || 0)
    }
    Rectangle {
        anchors { right: parent.right; rightMargin: 32; top: parent.top; topMargin: 28 }
        width: 62; height: 62; radius: 16; color: "transparent"; border { width: 3; color: Tema.bordeCromo }
        Text { anchors.centerIn: parent; text: "✕"; color: Tema.tinta; font { pixelSize: 34; weight: Font.Bold } }
        MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: raiz.cerrar() }
    }

    // ---- Las horas: se deslizan con el dedo
    Rectangle {
        id: marcoTira
        x: 36; y: 112; width: parent.width - 72; height: raiz.h ? 300 : 340
        radius: 16; color: Qt.rgba(0, 0, 0, 0.25); clip: true
        Flickable {
            id: tira
            anchors.fill: parent
            contentWidth: filaHoras.width; contentHeight: height
            flickableDirection: Flickable.HorizontalFlick; boundsBehavior: Flickable.StopAtBounds
            Row {
                id: filaHoras
                Repeater {
                    model: raiz.horas
                    Rectangle {
                        required property var modelData
                        readonly property bool actual: raiz.esHoy && modelData.hora === raiz.ahora
                        width: 100; height: marcoTira.height; radius: 14
                        color: actual ? Qt.rgba(Tema.acento.r, Tema.acento.g, Tema.acento.b, 0.2) : "transparent"
                        Column {
                            anchors.horizontalCenter: parent.horizontalCenter; y: 14; spacing: 6
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: String(modelData.hora).padStart(2, "0") + " h"
                                   color: parent.parent.actual ? Tema.acento : Tema.tintaSuave; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22 } }
                            IconoClima { anchors.horizontalCenter: parent.horizontalCenter; width: 58; height: 58; codigo: modelData.codigo || 0 }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.temp + "°"; color: Tema.numero
                                   font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 40 } }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: (modelData.lluvia || 0) + " %"; color: Tema.lluvia
                                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 21 } }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: raiz.num(modelData.mm, 1) + " mm"; color: Tema.lluvia; opacity: 0.8
                                   font { family: Tema.fuente600; pixelSize: 16 } }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "viento " + (modelData.viento || 0); color: Tema.tinta
                                   font { family: Tema.fuente600; pixelSize: 16 } }
                            Text { visible: !raiz.h; anchors.horizontalCenter: parent.horizontalCenter; text: "ráf. " + (modelData.rafagas || 0); color: Tema.tintaSuave
                                   font { family: Tema.fuente600; pixelSize: 15 } }
                        }
                    }
                }
            }
        }
    }
    Sub { x: 40; y: marcoTira.y + marcoTira.height + 8; tam: 12; text: "Deslizá para ver todas las horas ▸" }

    // ---- El día entero: temperatura (línea) y lluvia (barras)
    Sub { x: 40; y: marcoTira.y + marcoTira.height + 40; tam: 12; text: "Temperatura (línea) y lluvia (barras) en el día" }
    Canvas {
        id: grafico
        x: 36; y: marcoTira.y + marcoTira.height + 62; width: parent.width - 72; height: parent.height - y - 30
        onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
        onPaint: {
            const g = getContext("2d"); g.reset();
            const H = raiz.horas;
            if (!H.length) return;
            let tmin = 1e9, tmax = -1e9;
            for (const x of H) { tmin = Math.min(tmin, x.temp); tmax = Math.max(tmax, x.temp); }
            tmin = Math.floor(tmin / 5) * 5; tmax = Math.ceil(tmax / 5) * 5; if (tmax - tmin < 10) tmax = tmin + 10;
            const x0 = 40, x1 = width - 10, y0 = 10, y1 = height - 24;
            const X = hh => x0 + hh / 23 * (x1 - x0), Yt = t => y1 - (t - tmin) / (tmax - tmin) * (y1 - y0);
            g.font = "12px '" + Tema.fuente600 + "'"; g.fillStyle = String(Tema.tintaSuave); g.strokeStyle = "rgba(255,255,255,0.13)"; g.lineWidth = 1;
            for (let t = tmin; t <= tmax; t += 5) { g.beginPath(); g.moveTo(x0, Yt(t)); g.lineTo(x1, Yt(t)); g.stroke(); g.fillText(t + "°", 4, Yt(t) + 4); }
            const lluvia = String(Tema.lluvia);
            g.globalAlpha = 0.55; g.fillStyle = lluvia;
            const an = (x1 - x0) / 24 * 0.7;
            for (const x of H) if (x.lluvia > 0) { const hgt = x.lluvia / 100 * (y1 - y0); g.fillRect(X(x.hora) - an / 2, y1 - hgt, an, hgt); }
            g.globalAlpha = 1;
            g.beginPath(); H.forEach((x, i) => i ? g.lineTo(X(x.hora), Yt(x.temp)) : g.moveTo(X(x.hora), Yt(x.temp)));
            g.strokeStyle = String(Tema.acento); g.lineWidth = 4; g.lineJoin = "round"; g.stroke();
            g.fillStyle = String(Tema.tintaSuave); g.textAlign = "center";
            for (const hh of [0, 6, 12, 18, 23]) g.fillText(hh + " h", X(hh), height - 6);
            if (raiz.esHoy) {   // ahora
                g.strokeStyle = String(Tema.acento); g.setLineDash([6, 6]); g.lineWidth = 2;
                g.beginPath(); g.moveTo(X(raiz.ahora), y0); g.lineTo(X(raiz.ahora), y1); g.stroke(); g.setLineDash([]);
            }
        }
    }
}
