import QtQuick
import Betometro

// Cabecera del informe: volver, león, nombre y datos; el error total grande; «Ver en la tele», Informe/Video
// (o el 101) y Compartir. Ocupa toda la pantalla: cada cosa va en el mismo lugar que cuando estaba en Informe.qml.
Item {
    id: cab
    property bool h: true
    property var c: ({})             // Datos.carrera(id): nombre, fechaCorta, ideal, distancia, errorTotal, promedio, mejor
    property bool rec: false         // recorrido libre
    property var marcas: []
    property var vueltas: []
    property real dist: 0
    property bool conVideo: false    // la carrera tiene video: Informe / Video en vez del 101
    property bool verVideo: false
    property bool procesando: false  // el video todavía se graba o se pasa a MP4
    property bool botonTele: false
    property bool enTele: false
    property string numero: "101"
    signal volver()
    signal verEnTele()
    signal elegirVideo(bool si)
    signal compartir()
    anchors.fill: parent

    // Volver de 64 px como en las demás pantallas (TBarra), no el de 40 (07-3.5, 08-15)
    Rectangle {
        x: cab.h ? 24 : 20; y: cab.h ? 12 : 16
        width: 64; height: 60; radius: 18
        color: Qt.rgba(0, 0, 0, 0.35); border { width: 3; color: Tema.bordeCromo }
        TIcono { anchors.centerIn: parent; width: 38; height: 38; nombre: "atras" }
        MouseArea { anchors.fill: parent; anchors.margins: -10; onClicked: cab.volver() }
    }
    Row {
        x: cab.h ? 104 : 24; y: cab.h ? 14 : 96; spacing: 14
        Image { source: "qrc:/recursos/img/leon.png"; height: cab.h ? 46 : 50; fillMode: Image.PreserveAspectFit; anchors.verticalCenter: parent.verticalCenter; sourceSize.height: 100; smooth: true }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            TextoCromo { texto: (cab.c.nombre || "").toUpperCase(); tam: cab.h ? 36 : 38 }
            Sub { text: cab.rec ? cab.c.fechaCorta + " · recorrido libre" + (cab.marcas.length ? " · " + cab.marcas.length + (cab.marcas.length === 1 ? " marca" : " marcas") : "") + (cab.vueltas.some(v => v.interrumpida) ? " · se cortó" : "")
                  : cab.c.fechaCorta + " · ideal " + cab.c.ideal + (cab.dist > 0 ? " · " + cab.c.distancia + " m" : "") + " · " + cab.vueltas.length + " vueltas" }
        }
    }
    // Lo que el navegante quiere saber apenas para: el error total, grande y arriba (07-2.1)
    Column {
        id: errorArriba
        readonly property var med: cab.vueltas.filter(v => v.medida && v.difMs !== undefined)
        visible: !cab.rec && med.length > 0
        readonly property real promS: med.length ? med.reduce((a, v) => a + Math.abs(v.difMs), 0) / med.length / 1000 : 0
        anchors { right: cab.h ? derecha.left : undefined; rightMargin: 28 }
        x: cab.h ? 0 : 104   // de pie, a la derecha del volver de 64 px
        y: cab.h ? 14 : 12
        spacing: 0
        Sub { text: "Error total · " + errorArriba.med.length + (errorArriba.med.length === 1 ? " medida" : " medidas"); tam: 13
              anchors.right: cab.h ? parent.right : undefined }
        Text { anchors.right: cab.h ? parent.right : undefined
               text: cab.c.errorTotal || ""
               height: 50; verticalAlignment: Text.AlignVCenter   // alto fijo: con Chakra Petch (Deportivo) la línea de abajo se salía
               color: errorArriba.promS <= 0.2 ? Tema.bien : errorArriba.promS <= 0.5 ? Tema.acento : Tema.mal
               font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 46 }
               // Tema propio: sobre una pastilla gris oscura, que se lea con cualquier fondo (Matías: rojo sobre rojo)
               Rectangle { visible: Tema.propio; z: -1; anchors { fill: parent; leftMargin: -12; rightMargin: -12; topMargin: 2; bottomMargin: 2 }
                           radius: height / 2; color: "#0c0d0f"; opacity: 0.85 } }
        Sub { visible: cab.h; text: "promedio " + (cab.c.promedio || "–") + " · mejor " + (cab.c.mejor || "–"); tam: 12
              anchors.right: parent.right }
    }
    Row {
        id: derecha
        anchors { right: parent.right; rightMargin: cab.h ? 24 : 20 }
        y: cab.h ? 20 : 20; spacing: 16
        // 📺 Ver en la tele: con la tele enchufada tiene que notarse (Matías, 06/11/2026). Botón del tema (cromo,
        // como Compartir pero oscuro), ícono de línea del tema en el color de acento y el borde que «late» suave.
        Item {
            id: botonTele
            visible: cab.botonTele
            anchors.verticalCenter: parent.verticalCenter
            height: cab.h ? 56 : 54; width: filaTele.implicitWidth + 40
            readonly property real radio: Math.max(6, Tema.radio)
            Rectangle {   // el aura que late
                anchors.fill: parent; anchors.margins: -5; radius: botonTele.radio + 5
                color: "transparent"; border { width: 3; color: Tema.acento }
                SequentialAnimation on opacity {
                    running: botonTele.visible && !cab.enTele; loops: Animation.Infinite   // debajo del control de la tele, no (03-1.1)
                    NumberAnimation { from: 0.15; to: 0.9; duration: 900; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 0.9; to: 0.15; duration: 900; easing.type: Easing.InOutSine }
                }
            }
            Rectangle { anchors.fill: parent; anchors.margins: -2; radius: botonTele.radio + 2; color: Tema.bordeCromo }
            Rectangle {
                anchors.fill: parent; radius: botonTele.radio
                gradient: Gradient {
                    GradientStop { position: 0; color: zonaTele.pressed ? Qt.lighter(Tema.caja1, 1.4) : Qt.lighter(Tema.caja1, 1.15) }
                    GradientStop { position: 1; color: Tema.caja2 }
                }
            }
            Row {
                id: filaTele; anchors.centerIn: parent; spacing: 12
                TIcono { width: 36; height: 36; nombre: "tele"; color: Tema.acento; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "Ver en la tele"; color: Tema.tinta; anchors.verticalCenter: parent.verticalCenter
                       font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: cab.h ? 22 : 20 } }
            }
            MouseArea { id: zonaTele; anchors.fill: parent; anchors.margins: -6; onClicked: cab.verEnTele() }
        }
        Numero101 { visible: !cab.conVideo; d: cab.h ? 62 : 58; numero: cab.numero; anchors.verticalCenter: parent.verticalCenter }
        Segmentado {
            visible: cab.conVideo
            anchors.verticalCenter: parent.verticalCenter
            height: cab.h ? 44 : 42
            opciones: [{ texto: "Informe", valor: false }, { texto: cab.procesando ? "Video…" : "Video", valor: true }]
            valor: cab.verVideo
            onElegido: (v) => cab.elegirVideo(v)
        }
        BotonAmarillo { width: cab.h ? 190 : 170; height: cab.h ? 56 : 54; texto: "Compartir"; anchors.verticalCenter: parent.verticalCenter
                        onTocado: cab.compartir() }
    }
}
