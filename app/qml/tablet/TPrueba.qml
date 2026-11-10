import QtQuick
import Betometro

// Prueba completa (Matías, 07/11/2026, para la 1.0.30), con el mismo estilo que el Simulador (TSimulador.qml):
// arriba el resultado (verde: todo bien · naranja: revisar · rojo: avisar al desarrollador), la lista de pasos con
// su estado, mientras corre el paso actual y la barra de avance, los diálogos grandes «¿Se escuchó?» y «Apretá el
// pulsador», y a la derecha el QR a /prueba (el celu ve y arranca la prueba) y los botones.
TPantalla {
    id: raiz
    titulo: "Prueba completa"
    volverA: "ajustes"
    readonly property string e: PruebaCompleta.estado
    readonly property bool corre: e === "corriendo"
    readonly property string pregunta: PruebaCompleta.pregunta
    readonly property string url: "http://" + (Wifi.direccion || "betometro.local") + "/prueba"
    focus: true
    Component.onCompleted: { forceActiveFocus(); if (Contexto.prueba === "prueba-completa" && !e) PruebaCompleta.correr(); }   // capturas
    // El teclado también vale como pulsador (en la PC), igual que en el Chequeo previo
    Keys.onPressed: ev => {
        if (ev.key === Qt.Key_Return || ev.key === Qt.Key_Enter || ev.key === Qt.Key_Space || ev.key === Qt.Key_F9) { PruebaCompleta.pulsadorRecibido("el teclado"); ev.accepted = true; }
    }
    onCorreChanged: if (!corre) lista.contentY = 0   // terminó: la lista vuelve arriba
    function colorPaso(s) { return s === "ok" ? Tema.bien : s === "grave" ? Tema.rojo : s === "leve" ? Tema.naranja : s === "corriendo" ? Tema.acento : Tema.bordeCromoOscuro; }

    Item {
        id: izq
        width: raiz.h ? raiz.area.width - 420 - 20 : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - 680   // parada: lugar para el QR y los 3 botones (10/11/2026: el último se salía)
        Rectangle {
            id: cabeza
            width: parent.width; height: 118; radius: Math.max(14, Tema.radio * 1.7)
            border { width: 3; color: raiz.e === "grave" ? "#ffb3aa" : raiz.e === "leve" ? "#ffd29a" : raiz.e === "ok" ? "#9ff5bb" : Tema.bordeCromo }
            gradient: Gradient {
                GradientStop { position: 0; color: raiz.e === "grave" ? "#e0473a" : raiz.e === "leve" ? "#f5a13c" : raiz.e === "ok" ? "#3fa34d" : Qt.lighter(Tema.caja1, 1.15) }
                GradientStop { position: 1; color: raiz.e === "grave" ? "#8f130b" : raiz.e === "leve" ? "#a8520c" : raiz.e === "ok" ? "#1d5233" : Tema.caja2 }
            }
            Column {
                anchors.centerIn: parent; width: parent.width - 30; spacing: 6
                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                       color: raiz.e ? "#ffffff" : Tema.tinta
                       text: raiz.e === "grave" ? "Hay errores: comunicate con el desarrollador" : raiz.e === "leve" ? "Revisar: hay avisos" : raiz.e === "ok" ? "Todo bien"
                             : raiz.corre ? "Probando… paso " + PruebaCompleta.paso + " de " + PruebaCompleta.total : "Tocá «Empezar» y esperá 3 a 5 minutos"
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight; color: raiz.e ? "#ffffff" : Tema.acento; opacity: 0.9
                       text: raiz.corre ? (PruebaCompleta.haciendo || PruebaCompleta.resumen)
                             : raiz.e ? PruebaCompleta.resumen + " · " + PruebaCompleta.cuando + " · v" + Contexto.version
                             : "Revisa el sistema, el simulador, la voz, el pulsador, el GPS, la cámara, la tarjeta, la red y las pantallas"
                       font { family: Tema.fuente600; pixelSize: 21 } }
            }
        }
        // Barra de avance (solo mientras corre)
        Rectangle {
            id: barra
            visible: raiz.corre
            y: cabeza.height + 14; width: parent.width; height: 16; radius: 8
            color: "#ffffff33"; border { width: 1; color: Tema.bordeCromo }
            Rectangle { height: parent.height; radius: parent.radius; color: Tema.acento
                        width: parent.width * Math.max(0, PruebaCompleta.paso - 1) / Math.max(1, PruebaCompleta.total)
                        Behavior on width { NumberAnimation { duration: Contexto.fijo ? 0 : 300 } } }
        }
        Flickable {
            id: lista
            y: raiz.corre ? barra.y + barra.height + 14 : cabeza.height + 16
            width: parent.width; height: parent.height - y
            contentHeight: col.height; clip: true; boundsBehavior: Flickable.StopAtBounds
            // Mientras corre, el paso actual queda a la vista
            onContentHeightChanged: if (raiz.corre) contentY = Math.max(0, Math.min(contentHeight - height, (PruebaCompleta.paso - 1) * 76 - height / 2 + 40))
            Column {
                id: col; width: parent.width; spacing: 10
                Repeater {
                    model: PruebaCompleta.pasos
                    Rectangle {
                        width: col.width; height: Math.max(66, textos.height + 22); radius: Math.max(12, Tema.radio * 1.3)
                        border { width: modelData.estado === "corriendo" ? 3 : 2; color: modelData.estado === "corriendo" ? Tema.acento : Tema.bordeCromo }
                        gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
                        opacity: modelData.estado === "espera" ? 0.55 : 1
                        Rectangle {
                            x: 18; anchors.verticalCenter: parent.verticalCenter; width: 40; height: 40; radius: 20
                            color: raiz.colorPaso(modelData.estado)
                            border { width: 2; color: "#ffffff" }
                            Text { anchors.centerIn: parent; color: modelData.estado === "corriendo" ? Tema.acentoTinta : "#ffffff"
                                   text: modelData.estado === "ok" ? "✓" : modelData.estado === "leve" || modelData.estado === "grave" ? "!" : modelData.estado === "corriendo" ? "…" : modelData.estado === "saltado" ? "–" : String(index + 1)
                                   font { family: Tema.fuente800; pixelSize: 22 } }
                        }
                        Column {
                            id: textos
                            x: 74; anchors.verticalCenter: parent.verticalCenter; width: parent.width - 90; spacing: 1
                            Text { width: parent.width; elide: Text.ElideRight; color: Tema.tinta
                                   text: modelData.nombre + (modelData.estado === "ok" ? " · bien" : modelData.estado === "leve" ? " · revisar" : modelData.estado === "grave" ? " · error" : modelData.estado === "saltado" ? " · sin probar" : "")
                                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 23 } }
                            Text { visible: text !== ""; width: parent.width; wrapMode: Text.WordWrap; maximumLineCount: 3; elide: Text.ElideRight
                                   text: modelData.detalle || ""; color: Tema.acento
                                   font { family: Tema.fuente600; pixelSize: 18 } }
                        }
                    }
                }
            }
        }
        // Diálogos grandes: «¿Se escuchó?» y «Apretá el pulsador»
        Rectangle {
            visible: raiz.pregunta !== ""
            y: cabeza.height + 14; width: parent.width; height: parent.height - y; radius: Math.max(14, Tema.radio * 1.7)   // tapa la barra también
            color: Tema.caja2; border { width: 4; color: Tema.acento }
            Column {
                anchors.centerIn: parent; width: parent.width - 60; spacing: 26
                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: Tema.tinta
                       text: raiz.pregunta === "voz" ? "¿Se escuchó la voz?" : "Apretá el pulsador"
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 54 } }
                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; color: Tema.acento
                       text: (raiz.pregunta === "voz" ? "Sonó la prueba por los auriculares · " : "El de cable, el que marca las vueltas · ") + "quedan " + PruebaCompleta.segundos + " s"
                       font { family: Tema.fuente600; pixelSize: 24 } }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter; spacing: 24
                    TBoton { visible: raiz.pregunta === "voz"; width: 240; height: 130; icono: ""; nombre: "SÍ"; tipo: "acento"; tamNombre: 48; onTocado: PruebaCompleta.responder(true) }
                    TBoton { visible: raiz.pregunta === "voz"; width: 240; height: 130; icono: ""; nombre: "NO"; tipo: "peligro"; tamNombre: 48; onTocado: PruebaCompleta.responder(false) }
                    TBoton { visible: raiz.pregunta === "pulsador"; width: 300; height: 110; icono: "atras"; nombre: "Sin pulsador: saltar"; tamNombre: 24; onTocado: PruebaCompleta.responder(false) }
                }
            }
        }
    }
    // QR: el celu (en la misma wifi o en la red propia) abre /prueba: ve el resultado y puede arrancarla
    Rectangle {
        id: caja
        x: raiz.h ? izq.width + 20 : 0
        y: raiz.h ? 0 : izq.height + 16
        width: raiz.h ? 420 : raiz.area.width
        height: raiz.h ? raiz.area.height - 3 * 116 : 300
        radius: Math.max(14, Tema.radio * 1.7)
        color: "#fbf7ea"; border { width: 3; color: Tema.bordeCromo }
        Image {
            anchors.horizontalCenter: parent.horizontalCenter; y: 22
            width: Math.min(parent.width - 60, parent.height - 90); height: width
            source: "image://qr/" + encodeURIComponent(raiz.url); smooth: false
        }
        Text { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 16
               text: "Escaneá con el celu"; color: "#1d4a2c"; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 24 } }
    }
    TBoton {
        id: empezar
        x: caja.x; y: caja.y + caja.height + 16; width: caja.width; height: raiz.h ? 100 : 96
        tipo: "acento"; icono: ""; tamNombre: 27; activo: !raiz.corre
        nombre: raiz.corre ? "Probando…" : raiz.e ? "Probar de nuevo" : "Empezar la prueba"
        dato: raiz.corre ? "Paso " + PruebaCompleta.paso + " de " + PruebaCompleta.total : "Tarda 3 a 5 minutos"
        onTocado: if (!PruebaCompleta.correr()) Navegacion.avisar(PruebaCompleta.porQueNo())
    }
    TBoton {
        id: mandar
        x: caja.x; y: empezar.y + empezar.height + 16; width: caja.width; height: raiz.h ? 100 : 96
        icono: ""; tamNombre: 24; nombre: "Mandar el resultado al desarrollador"; activo: !!raiz.e && !raiz.corre
        onTocado: Navegacion.avisar(PruebaCompleta.mandar() ? "Mandando el resultado por internet" : "Sin llave para mandarlo: usá el QR con el celu")
    }
    // El manual del navegante queda dentro de la Pi (Matías, 08/11/2026)
    TBoton {
        x: caja.x; y: mandar.y + mandar.height + 16; width: caja.width; height: raiz.h ? 100 : raiz.area.height - y
        icono: ""; tamNombre: 24; nombre: "Manual del navegante"; dato: "Cómo se usa · si algo falla"; activo: !raiz.corre
        onTocado: Navegacion.ir("manual")
    }
}
