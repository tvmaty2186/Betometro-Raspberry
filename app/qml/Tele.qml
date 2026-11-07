pragma Singleton
import QtQuick

// Modo tele (docs/video.md): con una tele en el HDMI, el video de la carrera se ve grande en la tele
// y la pantalla de la Pi queda de control remoto. Un solo reproductor (el del informe): cuando está en
// la tele, su salida de video es la de la ventana de la tele (el video se decodifica una sola vez).
QtObject {
    id: tele
    // Qt ve una segunda pantalla. En la Raspberry, además, el sistema tiene que decir que la tele sigue
    // enchufada: si se desenchufa, la ventana se cierra (Qt no se entera solo).
    readonly property bool hay: Qt.application.screens.length > 1 && (!Hdmi || !Hdmi.vigila || Hdmi.conectada) && sePuede
    // La tele está enchufada pero Qt no la tiene (se enchufó con Betómetro abierto): hay que reiniciar
    readonly property bool hayQueReiniciar: !!Hdmi && Hdmi.vigila && Hdmi.conectada && Qt.application.screens.length < 2
    // La pantalla de la Pi: la de 5" (DSI) aunque Qt pusiera la tele primero. La tele: la primera que no es esa.
    readonly property var pantallaPi: {
        const ss = Qt.application.screens;
        for (let i = 0; i < ss.length; i++) if (ss[i].name.indexOf("DSI") === 0) return ss[i];
        return ss.length ? ss[0] : null;
    }
    readonly property var pantallaTele: {
        const ss = Qt.application.screens;
        for (let i = 0; i < ss.length; i++) if (pantallaPi && ss[i].name !== pantallaPi.name) return ss[i];
        return null;
    }
    // Solo si se sabe cuál es la de 5" (DSI) y la tele es otra. Si no, nunca se abre la ventana de la tele:
    // dos ventanas en la misma pantalla cierran Betómetro («EGLFS: OpenGL windows cannot be mixed with
    // others», visto en la Pi de Matías el 06/11/2026, con los nombres de kms.json mal puestos)
    readonly property bool sePuede: !!pantallaPi && pantallaPi.name.indexOf("DSI") === 0 && !!pantallaTele
                                    && pantallaTele.name !== pantallaPi.name
    property Item reproductor: null     // el Reproductor que se ve en la tele (null: la tele en reposo)
    property Item salida: null          // el VideoOutput de la ventana de la tele
    property string carrera: ""
    property string carreraId: ""
    readonly property bool activa: !!reproductor

    function empezar(rep, nombre, id) {
        if (!hay || !rep) return;
        if (reproductor && reproductor !== rep) terminar();
        carrera = nombre || "";
        carreraId = id || "";
        reproductor = rep;
        rep.editando = false;
        rep.tele = true;
    }
    // Primero el video vuelve a la Pi y después se cierra la vista de la tele
    function terminar() {
        if (reproductor) reproductor.tele = false;
        reproductor = null;
    }
    onHayChanged: if (!hay) terminar()

    // ---------- «Carreras»: cambiar de carrera como capítulos (en la Pi y en el celu) ----------
    // Las carreras que tienen video: {id, nombre, fecha, dia, vueltas, videos}. Solo con la tele en uso.
    readonly property var carreras: {
        if (!activa) return [];
        const r = [];
        for (const c of Datos.corridas.concat(Datos.proximas)) {
            if (!c.tieneVueltas) continue;
            const vs = Datos.videos(c.id);
            if (!vs.length) continue;
            // c.vueltas: la cuenta ya viene en el resumen (antes se armaba el informe entero de cada carrera) (04-3.7)
            r.push({ id: c.id, nombre: c.nombre, fecha: c.fechaLarga, dia: c.dia, vueltas: c.vueltas || 0, videos: vs.length });
        }
        return r;
    }
    // Cambia la tele a otra carrera: el informe de abajo pasa a esa carrera (Informe.qml se encarga del resto)
    function elegirCarrera(id) {
        if (!activa || !id || id === carreraId || Crono.enCurso) return;
        if (!carreras.some(c => c.id === id)) return;
        Navegacion.ir("informe", { id: id });
    }

    // ---------- El celu como control remoto (/control en «Betómetro en la red») ----------
    // Cómo está la tele, para la página del celu
    // Lo fijo (las vueltas) se arma una vez; antes se rehacía 10 veces por segundo con la posición del video (03-1.3)
    readonly property var vueltasTele: {
        const r = reproductor;
        if (!r) return [];
        r.videoActual;
        return r.vueltas.map((x, i) => {
            const m = r.momento(i, 0);
            return { nombre: r.nombreV(x), tiempo: Formato.tiempo(x.realMs || 0), desdeMs: m && m.v === r.videoActual ? Math.round(m.ms) : -1, video: !!m };
        });
    }
    readonly property var estado: {
        const r = reproductor;
        if (!r) return { activa: false };
        return { activa: true, carrera: carrera, carreraId: carreraId, vueltas: vueltasTele, vuelta: r.lapAhora,
                 pos: Math.round(r.arrastre >= 0 ? r.arrastre : r.pos), total: Math.round(r.total), andando: r.andando,
                 marcas: r.tramosVideo.map(t => Math.round(t.desde)) };
    }
    onEstadoChanged: ControlRemoto.ponerEstado(estado)
    onCarrerasChanged: ControlRemoto.ponerCarreras(carreras)
    // Lo que se tocó en el celu (ControlRemoto ya revisó que tenga sentido). Nunca en carrera.
    function hacer(tipo, valor) {
        const r = reproductor;
        if (!r || Crono.enCurso) return;
        switch (tipo) {
        case "play": if (!r.andando) r.alternar(); break;
        case "pausa": if (r.andando) r.alternar(); break;
        case "alternar": r.alternar(); break;
        case "saltar": r.saltar(valor); break;
        case "ir": r.irMs(r.videoActual, valor); break;
        case "vuelta": r.irA(valor, 0); break;
        case "carrera": elegirCarrera(valor); break;
        case "salir": terminar(); break;
        }
    }
    property Connections celu: Connections {
        target: ControlRemoto
        function onAccion(tipo, valor) { tele.hacer(tipo, valor); }
    }
}
