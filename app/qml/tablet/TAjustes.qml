import QtQuick
import Betometro

// Ajustes en Modo Tablet: 11 botones grandes (Pulsador desde la 1.0.29, Prueba completa desde la 1.0.30) (Google Drive, 05/11/2026).
// Muestra: docs/muestras/tablet/ajustes-h.png. Cada uno abre su sección (TAjustesSeccion) o su pantalla.
// Acostada: 2 filas de 4 y una última fila de 3 más anchos (Actualizar, Prueba completa, Apagar); de pie: 5 filas
// de 2 y «Apagar» solo abajo, a lo ancho (11 no entra parejo en ninguna grilla).
TPantalla {
    id: raiz
    titulo: "Ajustes"
    readonly property var a: Datos.ajustes
    readonly property var temas: ({ avispon: "Avispón", rally: "Rally", sport: "Deportivo", cabina: "Cabina" })
    property bool apagarArmado: false
    Timer { id: desarmar; interval: 3000; onTriggered: raiz.apagarArmado = false }
    function seccion(s) { Navegacion.ir("ajustesSeccion", { seccion: s }); }
    readonly property var orden: ["auto", "tema", "voz", "velocidad", "pulsador", "red", "camara", "drive", "actualizar", "prueba", "apagar"]
    readonly property int enGrilla: raiz.h ? 8 : 10
    readonly property int filas: raiz.h ? 3 : 6
    readonly property real alto: (raiz.area.height - (filas - 1) * 18) / filas
    function nombre(k) {
        switch (k) {
        case "auto": return "Mi auto"; case "tema": return "Pantalla y tema"; case "voz": return "Voz y sonido"; case "velocidad": return "Velocidad y GPS";
        case "pulsador": return "Pulsador"; case "red": return "Conectividad"; case "camara": return "Cámara"; case "drive": return "Google Drive";
        case "actualizar": return "Actualizar"; case "prueba": return "Prueba completa"; default: return raiz.apagarArmado ? "Tocá otra vez" : "Apagar";
        }
    }
    function icono(k) { return k === "prueba" ? "check" : k === "drive" ? "nube" : k === "pulsador" ? "bandera" : k; }
    function dato(k) {
        switch (k) {
        case "auto": return (raiz.a.numeroAuto || "101") + " · " + (raiz.a.nombreAuto || "");
        case "tema": return (raiz.temas[raiz.a.tema] || "Avispón") + " · " + Math.round(raiz.a.brillo) + " %";
        case "voz": return (raiz.a.voz === "tomas" ? "Tomás" : "Elena") + " · " + raiz.a.cuentaN + " s";
        case "velocidad": return Crono.gps.modulo ? Crono.gps.modulo.split(" · ")[0] : (raiz.a.velActiva !== false ? "Prendida" : "Apagada");
        case "pulsador": return raiz.a.marcarPantalla !== false ? "MARCAR en pantalla" : "Solo el pulsador";
        case "red": return Wifi.propia ? Wifi.nombrePropia : (Wifi.conectada || "Sin wifi");
        case "camara": return !raiz.a.camara ? "Apagada" : Camara.hay ? "Lista" : "Sin cámara";
        case "drive": return Drive.estado === "conectado" ? (Drive.subiendo ? "Subiendo" : Drive.error ? "Con un problema" : "Conectado") : "Sin conectar";
        case "actualizar": return Contexto.versionNueva ? "Hay " + Contexto.versionNueva : "v" + Contexto.version;
        // Prueba completa: cómo salió la última (verde OK · naranja revisar · rojo avisar)
        case "prueba": return PruebaCompleta.estado === "corriendo" ? "Probando…" : PruebaCompleta.estado === "ok" ? "Todo bien · " + PruebaCompleta.cuando
                              : PruebaCompleta.estado === "leve" ? "Revisar · " + PruebaCompleta.fallas + (PruebaCompleta.fallas === 1 ? " aviso" : " avisos")
                              : PruebaCompleta.estado === "grave" ? "Hay errores: avisá" : "La Pi se revisa sola";
        default: return raiz.apagarArmado ? "para apagar" : "Tocá 2 veces";
        }
    }
    function tipo(k) {
        // En Deportivo el acento es rojo como «Apagar»: con versión nueva va con borde rojo y fondo oscuro (Matías, 08-6)
        if (k === "actualizar") return Contexto.versionNueva ? (Tema.sport ? "borde" : "acento") : "";
        if (k === "prueba") return PruebaCompleta.estado === "grave" ? "peligro" : PruebaCompleta.estado === "leve" ? "naranja" : "";
        return k === "apagar" ? "peligro" : "";
    }
    function tocar(k) {
        switch (k) {
        case "auto": case "tema": case "voz": case "velocidad": case "pulsador": raiz.seccion(k === "tema" ? "pantalla" : k); break;
        case "red": Navegacion.ir("conectividad"); break;
        case "camara": Navegacion.ir("camara", { desde: "ajustes" }); break;
        case "drive": Navegacion.ir("drive"); break;
        case "actualizar": Navegacion.ir("actualizar"); break;
        case "prueba": Navegacion.ir("prueba"); break;
        default: if (raiz.apagarArmado) Sistema.apagar(); else { raiz.apagarArmado = true; desarmar.restart(); }
        }
    }
    Component {
        id: boton
        TBoton {
            required property string modelData
            required property int index
            readonly property string k: modelData
            height: raiz.alto; icono: raiz.icono(k); nombre: raiz.nombre(k); dato: raiz.dato(k); tipo: raiz.tipo(k)
            colorDato: k === "prueba" && PruebaCompleta.estado === "ok" ? Tema.bien : Tema.acento
            onTocado: raiz.tocar(k)
        }
    }
    Column {
        anchors.fill: raiz.area
        spacing: 18
        Grid {
            id: grilla
            width: parent.width
            columns: raiz.h ? 4 : 2
            spacing: 18
            readonly property real an: (width - (columns - 1) * spacing) / columns
            Repeater { model: raiz.orden.slice(0, raiz.enGrilla); delegate: boton; onItemAdded: (i, item) => item.width = Qt.binding(() => grilla.an) }
        }
        Row {
            id: fila
            width: parent.width
            spacing: 18
            readonly property int n: raiz.orden.length - raiz.enGrilla
            readonly property real an: (width - (n - 1) * spacing) / n
            Repeater { model: raiz.orden.slice(raiz.enGrilla); delegate: boton; onItemAdded: (i, item) => item.width = Qt.binding(() => fila.an) }
        }
    }
}
