import QtQuick
import Betometro

// Personalizar desde el celu (Matías, 08/11/2026): el QR abre en el celu la página para cambiar la foto del inicio,
// el número y el nombre del auto, los colores (tema propio) y el velocímetro del inicio. La dirección lleva una llave
// que cambia en cada arranque: sin escanear este QR no se puede guardar. Muestras: docs/muestras/personalizar/
TPantalla {
    id: raiz
    titulo: "Personalizar"
    volverA: "ajustes"
    readonly property string url: Wifi.direccion ? "http://" + Wifi.direccion + (Servidor.puerto !== 80 ? ":" + Servidor.puerto : "")
                                                    + "/personalizar?c=" + Servidor.llavePersonalizar : ""
    Rectangle {
        id: caja
        width: raiz.h ? 470 : raiz.area.width; height: raiz.h ? raiz.area.height : 560
        radius: Math.max(14, Tema.radio * 1.7)
        color: "#fbf7ea"; border { width: 3; color: Tema.bordeCromo }
        Image {
            visible: raiz.url !== ""
            anchors.horizontalCenter: parent.horizontalCenter; y: 24
            width: Math.min(parent.width - 60, parent.height - 110); height: width; smooth: false
            source: raiz.url ? "image://qr/" + encodeURIComponent(raiz.url) : ""
        }
        Text { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 20
               text: raiz.url ? "Escaneá con el celu" : "Conectá el Betómetro a una wifi\\no armá la red propia"
               horizontalAlignment: Text.AlignHCenter; color: "#1d4a2c"; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 26 } }
    }
    Column {
        x: raiz.h ? caja.width + 30 : 0; y: raiz.h ? 10 : caja.height + 24
        width: raiz.h ? raiz.area.width - x : raiz.area.width; spacing: 16
        Text { width: parent.width; wrapMode: Text.WordWrap; color: Tema.tinta
               text: "En el celu podés cambiar:"; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 } }
        Repeater {
            model: ["La foto del inicio: la movés con el dedo y la agrandás pellizcando, acostada y parada",
                    "El número y el nombre del auto",
                    "Los colores: fondo, acento y letras (parte del Deportivo)",
                    "El velocímetro del inicio: 6 estilos"]
            Text { required property string modelData; width: parent.width; wrapMode: Text.WordWrap; color: Tema.tinta
                   text: "•  " + modelData; font { family: Tema.fuente600; pixelSize: 23 } }
        }
        Text { width: parent.width; wrapMode: Text.WordWrap; color: Tema.tintaSuave
               text: "Ves cómo queda antes de guardar. Al tocar GUARDAR en el celu, cambia acá al instante. Los colores de aviso y el modo sol no cambian."
               font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 19 } }
        Text { width: parent.width; wrapMode: Text.WordWrap; color: Tema.tintaSuave; visible: Wifi.propia
               text: "Con la red propia: conectá el celu a «" + Wifi.nombrePropia + "», clave «" + Wifi.clavePropia + "»."
               font { family: Tema.fuente600; pixelSize: 19 } }
    }
}
