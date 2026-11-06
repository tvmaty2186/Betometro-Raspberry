import QtQuick
import QtQuick.Window
import Betometro

// La ventana de la tele (HDMI). Sin girar (el giro de Main.qml es solo para la pantalla de 5").
// Se arma en 1280×720 y se agranda o achica a lo que tenga la tele (en la Raspberry, kms.json la pone
// en 1280×720: escala 1, igual que la pantalla completa de la Pi en horizontal).
//  - Con el modo tele prendido: el video entero con los datos (la misma vista que la Pi, sin tocar).
//  - Si no: en reposo, fondo verde oscuro, el león y «BETÓMETRO».
Window {
    id: win
    objectName: "tele"
    // Sin «ventana madre»: si no, Qt la pone en la pantalla de la ventana principal
    transientParent: null
    screen: Tele.pantallaTele
    // Se muestra recién cuando ya está en su pantalla, y solo si es otra: en eglfs, dos ventanas en la
    // misma pantalla cierran el programa
    property bool lista: false
    visibility: lista && !!Tele.pantallaTele ? Window.FullScreen : Window.Hidden
    Component.onCompleted: lista = true
    flags: Qt.FramelessWindowHint
    // En su lugar del escritorio (si no, Qt la cambia a la pantalla que esté en 0,0: la de la Pi)
    x: Tele.pantallaTele ? Tele.pantallaTele.virtualX : 0
    y: Tele.pantallaTele ? Tele.pantallaTele.virtualY : 0
    width: 1280; height: 720
    color: "#000000"
    title: "Betómetro en la tele"

    Item {
        id: caja
        width: 1280; height: 720
        anchors.centerIn: parent
        scale: Math.min(win.width / 1280, win.height / 720)

        // ---------- En reposo ----------
        Rectangle {
            anchors.fill: parent
            visible: !cargador.item
            gradient: Gradient {
                GradientStop { position: 0; color: "#1d5233" }
                GradientStop { position: 1; color: "#0d2a18" }
            }
            Column {
                anchors.centerIn: parent
                spacing: 26
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "qrc:/recursos/img/leon.png"; height: 300; fillMode: Image.PreserveAspectFit; sourceSize.height: 600; smooth: true
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "BETÓMETRO"; color: "#f7f7f2"
                    font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 76; letterSpacing: 8 }
                }
            }
        }

        // ---------- Modo tele: el video con los datos ----------
        // El reproductor se fija antes de armar la vista (y no se suelta al cerrarla): así sus enlaces
        // nunca ven un reproductor vacío
        Loader {
            id: cargador
            anchors.fill: parent
            active: false
            property Item fijo: null
            function cambiar() {
                if (Tele.reproductor) { active = false; fijo = Tele.reproductor; active = true; }
                else active = false;
            }
            Component.onCompleted: cambiar()
            sourceComponent: VistaVideo {
                rep: cargador.fijo
                completo: true
                tocable: false
                Component.onCompleted: Tele.salida = salida
                Component.onDestruction: if (Tele.salida === salida) Tele.salida = null
            }
        }
        Connections { target: Tele; function onReproductorChanged() { cargador.cambiar(); } }
    }
}
