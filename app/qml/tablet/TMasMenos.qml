import QtQuick
import Betometro

// − valor + grandes (en vez de deslizadores o del teclado chico)
Row {
    id: raiz
    property string texto: ""
    property real anchoTexto: 140
    signal menos()
    signal mas()
    signal tocado()                      // tocar el número (para escribirlo con el teclado)
    spacing: 12
    component Tecla: Rectangle {
        property string t
        signal tocada()
        width: 80; height: 66; radius: 20
        color: zona.pressed ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
        border { width: 3; color: Tema.bordeCromo }
        Text { anchors.centerIn: parent; text: parent.t; color: zona.pressed ? Tema.acentoTinta : Tema.tinta; font { family: Tema.fuente800; pixelSize: 44 } }
        MouseArea { id: zona; anchors.fill: parent; anchors.margins: -4; onClicked: parent.tocada()
                    onPressAndHold: rep.start(); onReleased: rep.stop(); onCanceled: rep.stop() }
        Timer { id: rep; interval: 120; repeat: true; onTriggered: parent.tocada() }   // dejándolo apretado, sigue
    }
    Tecla { t: "−"; anchors.verticalCenter: parent.verticalCenter; onTocada: raiz.menos() }
    Text { width: raiz.anchoTexto; anchors.verticalCenter: parent.verticalCenter; horizontalAlignment: Text.AlignHCenter
           text: raiz.texto; color: Tema.numero; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 46 }
           MouseArea { anchors.fill: parent; onClicked: raiz.tocado() } }
    Tecla { t: "+"; anchors.verticalCenter: parent.verticalCenter; onTocada: raiz.mas() }
}
