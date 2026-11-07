import QtQuick
import Betometro

// Chequeo previo en Modo Tablet: cada cosa es un botón grande, en rojo si hay que revisarla.
// Pulsador: se toca «Pulsador» y se aprieta el pulsador (o la pantalla): dice por dónde llegó.
TPantalla {
    id: raiz
    titulo: "Chequeo previo"
    readonly property var g: Crono.gps
    property string pulsador: "sin probar"    // sin probar, esperando, funciona
    property string llegoPor: ""
    readonly property real umbral: Datos.ajustes.umbralPrecisionM || 20
    readonly property bool gpsOk: !!g.ok && g.hz > 0 && g.precision <= umbral
    readonly property bool todoListo: gpsOk || !Datos.ajustes.velActiva
    focus: true
    Component.onCompleted: { forceActiveFocus(); if (Contexto.prueba === "simulador") Simulador.correr(); }   // simulador: solo capturas
    function recibido(por) { if (pulsador !== "esperando") return; pulsador = "funciona"; llegoPor = por; }
    Connections { target: Pulsador; ignoreUnknownSignals: true; function onMarca() { raiz.recibido("el pulsador"); } }
    Keys.onPressed: e => {
        if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter || e.key === Qt.Key_Space || e.key === Qt.Key_F9) { recibido("el teclado"); e.accepted = true; }
    }

    // Muestra aprobada 1.0.29: TODO LISTO pasa a barra ancha abajo para que entre la casilla «Simulador»
    Grid {
        id: grilla
        width: raiz.area.width
        height: raiz.area.height - 110 - 18
        columns: raiz.h ? 4 : 2
        spacing: 18
        readonly property real an: (width - (columns - 1) * spacing) / columns
        readonly property real al: (height - ((raiz.h ? 2 : 4) - 1) * spacing) / (raiz.h ? 2 : 4)
        TBoton { width: parent.an; height: parent.al; icono: "gps"; nombre: "GPS"
                 tipo: Datos.ajustes.velActiva && !raiz.gpsOk ? "peligro" : ""
                 dato: !Datos.ajustes.velActiva ? "Velocidad apagada" : !raiz.g.ok ? (raiz.g.satelites ? raiz.g.satelites + " satélites…" : "Sin señal")
                       : (raiz.g.satelites ? raiz.g.satelites + " satélites · " : "GPS listo · ") + Number(raiz.g.precision).toFixed(1).replace(".", ",") + " m" }
        TBoton { width: parent.an; height: parent.al; icono: "velocidad"; nombre: "Módulo"
                 readonly property bool habla: !!raiz.g.ultimoDato && Date.now() - raiz.g.ultimoDato < 5000
                 tipo: raiz.g.modulo && !habla ? "peligro" : ""
                 dato: !raiz.g.modulo ? "Buscando…" : !habla ? "Sin comunicación" : (raiz.g.original ? "" : "Réplica ") + raiz.g.modulo.split(" · ")[0] + " · " + (raiz.g.hz || 0) + " por s" }
        TBoton { width: parent.an; height: parent.al; icono: "bandera"; nombre: "Pulsador"
                 tipo: raiz.pulsador === "funciona" ? "acento" : ""
                 dato: raiz.pulsador === "esperando" ? "Apretalo ahora…" : raiz.pulsador === "funciona" ? "Llegó por " + raiz.llegoPor : "Tocá para probar"
                 onTocado: { if (raiz.pulsador === "esperando") raiz.recibido("la pantalla"); else { raiz.pulsador = "esperando"; raiz.forceActiveFocus(); } } }
        TBoton { width: parent.an; height: parent.al; icono: "voz"; nombre: "Probar la voz"; dato: "Volumen " + Math.round(Datos.ajustes.volumen) + "%"
                 onTocado: Audio.lista ? Audio.probar() : Navegacion.avisar("Faltan los audios de la voz") }
        TBoton { width: parent.an; height: parent.al; icono: "apagar"; nombre: "Tensión"
                 tipo: Salud.tensionBaja ? "peligro" : ""
                 dato: !Salud.sabe ? "—" : Salud.tensionBaja ? "Baja ahora" : Salud.huboTensionBaja ? "Bajó desde que prendió" : "Bien" }
        TBoton { width: parent.an; height: parent.al; icono: "tema"; nombre: "Temperatura"
                 tipo: Salud.calor ? "peligro" : ""
                 dato: Salud.temperatura >= 0 ? Salud.temperatura + " °C" + (Salud.calor ? " · se frena" : "") : "—" }
        // En rojo: cámara prendida sin cámara, poco lugar o la base no se pudo guardar (07-2.4, 05-1.2)
        TBoton { width: parent.an; height: parent.al; icono: "camara"; nombre: "Cámara y lugar"
                 readonly property bool sinCamara: !!Datos.ajustes.camara && !Camara.hay
                 readonly property bool poco: { Datos.ajustes; return Datos.pocoLugar(); }
                 tipo: sinCamara || poco || !Datos.guardaBien ? "peligro" : ""
                 dato: !Datos.guardaBien ? "No se puede guardar"
                       : (!Datos.ajustes.camara ? "Apagada" : Camara.hay ? "Lista" : "Sin cámara") + " · " + (poco ? "Poco lugar " : "") + Datos.lugarLibre()
                 onTocado: Navegacion.ir("camara") }
        // Simulador de carrera (Matías, 07/11/2026): verde OK · naranja revisar · rojo avisar al desarrollador
        TBoton { width: parent.an; height: parent.al; icono: "simulador"; nombre: "Simulador"
                 readonly property string e: Simulador.estado
                 tipo: e === "grave" ? "peligro" : e === "leve" ? "naranja" : ""
                 colorDato: e === "ok" ? Tema.bien : Tema.acento
                 dato: e === "corriendo" ? "Probando…" : e === "ok" ? "Simulador OK · " + Simulador.total + " pruebas"
                       : e === "leve" ? "Revisar · " + Simulador.fallas + (Simulador.fallas === 1 ? " aviso" : " avisos")
                       : e === "grave" ? "Hay errores: avisá" : "Tocá para probar"
                 onTocado: { if (e === "leve" || e === "grave") Navegacion.ir("simulador"); else if (e !== "corriendo") Simulador.correr(); } }
    }
    TBoton {
        x: 0; y: grilla.height + 18; width: raiz.area.width; height: 110
        tipo: raiz.todoListo ? "acento" : ""; icono: ""; tamNombre: 40
        nombre: raiz.todoListo ? "TODO LISTO" : "Falta el GPS"; activo: raiz.todoListo
        onTocado: Navegacion.ir("inicio")
    }
}
