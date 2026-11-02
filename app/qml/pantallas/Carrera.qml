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

    readonly property var armado: Armado.cargar(h, Datos.ajustes)

    function marcar(fuente) {
        const r = Crono.marcar(fuente);
        if (r === "largada" || r === "marca") tablero.efectoMarcar();
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
        // Cámara: punto rojo «REC» mientras graba; gris con «Cámara» si falló. En «a mano» se toca para grabar.
        Rectangle {
            visible: !!Datos.ajustes.camara && (Camara.grabando || !!Camara.falla || Datos.ajustes.camaraInicio === "mano")
            height: 32; radius: 16; width: tRec.implicitWidth + 40
            color: Qt.rgba(0, 0, 0, 0.6); border { width: 1.5; color: "#c9ced3" }
            Rectangle { x: 12; anchors.verticalCenter: parent.verticalCenter; width: 11; height: 11; radius: 6
                        color: Camara.grabando ? "#ff3b30" : "#8a929b" }
            Text { id: tRec; x: 30; anchors.verticalCenter: parent.verticalCenter; color: Tema.tinta
                   text: Camara.grabando ? "REC" : Camara.falla ? "Cámara" : "Grabar"
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 13; letterSpacing: 1 } }
            MouseArea { anchors.fill: parent; anchors.margins: -8; enabled: Datos.ajustes.camaraInicio === "mano"; onClicked: Crono.grabarAMano() }
        }
        Chip {
            punto: Crono.gps.ok && !est.sinGps ? Tema.bien : Tema.alerta
            texto: Crono.gps.ok ? "GPS " + Crono.gps.hz + " Hz" + (raiz.h ? " · " + Math.round(Crono.gps.precision) + " m" : "") : "Sin GPS"
        }
        Rectangle {
            id: terminar
            property bool armado: false
            height: 32; radius: 16
            width: tt.implicitWidth + 28
            color: armado ? Tema.acento : Qt.rgba(0, 0, 0, 0.55)
            border { width: 1.5; color: "#c9ced3" }
            Text { id: tt; anchors.centerIn: parent; color: terminar.armado ? Tema.acentoTinta : Tema.tinta
                   text: Crono.enCurso ? (terminar.armado ? "Tocá otra vez" : "Terminar") : "Salir"
                   font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 13 } }
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

    // Tablero de widgets (el mismo que usa el editor)
    Tablero {
        id: tablero
        anchors.fill: parent
        armado: raiz.armado
        est: raiz.est
        velActiva: Datos.ajustes.velActiva !== false
        onMarcar: Crono.marcar("pantalla")
    }
}
