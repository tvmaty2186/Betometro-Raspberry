import QtQuick
import Betometro

// Pantalla 10: Informe de una carrera. Muestras #s10 y #v10.
// Los datos salen del mismo cálculo que la app (datosInforme): recorrido común, velocidad, aceleración y adelanto.
// Las piezas están en pantallas/informe/ (1.0.30, auditoría 01-1.4): cabecera, mapa, gráfico de velocidad,
// tabla de vueltas, recorrido libre y video. Cada una recibe lo que muestra por propiedades y avisa con señales;
// acá queda el armado: leer Datos, qué vuelta está elegida, el video y la tele.
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
    readonly property Item reproductor: video.reproductor
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
        tabla.limpiar(); completo = false;
        Tele.carrera = c.nombre || "";
        Tele.carreraId = id_;
        lap = conTraza.length ? conTraza[0] : -1;
        mapa.repintar(); grafico.repintar();
        reproductor.deNuevo();
        // A la primera vuelta que tiene video
        for (let i = 0; i < vueltas.length; i++) if (reproductor.irA(i, 0)) return;
        reproductor.irMs(0, 0);
    }
    readonly property var conTraza: vueltas.map((v, i) => v.m ? i : -1).filter(i => i >= 0)
    property int lap: conTraza.length ? conTraza[0] : -1
    property string modo: "vel"      // vel, acel, ade
    function restablecer() {   // ya confirmado en la tabla
        if (Datos.restablecer(id_)) { rev++; tabla.fila = -1; lap = conTraza.length ? conTraza[0] : -1; Navegacion.avisar("Carrera restablecida"); }
    }
    readonly property var vu: lap >= 0 ? vueltas[lap] : null
    readonly property real dist: inf.distanciaM || 0
    readonly property real sug: dist > 0 ? dist / (inf.idealMs / 1000) * 3.6 : 0

    function cambiarMedida(i) {
        const l = lap;
        if (Datos.cambiarMedida(id_, i)) { rev++; lap = l; }
    }
    // Una marca de más parte una vuelta en dos: se juntan desde la tabla (ya confirmado)
    function unir(i) {
        if (Datos.unirConSiguiente(id_, i)) {
            rev++; tabla.fila = -1;
            lap = vueltas[i] && vueltas[i].m ? i : (conTraza.length ? conTraza[0] : -1);
            Navegacion.avisar("Vueltas unidas");
        }
    }
    // Con el video andando, la vuelta elegida sigue al video
    Connections {
        target: raiz.reproductor
        function onLapAhoraChanged() { const i = raiz.reproductor.lapAhora; if (i >= 0 && raiz.vueltas[i] && raiz.vueltas[i].m && i !== raiz.lap) raiz.lap = i; }
    }

    Fondo { anchors.fill: parent }

    // Cabecera: volver, león, nombre y datos; 101 y Compartir
    CabeceraInforme {
        h: raiz.h; c: raiz.c; rec: raiz.rec; marcas: raiz.marcas; vueltas: raiz.vueltas; dist: raiz.dist
        conVideo: raiz.videos.length > 0; verVideo: raiz.verVideo; botonTele: raiz.botonTele; enTele: raiz.enTele
        numero: Datos.ajustes.numeroAuto || "101"
        onVolver: Navegacion.ir("ficha", { id: raiz.id_ })
        onVerEnTele: raiz.verEnTele()
        onElegirVideo: (v) => raiz.verVideo = v
        onCompartir: Navegacion.ir("compartir", { id: raiz.id_ })
    }

    // Mapa
    MapaInforme {
        id: mapa
        h: raiz.h; vv: raiz.vv; inf: raiz.inf; vu: raiz.vu; rec: raiz.rec; marcas: raiz.marcas
        dist: raiz.dist; sug: raiz.sug; lap: raiz.lap; modo: raiz.modo
        auto: raiz.reproductor.e
        onElegirModo: (m) => raiz.modo = m
        onIrVideo: (i, t) => raiz.irVideo(i, t)
    }

    // Velocidad de la vuelta elegida, a lo largo de la distancia
    GraficoVelocidad {
        id: grafico
        h: raiz.h; vv: raiz.vv; rec: raiz.rec; inf: raiz.inf; vu: raiz.vu; lap: raiz.lap; marcas: raiz.marcas; sug: raiz.sug
    }

    // Todas las vueltas
    TablaVueltas {
        id: tabla
        visible: !raiz.rec
        h: raiz.h; vv: raiz.vv; c: raiz.c; vueltas: raiz.vueltas; inf: raiz.inf; lap: raiz.lap
        reproductor: raiz.reproductor
        onVerVuelta: (i) => { if (raiz.vueltas[i].m) raiz.lap = i; }
        onIrVideo: (i, t) => raiz.irVideo(i, t)
        onCambiarMedida: (i) => raiz.cambiarMedida(i)
        onUnir: (i) => raiz.unir(i)
        onRestablecer: raiz.restablecer()
        onCompartir: Navegacion.ir("compartir", { id: raiz.id_ })
    }
    // Recorrido libre: resumen y marcas (muestra docs/muestras/recorrido/3-informe.png)
    RecorridoLibre {
        visible: raiz.rec
        h: raiz.h; inf: raiz.inf; marcas: raiz.marcas; conVideo: raiz.videos.length > 0
        onIrMarca: (mk) => raiz.irMarca(mk)
    }
    // Video a medias (arriba a la izquierda, con «En este momento») o en pantalla completa
    VideoInforme {
        id: video
        h: raiz.h; vv: raiz.vv; completo: raiz.completo; videos: raiz.videos; inf: raiz.inf; sug: raiz.sug; dist: raiz.dist
        onDobleToque: raiz.completo = !raiz.completo
    }

    // Modo tele: esta pantalla es el control remoto
    ControlTele {
        id: control
        visible: raiz.enTele
        anchors.fill: parent
        z: 70
        rep: raiz.reproductor
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
    MouseArea { anchors.fill: parent; visible: tabla.menuAbierto; onClicked: tabla.cerrarMenu() }
    Timer {   // solo para capturas: «Distancia» a medio mover, pegada a «Vuelta»
        id: tPruebaMover; interval: 400
        onTriggered: { const d = raiz.reproductor.datos[3]; d.moviendo = true; const r = raiz.reproductor.imantar(d, 470, 92); d.x = r.x; d.y = r.y; }
    }
    Timer {   // solo para capturas: lo mismo que el pulsador por cable (dos apretones juntos cuentan uno)
        id: tPruebaPulsador; interval: 600
        onTriggered: { raiz.reproductor.pulsador(); raiz.reproductor.pulsador(); }
    }
    Timer {   // solo para capturas: en la tele el video carga después; se vuelve a pedir la vuelta
        id: tPruebaTele; interval: 1500
        onTriggered: raiz.reproductor.irA(raiz.lap, raiz.vueltas.length > 20 ? 5 : 134.6)
    }
    Component.onCompleted: {   // solo para capturas
        // con muchas vueltas (BETO_MUESTRA_LARGA), una de las últimas
        const lapP = vueltas.length > 20 ? vueltas.length - 15 : 2, tP = vueltas.length > 20 ? 5 : 134.6;
        if (Contexto.prueba === "libre") cambiarMedida(1);
        else if (Contexto.prueba === "unir") tabla.fila = 3;
        else if (Contexto.prueba === "menu") tabla.menuAbierto = true;
        else if (Contexto.prueba === "cortado") verVideo = true;   // con --prueba cortado: el aviso del video cortado
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
