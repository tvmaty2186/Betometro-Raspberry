pragma Singleton
import QtQuick

// Modo tele (docs/video.md): con una tele en el HDMI, el video de la carrera se ve grande en la tele
// y la pantalla de la Pi queda de control remoto. Un solo reproductor (el del informe): cuando está en
// la tele, su salida de video es la de la ventana de la tele (el video se decodifica una sola vez).
QtObject {
    id: tele
    // Qt ve una segunda pantalla. En la Raspberry, además, el sistema tiene que decir que la tele sigue
    // enchufada: si se desenchufa, la ventana se cierra (Qt no se entera solo).
    readonly property bool hay: Qt.application.screens.length > 1 && (!Hdmi || !Hdmi.vigila || Hdmi.conectada)
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
    property Item reproductor: null     // el Reproductor que se ve en la tele (null: la tele en reposo)
    property Item salida: null          // el VideoOutput de la ventana de la tele
    property string carrera: ""
    readonly property bool activa: !!reproductor

    function empezar(rep, nombre) {
        if (!hay || !rep) return;
        if (reproductor && reproductor !== rep) terminar();
        carrera = nombre || "";
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
}
