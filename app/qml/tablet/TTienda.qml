import QtQuick
import Betometro

// Tienda (Matías, 08/11/2026): herramientas que se abren APARTE de Betómetro (Betómetro se cierra, se abre la
// herramienta y al salir vuelve solo). Por ahora sin herramientas: solo los lugares, en cuadraditos punteados.
// Cuando haya una, ocupa un cuadradito con su ícono y su nombre. Nunca con una carrera en curso.
// El catálogo se baja de Betometro-tienda al abrir la pantalla (app/tienda.h); por ahora está vacío.
TPantalla {
    id: raiz
    titulo: "Tienda"
    Component.onCompleted: Tienda.buscar()
    readonly property var lista: Tienda.herramientas
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
            TBoton {
                required property int index
                readonly property var h: index < raiz.lista.length ? raiz.lista[index] : null
                width: grilla.an; height: grilla.al
                activo: !!h; icono: h ? "tienda" : ""; nombre: h ? h.nombre : ""
                dato: h ? "Muy pronto" : ""
                onTocado: Navegacion.avisar(h.nombre + ": todavía no se puede instalar")
            }
        }
    }
    Text {
        id: nota
        anchors { left: raiz.area.left; right: raiz.area.right; bottom: raiz.area.bottom }
        horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        text: Tienda.estado === "buscando" ? "Buscando herramientas…"
            : Tienda.estado === "sinred" ? "Sin internet: se muestra lo último que se bajó."
            : raiz.lista.length ? "Se instalan por wifi, como las actualizaciones."
            : "Acá van a aparecer las herramientas. Se instalan por wifi, como las actualizaciones."
        color: Tema.tintaSuave
        font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 22 }
    }
}
