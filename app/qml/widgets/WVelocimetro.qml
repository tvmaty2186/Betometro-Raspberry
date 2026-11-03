import QtQuick
import Betometro

// Velocímetro analógico redondo (cara crema, como el del inicio) con la marca de la sugerida.
Item {
    id: raiz
    property var est: ({})
    property bool extra: false
    readonly property bool sinGps: est.vActKmh === undefined || est.vActKmh === null
    Velocimetro {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height); height: width
        valor: raiz.sinGps ? 0 : raiz.est.vActKmh
        hayValor: !raiz.sinGps
        extra: raiz.extra ? "máx " + Math.round(raiz.est.velMaxVuelta || 0) : ""
        sugerida: raiz.est.vNecKmh === undefined || raiz.est.vNecKmh === null ? -1 : raiz.est.vNecKmh
    }
}
