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
