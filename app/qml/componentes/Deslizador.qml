import QtQuick
import Betometro

// Barra para elegir un valor (brillo, volumen) arrastrando el dedo.
Item {
    id: raiz
    property real valor: 50
    property real minimo: 0
    property real maximo: 100
    signal movido(real valor)       // mientras se arrastra
    signal soltado(real valor)      // al soltar (se guarda)
    width: 220; height: 40
    readonly property real frac: (valor - minimo) / (maximo - minimo)
    Rectangle {
        id: carril
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width; height: 10; radius: 5
        color: Tema.caja2; border { width: 1.5; color: Tema.bordeCromo }
        Rectangle {
            width: Math.max(height, parent.width * raiz.frac); height: parent.height; radius: 5
            gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0; color: Tema.acentoOscuro } GradientStop { position: 1; color: Tema.acento } }
        }
    }
    Rectangle {
        width: 30; height: 30; radius: 15
        anchors.verticalCenter: parent.verticalCenter
        x: raiz.frac * (raiz.width - width)
        border { width: 2; color: "#4b5258" }
        gradient: Gradient { GradientStop { position: 0; color: "#ffffff" } GradientStop { position: 0.5; color: "#c9ced3" } GradientStop { position: 1; color: "#e8ebee" } }
    }
    MouseArea {
        anchors.fill: parent; anchors.margins: -12
        function valorEn(x) { return Math.round(raiz.minimo + Math.max(0, Math.min(1, (x - 12) / raiz.width)) * (raiz.maximo - raiz.minimo)); }
        onPressed: m => raiz.movido(valorEn(m.x))
        onPositionChanged: m => raiz.movido(valorEn(m.x))
        onReleased: m => raiz.soltado(valorEn(m.x))
    }
}
