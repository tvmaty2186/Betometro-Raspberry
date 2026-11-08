import QtQuick
import Betometro

// Un dato encima del video, en tira finita (muestras docs/muestras/video/). Con «Datos» abierto (en pausa) se mueve:
// mantener apretado → se pone translúcido con borde punteado → arrastrar → se pega a los otros datos y a los
// bordes → al soltar queda ahí y se guarda (para todos los videos).
Item {
    id: d
    required property string clave
    required property string nombre     // para «se pega a «Vuelta»»
    required property Item reproductor   // la VistaVideo (la de la Pi o la de la tele)
    default property alias contenido: cont.data
    property bool moviendo: false
    property real ox: 0
    property real oy: 0
    width: cont.childrenRect.width
    height: cont.childrenRect.height
    x: d.reproductor.xDato(clave, width, height)   // nunca encima de la X de cerrar
    y: d.reproductor.yDe(clave, height)
    z: moviendo ? 5 : 1
    visible: d.reproductor.completo && d.reproductor.prendido(clave) && (d.reproductor.editando || d.reproductor.e.lap >= 0 || clave === "vuelta")
    opacity: moviendo ? 0.55 : 1

    Item { id: cont }

    // Borde punteado mientras se mueve (se dibuja una sola vez, no en cada cuadro)
    Canvas {
        anchors { fill: parent; margins: -7 }
        visible: d.moviendo
        onVisibleChanged: if (visible) requestPaint()
        onWidthChanged: requestPaint()
        onPaint: {
            const g = getContext("2d");
            g.reset();
            g.strokeStyle = Tema.sugerida; g.lineWidth = 3; g.setLineDash([3, 2]);
            const r = Math.min(height / 2, 30);
            g.beginPath(); g.roundedRect(1.5, 1.5, width - 3, height - 3, r, r); g.stroke();
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: d.reproductor.editando
        pressAndHoldInterval: 350
        preventStealing: true
        onPressed: (m) => { const p = mapToItem(d.reproductor, m.x, m.y); d.ox = p.x - d.x; d.oy = p.y - d.y; }
        onPressAndHold: d.moviendo = true
        onPositionChanged: (m) => {
            if (!d.moviendo) return;
            const p = mapToItem(d.reproductor, m.x, m.y);
            const r = d.reproductor.imantar(d, p.x - d.ox, p.y - d.oy);
            d.x = r.x; d.y = r.y;
        }
        onReleased: soltar()
        onCanceled: soltar()
        function soltar() {
            if (!d.moviendo) return;
            d.moviendo = false;
            d.reproductor.guardarPos(d.clave, d.x, d.y, d.width, d.height);
            d.x = Qt.binding(() => d.reproductor.xDato(d.clave, d.width, d.height));
            d.y = Qt.binding(() => d.reproductor.yDe(d.clave, d.height));
        }
    }
}
