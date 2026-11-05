import QtQuick
import Betometro

// Botón grande del Modo Tablet (como una app de tablet): ícono, nombre y un dato útil abajo.
// tipo: "" normal · "acento" (el principal, color del tema) · "peligro" (rojo)
Rectangle {
    id: raiz
    property string nombre: ""
    property string dato: ""
    property string icono: ""
    property int clima: -1              // si es ≥ 0, ícono del clima en vez de uno de línea
    property string tipo: ""
    property bool activo: true
    property real tamIcono: 72
    signal tocado()
    readonly property color tinta: tipo === "acento" ? Tema.acentoTinta : Tema.tinta
    radius: Math.max(14, Tema.radio * 1.7)
    opacity: activo ? 1 : 0.45
    border { width: 3; color: tipo === "acento" ? Tema.acentoClaro : tipo === "peligro" ? "#ffb3aa" : Tema.bordeCromo }
    gradient: Gradient {
        GradientStop { position: 0; color: raiz.tipo === "acento" ? Tema.acentoClaro : raiz.tipo === "peligro" ? "#e0473a" : Qt.lighter(Tema.caja1, Tema.avispon ? 1.35 : 1.25) }
        GradientStop { position: 0.55; color: raiz.tipo === "acento" ? Tema.acento : raiz.tipo === "peligro" ? "#b8231a" : Tema.caja1 }
        GradientStop { position: 1; color: raiz.tipo === "acento" ? Tema.acentoOscuro : raiz.tipo === "peligro" ? "#8f130b" : Tema.caja2 }
    }
    // Brillo de arriba, solo en el Avispón (como las cajas cromadas del tema)
    Rectangle {
        visible: Tema.avispon
        x: parent.width * 0.08; y: 6; width: parent.width * 0.84; height: parent.height * 0.32; radius: height / 2
        gradient: Gradient { GradientStop { position: 0; color: Qt.rgba(1, 1, 1, 0.11) } GradientStop { position: 1; color: Qt.rgba(1, 1, 1, 0) } }
    }
    Rectangle { anchors.fill: parent; radius: parent.radius; color: "#ffffff"; opacity: zona.pressed ? 0.18 : 0 }
    Column {
        anchors.centerIn: parent
        width: parent.width - 20
        spacing: 10
        TIcono { visible: raiz.clima < 0 && raiz.icono !== ""; anchors.horizontalCenter: parent.horizontalCenter
                 width: raiz.tamIcono; height: raiz.tamIcono; nombre: raiz.icono; color: raiz.tinta }
        IconoClima { visible: raiz.clima >= 0; anchors.horizontalCenter: parent.horizontalCenter
                     width: raiz.tamIcono * 1.1; height: width; codigo: Math.max(0, raiz.clima) }
        Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; maximumLineCount: 2
               text: raiz.nombre; color: raiz.tinta
               font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 29 } }
        Text { visible: raiz.dato !== ""; width: parent.width; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight
               text: raiz.dato; color: raiz.tipo === "" ? Tema.acento : raiz.tinta; opacity: raiz.tipo === "" ? 1 : 0.85
               font { family: Tema.fuente600; pixelSize: 21 } }
    }
    MouseArea { id: zona; anchors.fill: parent; enabled: raiz.activo; onClicked: raiz.tocado() }
}
