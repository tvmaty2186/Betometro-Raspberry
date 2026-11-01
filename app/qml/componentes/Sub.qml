import QtQuick
import Betometro

// Texto chico de etiqueta, en mayúsculas y espaciado (.sub).
Text {
    property real tam: 11
    color: Tema.tintaSuave
    font { family: Tema.fuente600; italic: Tema.cursiva; weight: Font.DemiBold; pixelSize: tam; letterSpacing: tam * 0.14; capitalization: Font.AllUppercase }
}
