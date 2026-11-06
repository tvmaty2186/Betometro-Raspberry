import QtQuick
import Betometro

// Arriba de cada pantalla: Volver, título cromado y rayas (.cab + .rayasT).
Item {
    id: raiz
    property string titulo: ""
    property real derechaRayas: Contexto.horizontal ? 90 : 150
    signal volver()
    anchors.fill: parent
    // Muestra (06/11/2026): la barra del Modo Tablet también en el modo normal
    readonly property bool barra: Contexto.prueba.indexOf("barra") >= 0
    TBarra { visible: raiz.barra; titulo: raiz.titulo; onVolver: raiz.volver() }
    Rayas { visible: Tema.avispon && !raiz.barra; x: parent.width - raiz.derechaRayas - width; y: -30 }
    Row {
        visible: !raiz.barra
        x: Contexto.horizontal ? 28 : 24; y: 24
        spacing: 14
        Volver { anchors.verticalCenter: parent.verticalCenter; onTocado: raiz.volver() }
        TextoCromo { anchors.verticalCenter: parent.verticalCenter; texto: raiz.titulo; tam: 40 }
    }
}
