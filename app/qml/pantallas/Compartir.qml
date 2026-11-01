import QtQuick
import Betometro

// Pantalla 11: Compartir el informe por QR. Muestras #s11 y #v11.
// El informe se guarda como HTML (igual al de la app). La página que lo sirve por wifi va en E5.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property string id_: Navegacion.parametros.id || ""
    readonly property string archivo: Datos.nombreInforme(id_)
    readonly property string url: "http://" + Wifi.direccion + "/informe/" + id_
    Component.onCompleted: Datos.guardarInformeHtml(id_)

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "COMPARTIR INFORME"; derechaRayas: raiz.h ? 90 : 30; onVolver: Navegacion.ir("informe", { id: raiz.id_ }) }

    Item {
        id: cajaQr
        x: raiz.h ? 80 : 135; y: 130
        width: raiz.h ? 430 : 450; height: width
        Rectangle { anchors.fill: parent; anchors.margins: -8; radius: 32; color: Tema.bordeCromoOscuro }
        Rectangle { anchors.fill: parent; anchors.margins: -6; radius: 30; color: Tema.bordeCromo }
        Rectangle { anchors.fill: parent; radius: 24; color: "#ffffff" }
        Image { anchors.centerIn: parent; width: parent.width - 60; height: width; smooth: false; source: "image://qr/" + encodeURIComponent(raiz.url) }
    }
    Column {
        x: raiz.h ? 580 : 40; y: raiz.h ? 150 : 640
        width: raiz.h ? 640 : parent.width - 80
        spacing: 20
        Text { text: "Escaneá con el celular"; color: Tema.blanco; font { family: Tema.fuente800; italic: true; pixelSize: 40 } }
        Text {
            width: parent.width; wrapMode: Text.WordWrap; textFormat: Text.StyledText; lineHeight: 1.5
            color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 24 }
            text: "1. El celular tiene que estar en la misma red wifi: <b><font color='#ffffff'>" + (Wifi.conectada || "Betometro-" + (Datos.ajustes.numeroAuto || "101")) + "</font></b>.<br>"
                + "2. Abrí la cámara y apuntá al código.<br>"
                + "3. Se descarga <b><font color='#ffffff'>«" + raiz.archivo + "»</font></b>."
        }
        Item { width: 1; height: 20 }
        BotonCromo { width: 240; height: 60; tamTexto: 20; texto: "Listo"; onTocado: Navegacion.ir("informe", { id: raiz.id_ }) }
    }
}
