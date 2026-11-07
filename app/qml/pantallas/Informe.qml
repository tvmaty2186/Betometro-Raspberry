import QtQuick
import Betometro

// Pantalla 10: Informe de una carrera. Muestras #s10 y #v10.
// Los datos salen del mismo cálculo que la app (datosInforme): recorrido común, velocidad, aceleración y adelanto.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property string id_: Navegacion.parametros.id || ""
    property int rev: 0              // sube al cambiar una vuelta a medida/libre: se vuelve a leer
    readonly property var c: { rev; return Datos.carrera(id_); }
    readonly property var inf: { rev; return Datos.informe(id_); }
    readonly property var vueltas: inf.vueltas || []
    readonly property bool rec: !!inf.recorrido          // recorrido libre: resumen y marcas en vez de las vueltas
    readonly property var marcas: inf.marcas || []
    function reloj(ms) {
        const s = Math.max(0, Math.round(ms / 1000));
        return Math.floor(s / 3600) + ":" + String(Math.floor(s / 60) % 60).padStart(2, "0") + ":" + String(s % 60).padStart(2, "0");
    }
    function coma(x, d) { return (x === undefined || x === null) ? "–" : Number(x).toFixed(d).replace(".", ","); }
    function irMarca(mk) {   // con video: a ese momento; si no estaba a la vista, se abre
        if (!videos.length) return;
        if (!verVideo) verVideo = true;
        Qt.callLater(() => raiz.irVideo(mk.vuelta, mk.ms / 1000));
    }
    // Video de la carrera (E7): botón Informe / Video; a medias, y doble toque → pantalla completa
    readonly property var videos: { rev; return Datos.videos(id_); }
    property bool verVideo: false
    property bool completo: false
    readonly property bool vv: verVideo && videos.length > 0
    onVvChanged: {
        if (!vv) completo = false;
        else if (reproductor.pos === 0 && reproductor.pendiente < 0) irVideo(lap, 0);
    }
    function irVideo(i, t) { if (vv && i >= 0) reproductor.irA(i, t); }
    // Modo tele (docs/video.md): con una tele en el HDMI, el video va a la tele y esta pantalla es el control.
    // Si la tele se enchufó con Betómetro abierto, Qt no la ve: se ofrece reiniciar (nunca en carrera).
    readonly property bool enTele: Tele.reproductor === reproductor && !!reproductor
    readonly property bool botonTele: videos.length > 0 && (Tele.hay || (Tele.hayQueReiniciar && !Crono.enCurso))
    property bool pedirReinicio: false
    function verEnTele() {
        if (!Tele.hay) { pedirReinicio = true; return; }
        verVideo = true;
        completo = false;
        Tele.empezar(reproductor, c.nombre || "", id_);
    }
    Component.onDestruction: if (enTele) Tele.terminar()
    // «Carreras» en modo tele (Tele.elegirCarrera): cambia la carrera de este informe sin salir de la tele.
    // Se espera a que todo lo que depende de la carrera se vuelva a leer.
    onId_Changed: if (enTele) Qt.callLater(cambiarCarreraTele)
    function cambiarCarreraTele() {
        if (!enTele) return;
        if (!videos.length) { Tele.terminar(); return; }
        fila = -1; confirmarUnir = false; menuAbierto = false; confirmarRest = false; completo = false;
        Tele.carrera = c.nombre || "";
        Tele.carreraId = id_;
        lap = conTraza.length ? conTraza[0] : -1;
        mapa.requestPaint(); grafico.requestPaint();
        reproductor.deNuevo();
        // A la primera vuelta que tiene video
        for (let i = 0; i < vueltas.length; i++) if (reproductor.irA(i, 0)) return;
        reproductor.irMs(0, 0);
    }
    // Momento del video en que empieza cada vuelta (para la tabla)
    function inicioVideo(i) { const m = reproductor.momento(i, 0); return m ? reproductor.reloj(m.ms) : ""; }
    readonly property var conTraza: vueltas.map((v, i) => v.m ? i : -1).filter(i => i >= 0)
    property int lap: conTraza.length ? conTraza[0] : -1
    property string modo: "vel"      // vel, acel, ade
    property int fila: -1            // vuelta elegida para «Unir» (manteniéndola apretada 3 s, como la app 2.4.1)
    property bool confirmarUnir: false
    property bool menuAbierto: false
    property bool confirmarRest: false
    onFilaChanged: confirmarUnir = false
    Timer { id: tUnirVuelve; interval: 3000; onTriggered: raiz.confirmarUnir = false }
    Timer { id: tRestVuelve; interval: 3000; onTriggered: raiz.confirmarRest = false }
    function restablecer() {
        if (!confirmarRest) { confirmarRest = true; tRestVuelve.restart(); return; }
        confirmarRest = false; menuAbierto = false;
        if (Datos.restablecer(id_)) { rev++; fila = -1; lap = conTraza.length ? conTraza[0] : -1; Navegacion.avisar("Carrera restablecida"); }
    }
    readonly property var vu: lap >= 0 ? vueltas[lap] : null
    readonly property real dist: inf.distanciaM || 0
    readonly property real sug: dist > 0 ? dist / (inf.idealMs / 1000) * 3.6 : 0

    function nombreV(v) { return "V" + v.n + (inf.largadas > 1 ? " L" + v.largada : ""); }
    function colorPunto(p) {
        if (!p[7]) return "#555555";
        if (modo === "acel") { const a = p[3]; return a > 0.8 ? Tema.bien : a > 0.25 ? "#8fe3b0" : a < -0.8 ? "#d23b2c" : a < -0.25 ? "#f08a96" : "#9aa3ad"; }
        if (modo === "ade") { const s = Math.abs(p[4]); return s < 1 ? Tema.bien : s < 3 ? Tema.acento : "#d23b2c"; }
        if (!(sug > 0)) return Tema.bien;
        const d = p[2] - sug;
        return d > 6 ? "#d23b2c" : d > 2 ? Tema.acento : d >= -2 ? Tema.bien : d >= -6 ? "#7fd3ff" : "#3a8de0";
    }
    function cambiarMedida(i) {
        const l = lap;
        if (Datos.cambiarMedida(id_, i)) { rev++; lap = l; }
    }
    // Una marca de más parte una vuelta en dos: se juntan desde acá
    function puedeUnir(i) {
        if (i < 0 || i + 1 >= vueltas.length) return false;
        const a = vueltas[i], b = vueltas[i + 1];
        return a.largada === b.largada && !a.interrumpida && !b.interrumpida;
    }
    function unir() {
        if (!confirmarUnir) { confirmarUnir = true; tUnirVuelve.restart(); return; }
        const i = fila;
        if (Datos.unirConSiguiente(id_, i)) {
            rev++; fila = -1;
            lap = vueltas[i] && vueltas[i].m ? i : (conTraza.length ? conTraza[0] : -1);
            Navegacion.avisar("Vueltas unidas");
        }
        confirmarUnir = false;
    }
    function colorDif(ms) { const s = Math.abs(ms / 1000); return s <= 0.2 ? Tema.bien : s <= 0.5 ? Tema.acento : Tema.alerta; }
    onLapChanged: { mapa.requestPaint(); grafico.requestPaint(); }
    // Con el video andando, la vuelta elegida sigue al video
    Connections {
        target: reproductor
        function onLapAhoraChanged() { const i = reproductor.lapAhora; if (i >= 0 && raiz.vueltas[i] && raiz.vueltas[i].m && i !== raiz.lap) raiz.lap = i; }
    }
    onModoChanged: mapa.requestPaint()

    Fondo { anchors.fill: parent }

    // Cabecera: volver, león, nombre y datos; 101 y Compartir
    Volver { x: raiz.h ? 24 : 20; y: raiz.h ? 30 : 26; onTocado: Navegacion.ir("ficha", { id: raiz.id_ }) }
    Row {
        x: raiz.h ? 80 : 24; y: raiz.h ? 14 : 96; spacing: 14
        Image { source: "qrc:/recursos/img/leon.png"; height: raiz.h ? 46 : 50; fillMode: Image.PreserveAspectFit; anchors.verticalCenter: parent.verticalCenter; sourceSize.height: 100; smooth: true }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            TextoCromo { texto: (raiz.c.nombre || "").toUpperCase(); tam: raiz.h ? 36 : 38 }
            Sub { text: raiz.rec ? raiz.c.fechaCorta + " · recorrido libre" + (raiz.marcas.length ? " · " + raiz.marcas.length + (raiz.marcas.length === 1 ? " marca" : " marcas") : "") + (raiz.vueltas.some(v => v.interrumpida) ? " · se cortó" : "")
                  : raiz.c.fechaCorta + " · ideal " + raiz.c.ideal + (raiz.dist > 0 ? " · " + raiz.c.distancia + " m" : "") + " · " + raiz.vueltas.length + " vueltas" }
        }
    }
    // Lo que el navegante quiere saber apenas para: el error total, grande y arriba (07-2.1)
    Column {
        id: errorArriba
        readonly property var med: raiz.vueltas.filter(v => v.medida && v.difMs !== undefined)
        visible: !raiz.rec && med.length > 0
        readonly property real promS: med.length ? med.reduce((a, v) => a + Math.abs(v.difMs), 0) / med.length / 1000 : 0
        anchors { right: raiz.h ? derecha.left : undefined; rightMargin: 28 }
        x: raiz.h ? 0 : 84
        y: raiz.h ? 14 : 12
        spacing: 0
        Sub { text: "Error total · " + errorArriba.med.length + (errorArriba.med.length === 1 ? " medida" : " medidas"); tam: 13
              anchors.right: raiz.h ? parent.right : undefined }
        Text { anchors.right: raiz.h ? parent.right : undefined
               text: raiz.c.errorTotal || ""
               color: errorArriba.promS <= 0.2 ? Tema.bien : errorArriba.promS <= 0.5 ? Tema.acento : Tema.mal
               font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 46 } }
        Sub { visible: raiz.h; text: "promedio " + (raiz.c.promedio || "–") + " · mejor " + (raiz.c.mejor || "–"); tam: 12
              anchors.right: parent.right }
    }
    Row {
        id: derecha
        anchors { right: parent.right; rightMargin: raiz.h ? 24 : 20 }
        y: raiz.h ? 20 : 20; spacing: 16
        // 📺 Ver en la tele: con la tele enchufada tiene que notarse (Matías, 06/11/2026). Botón del tema (cromo,
        // como Compartir pero oscuro), ícono de línea del tema en el color de acento y el borde que «late» suave.
        Item {
            id: botonTele
            visible: raiz.botonTele
            anchors.verticalCenter: parent.verticalCenter
            height: raiz.h ? 56 : 54; width: filaTele.implicitWidth + 40
            readonly property real radio: Math.max(6, Tema.radio)
            Rectangle {   // el aura que late
                anchors.fill: parent; anchors.margins: -5; radius: botonTele.radio + 5
                color: "transparent"; border { width: 3; color: Tema.acento }
                SequentialAnimation on opacity {
                    running: botonTele.visible; loops: Animation.Infinite
                    NumberAnimation { from: 0.15; to: 0.9; duration: 900; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 0.9; to: 0.15; duration: 900; easing.type: Easing.InOutSine }
                }
            }
            Rectangle { anchors.fill: parent; anchors.margins: -2; radius: botonTele.radio + 2; color: Tema.bordeCromo }
            Rectangle {
                anchors.fill: parent; radius: botonTele.radio
                gradient: Gradient {
                    GradientStop { position: 0; color: zonaTele.pressed ? Qt.lighter(Tema.caja1, 1.4) : Qt.lighter(Tema.caja1, 1.15) }
                    GradientStop { position: 1; color: Tema.caja2 }
                }
            }
            Row {
                id: filaTele; anchors.centerIn: parent; spacing: 12
                TIcono { width: 36; height: 36; nombre: "tele"; color: Tema.acento; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "Ver en la tele"; color: Tema.tinta; anchors.verticalCenter: parent.verticalCenter
                       font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: raiz.h ? 22 : 20 } }
            }
            MouseArea { id: zonaTele; anchors.fill: parent; anchors.margins: -6; onClicked: raiz.verEnTele() }
        }
        Numero101 { visible: !raiz.videos.length; d: raiz.h ? 62 : 58; numero: Datos.ajustes.numeroAuto || "101"; anchors.verticalCenter: parent.verticalCenter }
        Segmentado {
            visible: raiz.videos.length > 0
            anchors.verticalCenter: parent.verticalCenter
            height: raiz.h ? 44 : 42
            opciones: [{ texto: "Informe", valor: false }, { texto: "Video", valor: true }]
            valor: raiz.verVideo
            onElegido: (v) => raiz.verVideo = v
        }
        BotonAmarillo { width: raiz.h ? 190 : 170; height: raiz.h ? 56 : 54; texto: "Compartir"; anchors.verticalCenter: parent.verticalCenter
                        onTocado: Navegacion.ir("compartir", { id: raiz.id_ }) }
    }

    // Mapa
    Tarjeta {
        id: cajaMapa
        oscura: true
        x: !raiz.vv ? (raiz.h ? 24 : 20) : (raiz.h ? 648 : 20)
        y: !raiz.vv ? (raiz.h ? 110 : 180) : (raiz.h ? 110 : 600)
        width: !raiz.vv ? (raiz.h ? 600 : parent.width - 40) : (raiz.h ? 608 : 330)
        height: !raiz.vv ? (raiz.h ? 590 : 470) : (raiz.h ? 230 : 280)
        Sub { x: 18; y: 16; visible: !raiz.vv || raiz.h; text: raiz.vv ? "Dónde está · tocá y el video va ahí" : raiz.modo === "vel" ? (raiz.rec ? "Recorrido · color = velocidad" : "Recorrido de la vuelta · color = velocidad") : raiz.modo === "acel" ? "Recorrido · color = aceleración" : "Recorrido · color = adelanto"; tam: 12 }
        Row {
            anchors { right: parent.right; rightMargin: 14; top: parent.top; topMargin: 10 }
            spacing: 6
            Repeater {
                model: [{ t: "Vel.", m: "vel" }, { t: "Acel.", m: "acel" }].concat(raiz.dist > 0 ? [{ t: "Adel.", m: "ade" }] : [])
                Rectangle {
                    required property var modelData
                    width: tm.implicitWidth + 18; height: 26; radius: 13
                    color: raiz.modo === modelData.m ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
                    border { width: 1; color: Tema.bordeCromo }
                    Text { id: tm; anchors.centerIn: parent; text: modelData.t; color: raiz.modo === modelData.m ? Tema.acentoTinta : Tema.tinta
                           font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 16 } }
                    MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: raiz.modo = modelData.m }
                }
            }
        }
        Canvas {
            id: mapa
            anchors { fill: parent; topMargin: 44; margins: 14 }
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            readonly property real k: Math.min(width / 364, height / 330)
            readonly property real ox: (width - 364 * k) / 2
            readonly property real oy: (height - 330 * k) / 2
            // Con video: el punto amarillo es donde va el auto, y tocar el recorrido lleva el video ahí
            Rectangle {
                visible: raiz.vv && reproductor.e.lap >= 0
                width: 22; height: 22; radius: 11
                x: mapa.ox + reproductor.e.x * mapa.k - 11; y: mapa.oy + reproductor.e.y * mapa.k - 11
                color: Tema.amarillo; border { width: 3; color: "#111111" }
            }
            MouseArea {
                anchors.fill: parent
                enabled: raiz.vv
                onClicked: (m) => {
                    const M = raiz.vu && raiz.vu.m ? raiz.vu.m : [];
                    let mejor = null, dm = 1e9;
                    for (const p of M) {
                        if (!p[7]) continue;
                        const dx = mapa.ox + p[5] * mapa.k - m.x, dy = mapa.oy + p[6] * mapa.k - m.y, dd = dx * dx + dy * dy;
                        if (dd < dm) { dm = dd; mejor = p; }
                    }
                    if (mejor && dm < 40 * 40) raiz.irVideo(raiz.lap, mejor[0]);
                }
            }
            onPaint: {
                const g = getContext("2d");
                g.reset();
                const P = raiz.inf.pista || [];
                if (!P.length) {
                    g.fillStyle = Tema.tintaSuave; g.font = "italic 18px '" + Tema.fuente600 + "'"; g.textAlign = "center";
                    g.fillText("Esta carrera no tiene el recorrido grabado.", width / 2, height / 2);
                    return;
                }
                const k = Math.min(width / 364, height / 330), ox = (width - 364 * k) / 2, oy = (height - 330 * k) / 2;
                const X = p => ox + p[0] * k, Y = p => oy + p[1] * k;
                g.lineJoin = "round"; g.lineCap = "round";
                g.beginPath(); g.moveTo(X(P[0]), Y(P[0]));
                for (const p of P) g.lineTo(X(p), Y(p));
                if (raiz.inf.cerrada) g.closePath();
                g.strokeStyle = "rgba(0,0,0,0.45)"; g.lineWidth = 20 * k; g.stroke();
                g.strokeStyle = "#3a3f45"; g.lineWidth = 11 * k; g.stroke();
                const M = raiz.vu && raiz.vu.m ? raiz.vu.m : [];
                g.lineWidth = 8 * k;
                for (let i = 1; i < M.length; i++) {
                    if (!M[i][7] || !M[i - 1][7]) continue;
                    g.beginPath(); g.moveTo(ox + M[i - 1][5] * k, oy + M[i - 1][6] * k); g.lineTo(ox + M[i][5] * k, oy + M[i][6] * k);
                    g.strokeStyle = raiz.colorPunto(M[i]); g.stroke();
                }
                g.beginPath(); g.arc(X(P[0]), Y(P[0]), 9 * k, 0, Math.PI * 2); g.fillStyle = "#ffffff"; g.fill();
                const izq = X(P[0]) > width * 0.7;   // cerca del borde derecho: el texto va a la izquierda
                g.fillStyle = Tema.tinta; g.font = "italic " + Math.round(11 * k) + "px '" + Tema.fuente800 + "'"; g.textAlign = izq ? "right" : "left";
                g.fillText("LARGADA", X(P[0]) + (izq ? -14 : 14) * k, Y(P[0]) + 4 * k);
                if (raiz.rec) {   // recorrido libre: la llegada (cuadradito) y las marcas numeradas
                    const F = P[P.length - 1], s = 8 * k;
                    if (!raiz.inf.cerrada) {
                        g.fillStyle = "#111111"; g.fillRect(X(F) - s, Y(F) - s, 2 * s, 2 * s);
                        g.strokeStyle = "#ffffff"; g.lineWidth = 2.5 * k; g.strokeRect(X(F) - s, Y(F) - s, 2 * s, 2 * s);
                        const izqF = X(F) > width * 0.7;
                        g.fillStyle = Tema.tinta; g.textAlign = izqF ? "right" : "left";
                        g.fillText("LLEGADA", X(F) + (izqF ? -14 : 14) * k, Y(F) + 4 * k);
                    }
                    g.textAlign = "center";
                    g.font = "italic " + Math.round(10 * k) + "px '" + Tema.fuente800 + "'";
                    for (const mk of raiz.marcas) {
                        if (mk.x === undefined) continue;
                        g.beginPath(); g.arc(ox + mk.x * k, oy + mk.y * k, 9 * k, 0, Math.PI * 2);
                        g.fillStyle = Tema.amarillo; g.fill(); g.strokeStyle = "#111111"; g.lineWidth = 2 * k; g.stroke();
                        g.fillStyle = "#111111"; g.fillText(String(mk.n), ox + mk.x * k, oy + mk.y * k + 3.5 * k);
                    }
                }
            }
        }
    }

    // Velocidad de la vuelta elegida, a lo largo de la distancia
    Tarjeta {
        id: cajaVel
        oscura: true
        visible: !raiz.vv
        x: raiz.h ? 648 : 20; y: raiz.h ? 110 : 670
        width: raiz.h ? 608 : parent.width - 40; height: raiz.h ? 230 : 210
        Sub { x: 18; y: 16; text: raiz.rec ? "Velocidad km/h · a lo largo del recorrido" : "Velocidad km/h" + (raiz.vu ? " · " + raiz.nombreV(raiz.vu) : "") + (raiz.sug > 0 ? " · sugerida " + raiz.sug.toFixed(1).replace(".", ",") : ""); tam: 12 }
        Canvas {
            id: grafico
            anchors { fill: parent; topMargin: 40; margins: 16 }
            onWidthChanged: requestPaint()
            onPaint: {
                const g = getContext("2d");
                g.reset();
                const M = raiz.vu && raiz.vu.m ? raiz.vu.m : [];
                if (!M.length) return;
                const L = raiz.vu.L || 1;
                let vmax = raiz.sug + 10;
                for (const p of M) if (p[7]) vmax = Math.max(vmax, p[2]);
                const esc = Math.max(20, Math.ceil(vmax / 10) * 10);
                const X = d => 30 + (width - 34) * Math.min(1, d / L), Y = v => height - 6 - (height - 12) * v / esc;
                g.strokeStyle = "rgba(255,255,255,0.12)"; g.lineWidth = 1;
                g.fillStyle = Tema.tintaSuave; g.font = "11px '" + Tema.fuente + "'"; g.textAlign = "right";
                for (let i = 0; i <= 4; i++) { const v = esc * i / 4; g.beginPath(); g.moveTo(30, Y(v)); g.lineTo(width - 4, Y(v)); g.stroke(); g.fillText(String(Math.round(v)), 24, Y(v) + 4); }
                if (raiz.sug > 0) {
                    g.strokeStyle = Tema.acento; g.lineWidth = 2; g.setLineDash([8, 6]);
                    g.beginPath(); g.moveTo(30, Y(raiz.sug)); g.lineTo(width - 4, Y(raiz.sug)); g.stroke(); g.setLineDash([]);
                }
                if (raiz.rec) {   // las marcas, como rayas amarillas con su número
                    g.strokeStyle = Tema.acento; g.lineWidth = 1.5; g.setLineDash([4, 4]);
                    g.fillStyle = Tema.acento; g.textAlign = "left"; g.font = "bold 12px '" + Tema.fuente800 + "'";
                    for (const mk of raiz.marcas) {
                        const x = X(mk.m || 0);
                        g.beginPath(); g.moveTo(x, 4); g.lineTo(x, height - 6); g.stroke();
                        g.fillText(String(mk.n), x + 4, 14);
                    }
                    g.setLineDash([]);
                }
                g.strokeStyle = Tema.numero; g.lineWidth = 3.5; g.lineJoin = "round";
                let abierto = false;
                g.beginPath();
                for (const p of M) {
                    if (!p[7]) { abierto = false; continue; }
                    if (!abierto) { g.moveTo(X(p[1]), Y(p[2])); abierto = true; } else g.lineTo(X(p[1]), Y(p[2]));
                }
                g.stroke();
            }
        }
    }

    // Todas las vueltas
    Tarjeta {
        visible: !raiz.rec
        z: raiz.menuAbierto ? 20 : 0   // con el menú abierto, la tarjeta queda arriba de la capa que lo cierra
        oscura: true
        x: cajaVel.x; y: raiz.h ? 362 : 900
        width: cajaVel.width; height: raiz.h ? 338 : 360
        Sub { x: 22; y: 16; text: "Todas las vueltas"; tam: 12 }
        // Con el menú abierto, tocar en otra parte de la tarjeta lo cierra
        MouseArea { anchors.fill: parent; z: 9; visible: raiz.menuAbierto; onClicked: { raiz.menuAbierto = false; raiz.confirmarRest = false; } }
        // Menú ⋮ (app 2.4.1): restablecer la carrera original y compartir
        Item {
            id: mas3
            anchors { right: parent.right; rightMargin: 10; top: parent.top; topMargin: 4 }
            width: 44; height: 36
            Rectangle { anchors.fill: parent; radius: 18; color: raiz.menuAbierto ? Qt.rgba(1, 1, 1, 0.12) : "transparent" }
            Text { anchors.centerIn: parent; text: "⋮"; color: Tema.tinta; font { pixelSize: 26; weight: Font.Black } }
            MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: { raiz.menuAbierto = !raiz.menuAbierto; raiz.confirmarRest = false; } }
        }
        Rectangle {
            visible: raiz.menuAbierto
            z: 10
            anchors { right: mas3.right; top: mas3.bottom; topMargin: 4 }
            width: 330; height: colMenu.height + 12; radius: 12
            color: "#22262b"; border { width: 1; color: Qt.rgba(1, 1, 1, 0.18) }
            Column {
                id: colMenu; x: 6; y: 6; width: parent.width - 12
                Rectangle {
                    width: parent.width; height: 52; radius: 8
                    readonly property bool puede: !!raiz.c.modificada
                    color: raiz.confirmarRest ? Tema.alerta : (mr.pressed ? Qt.rgba(1, 1, 1, 0.08) : "transparent")
                    opacity: puede ? 1 : 0.35
                    Text { x: 12; anchors.verticalCenter: parent.verticalCenter; width: parent.width - 24; elide: Text.ElideRight
                           text: raiz.confirmarRest ? "¿Seguro? Se pierden los cambios" : "↺ Restablecer la carrera original"
                           color: raiz.confirmarRest ? "#fff" : Tema.tinta; font { family: Tema.fuente600; pixelSize: 17 } }
                    MouseArea { id: mr; anchors.fill: parent; enabled: parent.puede; onClicked: raiz.restablecer() }
                }
                Rectangle {
                    width: parent.width; height: 52; radius: 8
                    color: mc.pressed ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                    Text { x: 12; anchors.verticalCenter: parent.verticalCenter; text: "⤴ Compartir informe"; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 17 } }
                    MouseArea { id: mc; anchors.fill: parent; onClicked: { raiz.menuAbierto = false; Navegacion.ir("compartir", { id: raiz.id_ }); } }
                }
            }
        }
        // «Unir V5 con V6»: barra amarilla, aparece al mantener apretada una vuelta 3 s
        BotonAmarillo {
            id: barraUnir
            visible: raiz.puedeUnir(raiz.fila)
            x: 22; y: 40; width: parent.width - 44; height: 40; tamTexto: 16
            texto: !visible ? "" : raiz.confirmarUnir ? "¿Seguro? Tocá de nuevo"
                   : "Unir " + raiz.nombreV(raiz.vueltas[raiz.fila]) + " con " + raiz.nombreV(raiz.vueltas[raiz.fila + 1])
            onTocado: raiz.unir()
        }
        Flickable {
            id: tabla
            anchors { fill: parent; topMargin: barraUnir.visible ? 90 : 40; leftMargin: 22; rightMargin: 22; bottomMargin: 12 }
            contentHeight: colT.height; clip: true; boundsBehavior: Flickable.StopAtBounds
            // Con muchas vueltas, la vuelta elegida (o la del video) queda a la vista
            function verFila(i) {
                if (i < 0 || moving) return;
                if (i * 40 < contentY) contentY = i * 40;
                else if (i * 40 + 40 > contentY + height) contentY = Math.min(contentHeight - height, i * 40 + 40 - height);
            }
            onContentHeightChanged: verFila(raiz.lap)
            Connections { target: raiz; function onLapChanged() { tabla.verFila(raiz.lap); } }
            Column {
                id: colT; width: tabla.width
                Repeater {
                    model: raiz.vueltas
                    Item {
                        required property var modelData
                        required property int index
                        width: colT.width; height: 40
                        Rectangle { anchors.fill: parent; radius: 8; color: Qt.rgba(244 / 255, 197 / 255, 49 / 255, index === raiz.fila ? 0.32 : 0.18); visible: index === (raiz.fila >= 0 ? raiz.fila : raiz.lap) }
                        Rectangle {   // se llena mientras se mantiene apretada
                            id: llenado; height: parent.height; radius: 8; width: 0
                            color: Qt.rgba(244 / 255, 197 / 255, 49 / 255, 0.25)
                            NumberAnimation on width { id: animLlenado; running: false; from: 0; to: colT.width; duration: 3000 }
                        }
                        MouseArea {
                            anchors.fill: parent
                            pressAndHoldInterval: 3000
                            onPressed: animLlenado.restart()
                            onReleased: { animLlenado.stop(); llenado.width = 0; }
                            onCanceled: { animLlenado.stop(); llenado.width = 0; }
                            onClicked: { raiz.fila = -1; if (modelData.m) raiz.lap = index; raiz.irVideo(index, 0); }
                            onPressAndHold: { raiz.fila = index; if (modelData.m) raiz.lap = index; }
                        }
                        Row {
                            anchors.verticalCenter: parent.verticalCenter; x: 6
                            readonly property real u: (colT.width - 12) / 10
                            Text { width: parent.u * 1.6; text: raiz.nombreV(modelData); color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 24 } }
                            // Etiqueta MEDIDA / LIBRE: se toca y cambia (la libre no suma error)
                            Item { width: parent.u * 3; height: 30
                                Rectangle {
                                    id: etiq
                                    readonly property color col: modelData.interrumpida ? "#8a929b" : modelData.medida ? "#d23b2c" : Tema.bien
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: tEtiq.implicitWidth + 22; height: 28; radius: 14
                                    color: Qt.rgba(col.r, col.g, col.b, 0.18); border { width: 1.5; color: col }
                                    Text { id: tEtiq; anchors.centerIn: parent; color: etiq.col
                                           text: modelData.interrumpida ? "INTERRUMPIDA" : modelData.medida ? "MEDIDA" : "LIBRE"
                                           font { family: Tema.fuente800; weight: Font.ExtraBold; pixelSize: 16; letterSpacing: 1 } }
                                    MouseArea { anchors.fill: parent; anchors.margins: -6; enabled: !modelData.interrumpida
                                                onClicked: raiz.cambiarMedida(index) }
                                } }
                            Text { width: parent.u * 2.2; text: Formato.tiempo(modelData.realMs); color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 24 } }
                            Text { width: parent.u * 2; text: modelData.difMs === undefined ? "–" : Formato.dif(modelData.difMs)
                                   color: modelData.difMs === undefined ? Tema.tintaSuave : raiz.colorDif(modelData.difMs)
                                   font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 24 } }
                            Text { visible: raiz.vv; width: parent.u * 1.2; horizontalAlignment: Text.AlignRight
                                   text: "▸ " + raiz.inicioVideo(index); color: index === raiz.lap ? Tema.acento : Tema.tinta
                                   font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 22 } }
                            Text { visible: !raiz.vv; width: parent.u * 1.2; horizontalAlignment: Text.AlignRight
                                   text: modelData.velPromKmh !== null && modelData.velPromKmh !== undefined ? String(modelData.velPromKmh).replace(".", ",") : "–"
                                   color: Tema.tintaSuave; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 20 } }
                        }
                    }
                }
                Item {
                    width: colT.width; height: 64
                    readonly property var med: raiz.vueltas.filter(v => v.medida && v.difMs !== undefined)
                    visible: med.length > 0
                    Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.2) }
                    Sub { x: 6; y: 12; tam: 13; text: "Error total · " + parent.med.length + (parent.med.length === 1 ? " medida" : " medidas") }
                    Sub { x: 6; y: 40; tam: 12; text: "promedio " + (raiz.c.promedio || "–") + " · mejor " + (raiz.c.mejor || "–") }
                    Text { anchors { right: parent.right; rightMargin: 6; top: parent.top; topMargin: 6 }
                           text: raiz.c.errorTotal || ""; color: Tema.acento; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 26 } }
                }
            }
        }
    }
    // Recorrido libre: resumen y marcas (muestra docs/muestras/recorrido/3-informe.png)
    Tarjeta {
        id: tarRec
        visible: raiz.rec
        oscura: true
        x: cajaVel.x; y: raiz.h ? 362 : 900
        width: cajaVel.width; height: raiz.h ? 338 : 360
        readonly property var r: raiz.inf.resumen || ({})
        Grid {
            id: gridRes
            x: 22; y: 14; width: parent.width - 44
            columns: 3; rowSpacing: 2
            Repeater {
                model: [
                    { e: "Tiempo", v: raiz.reloj(tarRec.r.tiempoMs || 0) },
                    { e: "Distancia", v: raiz.coma((tarRec.r.distM || 0) / 1000, 1) + " km" },
                    { e: "Promedio", v: raiz.coma(tarRec.r.promKmh, 1) + " km/h" },
                    { e: "Máxima", v: raiz.coma(tarRec.r.maxKmh, 0) + " km/h" },
                    { e: "Parado", v: raiz.reloj(tarRec.r.paradoMs || 0).replace(/^0:/, "") },
                    { e: "Acel. máx.", v: raiz.coma(tarRec.r.acelMax, 1) + " m/s²" }
                ]
                Column {
                    required property var modelData
                    width: gridRes.width / 3
                    Sub { text: modelData.e; tam: 11 }
                    Text { text: modelData.v; color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: raiz.h ? 30 : 28 } }
                }
            }
        }
        Sub { x: 22; y: gridRes.y + gridRes.height + 12; tam: 12
              text: raiz.marcas.length ? "Marcas" + (raiz.videos.length ? " · tocá una y el video va ahí" : "") : "Sin marcas" }
        Flickable {
            id: listaMarcas
            x: 22; y: gridRes.y + gridRes.height + 38; width: parent.width - 44; height: parent.height - y - 12
            contentHeight: colMarcas.height; clip: true; boundsBehavior: Flickable.StopAtBounds
            Column {
                id: colMarcas; width: parent.width
                Repeater {
                    model: raiz.marcas
                    Item {
                        required property var modelData
                        width: colMarcas.width; height: 40
                        Row {
                            anchors.verticalCenter: parent.verticalCenter; spacing: 18
                            Rectangle { width: 30; height: 30; radius: 15; color: Tema.amarillo; anchors.verticalCenter: parent.verticalCenter
                                        Text { anchors.centerIn: parent; text: modelData.n; color: "#111111"; font { family: Tema.fuente800; pixelSize: 17 } } }
                            Text { width: 110; text: raiz.reloj(modelData.ms); color: Tema.tinta; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 25 } }
                            Text { width: 120; text: raiz.coma((modelData.m || 0) / 1000, 1) + " km"; color: Tema.tinta; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 25 } }
                            Text { text: Math.round(modelData.kmh || 0) + " km/h"; color: Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22 } }
                        }
                        MouseArea { anchors.fill: parent; enabled: raiz.videos.length > 0; onClicked: raiz.irMarca(modelData) }
                    }
                }
            }
        }
    }
    // Video a medias: arriba a la izquierda; abajo, «En este momento»
    Tarjeta {
        id: cajaVideo
        visible: raiz.vv
        oscura: true
        x: raiz.h ? 24 : 20; y: raiz.h ? 110 : 180
        width: raiz.h ? 600 : parent.width - 40; height: 400
    }
    Tarjeta {
        visible: raiz.vv
        oscura: true
        x: raiz.h ? 24 : 370; y: raiz.h ? 530 : 600
        width: raiz.h ? 600 : 330; height: raiz.h ? 170 : 280
        Sub { x: 18; y: 16; text: "En este momento"; tam: 12 }
        Grid {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 18 }
            columns: raiz.h ? 3 : 1; rowSpacing: 4
            Repeater {
                model: [
                    { e: "Velocidad", v: reproductor.e.lap >= 0 && reproductor.e.ok ? String(Math.round(reproductor.e.kmh)) : "–", u: "km/h", c: Tema.numero },
                    { e: "Sugerida", v: raiz.sug > 0 ? raiz.sug.toFixed(1).replace(".", ",") : "–", u: "", c: Tema.tinta },
                    { e: "Adelanto", v: reproductor.e.lap >= 0 && raiz.dist > 0 ? Formato.dif(reproductor.e.ade * 1000) : "–", u: "",
                      c: reproductor.e.lap < 0 || Math.abs(reproductor.e.ade) < 1 ? Tema.bien : Math.abs(reproductor.e.ade) < 3 ? Tema.acento : "#d23b2c" }
                ]
                Item {
                    required property var modelData
                    width: raiz.h ? (cajaVideo.width - 36) / 3 : 294; height: raiz.h ? 96 : 64
                    Sub { text: modelData.e; tam: 11; anchors.top: raiz.h ? parent.top : undefined; anchors.verticalCenter: raiz.h ? undefined : parent.verticalCenter }
                    Row {
                        anchors { bottom: parent.bottom; right: raiz.h ? undefined : parent.right }
                        spacing: 4
                        Text { text: modelData.v; color: modelData.c; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: raiz.h ? 52 : 44 } }
                        Text { anchors.baseline: parent.children[0].baseline; text: modelData.u; color: Tema.tintaSuave
                               font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 20 } }
                    }
                }
            }
        }
    }
    Reproductor {
        id: reproductor
        visible: raiz.vv
        z: raiz.completo ? 50 : 1
        activo: raiz.vv
        completo: raiz.completo
        inf: raiz.inf
        videos: raiz.videos
        x: raiz.completo ? 0 : cajaVideo.x + 3; y: raiz.completo ? 0 : cajaVideo.y + 3
        width: raiz.completo ? raiz.width : cajaVideo.width - 6; height: raiz.completo ? raiz.height : cajaVideo.height - 6
        onDobleToque: raiz.completo = !raiz.completo
    }

    // Modo tele: esta pantalla es el control remoto
    ControlTele {
        id: control
        visible: raiz.enTele
        anchors.fill: parent
        z: 70
        rep: reproductor
        titulo: raiz.c.nombre || ""
        onSalir: Tele.terminar()
    }
    // La tele está enchufada pero Betómetro la tiene que volver a buscar
    MouseArea {
        visible: raiz.pedirReinicio
        anchors.fill: parent
        z: 80
        onClicked: raiz.pedirReinicio = false
        Rectangle { anchors.fill: parent; color: Qt.rgba(0, 0, 0, 0.6) }
        Tarjeta {
            oscura: true
            anchors.centerIn: parent
            width: Math.min(parent.width - 40, 640); height: 200
            MouseArea { anchors.fill: parent }
            Row {
                x: 30; y: 34; spacing: 14
                IconoTele { width: 34; height: 30; color: Tema.tinta; anchors.verticalCenter: parent.verticalCenter }
                Text { width: 640 - 110; wrapMode: Text.WordWrap; anchors.verticalCenter: parent.verticalCenter
                       text: "Hay que reiniciar Betómetro para usar la tele"; color: Tema.tinta
                       font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 26 } }
            }
            Row {
                anchors { right: parent.right; rightMargin: 30; bottom: parent.bottom; bottomMargin: 30 }
                spacing: 16
                BotonCromo { width: 170; height: 60; texto: "Cancelar"; tamTexto: 20; onTocado: raiz.pedirReinicio = false }
                BotonAmarillo { width: 220; height: 60; texto: "Reiniciar"; tamTexto: 22
                                activo: !Crono.enCurso; onTocado: Hdmi.reiniciar() }
            }
        }
    }

    // Menú ⋮ abierto: tocar afuera lo cierra
    MouseArea { anchors.fill: parent; visible: raiz.menuAbierto; onClicked: { raiz.menuAbierto = false; raiz.confirmarRest = false; } }
    Timer {   // solo para capturas: «Distancia» a medio mover, pegada a «Vuelta»
        id: tPruebaMover; interval: 400
        onTriggered: { const d = reproductor.datos[3]; d.moviendo = true; const r = reproductor.imantar(d, 470, 92); d.x = r.x; d.y = r.y; }
    }
    Timer {   // solo para capturas: lo mismo que el pulsador por cable (dos apretones juntos cuentan uno)
        id: tPruebaPulsador; interval: 600
        onTriggered: { reproductor.pulsador(); reproductor.pulsador(); }
    }
    Timer {   // solo para capturas: en la tele el video carga después; se vuelve a pedir la vuelta
        id: tPruebaTele; interval: 1500
        onTriggered: reproductor.irA(raiz.lap, raiz.vueltas.length > 20 ? 5 : 134.6)
    }
    Component.onCompleted: {   // solo para capturas
        // con muchas vueltas (BETO_MUESTRA_LARGA), una de las últimas
        const lapP = vueltas.length > 20 ? vueltas.length - 15 : 2, tP = vueltas.length > 20 ? 5 : 134.6;
        if (Contexto.prueba === "libre") cambiarMedida(1);
        else if (Contexto.prueba === "unir") fila = 3;
        else if (Contexto.prueba === "menu") menuAbierto = true;
        else if (Contexto.prueba === "tele-reiniciar") verEnTele();
        else if (Contexto.prueba.indexOf("tele") === 0) {   // tele, tele-datos, tele-qr, tele-carreras, tele-remoto (con dos pantallas)
            if (conTraza.indexOf(lapP) >= 0) lap = lapP;
            verEnTele();
            reproductor.irA(lap, tP);
            control.verDatos = Contexto.prueba === "tele-datos";
            control.verCelu = Contexto.prueba === "tele-qr";
            control.verCarreras = Contexto.prueba === "tele-carreras";
            if (vueltas.length > 20) tPruebaTele.start();
        }
        else if (Contexto.prueba.indexOf("video") === 0) {   // video, video-completo, video-andando, video-datos, video-mover
            verVideo = true;
            if (conTraza.indexOf(lapP) >= 0) lap = lapP;
            reproductor.irA(lap, tP);
            completo = Contexto.prueba !== "video" && Contexto.prueba !== "video-pulsador";
            reproductor.menuDatos = Contexto.prueba === "video-datos";
            if (Contexto.prueba === "video-apagar") {   // prueba: apagar un dato desde el menú y ver que se guarde
                reproductor.menuDatos = true;
                reproductor.guardar("velocidad", { on: false });
                console.warn("videoDatos:", JSON.stringify(Datos.ajustes.videoDatos), "velocidad:", reproductor.prendido("velocidad"));
            }
            reproductor.editando = Contexto.prueba === "video-mover";
            if (Contexto.prueba === "video-mover") tPruebaMover.start();
            if (Contexto.prueba === "video-andando") reproductor.alternar();
            if (Contexto.prueba === "video-pulsador") tPruebaPulsador.start();
        }
    }
}
