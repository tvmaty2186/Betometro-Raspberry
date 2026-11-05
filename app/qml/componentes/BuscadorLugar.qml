import QtQuick
import Betometro

// Buscar una localidad para el clima: se escribe con el teclado y se ELIGE de la lista de lugares
// reales (buscador de Open-Meteo y, si no encuentra, OpenStreetMap). Así no se guarda nada mal escrito.
// elegido(lugar): {nombre, detalle, lat, lon}
Item {
    id: raiz
    anchors.fill: parent
    z: 95
    readonly property bool abierto: teclado.visible   // (el Item siempre visible: si no, escondía al teclado)
    signal elegido(var lugar)
    readonly property bool h: Contexto.horizontal

    function abrir(texto) {
        teclado.abrir("texto", "Localidad: escribí y elegí de la lista", texto || "", 60);
        Clima.buscarLugar(texto || "");
    }
    function elegir(l) { teclado.cerrar(); raiz.elegido(l); }

    Teclado {
        id: teclado
        onValorChanged: espera.restart()
        onAceptado: v => { if (Clima.sugerencias.length > 0) raiz.elegido(Clima.sugerencias[0]); }
    }
    Timer { id: espera; interval: 450; onTriggered: Clima.buscarLugar(teclado.valor) }

    // Lugares encontrados, arriba del teclado
    Column {
        z: 100
        visible: raiz.abierto
        x: raiz.h ? 30 : 16; y: raiz.h ? 14 : 24
        width: parent.width - 2 * x
        spacing: 8
        Text {
            visible: Clima.buscandoLugar || Clima.sugerencias.length === 0
            text: Clima.buscandoLugar ? "Buscando…"
                  : teclado.valor.trim().length < 3 ? "Escribí al menos 3 letras"
                  : "No encontré «" + teclado.valor.trim() + "». Revisá cómo se escribe (hace falta wifi)."
            color: Tema.tinta; font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 20 }
        }
        Flow {
            width: parent.width
            spacing: 10
            Repeater {
                model: Clima.sugerencias.slice(0, raiz.h ? 4 : 8)
                Rectangle {
                    required property var modelData
                    width: raiz.h ? (parent.width - 10) / 2 : parent.width
                    height: raiz.h ? 66 : 72; radius: 18
                    color: m.pressed ? Tema.acento : Qt.rgba(0.04, 0.16, 0.09, 0.95)
                    border { width: 2; color: Tema.bordeCromo }
                    Column {
                        anchors { left: parent.left; leftMargin: 18; right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        Text { width: parent.width; elide: Text.ElideRight; text: modelData.nombre
                               color: m.pressed ? Tema.acentoTinta : Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: raiz.h ? 22 : 23 } }
                        Text { width: parent.width; elide: Text.ElideRight; text: modelData.detalle; visible: text !== ""
                               color: m.pressed ? Tema.acentoTinta : Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: raiz.h ? 15 : 16 } }
                    }
                    MouseArea { id: m; anchors.fill: parent; onClicked: raiz.elegir(modelData) }
                }
            }
        }
    }
}
