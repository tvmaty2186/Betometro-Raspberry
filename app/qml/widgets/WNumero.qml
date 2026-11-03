import QtQuick
import Betometro

// Widget genérico: etiqueta, número grande y dato extra (sugerida, faltan, transcurrido, ideal, próxima, última).
Caja {
    id: raiz
    property string etiqueta: ""
    property string numero: ""
    property string extra: ""
    property bool fila: width > height * 3   // muy ancho y bajito: etiqueta y número en una línea
    property color colorNumero: Tema.numero
    property real ancho: 1
    Column {
        visible: !raiz.fila
        anchors.centerIn: parent
        width: parent.width - 24
        Etiq { alto: raiz.height; width: parent.width; text: raiz.etiqueta; wrapMode: Text.WordWrap }
        Numero {
            width: parent.width
            pAlto: 58; alto: raiz.height - 16; anchoCaja: raiz.width - 24; ancho: raiz.ancho
            text: raiz.numero; color: raiz.colorNumero
        }
        Sub { visible: raiz.extra !== ""; width: parent.width; horizontalAlignment: Text.AlignHCenter; text: raiz.extra
              tam: Math.max(11, Math.min(17, raiz.height * 0.08)) }
    }
    Row {
        visible: raiz.fila
        anchors.centerIn: parent
        spacing: 30
        Etiq { alto: raiz.height * 2; anchors.verticalCenter: parent.verticalCenter; text: raiz.etiqueta }
        Numero { anchors.verticalCenter: parent.verticalCenter; width: raiz.width * 0.4; pAlto: 58; alto: raiz.height - 16; anchoCaja: raiz.width * 0.4; ancho: raiz.ancho; text: raiz.numero }
        Sub { visible: raiz.extra !== ""; anchors.verticalCenter: parent.verticalCenter; text: raiz.extra; tam: 14 }
    }
}
