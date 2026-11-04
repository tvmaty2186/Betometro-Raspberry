import QtQuick
import Betometro

// Pantalla 12: Actualizar. Muestras #s12 y #v12.
// Baja el paquete (por internet o de un pendrive) y corre su instalador, que guarda la versión
// anterior, pone la nueva y reinicia el equipo. En la PC la descarga es simulada.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property string estado: Crono.enCurso ? "carrera" : Actualizador.estado   // nada, descargando, instalando, lista, error, carrera
    readonly property real progreso: Actualizador.progreso
    Component.onCompleted: Actualizador.buscar()   // siempre: puede haber salido otra más nueva

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "ACTUALIZAR"; derechaRayas: raiz.h ? 90 : 30; onVolver: Navegacion.ir("inicio") }

    Tarjeta {
        x: raiz.h ? 200 : 24; y: raiz.h ? 130 : 180
        width: raiz.h ? 880 : parent.width - 48; height: raiz.h ? 500 : 800
        Column {
            anchors { fill: parent; margins: raiz.h ? 40 : 34; topMargin: raiz.h ? 34 : 60 }
            spacing: raiz.h ? 16 : 30
            Flow {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 50
                flow: raiz.h ? Flow.LeftToRight : Flow.TopToBottom
                Column { Sub { anchors.horizontalCenter: parent.horizontalCenter; text: "Tenés"; tam: 15 }
                         Digito { anchors.horizontalCenter: parent.horizontalCenter; text: Contexto.version; tam: 100 } }
                Text { visible: raiz.h; text: "→"; color: Tema.acento; font { family: Tema.fuente800; pixelSize: 60 } topPadding: 30 }
                Column { Sub { anchors.horizontalCenter: parent.horizontalCenter; text: "Nueva"; tam: 15 }
                         Digito { anchors.horizontalCenter: parent.horizontalCenter; text: Contexto.versionNueva || Contexto.version; tam: 100; color: Tema.acento } }
            }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                   text: Actualizador.versionNueva ? "Qué trae: " + (Actualizador.notas || "mejoras")
                         : Actualizador.busqueda === "buscando" ? "Buscando por internet…"
                         : Actualizador.busqueda === "sinred" ? "No hay internet: no se pudo buscar. Revisá el wifi en Ajustes → Conectividad."
                         : Actualizador.ultima ? "La última publicada es la " + Actualizador.ultima + "." : "No hay una versión nueva por internet."; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 22 } }
            Rectangle {
                visible: raiz.estado === "descargando" || raiz.estado === "instalando" || raiz.estado === "lista"
                width: parent.width; height: 22; radius: 11; color: "#0a2414"; border { width: 2; color: Tema.bordeCromo }
                Rectangle {
                    x: 2; y: 2; height: parent.height - 4; radius: 9; width: Math.max(height, (parent.width - 4) * raiz.progreso / 100)
                    gradient: Gradient { GradientStop { position: 0; color: Tema.acentoClaro } GradientStop { position: 1; color: Tema.acento } }
                }
            }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter
                   text: raiz.estado === "carrera" ? "Terminá la carrera antes de actualizar"
                         : raiz.estado === "lista" ? "Instalando… no la apagues"
                         : raiz.estado === "descargando" ? "Descargando… " + Math.floor(raiz.progreso) + " %"
                         : raiz.estado === "instalando" ? "Instalando… " + Math.floor(raiz.progreso) + " %"
                         : raiz.estado === "error" ? Actualizador.mensaje
                         : Actualizador.versionNueva ? "Versión " + Actualizador.versionNueva + (Actualizador.origen === "pendrive" ? " en el pendrive" : " disponible")
                         : Actualizador.busqueda === "buscando" ? "Buscando…"
                         : Actualizador.busqueda === "sinred" ? "Sin internet"
                         : "Estás al día"
                   color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; lineHeight: 1.3
                   text: raiz.estado === "lista" ? "Puede tardar hasta 10 minutos y la pantalla puede quedar negra.\nAl terminar, se reinicia sola en la versión nueva."
                         : "Al terminar, el equipo se reinicia solo.\nSe guarda la versión anterior por si algo falla."
                   color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 19 } }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter; spacing: 16
                BotonAmarillo { visible: false; width: 300; height: 64; texto: "REINICIAR"   // no: cortaba la instalación a la mitad
                                onTocado: { Actualizador.reiniciar(); if (!Sistema.raspberry) Navegacion.avisar("En la Raspberry reinicia en la versión nueva"); } }
                BotonAmarillo { visible: (raiz.estado === "nada" || raiz.estado === "error") && !!Actualizador.versionNueva
                                width: 300; height: 64; texto: "ACTUALIZAR"; onTocado: Actualizador.actualizar() }
                BotonAmarillo { visible: (raiz.estado === "nada" || raiz.estado === "error") && !Actualizador.versionNueva
                                enabled: Actualizador.busqueda !== "buscando"; opacity: enabled ? 1 : 0.5
                                width: 300; height: 64; texto: "BUSCAR"; onTocado: Actualizador.buscar() }
                BotonCromo { visible: raiz.estado === "nada" || raiz.estado === "error"; width: 260; height: 64; tamTexto: 18
                             texto: "Buscar en pendrive"; onTocado: Actualizador.buscarPendrive() }
            }
        }
    }
}
