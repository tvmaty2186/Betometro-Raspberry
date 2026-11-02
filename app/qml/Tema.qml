pragma Singleton
import QtQuick

// Colores y letras del tema elegido. Los 4 de la app (docs/app/diseno.md y los bloques
// :root[data-tema] de referencia/app-2.1.1/index.html) y el modo sol, que le gana a cualquiera.
QtObject {
    readonly property string nombre: Contexto.tema
    readonly property bool sol: Contexto.sol
    readonly property bool avispon: nombre === "avispon" && !sol
    readonly property bool rally: nombre === "rally" && !sol
    readonly property bool sport: nombre === "sport" && !sol
    readonly property bool cabina: nombre === "cabina" && !sol

    function de(av, ra, sp, ca, so) { return sol ? so : nombre === "rally" ? ra : nombre === "sport" ? sp : nombre === "cabina" ? ca : av; }

    // Fondo (degradé radial de arriba a la izquierda) y damero
    readonly property color fondo1: de("#3d8c57", "#2a2219", "#2b2f34", "#0e1215", "#fbfaf5")
    readonly property color fondo2: de("#1d5233", "#120e0a", "#1a1c1f", "#05080a", "#fbfaf5")
    readonly property color fondo3: de("#0d2a18", "#070504", "#111214", "#000000", "#fbfaf5")
    readonly property bool damero: avispon || sport

    // Tarjetas de los menús y caras de los widgets
    readonly property color tarjeta1: de(Qt.rgba(64 / 255, 140 / 255, 90 / 255, 0.55), "#2a241b", "#24272b", "#0f171c", "#ffffff")
    readonly property color tarjeta2: de(Qt.rgba(10 / 255, 38 / 255, 22 / 255, 0.92), "#16120d", "#16181b", "#070b0e", "#ffffff")
    readonly property color caja1: de("#1f4a2f", "#262019", "#2a2e33", "#10171c", "#ffffff")
    readonly property color caja2: de("#08190e", "#0b0906", "#0c0d0f", "#020304", "#ffffff")
    readonly property color bordeCromo: de("#cfd4d8", "#6d5f48", "#3a3e44", "#2b3640", "#000000")
    readonly property color bordeCromoOscuro: de("#4b5258", "#2a2118", "#15171a", "#0d1418", "#000000")
    readonly property real anchoBorde: sol ? 4 : avispon ? 2 : 1.5
    readonly property real radio: de(18, 12, 4, 6, 12)

    // Acento (botones LARGAR y MARCAR, lo elegido) y su tinta
    readonly property color acento: de("#f4c531", "#ffb000", "#e10600", "#2fd162", "#000000")
    readonly property color acentoClaro: de("#ffe07a", "#ffc94d", "#ff3b30", "#5ce68a", "#222222")
    readonly property color acentoOscuro: de("#d9a20f", "#c98900", "#a80400", "#1fa04a", "#000000")
    readonly property color acentoTinta: de("#0b0d0c", "#141008", "#ffffff", "#04140a", "#ffffff")
    readonly property color acentoBorde: de("#fff6d0", "#ffe2a0", "#ff6a60", "#9ff5bb", "#000000")

    // Letras
    readonly property color tinta: de("#f7f7f2", "#f3e7c8", "#ffffff", "#e8eef2", "#000000")
    readonly property color tintaSuave: de("#d6e6d2", "#b9a98a", "#9aa0a6", "#4fc3f7", "#1f1f1f")
    readonly property color numero: de("#f3ecd8", "#ffb000", "#ffffff", "#3cf06e", "#000000")   // números grandes
    readonly property color alerta: de("#ff8a7a", "#ff6b4a", "#ff4d4d", "#ff4d5e", "#a33a00")
    readonly property color bien: de("#7fd36b", "#3fa34d", "#19c463", "#3cf06e", "#0a8a35")
    readonly property color sugerida: de("#f4c531", "#ffb000", "#ffd400", "#ffd23f", "#d99a00")

    // Velocímetro redondo: cara crema en el Avispón; oscura en los otros
    readonly property bool caraClara: avispon || sol
    readonly property color cara1: de("#fffbef", "#262019", "#2a2e33", "#10171c", "#ffffff")
    readonly property color cara2: de("#d6caa9", "#0b0906", "#0c0d0f", "#020304", "#f3f1ea")
    readonly property color rayitas: de("#1d2a20", "#f3e7c8", "#9aa0a6", "#4fc3f7", "#000000")
    readonly property color numerosDial: de("#1d5233", "#f3e7c8", "#ffffff", "#e8eef2", "#000000")
    readonly property color aguja: de("#111111", "#f3e7c8", "#ffffff", "#e8eef2", "#000000")
    readonly property color zona: de("#2f7a49", "#3fa34d", "#19c463", "#3cf06e", "#0a9a3c")

    // Fijos del Avispón (rayas, banda, 101)
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
    readonly property color numeroVerde: "#1b5a33"
    readonly property var cromo: [[0, "#ffffff"], [0.30, "#c9ced3"], [0.52, "#6f777f"], [0.60, "#e8ebee"], [1, "#9aa1a8"]]

    // Letras de cada tema. Qt registra cada grosor como una familia aparte («Barlow ExtraBold»…)
    readonly property string fuente: de("Barlow", "Barlow", "Chakra Petch Medium", "B612", "Barlow")
    readonly property string fuente600: de("Barlow SemiBold", "Barlow SemiBold", "Chakra Petch", "B612", "Barlow SemiBold")
    readonly property string fuente800: de("Barlow ExtraBold", "Barlow ExtraBold", "Chakra Petch", "B612", "Barlow ExtraBold")
    readonly property string fuenteNum: de("Barlow Condensed", "Big Shoulders Display Thin", "Chakra Petch", "B612 Mono", "Barlow Condensed")
    readonly property bool cursiva: avispon || rally || sol
}
