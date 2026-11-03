import QtQuick
import Betometro

// Número grande de un widget.
// Con `pAlto` > 0 usa la fórmula de la app (index.html, .w-num y 1.3.2), en vez de achicarse hasta entrar:
//   tamaño = min(A % del alto, k·B/100 % del ancho / n / ancho)
//   - alto y ancho: el interior del widget (sin el relleno de 8×12 px)
//   - n: cantidad de caracteres (mínimo 3); el tamaño cambia solo si cambia el largo, no con cada décima
//   - k: según el tema (Tema.k); ancho: «Números − / +» del editor (0,7 a 1,8), estira a lo ancho
// Dígitos de ancho fijo (tnum) para que no bailen de costado, e interlineado 0,9.
// Sin `pAlto` (otros usos), se achica hasta entrar en su lugar, como antes.
Text {
    id: raiz
    property real maximo: 150
    property real pAlto: 0     // A de la app (% del alto)
    property real pAncho: 100  // B de la app
    property real alto: 100        // interior del widget
    property real anchoCaja: 100
    property real ancho: 1
    readonly property int n: Math.max(3, text.length)
    readonly property real tamFormula: Math.max(10, Math.min(pAlto / 100 * alto, Tema.k * pAncho / 100 * anchoCaja / 100 / n / ancho))
    height: pAlto > 0 ? 0.9 * tamFormula : implicitHeight
    color: Tema.numero
    style: Text.Raised; styleColor: Qt.rgba(0, 0, 0, 0.35)
    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
    fontSizeMode: pAlto > 0 ? Text.FixedSize : Text.Fit; minimumPixelSize: 10
    // Interlineado 0,9 del tamaño de letra, como `line-height: .9` de la app (no 0,9 del renglón de la fuente)
    lineHeightMode: pAlto > 0 ? Text.FixedHeight : Text.ProportionalHeight; lineHeight: pAlto > 0 ? 0.9 * tamFormula : 1
    font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: pAlto > 0 ? tamFormula : maximo; features: { "tnum": 1 } }
    transform: Scale { origin.x: raiz.width / 2; origin.y: raiz.height / 2; xScale: raiz.ancho }
}
