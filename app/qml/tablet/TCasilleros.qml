import QtQuick
import Betometro

// Tiempo por casilleros (muestra aprobada 1.0.29): MINUTOS y SEGUNDOS por separado, cada uno con − / +.
// Se toca un casillero y se escribe con TTecladoNum, de izquierda a derecha. Las décimas no se editan (quedan en 0).
Row {
    id: raiz
    property int ms: 0
    property string editando: ""         // "m", "s" o "" (lo maneja la pantalla)
    property string escrito: ""          // lo que se va escribiendo en el casillero tocado
    property bool chico: false           // fila de la ficha
    property bool etiquetas: true
    signal cambiar(int ms)
    signal editar(string que)
    readonly property int minutos: Math.floor(ms / 60000)
    readonly property int segundos: Math.floor(ms / 1000) % 60
    spacing: chico ? 8 : 10
    component Tecla: Rectangle {
        property string t
        signal tocada()
        width: raiz.chico ? 58 : 66; height: raiz.chico ? 58 : 62; radius: 18
        color: zona.pressed ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
        border { width: 3; color: Tema.bordeCromo }
        Text { anchors.centerIn: parent; text: parent.t; color: zona.pressed ? Tema.acentoTinta : Tema.tinta; font { family: Tema.fuente800; pixelSize: 38 } }
        MouseArea { id: zona; anchors.fill: parent; anchors.margins: -4; onClicked: parent.tocada()
                    onPressAndHold: rep.start(); onReleased: rep.stop(); onCanceled: rep.stop() }
        Timer { id: rep; interval: 120; repeat: true; onTriggered: parent.tocada() }
    }
    component Casillero: Column {
        id: cas
        property string que
        property string etiqueta
        property int valor
        spacing: 4
        Text { visible: raiz.etiquetas; text: cas.etiqueta; color: Tema.tintaSuave
               font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 21; letterSpacing: 2 } }
        Row {
            spacing: raiz.chico ? 6 : 8
            Tecla { t: "−"; onTocada: raiz.cambiar(Math.max(0, raiz.ms - (cas.que === "m" ? 60000 : 1000))) }
            Rectangle {
                readonly property bool activo: raiz.editando === cas.que
                width: raiz.chico ? 76 : 92; height: raiz.chico ? 58 : 62; radius: 14
                color: Qt.rgba(0, 0, 0, 0.35)
                border { width: activo ? 4 : 2; color: activo ? Tema.acento : Tema.bordeCromo }
                Text {
                    anchors.centerIn: parent
                    text: parent.activo ? raiz.escrito : (cas.que === "s" ? String(cas.valor).padStart(2, "0") : String(cas.valor))
                    color: Tema.numero
                    font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: raiz.chico ? 40 : 46 }
                    Rectangle {   // cursor
                        visible: parent.parent.activo; x: parent.width + 2; y: 6; width: 3; height: parent.height - 12; color: Tema.acento
                        SequentialAnimation on opacity { running: visible; loops: Animation.Infinite
                            NumberAnimation { to: 0; duration: 450 } NumberAnimation { to: 1; duration: 450 } }
                    }
                }
                MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: raiz.editar(cas.que) }
            }
            Tecla { t: "+"; onTocada: raiz.cambiar(Math.min(5999000, raiz.ms + (cas.que === "m" ? 60000 : 1000))) }
        }
    }
    Casillero { que: "m"; etiqueta: "MINUTOS"; valor: raiz.minutos }
    Text { anchors.bottom: parent.bottom; anchors.bottomMargin: raiz.chico ? 4 : 6; text: ":"; color: Tema.tinta
           font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: raiz.chico ? 40 : 46 } }
    Casillero { que: "s"; etiqueta: "SEGUNDOS"; valor: raiz.segundos }
}
