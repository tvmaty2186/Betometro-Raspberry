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
    color: Tema.fondo3
    title: "Betómetro"
    // Con una tele en el HDMI, la interfaz se queda siempre en la pantalla de 5" (DSI), aunque Qt
    // pusiera la tele primero
    screen: Tele.pantallaPi
    // y en su lugar del escritorio (si no, Qt la puede llevar a la pantalla que está en 0,0)
    x: Tele.pantallaPi ? Tele.pantallaPi.virtualX : 0
    y: Tele.pantallaPi ? Tele.pantallaPi.virtualY : 0

    Item {
        id: interfaz
        width: ventana.h ? 1280 : 720
        height: ventana.h ? 720 : 1280
        anchors.centerIn: parent
        rotation: ventana.girar ? 90 : 0
        clip: true

        Loader {
            anchors.fill: parent
            // Las pantallas que tienen versión grande (carpeta tablet/) van siempre así. El Modo Tablet solo
            // cambia el inicio (Matías, 06/11/2026: «lo único que cambia entre normal y tablet es el inicio»).
            // Las 10 versiones viejas de esas pantallas se borraron en la 1.0.29 (auditoría 01-1.1).
            readonly property var tablet: ({ inicio: "TInicio", practica: "TPractica", clima: "TClima", camara: "TCamara", drive: "TDrive",
                                              ajustes: "TAjustes", ajustesSeccion: "TAjustesSeccion", carreras: "TCarreras",
                                              ficha: "TFicha", chequeo: "TChequeo", simulador: "TSimulador", prueba: "TPrueba", manual: "TManual", tienda: "TTienda", conectividad: "TConectividad", actualizar: "TActualizar" })
            source: {
                if ((Navegacion.actual !== "inicio" || Tema.tablet) && tablet[Navegacion.actual]) return "tablet/" + tablet[Navegacion.actual] + ".qml";
                switch (Navegacion.actual) {
                case "inicio": return "pantallas/Inicio.qml";
                case "carrera": return "pantallas/Carrera.qml";
                case "wifi": return "pantallas/WifiClave.qml";
                case "informe": return "pantallas/Informe.qml";
                case "compartir": return "pantallas/Compartir.qml";
                case "editor": return "pantallas/Editor.qml";
                default: return "pantallas/EnConstruccion.qml";
                }
            }
            onLoaded: if (item && item.hasOwnProperty("pantallaNombre")) item.pantallaNombre = Navegacion.actual
        }
        Aviso { id: aviso }
        Connections { target: Navegacion; function onAviso(texto) { aviso.mostrar(texto); } }
        // La Prueba completa arrancada desde el celu (/prueba): se abre su pantalla, que es donde se contesta «¿Se escuchó?»
        Connections { target: PruebaCompleta; function onEmpezo() { if (Navegacion.actual !== "prueba") Navegacion.ir("prueba"); } }
        // Al prender: cómo salió la última actualización («Se instaló la 1.0.28» o que volvió a la anterior)
        Component.onCompleted: if (Actualizador.resultado) Qt.callLater(() => aviso.mostrar(Actualizador.resultado))
    }

    // Modo tele: la ventana de la tele (HDMI) solo si hay una segunda pantalla (docs/video.md)
    // (Instantiator y no Loader: un Window adentro de un Item queda metido en la ventana principal)
    Instantiator {
        active: Tele.hay && !!ventana.screen && ventana.screen.name === Tele.pantallaPi.name
        delegate: VentanaTele {}
    }
}
