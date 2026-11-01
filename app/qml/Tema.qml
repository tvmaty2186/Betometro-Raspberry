pragma Singleton
import QtQuick

// Colores, fuentes y medidas del tema elegido. Copiados de la app (docs/app/diseno.md y kit.css).
// Por ahora está completo el Avispón Verde; los otros tres se agregan más adelante en E2.
QtObject {
    readonly property string nombre: Contexto.tema
    readonly property bool avispon: nombre === "avispon"

    // Avispón Verde
    readonly property color verdeClaro: "#3d8c57"
    readonly property color verde1: "#2f7a49"
    readonly property color verde2: "#1d5233"
    readonly property color verde3: "#0d2a18"
    readonly property color crema: "#f3ecd8"
    readonly property color amarillo: "#f4c531"
    readonly property color amarillo2: "#d9a20f"
    readonly property color amarilloClaro: "#ffe07a"
    readonly property color blanco: "#f7f7f2"
    readonly property color negro: "#0b0d0c"
    readonly property color tinta: "#f7f7f2"
    readonly property color tintaSuave: "#d6e6d2"
    readonly property color bordeCromo: "#cfd4d8"
    readonly property color bordeCromoOscuro: "#4b5258"
    readonly property color numeroVerde: "#1b5a33"

    // Cromo: mismas paradas que --cromo
    readonly property var cromo: [[0, "#ffffff"], [0.30, "#c9ced3"], [0.52, "#6f777f"], [0.60, "#e8ebee"], [1, "#9aa1a8"]]

    // Fuentes
    // Qt registra cada grosor de Barlow como una familia aparte («Barlow ExtraBold»…)
    readonly property string fuente: "Barlow"                 // 400
    readonly property string fuente600: "Barlow SemiBold"     // 600 (y 700)
    readonly property string fuente800: "Barlow ExtraBold"    // 800 itálica (y 900)
    readonly property string fuenteNum: "Barlow Condensed"
}
