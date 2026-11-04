import QtQuick
import Betometro

// Pantalla 6: Chequeo previo. Muestras #s6 y #v6. Se actualiza cada 2 s, como la app.
// Probar el pulsador: la próxima marca no larga nada, solo dice por dónde llegó.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var g: Crono.gps
    property int tic: 0
    property string pulsador: "sin probar"    // sin probar, esperando, funciona
    property string llegoPor: ""
    property string hora: ""
    readonly property real umbral: Datos.ajustes.umbralPrecisionM || 20
    readonly property bool gpsOk: !!g.ok && g.hz > 0 && g.precision <= umbral
    readonly property bool todoListo: gpsOk || !Datos.ajustes.velActiva
    focus: true
    Component.onCompleted: { forceActiveFocus(); hora = Qt.formatTime(new Date(), "HH:mm:ss"); }
    Timer { interval: 2000; running: true; repeat: true; onTriggered: { raiz.tic++; raiz.hora = Qt.formatTime(new Date(), "HH:mm:ss"); } }
    function recibido(por) {
        if (pulsador !== "esperando") return;
        pulsador = "funciona"; llegoPor = por;
    }
    Connections { target: Pulsador; ignoreUnknownSignals: true; function onMarca() { raiz.recibido("el pulsador"); } }
    Keys.onPressed: e => {
        if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter || e.key === Qt.Key_Space || e.key === Qt.Key_F9) { recibido("el teclado"); e.accepted = true; }
    }

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "CHEQUEO PREVIO"; derechaRayas: raiz.h ? 90 : 30; onVolver: Navegacion.ir("inicio") }

    component Estado: Row {
        property bool bien: true
        property string texto: ""
        spacing: 8
        Rectangle { width: 12; height: 12; radius: 6; color: parent.bien ? Tema.bien : Tema.alerta; anchors.verticalCenter: parent.verticalCenter }
        Text { text: parent.texto; color: parent.bien ? Tema.bien : Tema.alerta; font { family: Tema.fuente800; pixelSize: 20 } }
    }

    Component {
        id: gGps
        Tarjeta {
            height: cG.height + 10
            Column { id: cG; width: parent.width
                TituloTarjeta { texto: "GPS" }
                Fila { etiqueta: "Señal"
                    Estado { bien: raiz.gpsOk; texto: !Datos.ajustes.velActiva ? "Velocidad apagada" : !raiz.g.ok ? "Sin señal todavía" : raiz.gpsOk ? (raiz.g.satelites ? raiz.g.satelites + " satélites" : "Funciona") : "Esperá al aire libre" } }
                Fila { etiqueta: "Precisión"; Campo { activo: false; opacity: 1; texto: raiz.g.ok ? raiz.g.precision.toFixed(1).replace(".", ",") + " m" : "–" } }
                // Verde si está conectado y hablando (original o réplica: las dos andan). Rojo solo si hay una
                // falla: dejó de mandar datos (Matías, 04/11/2026: la réplica en rojo parecía una falla)
                Fila { id: filaModulo; etiqueta: "Módulo"; visible: !!raiz.g.modulo
                    readonly property bool habla: raiz.tic >= 0 && !!raiz.g.ultimoDato && Date.now() - raiz.g.ultimoDato < 5000
                    Estado { bien: filaModulo.habla
                             texto: !filaModulo.habla ? "Sin comunicación"
                                    : (raiz.g.original ? "" : "Réplica ") + (raiz.g.modulo || "") + (raiz.g.firmware ? " · " + raiz.g.firmware : "") } }
                Fila { etiqueta: "Lecturas por segundo"; Campo { activo: false; opacity: 1; texto: raiz.g.ok ? raiz.g.hz : "–" } }
                Fila { etiqueta: raiz.g.hora ? "Hora tomada del GPS" : "Hora del equipo"; ultima: true; Campo { activo: false; opacity: 1; texto: raiz.g.hora || raiz.hora } }
            }
        }
    }
    Component {
        id: gPulsador
        Tarjeta {
            height: cP.height + 10
            Column { id: cP; width: parent.width
                TituloTarjeta { texto: "Pulsador" }
                Fila { etiqueta: raiz.pulsador === "esperando" ? "Apretalo ahora…" : "Apretalo para probar"; ultima: true
                    Estado { visible: raiz.pulsador === "funciona"; texto: "Funciona"; bien: true }
                    BotonCromo { visible: raiz.pulsador !== "funciona"; width: 160; height: 46; tamTexto: 17
                                 texto: raiz.pulsador === "esperando" ? "Esperando…" : "Probar"; onTocado: { raiz.pulsador = "esperando"; raiz.forceActiveFocus(); } } }
                Sub { visible: raiz.pulsador === "funciona"; x: 22; bottomPadding: 12; tam: 12; text: "¡Recibido! Llegó por " + raiz.llegoPor }
            }
        }
    }
    // Alimentación y temperatura (Salud): la tensión baja es típica con la fuente del auto
    Component {
        id: gSalud
        Tarjeta {
            height: cS.height + 10
            Column { id: cS; width: parent.width
                TituloTarjeta { texto: "Alimentación y temperatura" }
                Fila { etiqueta: "Tensión"
                    Estado { bien: !Salud.tensionBaja && !Salud.huboTensionBaja
                             texto: Salud.tensionBaja ? "Baja ahora" : Salud.huboTensionBaja ? "Bajó desde que prendió" : "Bien" } }
                Fila { etiqueta: "Temperatura"; ultima: true
                    Estado { bien: !Salud.calor && !Salud.huboCalor
                             texto: (Salud.temperatura >= 0 ? Salud.temperatura + " °C" : "—") + (Salud.calor ? " · se frena" : Salud.huboCalor ? " · se frenó" : "") } }
            }
        }
    }
    Component {
        id: gEquipo
        Tarjeta {
            height: cE.height + 10
            Column { id: cE; width: parent.width
                TituloTarjeta { texto: "Voz y auriculares" }
                Fila { etiqueta: "Volumen"; Sub { text: Math.round(Datos.ajustes.volumen) + " %"; tam: 16 } }
                Fila { etiqueta: "Probar la voz"
                    BotonCromo { width: 150; height: 48; tamTexto: 16; texto: "▶ Probar"; onTocado: Audio.lista ? Audio.probar() : Navegacion.avisar("Faltan los audios de la voz") } }
                TituloTarjeta { texto: "Wifi" }
                Fila { etiqueta: "Conectado a"
                    Text { text: Wifi.conectada || "Sin conexión"; color: Tema.tinta; font { family: Tema.fuente800; pixelSize: 20 } } }
                TituloTarjeta { texto: "Cámara y micrófono" }
                Fila { etiqueta: "Cámara"; tocable: true; onTocada: Navegacion.ir("camara")
                    Text { color: !Datos.ajustes.camara ? Tema.tintaSuave : Camara.hay ? Tema.bien : Tema.alerta
                           text: (!Datos.ajustes.camara ? "Apagada" : Camara.hay ? "Lista · entran " + (Camara.lugar.horas || 0) + " h" : Camara.problema) + "  ›"
                           font { family: Tema.fuente800; pixelSize: 18 } } }
                TituloTarjeta { texto: "Almacenamiento" }
                Fila { etiqueta: "Lugar libre"; ultima: true; Campo { activo: false; opacity: 1; texto: Datos.lugarLibre() + (raiz.tic * 0 === 0 ? "" : "") } }
                Item { width: 1; height: 8 }
                BotonAmarillo {
                    x: 22; width: parent.width - 44; height: 64
                    texto: raiz.todoListo ? "TODO LISTO" : "FALTA EL GPS"
                    activo: raiz.todoListo
                    onTocado: Navegacion.ir("inicio")
                }
                Item { width: 1; height: 14 }
            }
        }
    }

    Flickable {
        id: lista
        x: raiz.h ? 40 : 24; y: 96
        width: parent.width - 2 * x; height: parent.height - y
        contentHeight: (raiz.h ? Math.max(izq.height, der.height) : unica.height) + 40
        clip: true; boundsBehavior: Flickable.StopAtBounds
        Column { id: izq; visible: raiz.h; width: raiz.h ? 590 : 0; spacing: 18; topPadding: 6
            Loader { width: parent.width; sourceComponent: raiz.h ? gGps : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gPulsador : null }
            Loader { width: parent.width; sourceComponent: raiz.h && Salud.sabe ? gSalud : null } }
        Column { id: der; visible: raiz.h; x: 620; width: raiz.h ? 580 : 0; spacing: 18; topPadding: 6
            Loader { width: parent.width; sourceComponent: raiz.h ? gEquipo : null } }
        Column { id: unica; visible: !raiz.h; width: lista.width; spacing: 18; topPadding: 6
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gGps }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gPulsador }
            Loader { width: parent.width; sourceComponent: !raiz.h && Salud.sabe ? gSalud : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gEquipo } }
    }
    // Tocar la pantalla mientras se espera también cuenta (como en la app)
    MouseArea { anchors.fill: parent; enabled: raiz.pulsador === "esperando"; z: -1; onPressed: raiz.recibido("la pantalla") }
}
