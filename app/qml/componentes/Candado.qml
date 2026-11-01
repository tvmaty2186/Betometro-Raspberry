import QtQuick
import Betometro

// Candado chico (red con clave). Dibujado, para no depender de letras de emojis.
Item {
    width: 18; height: 22
    Rectangle { x: 4; y: 0; width: 10; height: 14; radius: 5; color: "transparent"; border { width: 2.5; color: Tema.amarillo } }
    Rectangle { x: 0; y: 9; width: 18; height: 13; radius: 3; color: Tema.amarillo }
}
