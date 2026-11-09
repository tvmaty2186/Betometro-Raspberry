import QtQuick
import Betometro

// Botón MARCAR / LARGAR. Marca al apoyar el dedo (no al soltar), como la app.
Item {
    id: raiz
    property var est: ({})
    property string subtitulo: ""        // en carrera, en vez de «FIN DE VUELTA n» (recorrido libre)
    signal marcar()
    readonly property bool enCurso: !!est.enCurso
    function efecto() { golpe.restart(); }
    BotonAmarillo { id: boton; anchors.fill: parent; radio: 22; texto: "" }
    Rectangle { id: brillo; anchors.fill: parent; radius: 22; color: Tema.acentoClaro; opacity: 0 }
    Column {
        anchors.centerIn: parent
        width: parent.width - 30
        Text {
            width: parent.width; horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.Fit; height: raiz.height * 0.5; verticalAlignment: Text.AlignVCenter
            text: raiz.enCurso ? "MARCAR" : "LARGAR"; color: Tema.acentoTinta
            readonly property real tam: Math.min(raiz.height * 0.47, raiz.width * 0.17)
            font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: tam; letterSpacing: tam * 0.14 }
        }
        Text {
            width: parent.width; horizontalAlignment: Text.AlignHCenter
            text: raiz.enCurso ? (raiz.subtitulo || "FIN DE VUELTA " + raiz.est.n) : "TOCÁ EN LA LARGADA"; color: Tema.acentoTinta
            font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: Math.max(14, Math.min(24, raiz.height * 0.095)); letterSpacing: 3 }
        }
    }
    transform: Scale { id: escala; origin.x: raiz.width / 2; origin.y: raiz.height / 2 }
    SequentialAnimation {
        id: golpe
        ParallelAnimation {
            NumberAnimation { target: escala; properties: "xScale,yScale"; to: 0.97; duration: 40 }
            NumberAnimation { target: brillo; property: "opacity"; to: 0.85; duration: 40 }
        }
        PauseAnimation { duration: 230 }   // ~0,35 s en total: antes 0,2 s y casi no se veía (Matías, 09/11/2026)
        ParallelAnimation {
            NumberAnimation { target: escala; properties: "xScale,yScale"; to: 1; duration: 80 }
            NumberAnimation { target: brillo; property: "opacity"; to: 0; duration: 80 }
        }
    }
    MouseArea { anchors.fill: parent; onPressed: { raiz.efecto(); raiz.marcar(); } }
}
