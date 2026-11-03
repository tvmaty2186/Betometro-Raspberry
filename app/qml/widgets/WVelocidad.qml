import QtQuick
import Betometro

// Velocímetro digital (cuadrado): km/h, «máx N» y barra contra la sugerida (±10 %, mínimo 5 km/h).
Caja {
    id: raiz
    property var est: ({})
    property bool barra: true
    property bool extra: false
    property real ancho: 1
    readonly property bool sinGps: est.vActKmh === undefined || est.vActKmh === null
    Column {
        anchors.centerIn: parent
        width: parent.width - 24
        spacing: raiz.height * 0.03
        Etiq { alto: raiz.height; width: parent.width; text: "Velocidad km/h" }
        Numero {
            width: parent.width
            pAlto: raiz.sinGps ? 20 : (raiz.barra ? 46 : 58); alto: raiz.height - 16; anchoCaja: raiz.width - 24; ancho: raiz.sinGps ? 1 : raiz.ancho
            text: raiz.sinGps ? "SIN GPS" : Math.round(raiz.est.vActKmh)
        }
        Sub { visible: raiz.extra; width: parent.width; horizontalAlignment: Text.AlignHCenter; text: "máx " + Math.round(raiz.est.velMaxVuelta || 0); tam: 14 }
        Barra {
            visible: raiz.barra
            anchors.horizontalCenter: parent.horizontalCenter
            width: raiz.width * 0.72; height: Math.max(8, raiz.height * 0.027)
            readonly property real vNec: raiz.est.vNecKmh || 0
            rango: Math.max(5, vNec * 0.10)
            valor: raiz.sinGps || !(vNec > 0) ? null : raiz.est.vActKmh - vNec
        }
    }
}
