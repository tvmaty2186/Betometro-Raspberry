import QtQuick
import Betometro

// Fila grande del Modo Tablet: el nombre a la izquierda y el control a la derecha.
Rectangle {
    id: raiz
    property string etiqueta: ""
    property string detalle: ""          // texto chico debajo del nombre (opcional)
    property bool tocable: false
    signal tocada()
    default property alias contenido: der.data
    width: parent ? parent.width : 600
    height: 96
    radius: Math.max(12, Tema.radio * 1.3)
    border { width: 3; color: Tema.bordeCromo }
    gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
    Rectangle { anchors.fill: parent; radius: parent.radius; color: "#ffffff"; opacity: zona.pressed ? 0.12 : 0 }
    MouseArea { id: zona; anchors.fill: parent; enabled: raiz.tocable; onClicked: raiz.tocada() }
    Column {
        anchors { left: parent.left; leftMargin: 26; verticalCenter: parent.verticalCenter; right: der.left; rightMargin: 16 }
        Text { width: parent.width; elide: Text.ElideRight; text: raiz.etiqueta; color: Tema.tinta
               font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 29 } }
        Text { visible: raiz.detalle !== ""; width: parent.width; elide: Text.ElideRight; text: raiz.detalle; color: Tema.acento
               font { family: Tema.fuente600; pixelSize: 21 } }
    }
    Row { id: der; anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter } spacing: 12 }
}
