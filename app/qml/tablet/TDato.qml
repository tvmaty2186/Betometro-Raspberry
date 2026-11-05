import QtQuick
import Betometro

// Caja grande con un número que se cambia con − / +, y tocándolo se escribe con el teclado.
Rectangle {
    id: raiz
    property string etiqueta: ""
    property string texto: ""
    property string unidad: ""
    property bool activo: true
    signal menos()
    signal mas()
    signal tocado()
    height: 136
    radius: Math.max(12, Tema.radio * 1.3)
    opacity: activo ? 1 : 0.5
    border { width: 3; color: Tema.bordeCromo }
    gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
    Text { x: 26; y: 12; text: raiz.etiqueta.toUpperCase(); color: Tema.tintaSuave
           font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 21; letterSpacing: 2 } }
    component Tecla: Rectangle {
        property string t
        signal tocada()
        width: 86; height: 70; radius: 20
        color: zona.pressed ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
        border { width: 3; color: Tema.bordeCromo }
        Text { anchors.centerIn: parent; text: parent.t; color: zona.pressed ? Tema.acentoTinta : Tema.tinta; font { family: Tema.fuente800; pixelSize: 46 } }
        MouseArea { id: zona; anchors.fill: parent; anchors.margins: -4; enabled: raiz.activo; onClicked: parent.tocada()
                    onPressAndHold: rep.start(); onReleased: rep.stop(); onCanceled: rep.stop() }
        Timer { id: rep; interval: 110; repeat: true; onTriggered: parent.tocada() }
    }
    Tecla { t: "−"; x: 22; y: 50; onTocada: raiz.menos() }
    Tecla { t: "+"; anchors.right: parent.right; anchors.rightMargin: 22; y: 50; onTocada: raiz.mas() }
    Text {
        anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; verticalCenterOffset: 16 }
        textFormat: Text.StyledText
        text: raiz.texto + (raiz.unidad ? "<font size='4'> " + raiz.unidad + "</font>" : "")
        color: Tema.numero
        font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 66 }
        MouseArea { anchors.fill: parent; anchors.margins: -10; enabled: raiz.activo; onClicked: raiz.tocado() }
    }
}
