import QtQuick
import Betometro

// Ajustes en Modo Tablet: 10 botones grandes (Pulsador desde la 1.0.29) (Google Drive, 05/11/2026). Muestra: docs/muestras/tablet/ajustes-h.png
// Cada uno abre su sección (TAjustesSeccion) o su pantalla.
TPantalla {
    id: raiz
    titulo: "Ajustes"
    readonly property var a: Datos.ajustes
    readonly property var temas: ({ avispon: "Avispón", rally: "Rally", sport: "Deportivo", cabina: "Cabina" })
    property bool apagarArmado: false
    Timer { id: desarmar; interval: 3000; onTriggered: raiz.apagarArmado = false }
    function seccion(s) { Navegacion.ir("ajustesSeccion", { seccion: s }); }

    Grid {
        anchors.fill: raiz.area
        columns: raiz.h ? 5 : 2
        spacing: 18
        readonly property int filas: raiz.h ? 2 : 5
        readonly property real an: (width - (columns - 1) * spacing) / columns
        readonly property real al: (height - (filas - 1) * spacing) / filas
        TBoton { width: parent.an; height: parent.al; icono: "auto"; nombre: "Mi auto"
                 dato: (raiz.a.numeroAuto || "101") + " · " + (raiz.a.nombreAuto || ""); onTocado: raiz.seccion("auto") }
        TBoton { width: parent.an; height: parent.al; icono: "tema"; nombre: "Pantalla y tema"
                 dato: (raiz.temas[raiz.a.tema] || "Avispón") + " · " + Math.round(raiz.a.brillo) + " %"; onTocado: raiz.seccion("pantalla") }
        TBoton { width: parent.an; height: parent.al; icono: "voz"; nombre: "Voz y sonido"
                 dato: (raiz.a.voz === "tomas" ? "Tomás" : "Elena") + " · " + raiz.a.cuentaN + " s"; onTocado: raiz.seccion("voz") }
        TBoton { width: parent.an; height: parent.al; icono: "velocidad"; nombre: "Velocidad y GPS"
                 dato: Crono.gps.modulo ? Crono.gps.modulo.split(" · ")[0] : (raiz.a.velActiva !== false ? "Prendida" : "Apagada")
                 onTocado: raiz.seccion("velocidad") }
        TBoton { width: parent.an; height: parent.al; icono: "bandera"; nombre: "Pulsador"
                 dato: raiz.a.marcarPantalla !== false ? "MARCAR en pantalla" : "Solo el pulsador"; onTocado: raiz.seccion("pulsador") }
        TBoton { width: parent.an; height: parent.al; icono: "red"; nombre: "Conectividad"
                 dato: Wifi.propia ? Wifi.nombrePropia : (Wifi.conectada || "Sin wifi"); onTocado: Navegacion.ir("conectividad") }
        TBoton { width: parent.an; height: parent.al; icono: "camara"; nombre: "Cámara"
                 dato: !raiz.a.camara ? "Apagada" : Camara.hay ? "Lista" : "Sin cámara"; onTocado: Navegacion.ir("camara", { desde: "ajustes" }) }
        TBoton { width: parent.an; height: parent.al; icono: "nube"; nombre: "Google Drive"
                 dato: Drive.estado === "conectado" ? (Drive.subiendo ? "Subiendo" : Drive.error ? "Con un problema" : "Conectado") : "Sin conectar"
                 onTocado: Navegacion.ir("drive") }
        // En Deportivo el acento es rojo como «Apagar»: con versión nueva va con borde rojo y fondo oscuro (Matías, 08-6)
        TBoton { width: parent.an; height: parent.al; icono: "actualizar"; nombre: "Actualizar"
                 tipo: Contexto.versionNueva ? (Tema.sport ? "borde" : "acento") : ""
                 dato: Contexto.versionNueva ? "Hay " + Contexto.versionNueva : "v" + Contexto.version; onTocado: Navegacion.ir("actualizar") }
        TBoton { width: parent.an; height: parent.al; icono: "apagar"; nombre: raiz.apagarArmado ? "Tocá otra vez" : "Apagar"
                 tipo: "peligro"; dato: raiz.apagarArmado ? "para apagar" : "Tocá 2 veces"
                 onTocado: { if (raiz.apagarArmado) Sistema.apagar(); else { raiz.apagarArmado = true; desarmar.restart(); } } }
    }
}
