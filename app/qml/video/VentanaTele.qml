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
    property var principal: null    // la pantalla de la Pi: la tele nunca va ahí
    // La primera pantalla que no es la de la Pi
    readonly property var otra: {
        const ss = Qt.application.screens;
        for (let i = 0; i < ss.length; i++) if (!principal || ss[i].name !== principal.name) return ss[i];
        return null;
    }
    screen: otra
    // En eglfs, dos ventanas en la misma pantalla cierran el programa: solo se muestra si es otra
    visibility: otra ? Window.FullScreen : Window.Hidden
    flags: Qt.FramelessWindowHint
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
                    source: "qrc:/recursos/img/leon.png"; height: 300; fillMode: Image.PreserveAspectFit; mipmap: true
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
