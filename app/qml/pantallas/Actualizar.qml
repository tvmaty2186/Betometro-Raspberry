import QtQuick
import Betometro

// Pantalla 12: Actualizar. Muestras #s12 y #v12.
// En la PC la descarga es simulada; la de verdad (paquete firmado, copia A/B) va en E5.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    property real progreso: 0
    property string estado: Crono.enCurso ? "carrera" : "descargando"   // descargando, lista, carrera
    Timer {
        running: raiz.estado === "descargando"; interval: 120; repeat: true
        onTriggered: { raiz.progreso = Math.min(100, raiz.progreso + 1.5); if (raiz.progreso >= 100) raiz.estado = "lista"; }
    }

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
                   text: "Qué trae: arreglos y mejoras de la pantalla de carrera"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 22 } }
            Rectangle {
                width: parent.width; height: 22; radius: 11; color: "#0a2414"; border { width: 2; color: Tema.bordeCromo }
                Rectangle {
                    x: 2; y: 2; height: parent.height - 4; radius: 9; width: Math.max(height, (parent.width - 4) * raiz.progreso / 100)
                    gradient: Gradient { GradientStop { position: 0; color: Tema.acentoClaro } GradientStop { position: 1; color: Tema.acento } }
                }
            }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter
                   text: raiz.estado === "carrera" ? "Terminá la carrera antes de actualizar" : raiz.estado === "lista" ? "Lista: se usa al reiniciar el equipo" : "Descargando… " + Math.floor(raiz.progreso) + " %"
                   color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; lineHeight: 1.3
                   text: "Podés seguir usando Betómetro. Se instala en la copia de reserva.\nSi algo falla, vuelve solo a la versión anterior."
                   color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 19 } }
            BotonAmarillo { visible: raiz.estado === "lista"; anchors.horizontalCenter: parent.horizontalCenter; width: 300; height: 64; texto: "REINICIAR"
                            onTocado: Navegacion.avisar("En la Raspberry reinicia en la versión nueva") }
        }
    }
}
