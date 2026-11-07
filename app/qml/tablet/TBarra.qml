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
    // Sin hora válida (la Pi no tiene reloj: la da el GPS o internet) se ve «--:--», no una hora inventada (07-1.4)
    property date ahora: new Date()
    readonly property string hora: Datos.horaValida ? Qt.formatTime(ahora, "HH:mm") : "--:--"
    Timer { interval: 5000; running: true; repeat: true; onTriggered: raiz.ahora = new Date() }

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
        // Título cromado solo en el Avispón, como el del Informe y la app (Matías, 08-13); en los demás, en tinta
        TextoCromo {
            anchors.verticalCenter: parent.verticalCenter
            // De pie, en el inicio no entra todo: queda el león y la versión
            visible: raiz.titulo !== "" || Contexto.horizontal
            texto: raiz.titulo !== "" ? raiz.titulo.toUpperCase() : "BETÓMETRO"
            tam: raiz.titulo !== "" ? 34 : 32
        }
        // En el inicio, al lado de BETÓMETRO: la versión; si hay una nueva, «Actualizar» en amarillo (se toca)
        Rectangle {
            visible: raiz.titulo === ""
            anchors.verticalCenter: parent.verticalCenter
            height: 40; radius: 20; width: verFila.implicitWidth + 28
            color: Contexto.versionNueva ? Tema.acento : Qt.rgba(0, 0, 0, 0.35)
            border { width: 2; color: Contexto.versionNueva ? Tema.acentoBorde : Tema.bordeCromo }
            Text {
                id: verFila
                anchors.centerIn: parent
                text: Contexto.versionNueva ? "Actualizar a " + Contexto.versionNueva : "v" + Contexto.version
                color: Contexto.versionNueva ? Tema.acentoTinta : Tema.tintaSuave
                font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 21 }
            }
            MouseArea { anchors.fill: parent; anchors.margins: -8; enabled: !!Contexto.versionNueva; onClicked: Navegacion.ir("actualizar") }
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
        Rectangle {   // wifi; con la red propia armada: en color, con cuántos celus hay conectados
            visible: Contexto.horizontal || raiz.titulo === ""
            height: 50; width: Wifi.propia ? filaPropia.implicitWidth + 30 : 72; radius: 25
            color: Wifi.propia ? Tema.acento : Qt.rgba(0, 0, 0, 0.35); border { width: 2; color: Wifi.propia ? Tema.acentoBorde : Tema.bordeCromo }
            IconoWifi { visible: !Wifi.propia; anchors.centerIn: parent; width: 38; height: 30; nivel: Wifi.conectada ? Math.max(1, Wifi.senal) : 0 }
            Row {
                id: filaPropia; visible: Wifi.propia; anchors.centerIn: parent; spacing: 6
                TIcono { width: 32; height: 32; nombre: "red"; color: Tema.acentoTinta; anchors.verticalCenter: parent.verticalCenter }
                Text { text: Wifi.conectadosPropia; anchors.verticalCenter: parent.verticalCenter; color: Tema.acentoTinta
                       font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 26 } }
            }
            MouseArea { anchors.fill: parent; onClicked: Navegacion.ir("conectividad") }
        }
        // En Cabina la hora va con B612 normal: la Mono deja huecos en «03: 39» (08-10)
        Text { text: raiz.hora; anchors.verticalCenter: parent.verticalCenter; color: Tema.tinta
               font { family: Tema.cabina ? Tema.fuente : Tema.fuenteNum; weight: Font.Bold; pixelSize: 42 } }
    }
}
