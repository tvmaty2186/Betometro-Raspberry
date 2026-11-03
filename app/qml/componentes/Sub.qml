import QtQuick
import Betometro

// Texto chico de etiqueta, en mayúsculas y espaciado (.sub).
// En la pantalla de 5" (misma resolución que la de 7", todo se ve 30 % más chico) las etiquetas se
// agrandan un 25 % y nunca bajan de 13 px (Matías, 03/11: las letras chicas no se leían).
Text {
    property real tam: 11
    readonly property real tamReal: Math.max(13, Math.round(tam * 1.25))
    color: Tema.tintaSuave
    font { family: Tema.fuente600; italic: Tema.cursiva; weight: Font.DemiBold; pixelSize: tamReal; letterSpacing: tamReal * 0.12; capitalization: Font.AllUppercase }
}
