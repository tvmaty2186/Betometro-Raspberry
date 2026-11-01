import QtQuick
import Betometro

// Número grande crema (.dig).
Text {
    property real tam: 64
    color: Tema.crema
    style: Text.Raised; styleColor: Qt.rgba(0, 0, 0, 0.35)
    font { family: Tema.fuenteNum; italic: true; weight: Font.Bold; pixelSize: tam }
}
