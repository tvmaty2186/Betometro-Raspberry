import QtQuick
import Betometro

// Detalle del simulador (muestra aprobada 1.0.29: docs/muestras/1.0.29/4-simulador-chequeo-d.png).
// Arriba el resultado (rojo: avisar al desarrollador · naranja: revisar), la lista de lo que falló, lo que anduvo
// bien, y a la derecha el QR: el celu abre el reporte de errores para mandarlo.
TPantalla {
    id: raiz
    Component.onCompleted: if (Contexto.prueba === "simulador" && !Simulador.estado) Simulador.correr()   // capturas
    titulo: "Simulador"
    readonly property bool grave: Simulador.estado === "grave"
    readonly property var malas: Simulador.pruebas.filter(p => !p.ok)
    readonly property var buenas: Simulador.pruebas.filter(p => p.ok)
    readonly property string url: "http://" + (Wifi.direccion || "betometro.local") + "/reportes"

    Item {
        id: izq
        width: raiz.h ? raiz.area.width - 420 - 20 : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - 560
        Rectangle {
            id: cabeza
            width: parent.width; height: 118; radius: Math.max(14, Tema.radio * 1.7)
            border { width: 3; color: raiz.grave ? "#ffb3aa" : "#ffd29a" }
            gradient: Gradient {
                GradientStop { position: 0; color: raiz.grave ? "#e0473a" : "#f5a13c" }
                GradientStop { position: 1; color: raiz.grave ? "#8f130b" : "#a8520c" }
            }
            Column {
                anchors.centerIn: parent; width: parent.width - 30; spacing: 6
                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: "#ffffff"
                       text: raiz.grave ? "Hay errores: comunicate con el desarrollador" : "Revisar: hay avisos"
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; color: "#ffffff"; opacity: 0.9
                       text: Simulador.fallas + " de " + Simulador.total + " pruebas fallaron · " + Simulador.cuando + " · v" + Contexto.version
                       font { family: Tema.fuente600; pixelSize: 21 } }
            }
        }
        Flickable {
            y: cabeza.height + 16; width: parent.width; height: parent.height - y
            contentHeight: lista.height; clip: true; boundsBehavior: Flickable.StopAtBounds
            Column {
                id: lista; width: parent.width; spacing: 12
                Repeater {
                    model: raiz.malas.concat(raiz.buenas.length ? [{ nombre: "Las otras " + raiz.buenas.length + " pruebas, bien", ok: true,
                                                                    detalle: raiz.buenas.map(p => p.nombre).join(", ") }] : [])
                    Rectangle {
                        width: lista.width; height: 84; radius: Math.max(12, Tema.radio * 1.3)
                        border { width: 3; color: Tema.bordeCromo }
                        gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
                        Rectangle {
                            x: 24; anchors.verticalCenter: parent.verticalCenter; width: 44; height: 44; radius: 22
                            color: modelData.ok ? Tema.bien : modelData.grave ? Tema.rojo : Tema.naranja
                            border { width: 2; color: "#ffffff" }
                            Text { anchors.centerIn: parent; text: modelData.ok ? "✓" : "!"; color: "#ffffff"; font { family: Tema.fuente800; pixelSize: 26 } }
                        }
                        Column {
                            x: 88; anchors.verticalCenter: parent.verticalCenter; width: parent.width - 108; spacing: 2
                            Text { width: parent.width; elide: Text.ElideRight; text: modelData.nombre; color: Tema.tinta
                                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 26 } }
                            Text { width: parent.width; elide: Text.ElideRight; text: modelData.detalle || ""; color: Tema.acento
                                   font { family: Tema.fuente600; pixelSize: 21 } }
                        }
                    }
                }
            }
        }
    }
    // QR: el celu (en la misma wifi o en la red propia) abre el reporte de errores
    Rectangle {
        id: caja
        x: raiz.h ? izq.width + 20 : 0
        y: raiz.h ? 0 : izq.height + 16
        width: raiz.h ? 420 : raiz.area.width
        height: raiz.h ? raiz.area.height - 136 : 400
        radius: Math.max(14, Tema.radio * 1.7)
        color: "#fbf7ea"; border { width: 3; color: Tema.bordeCromo }
        Image {
            anchors.horizontalCenter: parent.horizontalCenter; y: 30
            width: Math.min(parent.width - 60, parent.height - 110); height: width
            source: "image://qr/" + encodeURIComponent(raiz.url); smooth: false
        }
        Text { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 22
               text: "Escaneá con el celu"; color: "#1d4a2c"; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 24 } }
    }
    TBoton {
        x: caja.x; y: caja.y + caja.height + 16; width: caja.width; height: raiz.h ? 120 : raiz.area.height - y
        tipo: "acento"; icono: ""; tamNombre: 27; nombre: "Mandar el reporte al desarrollador"
        onTocado: Navegacion.avisar(Simulador.mandar() ? "Mandando el reporte por internet" : "Sin llave para mandarlo: usá el QR con el celu")
    }
}
