import QtQuick
import Betometro

// Manual del navegante en la Pi (Matías, 08/11/2026: «en el sector de prueba completa, que quede dentro de la Pi»).
// Es el mismo texto que /ayuda en el celu (recursos/web/ayuda.html), en una hoja clara que se lee con sol y en
// cualquier tema. Se recorre arrastrando. A la derecha, el QR para llevarlo en el celu (y bajar el PDF desde ahí).
TPantalla {
    id: raiz
    titulo: raiz.h ? "Manual del navegante" : "Manual"
    volverA: "prueba"
    readonly property string url: "http://" + (Wifi.direccion || "betometro.local") + "/ayuda"

    Rectangle {
        id: hoja
        width: raiz.h ? raiz.area.width - 360 - 20 : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - 380
        radius: Math.max(14, Tema.radio * 1.7)
        color: "#fbf7ea"; border { width: 3; color: Tema.bordeCromo }
        clip: true
        Flickable {
            id: lista
            anchors { fill: parent; margins: 24 }
            contentHeight: texto.height + 20
            boundsBehavior: Flickable.StopAtBounds
            Text {
                id: texto
                width: lista.width
                textFormat: Text.RichText
                wrapMode: Text.WordWrap
                color: "#1d1d1d"
                font { family: Tema.fuente600; pixelSize: 24 }
                text: Contexto.manual()
            }
        }
        // Barrita que muestra dónde estás
        Rectangle {
            visible: lista.contentHeight > lista.height
            x: parent.width - 12; width: 6; radius: 3; color: "#1d4a2c"; opacity: 0.5
            y: 24 + (lista.height - height) * (lista.contentY / Math.max(1, lista.contentHeight - lista.height))
            height: Math.max(60, lista.height * lista.height / Math.max(1, lista.contentHeight))
        }
    }
    Rectangle {
        x: raiz.h ? hoja.width + 20 : 0
        y: raiz.h ? 0 : hoja.height + 16
        width: raiz.h ? 360 : raiz.area.width
        height: raiz.h ? raiz.area.height : 364
        radius: Math.max(14, Tema.radio * 1.7)
        color: "#fbf7ea"; border { width: 3; color: Tema.bordeCromo }
        Image {
            id: qr
            anchors.horizontalCenter: parent.horizontalCenter; y: 22
            width: Math.min(parent.width - 60, parent.height - 130); height: width
            source: "image://qr/" + encodeURIComponent(raiz.url); smooth: false
        }
        Column {
            anchors { top: qr.bottom; topMargin: 12; horizontalCenter: parent.horizontalCenter }
            width: parent.width - 30; spacing: 4
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                   text: "Llevalo en el celu"; color: "#1d4a2c"; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 26 } }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                   text: "Ahí también se baja en PDF"; color: "#555555"; font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 19 } }
        }
    }
}
