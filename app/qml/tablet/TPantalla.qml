import QtQuick
import Betometro

// Base de las pantallas del Modo Tablet: fondo del tema, barra de arriba y el área de abajo (contenido).
Item {
    id: raiz
    property string titulo: ""
    property string volverA: "inicio"
    readonly property bool h: Contexto.horizontal
    default property alias contenido: area.data
    readonly property alias area: area
    Fondo { anchors.fill: parent }
    Item {
        id: area
        x: raiz.h ? 22 : 18; y: 96
        width: parent.width - 2 * x; height: parent.height - y - (raiz.h ? 18 : 18)
    }
    TBarra { titulo: raiz.titulo; onVolver: Navegacion.ir(raiz.volverA) }
}
