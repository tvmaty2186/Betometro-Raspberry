import QtQuick
import Betometro

// Pantalla 1: Inicio. Medidas tomadas de las muestras aprobadas
// (docs/muestras/pantallas-horizontal.html #s1 y pantallas-vertical.html #v1).
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var botones: [
        { texto: "Carreras", icono: "carreras", pantalla: "carreras" },
        { texto: "Práctica", icono: "practica", pantalla: "practica" },
        { texto: "Chequeo previo", icono: "chequeo", pantalla: "chequeo" },
        { texto: "Pantalla de carrera", icono: "pantalla", pantalla: "editor" },
        { texto: "Clima", icono: "clima", pantalla: "clima" },   // donde estaba Ajustes (Matías, 05/11/2026)
        { texto: "Ajustes", icono: "ajustes", pantalla: "ajustes" }
    ]

    Fondo { anchors.fill: parent }

    FotoInicio {
        x: 0; y: raiz.h ? 80 : 80
        width: raiz.h ? 860 : 720; height: raiz.h ? 470 : 370
        focoY: raiz.h ? 0.18 : 0.15
        fundeDerecha: raiz.h ? 0.75 : 1
        fundeAbajo: raiz.h ? 0.70 : 0.80
    }

    // Arriba, la misma barra del Modo Tablet: león, BETÓMETRO y la versión («Actualizar» en amarillo),
    // GPS, wifi y la hora (Matías, 06/11/2026: «me encanta, que quede en el modo normal»)
    TBarra { }

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
        // El del GPS se sacó (06/11/2026): ya está en la barra de arriba. Queda solo el aviso de tensión o calor.
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
        // Con GPS andando (lectura de hace menos de 3 s): la velocidad real del auto. Si no, la animación
        // de siempre (Matías, 05/11/2026). La aguja se mueve suave entre lecturas.
        readonly property bool real: !Contexto.prueba && !!Crono.gps.ok && Crono.gps.kmh !== undefined
                                     && t >= 0 && Date.now() - Crono.gps.kmhCuando < 3000
        property real kmhSuave: real ? Crono.gps.kmh : 0   // sin animación: 60 cuadros por segundo de más (03-1.1)
        valor: Contexto.prueba ? 47 : real ? kmhSuave : 47 + 2.5 * Math.sin(0.9 * t) + 1.2 * Math.sin(2.3 * t)
        sugerida: real ? -1 : 48   // con la velocidad real no hay sugerida
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
