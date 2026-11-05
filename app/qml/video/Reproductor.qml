import QtQuick
import QtMultimedia
import Betometro

// Reproductor del video de la carrera (E7, aprobado por Matías el 05/11/2026: docs/video.md y docs/muestras/video/).
// A medias (dentro del informe) o en pantalla completa (doble toque) con los datos encima en tiras finitas.
// Sincronía: el momento del video de un punto de la traza = desdeLargadaMs + (realMs de las vueltas anteriores
// de esa largada) + t·1000. Los datos se actualizan 10 veces por segundo, no en cada cuadro (Pi 3).
Item {
    id: rep
    property var inf: ({})          // Datos.informe(id)
    property var videos: []         // Datos.videos(id): [{archivo, largada, desdeLargadaMs, girar}]
    property bool activo: false     // con el botón en «Video»; si no, no se carga nada
    property bool completo: false   // pantalla completa
    property bool editando: false   // «Datos»: prender, apagar y mover los datos (solo en pausa)
    signal dobleToque()
    clip: true

    // ---------- Sincronía ----------
    readonly property var vueltas: inf.vueltas || []
    readonly property real distancia: inf.distanciaM || 0
    readonly property real sug: distancia > 0 && inf.idealMs > 0 ? distancia / (inf.idealMs / 1000) * 3.6 : 0
    // Un tramo por vuelta que tiene video: {v (índice del video), lap, desde, hasta} en ms del video
    readonly property var tramos: {
        const r = [];
        for (let k = 0; k < videos.length; k++) {
            let acum = videos[k].desdeLargadaMs || 0;
            for (let i = 0; i < vueltas.length; i++) {
                if (vueltas[i].largada !== videos[k].largada) continue;
                r.push({ v: k, lap: i, desde: acum, hasta: acum + (vueltas[i].realMs || 0) });
                acum += vueltas[i].realMs || 0;
            }
        }
        return r;
    }
    property int videoActual: 0
    readonly property var vid: videos[videoActual] || null
    readonly property string url: vid ? Camara.urlVideo(vid.archivo) : ""
    readonly property bool falta: !!vid && url === ""
    readonly property var tramosVideo: tramos.filter(t => t.v === videoActual)
    property real pendiente: -1      // posición a la que ir cuando termine de cargar
    property real pos: 0             // ms del video, 10 veces por segundo
    property real arrastre: -1       // mientras se arrastra la línea de tiempo
    readonly property bool andando: player.playbackState === MediaPlayer.PlayingState
    readonly property real total: player.duration > 0 ? player.duration : (tramosVideo.length ? tramosVideo[tramosVideo.length - 1].hasta : 1)

    function nombreV(v) { return "V" + v.n + (inf.largadas > 1 ? " L" + v.largada : ""); }
    // ¿En qué momento del video está la vuelta `lap` a los `t` segundos? Devuelve {v, ms} o null.
    function momento(lap, t) {
        let mejor = null;
        for (const tr of tramos) {
            if (tr.lap !== lap) continue;
            const ms = tr.desde + t * 1000;
            if (ms >= 0) return { v: tr.v, ms: ms };
            if (!mejor) mejor = { v: tr.v, ms: 0 };   // el video empezó después: lo más cerca
        }
        return mejor;
    }
    function irA(lap, t) {
        const m = momento(lap, t || 0);
        if (!m) return false;
        irMs(m.v, m.ms);
        return true;
    }
    function irMs(v, ms) {
        pos = ms;
        if (v !== videoActual || player.mediaStatus === MediaPlayer.NoMedia || player.mediaStatus === MediaPlayer.LoadingMedia) {
            pendiente = ms; videoActual = v;
        } else player.position = ms;
    }
    function alternar() {
        if (falta) return;
        if (andando) player.pause(); else { editando = false; player.play(); }
    }
    // Fila de la traza en el segundo t (interpolada entre las dos lecturas vecinas)
    function filaEn(M, t) {
        if (!M || !M.length) return null;
        let lo = 0, hi = M.length - 1;
        if (t <= M[0][0]) return M[0];
        if (t >= M[hi][0]) return M[hi];
        while (hi - lo > 1) { const mid = (lo + hi) >> 1; if (M[mid][0] <= t) lo = mid; else hi = mid; }
        const a = M[lo], b = M[hi], f = (t - a[0]) / ((b[0] - a[0]) || 1), r = [];
        for (let i = 0; i < 7; i++) r.push(a[i] + (b[i] - a[i]) * f);
        r.push(a[7] && b[7] ? 1 : 0);
        return r;
    }
    // Todo lo que muestran los datos, para la posición actual
    readonly property var e: {
        const p = arrastre >= 0 ? arrastre : pos;
        const r = { lap: -1, tMs: 0, d: 0, L: distancia, kmh: 0, ade: 0, x: 0, y: 0, ok: false, antes: true };
        for (const tr of tramosVideo) {
            if (p < tr.desde) break;
            r.antes = false;
            if (p >= tr.hasta) continue;
            const v = vueltas[tr.lap];
            r.lap = tr.lap; r.v = v; r.tMs = p - tr.desde; r.L = distancia > 0 ? distancia : (v.L || 0);
            const f = filaEn(v.m, r.tMs / 1000);
            if (f) { r.d = f[1]; r.kmh = f[2]; r.ade = f[4]; r.x = f[5]; r.y = f[6]; r.ok = !!f[7]; }
            break;
        }
        return r;
    }
    readonly property int lapAhora: e.lap

    // ---------- Reproductor ----------
    Rectangle { anchors.fill: parent; color: "#000000" }
    MediaPlayer {
        id: player
        source: rep.activo ? rep.url : ""
        videoOutput: salida
        audioOutput: AudioOutput {}
        onMediaStatusChanged: {
            if ((mediaStatus === MediaPlayer.LoadedMedia || mediaStatus === MediaPlayer.BufferedMedia) && rep.pendiente >= 0) {
                if (playbackState === MediaPlayer.StoppedState) pause();   // muestra el cuadro aunque no ande
                position = rep.pendiente;
                rep.pendiente = -1;
            }
        }
        onPositionChanged: if (!rep.andando) rep.pos = position
        onErrorOccurred: (codigo, texto) => console.warn("Video:", texto)
    }
    onActivoChanged: if (!activo) { player.pause(); editando = false; }
    Timer { interval: 100; repeat: true; running: rep.andando; onTriggered: rep.pos = player.position }
    VideoOutput {
        id: salida
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        orientation: rep.vid && rep.vid.girar ? 180 : 0
        // espejo (como cámara de selfie): solo el video, los datos de encima no
        transform: Scale { origin.x: salida.width / 2; xScale: rep.vid && rep.vid.espejo ? -1 : 1 }
    }
    Text {
        anchors.centerIn: parent
        visible: rep.falta || !rep.vid
        text: rep.vid ? "No está el archivo del video" : "Esta carrera no tiene video"
        color: Tema.tinta
        font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: rep.completo ? 30 : 22 }
    }

    // Un toque: anda / pausa. Doble toque: pantalla completa (otro doble toque vuelve).
    Timer { id: unToque; interval: 280; onTriggered: rep.alternar() }
    TapHandler {
        enabled: !rep.editando
        onTapped: (punto, boton) => { if (tapCount === 1) unToque.restart(); }
        onDoubleTapped: { unToque.stop(); rep.dobleToque(); }
    }

    // Botón grande de play en pausa (pantalla completa)
    Rectangle {
        visible: rep.completo && !rep.andando && !rep.editando && !rep.falta && !!rep.vid
        anchors.centerIn: parent
        width: 120 * rep.s; height: width; radius: width / 2
        color: Qt.rgba(0, 0, 0, 0.53); border { width: 3; color: "#ffffff" }
        Triangulo { anchors.centerIn: parent; anchors.horizontalCenterOffset: 6 * rep.s; width: 44 * rep.s; height: 56 * rep.s }
    }

    // ---------- Datos encima (pantalla completa) ----------
    readonly property real s: Math.max(0.55, Math.min(width / 1280, height / 720))
    readonly property real margen: 22 * s
    readonly property real arriba: 18 * s
    readonly property real abajo: height - 84 * s     // arriba de la barra
    readonly property var cfgDatos: Datos.ajustes.videoDatos || ({})
    readonly property var fabrica: ({
        vuelta: { x: 0, y: 0 }, adelanto: { x: 1, y: 0 }, tiempo: { x: 0, y: 0.781 },
        distancia: { x: 0, y: 0.89 }, velocidad: { x: 0, y: 1 }, pista: { x: 1, y: 1 }
    })
    function lugar(clave) { const c = cfgDatos[clave] || {}; return c.x !== undefined ? c : fabrica[clave]; }
    function prendido(clave) {
        if (clave === "adelanto" && !(distancia > 0)) return false;
        const c = cfgDatos[clave];
        return !c || c.on !== false;
    }
    function xDe(clave, w) { return margen + lugar(clave).x * Math.max(0, width - 2 * margen - w); }
    function yDe(clave, h) { return arriba + lugar(clave).y * Math.max(0, abajo - arriba - h); }
    function guardar(clave, cambio) {
        const todo = JSON.parse(JSON.stringify(cfgDatos));
        const c = todo[clave] || {};
        for (const k in cambio) c[k] = cambio[k];
        todo[clave] = c;
        Datos.ponerCfg("videoDatos", todo);
    }
    function guardarPos(clave, x, y, w, h) {
        const aw = width - 2 * margen - w, ah = abajo - arriba - h;
        const fx = aw > 0 ? (x - margen) / aw : 0, fy = ah > 0 ? (y - arriba) / ah : 0;
        guardar(clave, { x: Math.round(Math.max(0, Math.min(1, fx)) * 1000) / 1000, y: Math.round(Math.max(0, Math.min(1, fy)) * 1000) / 1000 });
        guiaX = -1; guiaY = -1; pegaA = "";
    }
    // Imán: el dato se pega a los bordes y a los otros datos (bordes alineados o uno al lado del otro)
    property real guiaX: -1
    property real guiaY: -1
    property string pegaA: ""
    function imantar(item, nx, ny) {
        const w = item.width, h = item.height, u = 16 * s, sep = 10 * s;
        nx = Math.max(margen, Math.min(width - margen - w, nx));
        ny = Math.max(arriba, Math.min(abajo - h, ny));
        let bx = null, by = null;
        const px = (v, gx, quien) => { const dd = Math.abs(v - nx); if (dd < u && (!bx || dd < bx.dd)) bx = { v: v, dd: dd, g: gx, q: quien }; };
        const py = (v, gy, quien) => { const dd = Math.abs(v - ny); if (dd < u && (!by || dd < by.dd)) by = { v: v, dd: dd, g: gy, q: quien }; };
        px(margen, margen, ""); px(width - margen - w, width - margen, ""); px(width / 2 - w / 2, width / 2, "");
        py(arriba, arriba, ""); py(abajo - h, abajo, "");
        for (const o of datos) {
            if (o === item || !o.visible) continue;
            px(o.x, o.x, o.nombre); px(o.x + o.width - w, o.x + o.width, o.nombre);
            px(o.x + o.width + sep, o.x + o.width + sep, o.nombre); px(o.x - sep - w, o.x - sep, o.nombre);
            py(o.y, o.y, o.nombre); py(o.y + o.height - h, o.y + o.height, o.nombre);
            py(o.y + o.height + sep, o.y + o.height + sep, o.nombre); py(o.y - sep - h, o.y - sep, o.nombre);
        }
        guiaX = bx ? bx.g : -1; guiaY = by ? by.g : -1;
        pegaA = by && by.q ? by.q : bx && bx.q ? bx.q : "";
        return { x: bx ? bx.v : nx, y: by ? by.v : ny };
    }
    readonly property var datos: [dVuelta, dAdelanto, dTiempo, dDistancia, dVelocidad, dPista]
    readonly property var lista: [
        { clave: "vuelta", nombre: "Vuelta y tipo" }, { clave: "tiempo", nombre: "Tiempo de la vuelta" },
        { clave: "adelanto", nombre: "Adelanto" }, { clave: "velocidad", nombre: "Velocidad" },
        { clave: "distancia", nombre: "Distancia" }, { clave: "pista", nombre: "Pista" }
    ]

    // Tira finita de una línea
    component Tira: Rectangle {
        default property alias hijos: fila.data
        property real s: rep.s
        width: fila.implicitWidth + 36 * s; height: 52 * s; radius: height / 2
        color: Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.8)
        border { width: 2; color: Tema.bordeCromo }
        Row { id: fila; anchors.centerIn: parent; spacing: 10 * parent.s }
    }
    component Etiq: Text {
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        color: Tema.tintaSuave
        font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 14 * rep.s; letterSpacing: 2 * rep.s; capitalization: Font.AllUppercase }
    }
    component Num: Text {
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        color: Tema.tinta
        font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 38 * rep.s }
    }
    component Uni: Text {
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        color: Tema.tintaSuave
        font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 20 * rep.s }
    }
    component Triangulo: Canvas {
        property color color: "#ffffff"
        onWidthChanged: requestPaint()
        onColorChanged: requestPaint()
        onPaint: { const g = getContext("2d"); g.reset(); g.fillStyle = color; g.beginPath(); g.moveTo(0, 0); g.lineTo(width, height / 2); g.lineTo(0, height); g.closePath(); g.fill(); }
    }

    DatoVideo {
        id: dVuelta; reproductor: rep; clave: "vuelta"; nombre: "Vuelta"
        Rectangle {
            width: tv.implicitWidth + 40 * rep.s; height: 52 * rep.s; radius: height / 2
            color: Tema.acento; border { width: 2; color: Tema.acento }
            Text {
                id: tv; anchors.centerIn: parent
                text: rep.e.lap >= 0 ? rep.nombreV(rep.e.v) + " · " + (rep.e.v.interrumpida ? "INTERRUMPIDA" : rep.e.v.medida ? "MEDIDA" : "LIBRE")
                      : rep.e.antes ? "ANTES DE LARGAR" : "TERMINADA"
                color: Tema.acentoTinta
                font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 22 * rep.s }
            }
        }
    }
    DatoVideo {
        id: dAdelanto; reproductor: rep; clave: "adelanto"; nombre: "Adelanto"
        Tira {
            // ade > 0: llegó antes que el ideal (adelantado). Se muestra como en la tabla: + atrasado, − adelantado
            id: tAde
            readonly property real a: rep.e.ade
            readonly property bool enTiempo: Math.abs(a) < 0.2
            readonly property color col: enTiempo ? Tema.bien : a < 0 ? Tema.alerta : Tema.sugerida
            Num { visible: !tAde.enTiempo; text: Formato.dif(-tAde.a * 1000); color: tAde.col }
            Etiq { text: tAde.enTiempo ? "en tiempo" : tAde.a < 0 ? "atrasado" : "adelantado"; color: tAde.col }
        }
    }
    DatoVideo {
        id: dTiempo; reproductor: rep; clave: "tiempo"; nombre: "Vuelta"
        Tira { Etiq { text: "Vuelta" } Num { text: rep.e.lap >= 0 ? Formato.tiempo(rep.e.tMs) : "–" } }
    }
    DatoVideo {
        id: dDistancia; reproductor: rep; clave: "distancia"; nombre: "Distancia"
        Tira {
            Etiq { text: "Dist." }
            Num { text: rep.e.lap >= 0 ? Formato.miles(rep.e.d) : "–" }
            Uni { text: "/ " + Formato.miles(rep.e.L) + " m" }
        }
    }
    DatoVideo {
        id: dVelocidad; reproductor: rep; clave: "velocidad"; nombre: "Velocidad"
        Tira {
            Etiq { text: "Vel." }
            Num { text: rep.e.lap >= 0 && rep.e.ok ? String(Math.round(rep.e.kmh)) : "–" }
            Uni { text: "km/h" }
            Item {   // barrita contra la sugerida: el medio es ir justo
                visible: rep.sug > 0
                width: 90 * rep.s; height: 17 * rep.s; anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 7 * rep.s; radius: height / 2
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: "#d23b2c" } GradientStop { position: 0.25; color: Tema.sugerida }
                        GradientStop { position: 0.5; color: Tema.bien } GradientStop { position: 0.75; color: Tema.sugerida }
                        GradientStop { position: 1; color: "#d23b2c" }
                    }
                }
                Rectangle {
                    width: 4 * rep.s; height: parent.height; radius: 2 * rep.s; color: "#ffffff"
                    x: parent.width * Math.max(0, Math.min(1, 0.5 + (rep.e.kmh - rep.sug) / 20)) - width / 2
                }
            }
            Etiq { visible: rep.sug > 0; text: "sug." }
            Uni { visible: rep.sug > 0; text: rep.sug.toFixed(1).replace(".", ","); color: Tema.sugerida; font.pixelSize: 18 * rep.s }
        }
    }
    DatoVideo {
        id: dPista; reproductor: rep; clave: "pista"; nombre: "Pista"
        Rectangle {
            width: 246 * rep.s; height: 166 * rep.s; radius: 18 * rep.s
            color: Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.8); border { width: 2; color: Tema.bordeCromo }
            Pista { anchors { fill: parent; margins: 10 * rep.s } inf: rep.inf; punto: rep.e; grosor: 0.6 }
        }
    }

    // Guías del imán mientras se mueve un dato
    Rectangle { visible: rep.guiaX >= 0; x: rep.guiaX - 1; width: 2; height: parent.height; color: Tema.sugerida; opacity: 0.8; z: 4 }
    Rectangle { visible: rep.guiaY >= 0; y: rep.guiaY - 1; height: 2; width: parent.width; color: Tema.sugerida; opacity: 0.8; z: 4 }
    Text {
        readonly property Item m: rep.datos.find(o => o.moviendo) || null
        visible: !!m && rep.pegaA !== ""
        z: 6
        x: m ? Math.max(8, Math.min(rep.width - width - 8, m.x + m.width / 2 - width / 2)) : 0
        y: m ? m.y + m.height + 14 * rep.s : 0
        text: "se pega a «" + rep.pegaA + "»"
        color: Tema.sugerida; style: Text.Outline; styleColor: "#000000"
        font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 18 * rep.s }
    }

    // ---------- Chips a medias (dentro del informe) ----------
    Row {
        visible: !rep.completo && !!rep.vid && !rep.falta
        x: 14; y: 12; spacing: 8
        Repeater {
            model: rep.e.lap >= 0 ? [rep.nombreV(rep.e.v) + " · " + (rep.e.v.medida ? "medida" : "libre"), Formato.tiempo(rep.e.tMs) + " de la vuelta"]
                                  : [rep.e.antes ? "Antes de largar" : "Terminada"]
            Rectangle {
                required property var modelData
                required property int index
                width: tc.implicitWidth + 24; height: 32; radius: 16
                color: index === 0 ? Tema.acento : Qt.rgba(0, 0, 0, 0.55); border { width: 1; color: index === 0 ? Tema.acento : Tema.bordeCromo }
                Text { id: tc; anchors.centerIn: parent; text: modelData; color: index === 0 ? Tema.acentoTinta : Tema.tinta
                       font { family: Tema.fuente600; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 16 } }
            }
        }
    }

    // ---------- Barra de abajo: play/pausa, tiempo, línea con una rayita por vuelta, total, engranaje ----------
    Rectangle {
        id: barra
        visible: !!rep.vid && !rep.falta
        readonly property real b: rep.completo ? rep.s : 0.85
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: rep.completo ? 22 * rep.s : 12 }
        anchors.bottomMargin: rep.completo ? 18 * rep.s : 12
        height: 52 * b; radius: height / 2
        color: Qt.rgba(0, 0, 0, 0.67); border { width: 2; color: Tema.bordeCromo }
        z: 3
        Item {   // play / pausa
            id: bPlay
            x: 14 * barra.b; width: 34 * barra.b; height: parent.height
            Triangulo { visible: !rep.andando; anchors.centerIn: parent; width: 20 * barra.b; height: 24 * barra.b }
            Row {
                visible: rep.andando; anchors.centerIn: parent; spacing: 6 * barra.b
                Rectangle { width: 6 * barra.b; height: 22 * barra.b; color: "#ffffff" }
                Rectangle { width: 6 * barra.b; height: 22 * barra.b; color: "#ffffff" }
            }
            MouseArea { anchors.fill: parent; anchors.margins: -8; onClicked: rep.alternar() }
        }
        Text {
            id: tAhora
            x: bPlay.x + bPlay.width + 10 * barra.b; anchors.verticalCenter: parent.verticalCenter
            text: rep.reloj(rep.arrastre >= 0 ? rep.arrastre : rep.pos)
            color: "#ffffff"; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 26 * barra.b }
        }
        Text {
            id: tTotal
            anchors { right: bDatos.visible ? bDatos.left : parent.right; rightMargin: bDatos.visible ? 12 * barra.b : 20 * barra.b; verticalCenter: parent.verticalCenter }
            text: rep.reloj(rep.total)
            color: "#ffffff"; opacity: 0.8; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 26 * barra.b }
        }
        // Engranaje en pausa (pantalla completa); con «Datos» abierto, el botón que lo cierra
        Rectangle {
            id: bDatos
            visible: rep.completo && (!rep.andando || rep.editando)
            anchors { right: parent.right; rightMargin: 18 * barra.b; verticalCenter: parent.verticalCenter }
            height: 42 * barra.b; width: rep.editando ? tDatos.implicitWidth + 36 * barra.b : height; radius: height / 2
            color: Tema.acento
            Text { id: tDatos; visible: rep.editando; anchors.centerIn: parent; text: "✎ Datos"; color: Tema.acentoTinta
                   font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 18 * barra.b } }
            Canvas {   // engranaje
                visible: !rep.editando; anchors.centerIn: parent; width: 28 * barra.b; height: width
                onWidthChanged: requestPaint()
                onPaint: {
                    const g = getContext("2d"); g.reset(); g.scale(width / 64, height / 64);
                    g.strokeStyle = Tema.acentoTinta; g.lineWidth = 6; g.lineCap = "round";
                    g.beginPath(); g.arc(32, 32, 9, 0, Math.PI * 2); g.stroke();
                    for (let i = 0; i < 8; i++) {
                        const a = i * Math.PI / 4;
                        g.beginPath(); g.moveTo(32 + 17 * Math.cos(a), 32 + 17 * Math.sin(a)); g.lineTo(32 + 26 * Math.cos(a), 32 + 26 * Math.sin(a)); g.stroke();
                    }
                }
            }
            MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: rep.editando = !rep.editando }
        }
        // Línea de tiempo
        Item {
            id: linea
            anchors { left: tAhora.right; leftMargin: 16 * barra.b; right: tTotal.left; rightMargin: 16 * barra.b; verticalCenter: parent.verticalCenter }
            height: 24 * barra.b
            readonly property real f: Math.max(0, Math.min(1, (rep.arrastre >= 0 ? rep.arrastre : rep.pos) / rep.total))
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 8 * barra.b; radius: height / 2; color: Qt.rgba(1, 1, 1, 0.2) }
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width * linea.f; height: 8 * barra.b; radius: height / 2; color: Tema.acento }
            Repeater {
                model: rep.tramosVideo
                Rectangle {
                    required property var modelData
                    visible: modelData.desde >= 0 && modelData.desde <= rep.total
                    x: linea.width * modelData.desde / rep.total - width / 2; width: 3 * barra.b; height: linea.height; color: "#ffffff"
                }
            }
            MouseArea {
                anchors { fill: parent; topMargin: -14; bottomMargin: -14 }
                preventStealing: true
                function msEn(mx) {
                    const ms = Math.max(0, Math.min(1, mx / linea.width)) * rep.total;
                    for (const tr of rep.tramosVideo)   // cerca de una rayita: salta justo al empezar esa vuelta
                        if (Math.abs((tr.desde - ms) / rep.total * linea.width) < 14) return Math.max(0, tr.desde);
                    return ms;
                }
                onPressed: (m) => rep.arrastre = msEn(m.x)
                onPositionChanged: (m) => rep.arrastre = msEn(m.x)
                onReleased: { const ms = rep.arrastre; rep.arrastre = -1; rep.irMs(rep.videoActual, ms); }
                onCanceled: rep.arrastre = -1
            }
        }
    }
    function reloj(ms) {
        const s = Math.max(0, Math.floor(ms / 1000)), h = Math.floor(s / 3600), m = Math.floor(s / 60) % 60, ss = String(s % 60).padStart(2, "0");
        return h > 0 ? h + ":" + String(m).padStart(2, "0") + ":" + ss : m + ":" + ss;
    }

    // ---------- «Datos sobre el video»: prender y apagar cada uno ----------
    Rectangle {
        visible: rep.editando && rep.completo
        z: 4
        width: Math.min(rep.width - 40, 616 * rep.s); height: colDatos.height + 36 * rep.s
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 8 * rep.s }
        radius: 18 * rep.s
        color: Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.95); border { width: 2; color: Tema.bordeCromo }
        MouseArea { anchors.fill: parent }   // que los toques no lleguen al video
        Column {
            id: colDatos
            x: 24 * rep.s; y: 18 * rep.s; width: parent.width - 48 * rep.s
            Text { text: "Datos sobre el video"; color: Tema.tinta; height: 44 * rep.s
                   font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 24 * rep.s } }
            Repeater {
                model: rep.lista
                Item {
                    required property var modelData
                    required property int index
                    width: colDatos.width; height: 54 * rep.s
                    Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.12); visible: index > 0 }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: modelData.nombre; color: Tema.tinta
                           font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 22 * rep.s } }
                    Llave {
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        scale: 1.4 * rep.s; transformOrigin: Item.Right
                        prendida: rep.prendido(modelData.clave)
                        opacity: modelData.clave === "adelanto" && !(rep.distancia > 0) ? 0.4 : 1
                        onCambiada: (v) => rep.guardar(modelData.clave, { on: v })
                    }
                }
            }
            Text {
                width: parent.width; wrapMode: Text.WordWrap; topPadding: 8 * rep.s
                text: "Mantené apretado un dato para moverlo · se pega a los otros y a los bordes"
                color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 17 * rep.s }
            }
        }
    }
}
