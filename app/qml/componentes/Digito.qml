import QtQuick
import Betometro

// Número grande crema (.dig).
Text {
    property real tam: 64
    color: Tema.numero
    style: Text.Raised; styleColor: Qt.rgba(0, 0, 0, 0.35)
    font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: tam }
}
