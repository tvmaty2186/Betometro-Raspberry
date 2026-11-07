import QtQuick
import Betometro

// Pantalla 4: la carrera en curso (crono). Muestras #s4 y #v4.
// Los widgets se ubican en fracciones de la pantalla (6.6 de la app); acá están los armados de fábrica.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var est: Crono.estado
    readonly property var info: Crono.info
    readonly property bool recorrido: !!info.recorrido   // recorrido libre: sin vueltas (TableroRecorrido)
    focus: true

    readonly property var armado: Armado.cargar(h, Datos.ajustes)

    function marcar(fuente, edadMs) {
        const r = Crono.marcar(fuente, edadMs || 0);
        if (r === "largada" || r === "marca") (raiz.recorrido ? tableroRec : tablero).efectoMarcar();
    }
    // Pulsador por cable: la marca vale cuando se apoyó el dedo (edadMs atrás)
    Connections { target: Pulsador; ignoreUnknownSignals: true; function onMarca(edadMs) { raiz.marcar("pulsador", edadMs); } }
    function volver() {
        if (Crono.enCurso) { Navegacion.avisar("Para salir, terminá la carrera"); return; }
        const id = Crono.carreraId();   // es una función: sin () daba la función misma y abría «nueva carrera»
        Crono.salir();
        if (id) Navegacion.ir("ficha", { id: id }); else Navegacion.ir("practica");
    }

    Component.onCompleted: {
        const pruebaRec = Contexto.prueba.indexOf("recorrido") === 0;   // solo para capturas: recorrido, recorrido-largo
        if (!Crono.enCurso) Crono.preparar(Navegacion.parametros.recorrido || pruebaRec ? "recorrido" : Navegacion.parametros.practica ? "" : (Navegacion.parametros.id || ""));
        if (pruebaRec) { Crono.marcar("pantalla"); tPruebaRec.start(); }
        if (Contexto.prueba === "largar") Crono.marcar("pantalla");
        if (Contexto.prueba === "salir") Qt.callLater(raiz.volver);   // solo para capturas: Salir sin largar
        if (Contexto.prueba === "largar-60")   // solo para capturas: 59 vueltas cerradas, va por la 60
            for (let i = 60; i > 0; i--) Crono.marcar("pantalla", i * 4000);
        forceActiveFocus();
    }
    Timer {   // solo para capturas: marcas cada 6 s; «recorrido-largo» termina a los 40 s
        id: tPruebaRec; interval: 6000; repeat: true
        property int veces: 0
        onTriggered: {
            veces++;
            if (Contexto.prueba === "recorrido-largo" && veces >= 7) { stop(); Crono.terminar(); return; }
            raiz.marcar("pantalla");
        }
    }
    Keys.onPressed: e => {   // teclado USB: Enter, espacio o F9 (como la app)
        if (e.isAutoRepeat) return;
        if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter || e.key === Qt.Key_Space || e.key === Qt.Key_F9) { raiz.marcar("teclado"); e.accepted = true; }
    }
    // Tarjeta llena o dañada: el aviso chico de siempre (sin carteles grandes en carrera) (05-1.2)
    Connections {
        target: Datos
        function onGuardaBienChanged() { if (!Datos.guardaBien) Navegacion.avisar("No se pudo guardar: revisá la tarjeta"); }
    }
    Connections {
        target: Crono
        function onAviso(texto) { Navegacion.avisar(texto); }
        function onTerminada(motivo, vueltas, id) {
            Navegacion.avisar(raiz.recorrido ? "Recorrido guardado" : motivo + ": " + vueltas + (vueltas === 1 ? " vuelta guardada" : " vueltas guardadas"));
            Crono.salir();
            Navegacion.ir("ficha", { id: id });
        }
    }

    // Fondo: del tema, o del color de la vuelta con la carrera en curso (indicador «fondo», fábrica)
    Fondo { anchors.fill: parent }
    Rectangle {
        anchors.fill: parent
        readonly property bool fondo: Datos.cfg("indicador") !== "marco"
        color: est.medida && !raiz.recorrido ? Datos.cfg("colMedida") : Datos.cfg("colPractica")
        opacity: Crono.enCurso && fondo ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
        Behavior on color { ColorAnimation { duration: 250 } }
    }
    Rectangle {   // indicador «marco»: borde interior de 7 px
        anchors.fill: parent; color: "transparent"
        border { width: Tema.sol ? 11 : 7; color: est.medida && !raiz.recorrido ? Datos.cfg("colMedida") : Datos.cfg("colPractica") }   // en sol 11 px, como la app
        visible: Crono.enCurso && Datos.cfg("indicador") === "marco"
    }

    // Línea de arriba: carrera · ideal · distancia | GPS | Terminar / Salir
    Sub {
        // Modo Tablet: la franja de arriba más grande (los widgets no cambian)
        x: raiz.h ? 24 : 20; y: Tema.grandes ? 12 : (raiz.h ? 22 : 20)
        width: Math.max(80, arriba.x - x - 14); elide: Text.ElideRight   // termina antes de REC / GPS / Terminar
        tam: Tema.grandes ? (raiz.h ? 19 : 16) : (raiz.h ? 15 : 13)
        text: raiz.recorrido ? "RECORRIDO LIBRE" + (est.enCurso ? "" : " · tocá LARGAR") : (info.nombre || "") + " · " + (raiz.h ? "ideal " : "") + (info.ideal || "") + (info.distanciaM > 0 ? " · " + info.distancia + " m" : "")
    }
    Row {
        id: arriba
        anchors { right: parent.right; rightMargin: raiz.h ? 24 : 20 }
        y: Tema.grandes ? 3 : 6; spacing: 10; z: 5
        // REC, GPS y Terminar con el mismo alto, borde y letra, alineados (Matías, 07/11/2026)
        readonly property real alto: Tema.grandes ? 50 : 42   // como Terminar: cuesta tocarlo en movimiento (Matías, 03/11)
        readonly property int letra: Tema.grandes ? 23 : 17
        component Pastilla: Rectangle {
            id: pas
            property string texto: ""
            property color punto: "transparent"
            property bool conPunto: true
            property alias puntoVisible: bolita.visible
            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            height: arriba.alto; radius: height / 2
            width: filaP.implicitWidth + arriba.letra * 1.6
            color: Tema.sol ? "#ffffff" : Qt.rgba(0, 0, 0, 0.55)
            border { width: 2; color: Tema.bordeCromo }
            Row {
                id: filaP; anchors.centerIn: parent; spacing: arriba.letra * 0.4
                Item {   // lugar fijo del punto: no se corre el texto cuando parpadea
                    visible: pas.conPunto
                    width: arriba.letra * 0.7; height: width; anchors.verticalCenter: parent.verticalCenter
                    Rectangle { id: bolita; anchors.fill: parent; radius: width / 2; color: pas.punto }
                }
                Text { text: pas.texto; color: Tema.tinta; anchors.verticalCenter: parent.verticalCenter
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: arriba.letra; letterSpacing: 0.5 } }
            }
        }
        // Cámara: punto rojo «REC» que parpadea mientras graba (cada 500 ms); gris con «Cámara» si falló.
        // En «a mano» se toca para grabar.
        Pastilla {
            id: rec
            visible: !!Datos.ajustes.camara && (Camara.grabando || !!Camara.falla || Datos.ajustes.camaraInicio === "mano")
            texto: Camara.grabando ? "REC" : Camara.falla ? "Cámara" : "Grabar"
            punto: Camara.grabando ? "#ff3b30" : "#8a929b"
            property bool prendido: true
            puntoVisible: !Camara.grabando || prendido
            Timer { interval: 500; repeat: true; running: Camara.grabando && rec.visible
                    onTriggered: rec.prendido = !rec.prendido; onRunningChanged: rec.prendido = true }
            MouseArea { anchors.fill: parent; anchors.margins: -8; enabled: Datos.ajustes.camaraInicio === "mano"; onClicked: Crono.grabarAMano() }
        }
        Pastilla {   // tensión baja o calor (Salud): solo antes de largar; en carrera no distrae (Matías, 04/11)
            visible: (Salud.tensionBaja || Salud.calor) && !Crono.enCurso
            punto: Tema.alerta
            texto: Salud.tensionBaja ? "⚡ Tensión baja" : "Calor " + Salud.temperatura + " °C"
        }
        Pastilla {
            punto: Crono.gps.ok && !est.sinGps ? Tema.bien : Tema.alerta
            texto: Crono.gps.ok ? "GPS " + Crono.gps.hz + " Hz" + (raiz.h ? " · " + Math.round(Crono.gps.precision) + " m" : "") : "Sin GPS"
        }
        Rectangle {
            id: terminar
            property bool armado: false
            height: arriba.alto; radius: height / 2
            width: tt.implicitWidth + arriba.letra * 1.6
            color: armado ? Tema.acento : Tema.sol ? "#ffffff" : Qt.rgba(0, 0, 0, 0.55)
            border { width: 2; color: Tema.bordeCromo }
            Text { id: tt; anchors.centerIn: parent; color: terminar.armado ? Tema.acentoTinta : Tema.tinta
                   text: Crono.enCurso ? (terminar.armado ? "Tocá otra vez" : "Terminar") : "Salir"
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: arriba.letra; letterSpacing: 0.5 } }
            MouseArea {
                anchors.fill: parent; anchors.margins: -14
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
        visible: !raiz.recorrido
        anchors.fill: parent
        armado: raiz.armado
        est: raiz.est
        velActiva: Datos.ajustes.velActiva !== false
        onMarcar: Crono.marcar("pantalla")
    }
    TableroRecorrido {
        id: tableroRec
        visible: raiz.recorrido
        anchors.fill: parent
        est: raiz.est
        onMarcar: raiz.marcar("pantalla")
    }
}
