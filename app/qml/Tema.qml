pragma Singleton
import QtQuick

// Colores y letras del tema elegido. Los 4 de la app (docs/app/diseno.md y los bloques
// :root[data-tema] de referencia/app-2.1.1/index.html) y el modo sol, que le gana a cualquiera.
QtObject {
    readonly property string nombre: Contexto.tema
    readonly property bool sol: Contexto.sol
    readonly property bool avispon: nombre === "avispon" && !sol
    readonly property bool rally: nombre === "rally" && !sol
    readonly property bool sport: (nombre === "sport" || nombre === "propio") && !sol
    // Tema propio (Matías, 08/11/2026): parte del Deportivo y cambia solo 3 cosas desde el celu (Personalizar):
    // fondo, acento y letras (blanco, crema o negro). Los colores de aviso, la barra de promedio, adelantado /
    // atrasado y medida / libre NO cambian. El modo sol le gana igual que a los otros.
    readonly property bool propio: nombre === "propio" && !sol
    readonly property var pp: Datos.ajustes.temaPropio || ({})
    readonly property color pFondo: pp.fondo || "#1a1c1f"
    readonly property color pAcento: pp.acento || "#e10600"
    readonly property color pLetras: pp.letras === "negro" ? "#0b0d0c" : pp.letras === "crema" ? "#f3ecd8" : "#ffffff"
    function oscuro(c) { return 0.299 * c.r + 0.587 * c.g + 0.114 * c.b < 0.55; }
    readonly property bool cabina: nombre === "cabina" && !sol
    // Modo Tablet (05/11/2026): botones grandes, dentro de cualquier tema (Ajustes → Pantalla y tema).
    // «--prueba tablet…» lo prende para las capturas.
    readonly property bool tablet: !!Datos.ajustes.modoTablet || Contexto.prueba.indexOf("tablet") === 0
    // Todas las pantallas (menos el inicio) van grandes, como en el Modo Tablet (Matías, 06/11/2026)
    readonly property bool grandes: true

    function de(av, ra, sp, ca, so) { return sol ? so : nombre === "rally" ? ra : nombre === "sport" || nombre === "propio" ? sp : nombre === "cabina" ? ca : av; }

    // Fondo (degradé radial de arriba a la izquierda) y damero
    readonly property color fondo1: propio ? Qt.lighter(pFondo, 1.6) : de("#3d8c57", "#2a2219", "#2b2f34", "#0e1215", "#fbfaf5")
    readonly property color fondo2: propio ? pFondo : de("#1d5233", "#120e0a", "#1a1c1f", "#05080a", "#fbfaf5")
    readonly property color fondo3: propio ? Qt.darker(pFondo, 1.5) : de("#0d2a18", "#070504", "#111214", "#000000", "#fbfaf5")
    readonly property bool damero: avispon || sport

    // Tarjetas de los menús y caras de los widgets
    readonly property color tarjeta1: propio ? Qt.lighter(pFondo, 1.4) : de(Qt.rgba(64 / 255, 140 / 255, 90 / 255, 0.55), "#2a241b", "#24272b", "#0f171c", "#ffffff")
    readonly property color tarjeta2: propio ? Qt.darker(pFondo, 1.3) : de(Qt.rgba(10 / 255, 38 / 255, 22 / 255, 0.92), "#16120d", "#16181b", "#070b0e", "#ffffff")
    readonly property color caja1: propio ? Qt.lighter(pFondo, 1.6) : de("#1f4a2f", "#262019", "#2a2e33", "#10171c", "#ffffff")
    readonly property color caja2: propio ? Qt.darker(pFondo, 2.2) : de("#08190e", "#0b0906", "#0c0d0f", "#020304", "#ffffff")
    readonly property color bordeCromo: propio ? Qt.lighter(pFondo, 2.2) : de("#cfd4d8", "#6d5f48", "#3a3e44", "#2b3640", "#000000")
    readonly property color bordeCromoOscuro: propio ? Qt.darker(pFondo, 1.6) : de("#4b5258", "#2a2118", "#15171a", "#0d1418", "#000000")
    readonly property real anchoBorde: sol ? 4 : avispon ? 2 : 1.5
    readonly property real radio: de(18, 12, 4, 6, 12)
    // Cuánto entra por carácter en los números grandes (--k de la app). En sol, el del tema elegido.
    readonly property real k: nombre === "rally" ? 235 : nombre === "sport" || nombre === "propio" ? 165 : nombre === "cabina" ? 150 : 190

    // Acento (botones LARGAR y MARCAR, lo elegido) y su tinta
    readonly property color acento: propio ? pAcento : de("#f4c531", "#ffb000", "#e10600", "#2fd162", "#000000")
    readonly property color acentoClaro: propio ? Qt.lighter(pAcento, 1.3) : de("#ffe07a", "#ffc94d", "#ff3b30", "#5ce68a", "#222222")
    readonly property color acentoOscuro: propio ? Qt.darker(pAcento, 1.35) : de("#d9a20f", "#c98900", "#a80400", "#1fa04a", "#000000")
    readonly property color acentoTinta: propio ? oscuro(pAcento) ? "#ffffff" : "#0b0d0c" : de("#0b0d0c", "#141008", "#ffffff", "#04140a", "#ffffff")
    readonly property color acentoBorde: propio ? Qt.lighter(pAcento, 1.6) : de("#fff6d0", "#ffe2a0", "#ff6a60", "#9ff5bb", "#000000")

    // Letras
    readonly property color tinta: propio ? pLetras : de("#f7f7f2", "#f3e7c8", "#ffffff", "#e8eef2", "#000000")
    readonly property color tintaSuave: propio ? Qt.rgba(pLetras.r, pLetras.g, pLetras.b, 0.68) : de("#d6e6d2", "#b9a98a", "#9aa0a6", "#4fc3f7", "#1f1f1f")
    // Botón apagado (08-7): fondo oscuro, borde punteado gris y letras tenues (muestra aprobada 1.0.29)
    readonly property color apagadoFondo: de("#38000000", "#38000000", "#38000000", "#38000000", "#14000000")
    readonly property color apagadoBorde: de("#7f8a84", "#857d70", "#7c8186", "#5f7f8c", "#8a8f93")
    readonly property color apagadoTinta: de("#9fb0a5", "#a89f8f", "#9ea3a8", "#86a7b3", "#6f7478")
    readonly property color numero: propio ? pLetras : de("#f3ecd8", "#ffb000", "#ffffff", "#3cf06e", "#000000")   // números grandes
    readonly property color alerta: de("#ff8a7a", "#ff6b4a", "#ff4d4d", "#ff4d5e", "#a33a00")
    readonly property color bien: de("#7fd36b", "#3fa34d", "#19c463", "#3cf06e", "#0a8a35")
    readonly property color sugerida: de("#f4c531", "#ffb000", "#ffd400", "#ffd23f", "#d99a00")

    // Velocímetro redondo: cara crema en el Avispón; oscura en los otros
    readonly property bool caraClara: avispon || sol
    readonly property color cara1: propio ? Qt.lighter(pFondo, 1.6) : de("#fffbef", "#262019", "#2a2e33", "#10171c", "#ffffff")
    readonly property color cara2: propio ? Qt.darker(pFondo, 2.2) : de("#d6caa9", "#0b0906", "#0c0d0f", "#020304", "#f3f1ea")
    readonly property color rayitas: propio ? Qt.rgba(pLetras.r, pLetras.g, pLetras.b, 0.7) : de("#1d2a20", "#f3e7c8", "#9aa0a6", "#4fc3f7", "#000000")
    readonly property color numerosDial: propio ? pLetras : de("#1d5233", "#f3e7c8", "#ffffff", "#e8eef2", "#000000")
    readonly property color aguja: propio ? pLetras : de("#111111", "#f3e7c8", "#ffffff", "#e8eef2", "#000000")
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
    readonly property bool cursiva: avispon   // como la app: números en cursiva solo en Avispón (no en sol)
    // «Pasado» de la cuenta regresiva y «ATRASADO» (--warn) y «Pasado el ideal» de la progresiva (--mal), como la app
    // Brillo alrededor de los widgets (0 0 20px 5px var(--glow) de la app); Avispón y sol no tienen
    readonly property color glow: sol ? "transparent" : propio ? Qt.rgba(pAcento.r, pAcento.g, pAcento.b, 0.5) : de("transparent", Qt.rgba(1, 170 / 255, 40 / 255, 0.5), Qt.rgba(1, 40 / 255, 20 / 255, 0.55), Qt.rgba(79 / 255, 195 / 255, 247 / 255, 0.32), "transparent")
    readonly property color warn: de("#f4c531", "#ffb000", "#ffd400", "#ffd23f", "#a33a00")
    // Rojo y naranja de estado (pasada, grave, revisar). Iguales en los temas oscuros; más oscuros en sol, para que se
    // lean sobre blanco (auditoría 12: estaban escritos a mano en 8 archivos)
    readonly property color rojo: de("#d23b2c", "#d23b2c", "#d23b2c", "#d23b2c", "#b3261e")
    readonly property color naranja: de("#d9741a", "#d9741a", "#d9741a", "#d9741a", "#a34f00")
    readonly property color mal: de("#ff6b5a", "#c0162c", "#e10600", "#ff4d5e", "#c4001a")

    // Colores que estaban fijos en las pantallas (08-8, 1.0.29)
    // Botón de peligro (Apagar, Borrar, Desconectar): rojo en todos los temas, letras blancas siempre (08-14)
    readonly property color peligroClaro: de("#e0473a", "#e0473a", "#e0473a", "#e0473a", "#d93a2c")
    readonly property color peligro: de("#b8231a", "#b8231a", "#b8231a", "#b8231a", "#b8231a")
    readonly property color peligroOscuro: de("#8f130b", "#8f130b", "#8f130b", "#8f130b", "#8f130b")
    readonly property color peligroBorde: de("#ffb3aa", "#ffb3aa", "#ffb3aa", "#ffb3aa", "#000000")
    readonly property color peligroTinta: "#ffffff"
    // Lluvia (Clima, Carreras): celeste; en sol, un azul que se lea sobre blanco
    readonly property color lluvia: de("#5fb4ff", "#5fb4ff", "#5fb4ff", "#5fb4ff", "#0d5fb5")
    // Fondo de la barra de progreso (Drive): antes verde del Avispón en todos los temas
    readonly property color barraFondo: propio ? Qt.darker(pFondo, 2) : de("#0a2414", "#0b0906", "#0c0d0f", "#020304", "#e6e4dc")
    // Cuadro del video de la Cámara: siempre oscuro (en sol también, así se lee «No se encontró la cámara») (08-2)
    readonly property color videoFondo: de("#05110a", "#0b0906", "#0c0d0f", "#020304", "#111111")
    readonly property color videoTinta: "#ffffff"          // letras y etiquetas sobre el video
    readonly property color videoBorde: "#c9ced3"          // borde cromo de las etiquetas sobre el video
}
