import QtQuick
import Betometro

// Etiqueta de un widget (Falta, Sugerida…): entre 10 y 15 px según el alto, como la app; en la Pi un poco más grande.
Sub {
    property real alto: 200
    tam: Math.max(12, Math.min(16, alto * 0.075))
    horizontalAlignment: Text.AlignHCenter
}
