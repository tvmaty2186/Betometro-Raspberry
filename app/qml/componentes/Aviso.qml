import QtQuick
import Betometro

// Cartel amarillo arriba que se va solo a los 2,2 s (avisar() de la app).
Rectangle {
    id: raiz
    function mostrar(texto) { t.text = texto; visible = true; reloj.restart(); }
    visible: false
    z: 100
    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 18 }
    width: Math.min(parent.width - 40, t.implicitWidth + 48); height: 52; radius: 26
    color: Tema.amarillo
    border { width: 2; color: "#6b5200" }
    Text { id: t; anchors.centerIn: parent; color: Tema.negro; font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 20 } }
    Timer { id: reloj; interval: 2200; onTriggered: raiz.visible = false }
}
