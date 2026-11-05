import QtQuick
import Betometro

// Actualizar en Modo Tablet. Misma lógica que Actualizar.qml (busca al entrar, BUSCAR, pendrive, sin REINICIAR).
TPantalla {
    id: raiz
    titulo: "Actualizar"
    volverA: "ajustes"
    readonly property string estado: Crono.enCurso ? "carrera" : Actualizador.estado
    Component.onCompleted: Actualizador.buscar()
    readonly property bool quieto: estado === "nada" || estado === "error"

    Rectangle {
        id: caja
        width: raiz.area.width; height: raiz.h ? 330 : 520
        radius: Math.max(14, Tema.radio * 1.5)
        border { width: 3; color: Tema.bordeCromo }
        gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
        Grid {
            anchors.horizontalCenter: parent.horizontalCenter; y: 24
            columns: raiz.h ? 3 : 1; spacing: raiz.h ? 50 : 10
            horizontalItemAlignment: Grid.AlignHCenter; verticalItemAlignment: Grid.AlignVCenter
            Column { Text { anchors.horizontalCenter: parent.horizontalCenter; text: "TENÉS"; color: Tema.tintaSuave; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22; letterSpacing: 3 } }
                     Text { anchors.horizontalCenter: parent.horizontalCenter; text: Contexto.version; color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 110 } } }
            Text { visible: raiz.h; text: "→"; color: Tema.acento; font { family: Tema.fuente800; pixelSize: 70 } }
            Column { Text { anchors.horizontalCenter: parent.horizontalCenter; text: "NUEVA"; color: Tema.tintaSuave; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22; letterSpacing: 3 } }
                     Text { anchors.horizontalCenter: parent.horizontalCenter; text: Actualizador.versionNueva || Contexto.version; color: Tema.acento; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 110 } } }
        }
        Text {
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 26 }
            width: parent.width - 60; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
            color: raiz.estado === "error" ? Tema.alerta : Tema.tinta
            font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 }
            text: raiz.estado === "carrera" ? "Terminá la carrera antes de actualizar"
                  : raiz.estado === "lista" ? "Instalando… no la apagues: se reinicia sola"
                  : raiz.estado === "descargando" ? "Descargando… " + Math.floor(Actualizador.progreso) + " %"
                  : raiz.estado === "instalando" ? "Instalando… " + Math.floor(Actualizador.progreso) + " %"
                  : raiz.estado === "error" ? Actualizador.mensaje
                  : Actualizador.versionNueva ? "Versión " + Actualizador.versionNueva + " disponible"
                  : Actualizador.busqueda === "buscando" ? "Buscando…"
                  : Actualizador.busqueda === "sinred" ? "Sin internet: revisá el wifi"
                  : "Estás al día"
        }
    }
    Grid {
        y: caja.height + 18
        width: raiz.area.width; height: raiz.area.height - y
        columns: raiz.h ? 3 : 1; spacing: 18
        readonly property real an: (width - (columns - 1) * spacing) / columns
        readonly property real al: (height - ((raiz.h ? 1 : 3) - 1) * spacing) / (raiz.h ? 1 : 3)
        TBoton { width: parent.an; height: parent.al; tipo: "acento"; icono: "actualizar"; nombre: "ACTUALIZAR"
                 activo: raiz.quieto && !!Actualizador.versionNueva; onTocado: Actualizador.actualizar() }
        TBoton { width: parent.an; height: parent.al; icono: "buscar"; nombre: "Buscar"; dato: "por internet"
                 activo: raiz.quieto && Actualizador.busqueda !== "buscando"; onTocado: Actualizador.buscar() }
        TBoton { width: parent.an; height: parent.al; icono: "tarjeta"; nombre: "Pendrive"; dato: "buscar ahí"
                 activo: raiz.quieto; onTocado: Actualizador.buscarPendrive() }
    }
}
