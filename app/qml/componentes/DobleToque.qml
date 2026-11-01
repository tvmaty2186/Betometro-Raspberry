import QtQuick
import Betometro

// Botón que pide un segundo toque dentro de 3 s (borrar, terminar, restablecer).
BotonCromo {
    id: raiz
    property string textoNormal: "Borrar"
    signal confirmado()
    property bool armado: false
    texto: armado ? "Tocá otra vez…" : textoNormal
    onTocado: {
        if (armado) { armado = false; reloj.stop(); confirmado(); }
        else { armado = true; reloj.restart(); }
    }
    Timer { id: reloj; interval: 3000; onTriggered: raiz.armado = false }
}
