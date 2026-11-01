import QtQuick
import Betometro

// Pantalla 4: la carrera en curso (crono). Muestras #s4 y #v4.
// Los widgets se ubican en fracciones de la pantalla (6.6 de la app); acá están los armados de fábrica.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var est: Crono.estado
    readonly property var info: Crono.info
    focus: true

    // Armados de fábrica, medidos en las muestras aprobadas (px de 1280×720 y 720×1280)
    readonly property var fabricaH: [
        { t: "cuenta", x: 24, y: 56, w: 1232, h: 320, extra: true, barra: true },
        { t: "tipo", x: 50, y: 90, w: 210, h: 210 },
        { t: "necesaria", x: 1010, y: 96, w: 220, h: 200 },
        { t: "velocimetro", x: 24, y: 400, w: 330, h: 300, barra: true },
        { t: "ultima", x: 378, y: 400, w: 270, h: 300 },
        { t: "marcar", x: 672, y: 400, w: 584, h: 300 }
    ]
    readonly property var fabricaV: [
        { t: "velocimetro", forma: "redondo", x: 150, y: 72, w: 420, h: 420 },
        { t: "tipo", x: 20, y: 410, w: 190, h: 190 },
        { t: "necesaria", x: 500, y: 430, w: 200, h: 160 },
        { t: "cuenta", x: 20, y: 620, w: 680, h: 270, extra: true, barra: true },
        { t: "ultima", x: 20, y: 910, w: 680, h: 96 },
        { t: "marcar", x: 20, y: 1026, w: 680, h: 234 }
    ]
    readonly property var armado: h ? fabricaH : fabricaV

    function marcar(fuente) {
        const r = Crono.marcar(fuente);
        if (r === "largada" || r === "marca") boton.efecto();
    }
    function volver() {
        if (Crono.enCurso) { Navegacion.avisar("Para salir, terminá la carrera"); return; }
        const id = Crono.carreraId;
        Crono.salir();
        if (id) Navegacion.ir("ficha", { id: id }); else Navegacion.ir("practica");
    }

    Component.onCompleted: {
        if (!Crono.enCurso) Crono.preparar(Navegacion.parametros.practica ? "" : (Navegacion.parametros.id || ""));
        if (Contexto.prueba === "largar") Crono.marcar("pantalla");
        forceActiveFocus();
    }
    Keys.onPressed: e => {   // teclado USB: Enter, espacio o F9 (como la app)
        if (e.isAutoRepeat) return;
        if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter || e.key === Qt.Key_Space || e.key === Qt.Key_F9) { raiz.marcar("teclado"); e.accepted = true; }
    }
    Connections {
        target: Crono
        function onAviso(texto) { Navegacion.avisar(texto); }
        function onTerminada(motivo, vueltas, id) {
            Navegacion.avisar(motivo + ": " + vueltas + (vueltas === 1 ? " vuelta guardada" : " vueltas guardadas"));
            Crono.salir();
            Navegacion.ir("ficha", { id: id });
        }
    }

    // Fondo: del tema, o del color de la vuelta con la carrera en curso (indicador «fondo», fábrica)
    Fondo { anchors.fill: parent }
    Rectangle {
        anchors.fill: parent
        readonly property bool fondo: Datos.cfg("indicador") !== "marco"
        color: est.medida ? Datos.cfg("colMedida") : Datos.cfg("colPractica")
        opacity: Crono.enCurso && fondo ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
        Behavior on color { ColorAnimation { duration: 250 } }
    }
    Rectangle {   // indicador «marco»: borde interior de 7 px
        anchors.fill: parent; color: "transparent"
        border { width: 7; color: est.medida ? Datos.cfg("colMedida") : Datos.cfg("colPractica") }
        visible: Crono.enCurso && Datos.cfg("indicador") === "marco"
    }

    // Línea de arriba: carrera · ideal · distancia | GPS | Terminar / Salir
    Sub {
        x: raiz.h ? 24 : 20; y: raiz.h ? 22 : 20
        width: parent.width * (raiz.h ? 0.6 : 0.5); elide: Text.ElideRight
        tam: raiz.h ? 15 : 13
        text: (info.nombre || "") + " · " + (raiz.h ? "ideal " : "") + (info.ideal || "") + (info.distanciaM > 0 ? " · " + info.distancia + " m" : "")
    }
    Row {
        anchors { right: parent.right; rightMargin: raiz.h ? 24 : 20 }
        y: 10; spacing: 10; z: 5
        Chip {
            punto: Crono.gps.ok && !est.sinGps ? "#7fd36b" : "#ff8a7a"
            texto: Crono.gps.ok ? "GPS " + Crono.gps.hz + " Hz" + (raiz.h ? " · " + Math.round(Crono.gps.precision) + " m" : "") : "Sin GPS"
        }
        Rectangle {
            id: terminar
            property bool armado: false
            height: 32; radius: 16
            width: tt.implicitWidth + 28
            color: armado ? Tema.amarillo : Qt.rgba(0, 0, 0, 0.55)
            border { width: 1.5; color: "#c9ced3" }
            Text { id: tt; anchors.centerIn: parent; color: terminar.armado ? Tema.negro : Tema.blanco
                   text: Crono.enCurso ? (terminar.armado ? "Tocá otra vez" : "Terminar") : "Salir"
                   font { family: Tema.fuente600; italic: true; pixelSize: 13 } }
            MouseArea {
                anchors.fill: parent; anchors.margins: -10
                onClicked: {
                    if (!Crono.enCurso) { raiz.volver(); return; }
                    if (terminar.armado) { terminar.armado = false; Crono.terminar(); }
                    else { terminar.armado = true; reloj.restart(); }
                }
            }
            Timer { id: reloj; interval: 3000; onTriggered: terminar.armado = false }
        }
    }

    // Tablero: los más grandes primero, así los chicos quedan adelante
    Repeater {
        model: raiz.armado.slice().sort((a, b) => b.w * b.h - a.w * a.h)
        Loader {
            required property var modelData
            x: modelData.x; y: modelData.y; width: modelData.w; height: modelData.h
            sourceComponent: {
                switch (modelData.t) {
                case "cuenta": return cCuenta;
                case "tipo": return cTipo;
                case "necesaria": return cNecesaria;
                case "velocimetro": return modelData.forma === "redondo" ? cVelocimetro : cVelocidad;
                case "ultima": return cUltima;
                case "marcar": return cMarcar;
                case "faltan": return cFaltan;
                case "transcurrido": return cTranscurrido;
                case "ideal": return cIdeal;
                case "proxima": return cProxima;
                }
                return null;
            }
            property var def: modelData
        }
    }

    Component { id: cCuenta; WCuenta { est: raiz.est; extra: parent.def.extra !== false; barra: parent.def.barra !== false } }
    Component { id: cTipo; WTipo { est: raiz.est } }
    Component {
        id: cNecesaria
        WNumero {
            etiqueta: "Sugerida"
            numero: raiz.est.vNecKmh === undefined || raiz.est.vNecKmh === null ? "–" : Math.round(raiz.est.vNecKmh) + (raiz.est.congelada ? "■" : "")
            extra: parent.def.extra ? "base " + Math.round(raiz.est.vBaseKmh || 0) : ""
        }
    }
    Component { id: cVelocidad; WVelocidad { est: raiz.est; barra: parent.def.barra !== false; extra: !!parent.def.extra } }
    Component { id: cVelocimetro; WVelocimetro { est: raiz.est } }
    Component {
        id: cUltima
        WNumero {
            etiqueta: fila ? "Última vuelta" + (Crono.ultima.n ? " · V" + Crono.ultima.n : "")
                           : "Última vuelta" + (Crono.ultima.n ? "\nV" + Crono.ultima.n : "")
            numero: Crono.ultima.realMs ? Formato.reloj(Crono.ultima.realMs, false) : "–"
            extra: parent.def.extra && Crono.ultima.realMs ? Formato.dif(Crono.ultima.difMs) + " del ideal" : ""
        }
    }
    Component { id: cMarcar; WMarcar { id: wm; est: raiz.est; onMarcar: Crono.marcar("pantalla"); Component.onCompleted: boton.w = wm } }
    Component { id: cFaltan; WNumero { etiqueta: "Faltan"; numero: Formato.miles(Math.max(0, raiz.est.restanteM || 0)) + " m"; extra: "de " + Formato.miles(raiz.est.distanciaM || 0) + " m" } }
    Component { id: cTranscurrido; WNumero { etiqueta: "Transcurrido"; numero: Formato.reloj(raiz.est.transMs || 0, false) } }
    Component { id: cIdeal; WNumero { etiqueta: "Ideal"; numero: Formato.reloj(raiz.est.tiempoIdealMs || 0, false) } }
    Component { id: cProxima; WNumero { etiqueta: "Próxima vuelta"; numero: raiz.est.proxMedida ? "MEDIDA" : "PRÁCTICA" } }
    QtObject { id: boton; property var w: null; function efecto() { if (w) w.efecto(); } }
}
