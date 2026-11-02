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
                case "carreras": return "pantallas/Carreras.qml";
                case "ficha": return "pantallas/Ficha.qml";
                case "practica": return "pantallas/Practica.qml";
                case "carrera": return "pantallas/Carrera.qml";
                case "ajustes": return "pantallas/Ajustes.qml";
                case "conectividad": return "pantallas/Conectividad.qml";
                case "camara": return "pantallas/AjustesCamara.qml";
                case "wifi": return "pantallas/WifiClave.qml";
                case "chequeo": return "pantallas/Chequeo.qml";
                case "informe": return "pantallas/Informe.qml";
                case "compartir": return "pantallas/Compartir.qml";
                case "actualizar": return "pantallas/Actualizar.qml";
                case "editor": return "pantallas/Editor.qml";
                default: return "pantallas/EnConstruccion.qml";
                }
            }
            onLoaded: if (item && item.hasOwnProperty("pantallaNombre")) item.pantallaNombre = Navegacion.actual
        }
        Aviso { id: aviso }
        Connections { target: Navegacion; function onAviso(texto) { aviso.mostrar(texto); } }
    }
}
