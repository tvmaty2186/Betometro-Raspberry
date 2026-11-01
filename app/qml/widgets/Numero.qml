import QtQuick
import Betometro

// Número grande que se achica para entrar en su lugar (6.5 de la app).
Text {
    property real maximo: 150
    color: Tema.crema
    style: Text.Raised; styleColor: Qt.rgba(0, 0, 0, 0.35)
    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
    fontSizeMode: Text.Fit; minimumPixelSize: 10
    font { family: Tema.fuenteNum; italic: true; weight: Font.Bold; pixelSize: maximo }
}
