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
        { texto: "Clima", icono: "clima", pantalla: "clima" },   // donde estaba Ajustes (Matías, 05/11/2026)
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
        gradient: Gradient { GradientStop { position: 0; color: Tema.avispon ? "#123d24" : Tema.caja1 } GradientStop { position: 1; color: Tema.avispon ? "#081a0f" : Tema.caja2 } }
        Rectangle { anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 4; color: Tema.acento }
    }
    Rayas { visible: Tema.avispon; x: raiz.h ? parent.width - 200 - 80 : parent.width - 30 - 70; y: -30; width: raiz.h ? 80 : 70; height: raiz.h ? 130 : 150 }
    Cabecera { x: raiz.h ? 28 : 22; y: raiz.h ? 17 : 15; alto: raiz.h ? 62 : 56; tamTitulo: raiz.h ? 44 : 40 }

    Text {
        id: version
        textFormat: Text.StyledText
        text: "v" + Contexto.version + (Contexto.versionNueva ? " <font color='" + Tema.acento + "'>· Actualizar</font>" : "")
        color: Tema.tinta
        font { family: Tema.fuente600; italic: Tema.cursiva; weight: Font.Bold; pixelSize: raiz.h ? 18 : 16 }
        x: raiz.h ? parent.width - 28 - width : 24
        y: raiz.h ? 30 : 84
        MouseArea { anchors.fill: parent; anchors.margins: -10; enabled: !!Contexto.versionNueva; onClicked: Navegacion.ir("actualizar") }
    }
    // Ícono del wifi (abanico), a la izquierda de las franjas en diagonal, lejos de la versión y del
    // «· Actualizar» (Matías, 04/11/2026). Tocarlo abre Conectividad.
    IconoWifi {
        id: iconoWifi
        nivel: Wifi.conectada ? Math.max(1, Wifi.senal) : 0
        width: 44; height: 34
        x: raiz.h ? parent.width - 200 - 80 - 18 - width : parent.width - 30 - 70 - 18 - width
        y: raiz.h ? 28 : 34
        MouseArea { anchors.fill: parent; anchors.margins: -12; onClicked: Navegacion.ir("conectividad") }
    }

    Banda { visible: Tema.avispon; x: 0; y: raiz.h ? 560 : 478; width: raiz.h ? 820 : 720 }
    Numero101 { visible: Tema.avispon; x: raiz.h ? 40 : 28; y: raiz.h ? 510 : 430; numero: Datos.ajustes.numeroAuto || "101" }
    Text {
        id: nombreAuto
        x: raiz.h ? 190 : 28; y: raiz.h ? 600 : 556
        text: Datos.ajustes.nombreAuto || ""
        color: Tema.tinta
        style: Text.Raised; styleColor: "#000"
        font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: raiz.h ? 30 : 28 }
    }
    Flow {
        // En horizontal, al lado del nombre del auto (abajo ya no entraban, por la franja curva)
        x: raiz.h ? nombreAuto.x + nombreAuto.width + 18 : parent.width - 16 - width
        y: raiz.h ? nombreAuto.y + (nombreAuto.height - 40) / 2 : 130
        width: raiz.h ? 770 - x : 330
        spacing: 10
        layoutDirection: raiz.h ? Qt.LeftToRight : Qt.RightToLeft
        // Más grande, con el punto verde si anda y rojo si no (Matías, 04/11/2026: no se llegaba a leer).
        // El de wifi se sacó (05/11): con nombres largos no entraba, y ya está el ícono de abanico arriba.
        Chip { tam: 20; punto: Crono.gps.ok ? Tema.bien : Tema.alerta
               texto: !Crono.gps.ok ? "Sin GPS" : Crono.gps.satelites ? "GPS " + Crono.gps.satelites + " satélites" : "GPS " + Crono.gps.hz + " Hz" }
        Chip {   // tensión baja o calor (Salud): solo si pasa
            tam: 20
            visible: Salud.tensionBaja || Salud.calor
            punto: Tema.alerta
            texto: Salud.tensionBaja ? "⚡ Tensión baja" : "Calor " + Salud.temperatura + " °C"
        }

    }

    // Se mueve solo, cerca de la sugerida (48), como el inicio de la app: 47 + 2,5·sen(0,9 t) + 1,2·sen(2,3 t)
    Velocimetro {
        id: velInicio
        x: raiz.h ? 930 : 379; y: raiz.h ? 280 : 668
        width: raiz.h ? 320 : 500; height: width
        property real t: 0
        valor: Contexto.prueba ? 47 : 47 + 2.5 * Math.sin(0.9 * t) + 1.2 * Math.sin(2.3 * t)
        sugerida: 48
        Timer { interval: 80; repeat: true; running: raiz.visible && !Contexto.prueba; onTriggered: velInicio.t += 0.08 }
    }

    Repeater {
        model: raiz.botones
        Disco {
            required property var modelData
            required property int index
            // Seis botones repartidos en el mismo arco que antes ocupaban cinco
            readonly property real ang: (raiz.h ? 232 - index * 23 : 150 + index * 24) * Math.PI / 180
            readonly property real cx: raiz.h ? 1090 + Math.cos(ang) * 288 : 629 + Math.cos(ang) * 397
            readonly property real cy: raiz.h ? 436 - Math.sin(ang) * 288 : 918 + Math.sin(ang) * 397
            d: raiz.h ? 86 : 104
            x: cx - d / 2; y: cy - d / 2
            icono: modelData.icono
            texto: modelData.texto
            etiquetaIzquierda: raiz.h
            onTocado: Navegacion.ir(modelData.pantalla)
        }
    }

    Swoosh {
        visible: Tema.avispon
        x: -80; width: parent.width + 80 - (raiz.h ? 500 : 200)
        height: 120; y: parent.height + (raiz.h ? 70 : 80) - height
    }
}
