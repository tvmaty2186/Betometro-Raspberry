import QtQuick
import QtMultimedia
import Betometro
import Betometro.Video

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
    property bool editando: false   // moviendo los datos en la pantalla (hasta «Guardar»)
    property bool menuVolumen: false   // el panel del volumen (− / +), se cierra solo
    // Volumen del video (Matías, 08/11/2026), guardado en la configuración. Silencio: 0, y vuelve al que estaba
    readonly property real volumen: Datos.ajustes.videoVolumen !== undefined ? Datos.ajustes.videoVolumen : 1
    function ponerVolumen(v) { Datos.ponerCfg("videoVolumen", Math.round(Math.max(0, Math.min(1, v)) * 10) / 10); }
    function silencio() {
        if (volumen > 0) { Datos.ponerCfg("videoVolumenAntes", volumen); ponerVolumen(0); }
        else ponerVolumen(Datos.ajustes.videoVolumenAntes || 1);
    }
    property bool menuDatos: false  // el menú del engranaje (solo en pausa): prender y apagar los datos
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
        if (andando) player.pause(); else { editando = false; menuDatos = false; player.play(); }
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

    // ---------- Reproductor (uno solo: en la Pi o, en modo tele, en la tele) ----------
    // En modo tele la imagen sale por la ventana de la tele (Tele.salida) y la vista de la Pi no se usa:
    // el video se decodifica una sola vez.
    property bool tele: false
    readonly property bool enTele: tele && !!Tele.salida
    // GStreamer (no el MediaPlayer de Qt): usa el descompresor de video de la Raspberry. Mismos nombres
    // y estados que MediaPlayer (app/video_gst.h)
    VideoGst {
        id: player
        source: rep.activo ? rep.url : ""
        videoOutput: rep.enTele ? Tele.salida : vista.salida
        sonidoTele: rep.enTele   // en la tele, el sonido también por el HDMI
        volumen: rep.volumen
        onMediaStatusChanged: {
            if ((mediaStatus === MediaPlayer.LoadedMedia || mediaStatus === MediaPlayer.BufferedMedia) && rep.pendiente >= 0) {
                if (playbackState === MediaPlayer.StoppedState) pause();   // muestra el cuadro aunque no ande
                position = rep.pendiente;
                rep.pendiente = -1;
            }
        }
        onPositionChanged: if (!rep.andando) rep.pos = player.position
        onErrorOccurred: (codigo, texto) => console.warn("Video:", texto)
    }
    onActivoChanged: if (!activo) { player.pause(); editando = false; menuDatos = false; }
    Timer { interval: 100; repeat: true; running: rep.andando; onTriggered: rep.pos = player.position }

    // ±10 s (control de la tele), sin salirse del video
    function saltar(dms) {
        if (falta || !vid) return;
        const base = pendiente >= 0 ? pendiente : pos;
        irMs(videoActual, Math.max(0, Math.min(total, base + dms)));
    }
    // Línea de tiempo: de la posición del dedo (mx sobre `ancho`) a ms; cerca de una rayita salta justo
    // al empezar esa vuelta
    function msEn(mx, ancho) {
        const ms = Math.max(0, Math.min(1, mx / ancho)) * total;
        let cerca = null, d = 14;
        for (const tr of rayitas(ancho)) {
            const dx = Math.abs((tr.desde - ms) / total * ancho);
            if (dx < d) { d = dx; cerca = tr; }
        }
        return cerca ? Math.max(0, cerca.desde) : ms;
    }
    // Las rayitas que se dibujan en una línea de `ancho` px: con muchas vueltas, una cada 2, 5 o 10,
    // así no se pegan (separadas por lo menos `minimo` px)
    function rayitas(ancho, minimo) {
        const n = tramosVideo.length;
        if (n < 2 || !(ancho > 0)) return tramosVideo;
        const sep = ancho * (tramosVideo[n - 1].desde - tramosVideo[0].desde) / (n - 1) / total;
        const m = minimo || 36;
        const paso = [1, 2, 5, 10, 20].find(p => sep * p >= m) || Math.ceil(m / Math.max(sep, 0.01));
        return paso === 1 ? tramosVideo : tramosVideo.filter((t, i) => i % paso === 0);
    }
    // Otra carrera en el mismo reproductor (modo tele, «Carreras»): de cero y en pausa
    function deNuevo() {
        if (andando) player.pause();
        arrastre = -1; pendiente = -1; pos = 0; videoActual = 0; editando = false; menuDatos = false;
    }
    function soltarArrastre() { const ms = arrastre; arrastre = -1; if (ms >= 0) irMs(videoActual, ms); }

    // Pulsador por cable: con el video a la vista (a medias, pantalla completa o en la tele) anda / pausa.
    // Dos apretones a menos de 300 ms cuentan como uno.
    signal destello()                // para que la vista muestre un ratito el botón grande
    property real ultimoPulsador: 0
    function pulsador() {
        const ahora = Date.now();
        if (ahora - ultimoPulsador < 300) return;
        ultimoPulsador = ahora;
        if (!activo || !vid || falta) return;
        alternar();
        destello();
    }
    Connections {
        target: Pulsador
        enabled: rep.activo
        ignoreUnknownSignals: true
        function onMarca(edadMs) { rep.pulsador(); }
    }

    // ---------- Datos encima: dónde va cada uno (fracciones guardadas, las mismas en la Pi y en la tele) ----------
    // Solo si es un objeto (antes de la 1.0.23 podía haber quedado el texto «[object Object]»)
    readonly property var cfgDatos: { const v = Datos.ajustes.videoDatos; return v && typeof v === "object" && !Array.isArray(v) ? v : ({}); }
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
    function guardar(clave, cambio) {
        const todo = JSON.parse(JSON.stringify(cfgDatos));
        const c = todo[clave] || {};
        for (const k in cambio) c[k] = cambio[k];
        todo[clave] = c;
        Datos.ponerCfg("videoDatos", todo);
    }
    readonly property var lista: [
        { clave: "vuelta", nombre: "Vuelta y tipo" }, { clave: "tiempo", nombre: "Tiempo de la vuelta" },
        { clave: "adelanto", nombre: "Adelanto" }, { clave: "velocidad", nombre: "Velocidad" },
        { clave: "distancia", nombre: "Distancia" }, { clave: "pista", nombre: "Pista" }
    ]

    // ---------- La vista de la Pi (imagen, datos, chips y barra: VistaVideo.qml) ----------
    VistaVideo {
        id: vista
        anchors.fill: parent
        rep: rep
        completo: rep.completo
        visible: !rep.enTele
    }
    // Para el informe (capturas): los datos de la vista de la Pi y su imán
    readonly property var datos: vista.datos
    function imantar(item, nx, ny) { return vista.imantar(item, nx, ny); }
}
