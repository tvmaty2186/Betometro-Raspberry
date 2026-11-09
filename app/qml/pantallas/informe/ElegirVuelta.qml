import QtQuick
import Betometro

// Fila de botones para cambiar de vuelta desde adentro del mapa y del gráfico a pantalla completa
// (Matías, 09/11/2026, muestra aprobada): ‹ [TODAS] V1 V2 … ›. La elegida en amarillo, como «Vel.».
// Solo las vueltas con recorrido grabado. Con muchas vueltas, la fila se desliza con el dedo.
Item {
    id: raiz
    property var vueltas: []         // inf.vueltas: las que tienen recorrido (m) se pueden elegir
    property int elegida: -1         // índice en `vueltas`; -1 = TODAS
    property bool conTodas: false    // el gráfico tiene «TODAS» (toda la carrera)
    property int largadas: 1
    signal elegir(int indice)        // -1 = TODAS

    readonly property var lista: {
        const r = [];
        if (conTodas) r.push(-1);
        for (let i = 0; i < vueltas.length; i++) if (vueltas[i] && vueltas[i].m) r.push(i);
        return r;
    }
    readonly property int pos: lista.indexOf(elegida)
    function mover(paso) {
        if (!lista.length) return;
        const p = pos < 0 ? 0 : Math.max(0, Math.min(lista.length - 1, pos + paso));
        if (lista[p] !== elegida) elegir(lista[p]);
    }
    function nombre(i) {
        if (i < 0) return "TODAS";
        const v = vueltas[i];
        return "V" + v.n + (largadas > 1 ? " L" + v.largada : "");
    }
    visible: lista.length > 1
    height: 44

    component Boton: Rectangle {
        property string texto
        property bool activo: false
        property bool apagado: false
        signal tocado()
        width: Math.max(62, t.implicitWidth + 28); height: 44; radius: 22
        color: activo ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
        border { width: 1.5; color: activo ? Tema.acento : Tema.bordeCromo }
        opacity: apagado ? 0.35 : 1
        Text { id: t; anchors.centerIn: parent; text: parent.texto; color: parent.activo ? Tema.acentoTinta : Tema.tinta
               font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 21 } }
        MouseArea { anchors.fill: parent; anchors.margins: -4; enabled: !parent.apagado; onClicked: parent.tocado() }
    }

    Boton { id: ant; anchors.left: parent.left; texto: "‹"; apagado: raiz.pos === 0; onTocado: raiz.mover(-1) }
    Boton { id: sig; anchors.right: parent.right; texto: "›"; apagado: raiz.pos >= 0 && raiz.pos === raiz.lista.length - 1; onTocado: raiz.mover(1) }
    Flickable {
        id: desliza
        anchors { left: ant.right; right: sig.left; leftMargin: 10; rightMargin: 10 }
        height: parent.height
        clip: true
        contentWidth: fila.width; boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalFlick
        interactive: contentWidth > width
        Row {
            id: fila
            x: Math.max(0, (desliza.width - width) / 2)   // pocas vueltas: centradas
            spacing: 10
            Repeater {
                model: raiz.lista
                Boton {
                    required property int modelData
                    texto: raiz.nombre(modelData)
                    activo: modelData === raiz.elegida
                    onTocado: raiz.elegir(modelData)
                }
            }
        }
        // La elegida siempre a la vista
        function verElegida() {
            const b = raiz.pos >= 0 ? fila.children[raiz.pos] : null;
            if (!b || !interactive) return;
            if (b.x < contentX) contentX = b.x;
            else if (b.x + b.width > contentX + width) contentX = b.x + b.width - width;
        }
        Connections { target: raiz; function onElegidaChanged() { Qt.callLater(desliza.verElegida); } }
    }
}
