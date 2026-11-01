import QtQuick
import Betometro

// Cabecera del inicio: león | rayita cromo | título cromado y subtítulo.
Row {
    id: raiz
    property real alto: 62
    property real tamTitulo: 44
    spacing: 18
    Image {
        source: "qrc:/recursos/img/leon.png"
        height: raiz.alto; fillMode: Image.PreserveAspectFit
        anchors.verticalCenter: parent.verticalCenter
        smooth: true; mipmap: true
    }
    Canvas {
        width: 18; height: raiz.alto * 0.9
        anchors.verticalCenter: parent.verticalCenter
        onPaint: {
            const c = getContext("2d"), H = height, s = Math.tan(15 * Math.PI / 180) * H / 2, x = width / 2;
            c.reset();
            const g = c.createLinearGradient(0, 0, 0, H);
            for (const p of Tema.cromo) g.addColorStop(p[0], p[1]);
            c.beginPath(); c.moveTo(x - 1.5 + s, 0); c.lineTo(x + 1.5 + s, 0); c.lineTo(x + 1.5 - s, H); c.lineTo(x - 1.5 - s, H); c.closePath();
            c.fillStyle = g; c.fill();
        }
        Component.onCompleted: requestPaint()
    }
    Column {
        anchors.verticalCenter: parent.verticalCenter
        TextoCromo { texto: "BETÓMETRO"; tam: raiz.tamTitulo }
        Text {
            text: "REGULARIDAD PARA EL NAVEGANTE"
            color: Tema.tintaSuave
            font { family: Tema.fuente600; italic: true; weight: Font.DemiBold; pixelSize: 11; letterSpacing: 11 * 0.14 }
            topPadding: -4
        }
    }
}
