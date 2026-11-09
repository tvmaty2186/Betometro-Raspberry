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
    readonly property var temas: ({ avispon: "Avispón", rally: "Rally", sport: "Deportivo", cabina: "Cabina", propio: "Propio" })
    // Apagar: una ventana con Apagar, Reiniciar y Cancelar (Matías, 08/11/2026)
    property bool menuApagar: Contexto.prueba.indexOf("apagar") === 0   // --prueba apagar: abierta, para las capturas
    // Qué está trabajando que no se puede cortar ("" = nada): grabando, pasando a MP4 o subiendo a Google Drive
    // (09/11/2026) También: actualizando, Prueba completa y un comando del buzón (revisión a distancia)
    readonly property string ocupado: Contexto.prueba === "apagar-ocupado" ? "Subiendo a Google Drive"
        : typeof Actualizador !== "undefined" && Actualizador && (Actualizador.estado === "descargando" || Actualizador.estado === "instalando") ? "Actualizando Betómetro"
        : typeof PruebaCompleta !== "undefined" && PruebaCompleta && PruebaCompleta.estado === "corriendo" ? "Prueba completa en marcha"
        : typeof Buzon !== "undefined" && Buzon && Buzon.corriendo ? "Revisión a distancia en marcha"
        : typeof Camara !== "undefined" && Camara && Camara.grabando ? "Grabando video"
        : typeof Camara !== "undefined" && Camara && Camara.convirtiendo ? "Preparando el video"
        : typeof Drive !== "undefined" && Drive && Drive.subiendo ? "Subiendo a Google Drive" : ""
    function seccion(s) { Navegacion.ir("ajustesSeccion", { seccion: s }); }
    readonly property var orden: ["auto", "tema", "voz", "velocidad", "pulsador", "red", "camara", "drive", "actualizar", "prueba", "apagar"]
    readonly property int enGrilla: raiz.h ? 8 : 10
    readonly property int filas: raiz.h ? 3 : 6
    readonly property real alto: (raiz.area.height - (filas - 1) * 18) / filas
    function nombre(k) {
        switch (k) {
        case "auto": return "Mi auto"; case "tema": return "Pantalla y tema"; case "voz": return "Voz y sonido"; case "velocidad": return "Velocidad y GPS";
        case "pulsador": return "Pulsador"; case "red": return "Conectividad"; case "camara": return "Cámara"; case "drive": return "Google Drive";
        case "actualizar": return "Actualizar"; case "prueba": return "Prueba completa"; default: return "Apagar";
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
        default: return "o reiniciar";
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
        default: raiz.menuApagar = true;
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

    // Ventana de Apagar: tapa toda la pantalla; tocar afuera es como Cancelar
    Item {
        parent: raiz
        anchors.fill: parent
        z: 100
        visible: raiz.menuApagar
        Rectangle { anchors.fill: parent; color: "#000000"; opacity: 0.72 }
        MouseArea { anchors.fill: parent; onClicked: raiz.menuApagar = false }
        Rectangle {
            id: panel
            anchors.centerIn: parent
            width: raiz.h ? 1040 : 620
            height: raiz.h ? 400 : 860
            radius: Math.max(14, Tema.radio * 1.7)
            gradient: Gradient { GradientStop { position: 0; color: Tema.tarjeta1 } GradientStop { position: 1; color: Tema.tarjeta2 } }
            border { width: 3; color: Tema.bordeCromo }
            MouseArea { anchors.fill: parent }   // que tocar el panel no cierre
            Text {
                id: pregunta
                anchors { top: parent.top; topMargin: 26; horizontalCenter: parent.horizontalCenter }
                text: raiz.ocupado ? raiz.ocupado + "…" : "¿Qué querés hacer?"; color: raiz.ocupado ? Tema.warn : Tema.tinta
                width: parent.width - 52; horizontalAlignment: Text.AlignHCenter; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 20
                font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 40 }
            }
            Grid {
                anchors { top: pregunta.bottom; topMargin: 26; left: parent.left; right: parent.right; bottom: parent.bottom; margins: 26 }
                columns: raiz.h ? 3 : 1
                spacing: 22
                readonly property real an: raiz.h ? (width - 2 * spacing) / 3 : width
                readonly property real al: raiz.h ? height : (height - 2 * spacing) / 3
                // No se apaga ni reinicia con un video grabándose o pasándose a MP4, ni subiendo a Google Drive:
                // se perdería o quedaría a medias (Matías, 09/11/2026). Se habilita solo cuando termina.
                TBoton { width: parent.an; height: parent.al; icono: "apagar"; nombre: "Apagar"; tipo: "peligro"
                         dato: raiz.ocupado ? "Esperá a que termine" : "Hasta la próxima"; opacity: raiz.ocupado ? 0.45 : 1
                         onTocado: raiz.ocupado ? Navegacion.avisar(raiz.ocupado + ": esperá a que termine") : Sistema.apagar() }
                TBoton { width: parent.an; height: parent.al; icono: "actualizar"; nombre: "Reiniciar"
                         dato: raiz.ocupado ? "Esperá a que termine" : "Apaga y vuelve a prender"; opacity: raiz.ocupado ? 0.45 : 1
                         onTocado: raiz.ocupado ? Navegacion.avisar(raiz.ocupado + ": esperá a que termine") : Sistema.reiniciar() }
                TBoton { width: parent.an; height: parent.al; icono: "atras"; nombre: "Cancelar"; dato: "Seguir usando"
                         onTocado: raiz.menuApagar = false }
            }
        }
    }
}
