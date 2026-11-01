import QtQuick
import Betometro

// Pantalla 1: Inicio. Medidas tomadas de las muestras aprobadas
// (docs/muestras/pantallas-horizontal.html #s1 y pantallas-vertical.html #v1).
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var botones: [
        { texto: "Carreras", icono: "carreras", pantalla: "carreras" },
        { texto: "Práctica libre", icono: "practica", pantalla: "practica" },
        { texto: "Chequeo previo", icono: "chequeo", pantalla: "chequeo" },
        { texto: "Pantalla de carrera", icono: "pantalla", pantalla: "editor" },
        { texto: "Ajustes", icono: "ajustes", pantalla: "ajustes" }
    ]

    Fondo { anchors.fill: parent }

    FotoInicio {
        x: 0; y: raiz.h ? 100 : 116
        width: raiz.h ? 860 : 720; height: raiz.h ? 470 : 370
        focoY: raiz.h ? 0.18 : 0.15
        fundeDerecha: raiz.h ? 0.75 : 1
        fundeAbajo: raiz.h ? 0.70 : 0.80
    }

    // Cabecera verde con filete amarillo
    Rectangle {
        x: 0; y: 0; width: parent.width; height: raiz.h ? 96 : 112
        gradient: Gradient { GradientStop { position: 0; color: "#123d24" } GradientStop { position: 1; color: "#081a0f" } }
        Rectangle { anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 4; color: Tema.amarillo }
    }
    Rayas { x: raiz.h ? parent.width - 200 - 80 : parent.width - 30 - 70; y: -30; width: raiz.h ? 80 : 70; height: raiz.h ? 130 : 150 }
    Cabecera { x: raiz.h ? 28 : 22; y: raiz.h ? 17 : 15; alto: raiz.h ? 62 : 56; tamTitulo: raiz.h ? 44 : 40 }

    Text {
        id: version
        textFormat: Text.StyledText
        text: "v" + Contexto.version + (Contexto.versionNueva ? " <font color='" + Tema.amarillo + "'>· Actualizar</font>" : "")
        color: Tema.blanco
        font { family: Tema.fuente600; italic: true; weight: Font.Bold; pixelSize: raiz.h ? 18 : 16 }
        x: raiz.h ? parent.width - 28 - width : 24
        y: raiz.h ? 30 : 84
        MouseArea { anchors.fill: parent; anchors.margins: -10; enabled: !!Contexto.versionNueva; onClicked: Navegacion.ir("actualizar") }
    }

    Banda { x: 0; y: raiz.h ? 560 : 478; width: raiz.h ? 820 : 720 }
    Numero101 { x: raiz.h ? 40 : 28; y: raiz.h ? 510 : 430; numero: Contexto.numeroAuto }
    Text {
        x: raiz.h ? 190 : 28; y: raiz.h ? 600 : 556
        text: Contexto.nombreAuto
        color: Tema.blanco
        style: Text.Raised; styleColor: "#000"
        font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: raiz.h ? 30 : 28 }
    }
    Flow {
        x: raiz.h ? 190 : parent.width - 16 - width
        y: raiz.h ? 640 : 130
        width: raiz.h ? 600 : 260
        spacing: 8
        layoutDirection: raiz.h ? Qt.LeftToRight : Qt.RightToLeft
        Chip { texto: "GPS " + Contexto.satelites + " satélites" }
        Chip { texto: Contexto.redWifi ? "Wifi: " + Contexto.redWifi : "Sin wifi" }
    }

    Velocimetro {
        x: raiz.h ? 930 : 379; y: raiz.h ? 280 : 668
        width: raiz.h ? 320 : 500; height: width
        valor: 78
    }

    Repeater {
        model: raiz.botones
        Disco {
            required property var modelData
            required property int index
            readonly property real ang: (raiz.h ? 225 - index * 25 : 150 + index * 30) * Math.PI / 180
            readonly property real cx: raiz.h ? 1090 + Math.cos(ang) * 270 : 629 + Math.cos(ang) * 397
            readonly property real cy: raiz.h ? 440 - Math.sin(ang) * 270 : 918 + Math.sin(ang) * 397
            d: raiz.h ? 96 : 104
            x: cx - d / 2; y: cy - d / 2
            icono: modelData.icono
            texto: modelData.texto
            etiquetaIzquierda: raiz.h
            onTocado: Navegacion.ir(modelData.pantalla)
        }
    }

    Swoosh {
        x: -80; width: parent.width + 80 - (raiz.h ? 500 : 200)
        height: 120; y: parent.height + (raiz.h ? 70 : 80) - height
    }
}
