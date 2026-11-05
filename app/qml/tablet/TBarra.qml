import QtQuick
import Betometro

// Barra fija de arriba del Modo Tablet: volver (o el león en el inicio), título, GPS, wifi y la hora grande.
Rectangle {
    id: raiz
    property string titulo: ""          // vacío: inicio (león y BETÓMETRO)
    signal volver()
    width: parent ? parent.width : 1280
    height: 76
    gradient: Gradient { GradientStop { position: 0; color: Tema.caja1 } GradientStop { position: 1; color: Tema.caja2 } }
    Rectangle { anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 4; color: Tema.acento }
    property string hora: Qt.formatTime(new Date(), "HH:mm")
    Timer { interval: 5000; running: true; repeat: true; onTriggered: raiz.hora = Qt.formatTime(new Date(), "HH:mm") }

    Row {
        x: 20; anchors.verticalCenter: parent.verticalCenter; anchors.verticalCenterOffset: -2
        spacing: 16
        Rectangle {
            visible: raiz.titulo !== ""
            width: 64; height: 60; radius: 18
            color: Qt.rgba(0, 0, 0, 0.35); border { width: 3; color: Tema.bordeCromo }
            TIcono { anchors.centerIn: parent; width: 38; height: 38; nombre: "atras" }
            MouseArea { anchors.fill: parent; anchors.margins: -10; onClicked: raiz.volver() }
        }
        Image { visible: raiz.titulo === ""; source: "qrc:/recursos/img/leon.png"; height: 52; fillMode: Image.PreserveAspectFit; anchors.verticalCenter: parent.verticalCenter }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: raiz.titulo !== "" ? raiz.titulo.toUpperCase() : "BETÓMETRO"
            color: Tema.tinta
            font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: raiz.titulo !== "" ? 34 : 32; letterSpacing: 1 }
        }
    }
    Row {
        anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter; verticalCenterOffset: -2 }
        spacing: 12
        Rectangle {   // GPS
            height: 50; radius: 25; width: gpsFila.implicitWidth + 34
            color: Qt.rgba(0, 0, 0, 0.35); border { width: 2; color: Tema.bordeCromo }
            Row { id: gpsFila; anchors.centerIn: parent; spacing: 10
                Rectangle { width: 16; height: 16; radius: 8; anchors.verticalCenter: parent.verticalCenter; color: Crono.gps.ok ? Tema.bien : Tema.alerta }
                Text { text: Crono.gps.ok ? "GPS " + (Crono.gps.satelites || Crono.gps.hz) : "Sin GPS"; color: Tema.tinta
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 23 } } }
        }
        Rectangle {   // wifi
            visible: Contexto.horizontal || raiz.titulo === ""
            height: 50; width: 72; radius: 25
            color: Qt.rgba(0, 0, 0, 0.35); border { width: 2; color: Tema.bordeCromo }
            IconoWifi { anchors.centerIn: parent; width: 38; height: 30; nivel: Wifi.conectada ? Math.max(1, Wifi.senal) : 0 }
            MouseArea { anchors.fill: parent; onClicked: Navegacion.ir("conectividad") }
        }
        Text { text: raiz.hora; anchors.verticalCenter: parent.verticalCenter; color: Tema.tinta
               font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 42 } }
    }
}
