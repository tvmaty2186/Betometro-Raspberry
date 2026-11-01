import QtQuick
import Betometro

// Banda blanca y amarilla, como en la puerta del auto (.banda).
Rectangle {
    height: 18
    gradient: Gradient {
        GradientStop { position: 0; color: Tema.blanco }
        GradientStop { position: 0.34; color: Tema.blanco }
        GradientStop { position: 0.3401; color: Tema.amarillo }
        GradientStop { position: 1; color: Tema.amarillo }
    }
}
