import QtQuick
import Betometro

// Etiqueta de un widget (Falta, Sugerida…): según el alto. Sub ya la agranda para la pantalla de 5".
Sub {
    property real alto: 200
    tam: Math.max(11, Math.min(16, alto * 0.075))
    horizontalAlignment: Text.AlignHCenter
}
