import QtQuick
import Betometro

// Modo tele: la pantalla de la Pi es el control remoto (muestra docs/muestras/video/tele.png).
// Ir a la vuelta, −10 s / play-pausa / +10 s, la línea de tiempo (se arrastra) y «Datos en la tele»
// (prender y apagar). Los datos no se mueven desde acá: se mueven en la pantalla completa de la Pi y
// la tele usa los mismos lugares.
Item {
    id: c
    property Item rep: null
    property string titulo: ""
    property bool verDatos: false
    signal salir()
    readonly property bool h: width > height
    readonly property real m: h ? 36 : 28
    readonly property real kc: Math.min(1, (width - 2 * m) / 966)   // tamaño de los botones grandes

    Fondo { anchors.fill: parent }
    MouseArea { anchors.fill: parent }   // que nada llegue al informe de abajo

    component Texto: Text {
        color: Tema.tinta
        font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 30 }
    }
    component Etiq: Text {
        color: Tema.tintaSuave
        font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 20; letterSpacing: 3; capitalization: Font.AllUppercase }
    }
    // Ficha oscura con borde cromo (como las de la muestra)
    component Ficha: Rectangle {
        property bool amarilla: false
        signal tocado()
        radius: 28
        color: amarilla ? Tema.acento : Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.8)
        border { width: 3; color: amarilla ? Tema.acento : Tema.bordeCromo }
        opacity: enabled ? 1 : 0.35
        scale: ma.pressed ? 0.95 : 1
        MouseArea { id: ma; anchors.fill: parent; anchors.margins: -6; onClicked: parent.tocado() }
    }

    // ---------- Arriba: título y Salir ----------
    Row {
        x: c.m; y: c.h ? 30 : 44; spacing: 14
        IconoTele { width: 38; height: 33; anchors.verticalCenter: parent.verticalCenter; color: Tema.tinta }
        Texto { anchors.verticalCenter: parent.verticalCenter; text: "EN LA TELE · " + c.titulo; font.letterSpacing: 2
                width: Math.min(implicitWidth, c.width - 2 * c.m - 52 - bSalir.width - 20); elide: Text.ElideRight }
    }
    Ficha {
        id: bSalir
        anchors { right: parent.right; rightMargin: c.m }
        y: c.h ? 20 : 34; height: 62; width: tSalir.implicitWidth + 56; radius: 31
        Etiq { id: tSalir; anchors.centerIn: parent; text: "Salir"; color: Tema.tinta; font.pixelSize: 22 }
        onTocado: c.salir()
    }

    // ---------- Ir a la vuelta ----------
    Etiq { id: eVuelta; x: c.m; y: c.h ? 112 : 150; text: "Ir a la vuelta" }
    Flickable {
        id: fVueltas
        x: c.m; y: eVuelta.y + 36; width: c.width - 2 * c.m; height: 104
        contentWidth: filaV.width; clip: true; boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalFlick
        Row {
            id: filaV
            spacing: 16; y: 2
            Repeater {
                model: c.rep ? c.rep.vueltas : []
                Ficha {
                    id: fv
                    required property var modelData
                    required property int index
                    width: 142; height: 98
                    amarilla: c.rep.lapAhora === index
                    enabled: !!c.rep.momento(index, 0)
                    onTocado: c.rep.irA(index, 0)
                    Column {
                        anchors.centerIn: parent
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: c.rep.nombreV(modelData)
                               color: fv.amarilla ? Tema.acentoTinta : Tema.tinta
                               font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 32 } }
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: Formato.tiempo(modelData.realMs || 0)
                               color: fv.amarilla ? Tema.acentoTinta : Tema.tintaSuave
                               font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 30 } }
                    }
                }
            }
        }
    }

    // ---------- −10 s / play-pausa / +10 s ----------
    Item {
        anchors { left: parent.left; right: parent.right; top: fVueltas.bottom; bottom: linea.top; bottomMargin: 24 }
        Row {
            anchors.centerIn: parent
            spacing: 80 * c.kc
            Ficha {
                anchors.verticalCenter: parent.verticalCenter
                width: 248 * c.kc; height: 186 * c.kc; radius: 62 * c.kc
                Texto { anchors.centerIn: parent; text: "−10 s"; font { family: Tema.fuenteNum; italic: false; weight: Font.Bold; pixelSize: 52 * c.kc } }
                onTocado: c.rep.saltar(-10000)
            }
            Ficha {
                anchors.verticalCenter: parent.verticalCenter
                width: 310 * c.kc; height: 248 * c.kc; radius: height / 2
                amarilla: true; border.color: "#fff6d0"
                onTocado: c.rep.alternar()
                Row {
                    visible: c.rep.andando; anchors.centerIn: parent; spacing: 26 * c.kc
                    Rectangle { width: 36 * c.kc; height: 104 * c.kc; radius: 8 * c.kc; color: "#111111" }
                    Rectangle { width: 36 * c.kc; height: 104 * c.kc; radius: 8 * c.kc; color: "#111111" }
                }
                Canvas {   // play
                    visible: !c.rep.andando
                    anchors.centerIn: parent; anchors.horizontalCenterOffset: 10 * c.kc
                    width: 84 * c.kc; height: 104 * c.kc
                    onWidthChanged: requestPaint()
                    onPaint: { const g = getContext("2d"); g.reset(); g.fillStyle = "#111111"; g.beginPath(); g.moveTo(0, 0); g.lineTo(width, height / 2); g.lineTo(0, height); g.closePath(); g.fill(); }
                }
            }
            Ficha {
                anchors.verticalCenter: parent.verticalCenter
                width: 248 * c.kc; height: 186 * c.kc; radius: 62 * c.kc
                Texto { anchors.centerIn: parent; text: "+10 s"; font { family: Tema.fuenteNum; italic: false; weight: Font.Bold; pixelSize: 52 * c.kc } }
                onTocado: c.rep.saltar(10000)
            }
        }
    }

    // ---------- Línea de tiempo con una rayita por vuelta (se arrastra) ----------
    Item {
        id: linea
        x: c.m; width: c.width - 2 * c.m; height: 52
        anchors { bottom: tTiempo.top; bottomMargin: c.h ? 8 : 24 }
        readonly property real f: !c.rep ? 0 : Math.max(0, Math.min(1, (c.rep.arrastre >= 0 ? c.rep.arrastre : c.rep.pos) / c.rep.total))
        Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 20; radius: 10; color: Qt.rgba(1, 1, 1, 0.2) }
        Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width * linea.f; height: 20; radius: 10; color: Tema.acento }
        Repeater {
            model: c.rep ? c.rep.tramosVideo : []
            Rectangle {
                required property var modelData
                visible: modelData.desde > 0 && modelData.desde <= c.rep.total
                x: linea.width * modelData.desde / c.rep.total - 2; width: 5; height: 40; radius: 2
                anchors.verticalCenter: parent.verticalCenter; color: Qt.rgba(1, 1, 1, 0.6)
            }
        }
        Rectangle {
            x: linea.width * linea.f - width / 2; anchors.verticalCenter: parent.verticalCenter
            width: 50; height: 50; radius: 25; color: "#ffffff"
        }
        MouseArea {
            anchors { fill: parent; topMargin: -16; bottomMargin: -16 }
            preventStealing: true
            onPressed: (m) => c.rep.arrastre = c.rep.msEn(m.x, linea.width)
            onPositionChanged: (m) => c.rep.arrastre = c.rep.msEn(m.x, linea.width)
            onReleased: c.rep.soltarArrastre()
            onCanceled: c.rep.arrastre = -1
        }
    }
    Text {
        id: tTiempo
        x: c.m; anchors { bottom: parent.bottom; bottomMargin: c.h ? 30 : 60 }
        text: !c.rep ? "" : c.rep.reloj(c.rep.arrastre >= 0 ? c.rep.arrastre : c.rep.pos) + " / " + c.rep.reloj(c.rep.total)
        color: Tema.tinta; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 46 }
    }
    Ficha {
        anchors { right: parent.right; rightMargin: c.m; verticalCenter: tTiempo.verticalCenter }
        height: 72; width: filaD.implicitWidth + 56; radius: 36
        Row {
            id: filaD; anchors.centerIn: parent; spacing: 14
            Engranaje { width: 34; height: 34; color: Tema.tinta; anchors.verticalCenter: parent.verticalCenter }
            Etiq { text: "Datos en la tele"; color: Tema.tinta; font.pixelSize: 22; anchors.verticalCenter: parent.verticalCenter }
        }
        onTocado: c.verDatos = true
    }

    // ---------- «Datos en la tele»: prender y apagar cada uno ----------
    MouseArea {
        anchors.fill: parent
        visible: c.verDatos
        onClicked: c.verDatos = false
        Rectangle { anchors.fill: parent; color: Qt.rgba(0, 0, 0, 0.55) }
    }
    Rectangle {
        visible: c.verDatos
        width: Math.min(c.width - 2 * c.m, 640); height: colDatos.height + 48
        anchors.centerIn: parent
        radius: 24
        color: Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.97); border { width: 2; color: Tema.bordeCromo }
        MouseArea { anchors.fill: parent }
        Column {
            id: colDatos
            x: 30; y: 24; width: parent.width - 60
            Item {
                width: parent.width; height: 60
                Texto { anchors.verticalCenter: parent.verticalCenter; text: "Datos en la tele" }
                Ficha {
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                    height: 52; width: tListo.implicitWidth + 44; radius: 26; amarilla: true
                    Etiq { id: tListo; anchors.centerIn: parent; text: "Listo"; color: Tema.acentoTinta; font.pixelSize: 20 }
                    onTocado: c.verDatos = false
                }
            }
            Repeater {
                model: c.rep ? c.rep.lista : []
                Item {
                    required property var modelData
                    required property int index
                    width: colDatos.width; height: 64
                    Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.12) }
                    Texto { anchors.verticalCenter: parent.verticalCenter; text: modelData.nombre; font.pixelSize: 26 }
                    Llave {
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        scale: 1.4; transformOrigin: Item.Right
                        prendida: c.rep.prendido(modelData.clave)
                        opacity: modelData.clave === "adelanto" && !(c.rep.distancia > 0) ? 0.4 : 1
                        onCambiada: (v) => c.rep.guardar(modelData.clave, { on: v })
                    }
                }
            }
            Text {
                width: parent.width; wrapMode: Text.WordWrap; topPadding: 10
                text: "Para moverlos, usá la pantalla completa de la Pi: la tele los pone en el mismo lugar"
                color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 19 }
            }
        }
    }
}
