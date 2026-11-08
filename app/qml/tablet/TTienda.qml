import QtQuick
import Betometro

// Tienda (Matías, 08/11/2026): herramientas que se abren APARTE de Betómetro (Betómetro se cierra, se abre la
// herramienta y al salir vuelve solo). Por ahora sin herramientas: solo los lugares, en cuadraditos punteados.
// Cuando haya una, ocupa un cuadradito con su ícono y su nombre. Nunca con una carrera en curso.
TPantalla {
    id: raiz
    titulo: "Tienda"
    Grid {
        id: grilla
        anchors { left: raiz.area.left; right: raiz.area.right; top: raiz.area.top; bottom: nota.top; bottomMargin: 16 }
        columns: raiz.h ? 4 : 2
        rows: raiz.h ? 2 : 4
        spacing: 18
        readonly property real an: (width - (columns - 1) * spacing) / columns
        readonly property real al: (height - (rows - 1) * spacing) / rows
        Repeater {
            model: grilla.columns * grilla.rows
            TBoton { width: grilla.an; height: grilla.al; activo: false; icono: ""; nombre: "" }
        }
    }
    Text {
        id: nota
        anchors { left: raiz.area.left; right: raiz.area.right; bottom: raiz.area.bottom }
        horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        text: "Acá van a aparecer las herramientas. Se instalan por wifi, como las actualizaciones."
        color: Tema.tintaSuave
        font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 22 }
    }
}
