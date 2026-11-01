import QtQuick
import QtQuick.Window
import Betometro

// Ventana principal. La pantalla real (Touch Display 2) es de pie, 720×1280:
// en horizontal se gira toda la interfaz desde acá (y el tacto gira con ella).
Window {
    id: ventana
    readonly property bool h: Contexto.horizontal
    readonly property bool girar: Contexto.fisica && h
    width: Contexto.fisica ? 720 : (h ? 1280 : 720)
    height: Contexto.fisica ? 1280 : (h ? 720 : 1280)
    visible: true
    color: "#070908"
    title: "Betómetro"

    Item {
        id: interfaz
        width: ventana.h ? 1280 : 720
        height: ventana.h ? 720 : 1280
        anchors.centerIn: parent
        rotation: ventana.girar ? 90 : 0
        clip: true

        Loader {
            anchors.fill: parent
            source: {
                switch (Navegacion.actual) {
                case "inicio": return "pantallas/Inicio.qml";
                default: return "pantallas/EnConstruccion.qml";
                }
            }
            onLoaded: if (item && item.hasOwnProperty("nombre")) item.nombre = Navegacion.actual
        }
    }
}
