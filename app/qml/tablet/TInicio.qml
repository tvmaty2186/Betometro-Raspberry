import QtQuick
import Betometro

// Inicio en Modo Tablet: 9 botones grandes (3×3; con la Tienda desde el 08/11/2026). Muestra: docs/muestras/tablet/inicio-h.png
TPantalla {
    id: raiz
    titulo: ""
    readonly property var proxima: Datos.proximas.length > 0 ? Datos.proximas[0] : null
    // Clima del lugar elegido (o del GPS) para el dato del botón
    readonly property bool climaGps: Datos.ajustes.climaModo !== "lugar"
    readonly property var lugar: Datos.ajustes.climaLugar || null
    readonly property bool hayLugar: climaGps ? Clima.hayGps : !!(lugar && lugar.lat !== undefined)
    readonly property var p: hayLugar && Clima.version >= 0 ? Clima.pronostico(climaGps ? Clima.gpsLat : lugar.lat, climaGps ? Clima.gpsLon : lugar.lon) : ({})
    readonly property string nombreLugar: climaGps ? (Clima.nombreGps || "GPS") : (lugar && lugar.nombre ? lugar.nombre : "")

    Grid {
        anchors.fill: raiz.area
        columns: 3
        spacing: 18
        readonly property real an: (width - (columns - 1) * spacing) / columns
        readonly property real al: (height - 2 * spacing) / 3
        TBoton { width: parent.an; height: parent.al; tipo: "acento"; icono: "bandera"; nombre: "Carreras"
                 dato: raiz.proxima ? "Próxima: " + raiz.proxima.nombre + " " + raiz.proxima.fecha.slice(8, 10) + "/" + raiz.proxima.fecha.slice(5, 7) : "Cargar una carrera"
                 onTocado: Navegacion.ir("carreras") }
        TBoton { width: parent.an; height: parent.al; icono: "crono"; nombre: "Práctica"
                 dato: Formato.tiempo(Datos.ajustes.tiempoIdealMs).replace(/:\d$/, "") + " · " + (Datos.ajustes.distanciaM / 1000).toFixed(1).replace(".", ",") + " km"
                 onTocado: Navegacion.ir("practica") }
        TBoton { width: parent.an; height: parent.al; icono: "check"; nombre: "Chequeo previo"
                 tipo: Salud.tensionBaja || Salud.calor ? "peligro" : ""
                 dato: Salud.tensionBaja ? "Tensión baja" : Salud.calor ? "Calor" : Crono.gps.ok ? "GPS listo" : "Esperando GPS"
                 onTocado: Navegacion.ir("chequeo") }
        TBoton { width: parent.an; height: parent.al; nombre: "Clima"
                 clima: raiz.p.ok ? (raiz.p.actual.codigo || 0) : 2
                 dato: raiz.p.ok ? raiz.p.actual.temp + "° · " + raiz.nombreLugar : "Ver pronóstico"
                 onTocado: Navegacion.ir("clima") }
        TBoton { width: parent.an; height: parent.al; icono: "pantalla"; nombre: "Pantalla de carrera"; onTocado: Navegacion.ir("editor") }
        TBoton { width: parent.an; height: parent.al; icono: "camara"; nombre: "Cámara"
                 dato: !Datos.ajustes.camara ? "Apagada" : Camara.hay ? "Lista" : "Sin cámara"
                 onTocado: Navegacion.ir("camara") }
        TBoton { width: parent.an; height: parent.al; icono: "red"; nombre: "Wifi e informes"
                 dato: Wifi.propia ? Wifi.nombrePropia : Wifi.conectada ? Wifi.direccion : "Sin wifi"
                 onTocado: Navegacion.ir("conectividad") }
        // Herramientas que se abren aparte de Betómetro (Matías, 08/11/2026). Por ahora vacía.
        TBoton { width: parent.an; height: parent.al; icono: "tienda"; nombre: "Tienda"; tipo: "borde"
                 dato: "Herramientas"
                 onTocado: Navegacion.ir("tienda") }
        TBoton { width: parent.an; height: parent.al; icono: "ajustes"; nombre: "Ajustes"
                 dato: Contexto.versionNueva ? "Actualizar a " + Contexto.versionNueva : "v" + Contexto.version
                 onTocado: Navegacion.ir("ajustes") }
    }
}
