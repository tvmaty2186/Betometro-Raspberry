import QtQuick
import Betometro

// Ajustes → Cámara y micrófono. Muestra aprobada: docs/muestras/camara.html (#k1).
// Mientras está abierta: vista previa de la cámara y barra del micrófono.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var a: Datos.ajustes
    function poner(k, v) { Datos.ponerCfg(k, v); }

    Component.onCompleted: {
        Camara.detectar(); Camara.empezarVista(); Camara.empezarNivel();
        if (Contexto.prueba.indexOf("videos") === 0) videos.abrir();   // capturas: videos, videos-borrar
        if (Contexto.prueba === "videos-borrar" && Camara.videos.length) videos.tocarBorrar(Camara.videos[0].archivo);
        if (Contexto.prueba === "videos-ver") videos.verPrimero();
    }
    Component.onDestruction: { Camara.pararVista(); Camara.pararNivel(); }
    // Al terminar una grabación (Probar 10 s) se vuelve a prender la vista previa y la barra del micrófono:
    // antes quedaba en «Prendiendo la cámara…» (Matías, 05/11/2026)
    Connections { target: Camara; function onGrabandoChanged() { if (!Camara.grabando) reprender.restart(); } }
    Timer { id: reprender; interval: 600; onTriggered: { Camara.empezarVista(); Camara.empezarNivel(); } }

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "CÁMARA Y MICRÓFONO"; derechaRayas: raiz.h ? 90 : 30; onVolver: Navegacion.ir("ajustes") }

    Component {
        id: gVista
        Tarjeta {
            height: colV.height + 14
            Column {
                id: colV; width: parent.width; spacing: 8
                TituloTarjeta { texto: "Vista previa" }
                Row {
                    x: 22; spacing: 10
                    Rectangle { width: 14; height: 14; radius: 7; anchors.verticalCenter: parent.verticalCenter
                                color: Camara.hay ? Tema.bien : Tema.alerta }
                    Text { text: Camara.hay ? Camara.nombre : Camara.problema; color: Tema.tinta
                           font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22 } }
                }
                Rectangle {
                    x: 20; width: parent.width - 40; height: width * 9 / 16 * (raiz.h ? 0.86 : 1); radius: 12
                    color: "#05110a"; clip: true
                    border { width: 2; color: Tema.bordeCromo }
                    Image {
                        id: imgVista
                        anchors { fill: parent; margins: 2 }
                        source: Camara.vista; cache: false; asynchronous: false
                        fillMode: Image.PreserveAspectCrop
                        rotation: raiz.a.camaraGirar ? 180 : 0
                        transform: Scale { origin.x: imgVista.width / 2; origin.y: imgVista.height / 2; xScale: raiz.a.camaraEspejo ? -1 : 1 }   // espejo, como cámara de selfie
                        visible: Camara.viendo && Camara.vista.toString() !== ""
                    }
                    Text { anchors.centerIn: parent; visible: !Camara.viendo; color: Tema.tintaSuave
                           text: Camara.grabando ? "Grabando…" : Camara.hay ? "Prendiendo la cámara…" : "Conectá la cámara y tocá «Buscar»"
                           font { family: Tema.fuente600; pixelSize: 18 } }
                    Rectangle {
                        x: 12; y: 10; radius: 14; height: 28; width: tAviso.implicitWidth + 24; color: Qt.rgba(0, 0, 0, 0.55)
                        border { width: 1.5; color: "#c9ced3" }
                        Text { id: tAviso; anchors.centerIn: parent; text: "Solo para apuntarla · en carrera no se ve"; color: "#fff"
                               font { family: Tema.fuente600; italic: true; pixelSize: 16 } }
                    }
                }
                Row {
                    x: 20; spacing: 12
                    readonly property real ancho: (colV.width - 40 - 36) / 4
                    BotonCromo { width: parent.ancho; height: 46; tamTexto: 16; texto: "↻ Girar 180°"
                                 onTocado: raiz.poner("camaraGirar", !raiz.a.camaraGirar) }
                    BotonCromo { width: parent.ancho; height: 46; tamTexto: 16; texto: "⇋ Espejo" + (raiz.a.camaraEspejo ? " ✓" : "")
                                 onTocado: raiz.poner("camaraEspejo", !raiz.a.camaraEspejo) }
                    BotonCromo { width: parent.ancho; height: 46; tamTexto: 16; texto: Camara.grabando ? "Grabando 10 s…" : "Probar 10 s"
                                 activo: Camara.hay && !Camara.grabando; onTocado: Camara.probarGrabacion() }
                    BotonCromo { width: parent.ancho; height: 46; tamTexto: 16; texto: "Buscar"
                                 onTocado: { Camara.detectar(); Camara.empezarVista(); Camara.empezarNivel(); } }
                }
                Text { x: 22; visible: !!Camara.falla; text: Camara.falla; color: Tema.alerta; font { family: Tema.fuente600; pixelSize: 16 } }
            }
        }
    }
    Component {
        id: gMic
        Tarjeta {
            height: colM.height + 14
            Column {
                id: colM; width: parent.width; spacing: 8
                TituloTarjeta { texto: "Micrófono" }
                Fila { altoMinimo: 56; etiqueta: raiz.a.camaraSonido === "no" ? "Sin sonido (elegilo en «Sonido»)" : Camara.microfonos.length ? "Hablá y mirá la barra" : "No hay micrófono"
                    Row { spacing: 4
                        Repeater { model: 10
                            Rectangle { required property int index; width: 12; height: 22; radius: 3
                                color: Camara.nivel * 10 > index ? (index < 6 ? Tema.bien : index < 8 ? Tema.acento : Tema.alerta) : Qt.rgba(1, 1, 1, 0.18) } } } }
                BotonCromo {
                    x: 20; width: parent.width - 40; height: 46; tamTexto: 16
                    activo: raiz.a.camaraSonido !== "no" && Camara.microfonos.length > 0 && Camara.pruebaMic === ""
                    texto: Camara.pruebaMic === "grabando" ? "Grabando… hablá" : Camara.pruebaMic === "escuchando" ? "Escuchá por los auriculares…" : "● Grabar 5 s y escuchar"
                    onTocado: Camara.probarMicrofono()
                }
            }
        }
    }
    Component {
        id: gGrabacion
        Tarjeta {
            height: colG.height + 10
            Column {
                id: colG; width: parent.width
                TituloTarjeta { texto: "Grabación" }
                Fila { altoMinimo: 58; etiqueta: "Cámara"
                    Llave { prendida: !!raiz.a.camara; onCambiada: v => raiz.poner("camara", v) } }
                Fila { altoMinimo: 58; etiqueta: "Empieza"
                    Segmentado { opciones: [{ texto: "Al abrir la carrera", valor: "abrir" }, { texto: "En la largada", valor: "largada" }, { texto: "A mano", valor: "mano" }]
                        valor: raiz.a.camaraInicio || "abrir"; onElegido: v => raiz.poner("camaraInicio", v) } }
                Fila { altoMinimo: 58; etiqueta: "Termina"
                    Row { spacing: 10
                        BotonCromo { width: 46; height: 46; texto: "−"; tamTexto: 26; anchors.verticalCenter: parent.verticalCenter
                            onTocado: raiz.poner("camaraFinS", Math.max(0, raiz.a.camaraFinS - 5)) }
                        Campo { texto: raiz.a.camaraFinS + " s"; activo: false; opacity: 1 }
                        BotonCromo { width: 46; height: 46; texto: "+"; tamTexto: 26; anchors.verticalCenter: parent.verticalCenter
                            onTocado: raiz.poner("camaraFinS", Math.min(120, raiz.a.camaraFinS + 5)) }
                        Sub { anchors.verticalCenter: parent.verticalCenter; text: "después de «Terminar»"; tam: 13 } } }
                Fila { altoMinimo: 58; etiqueta: "Calidad"
                    Segmentado { opciones: [{ texto: "480p", valor: "480p" }, { texto: "720p", valor: "720p" }]
                        valor: raiz.a.camaraCalidad || "720p"; onElegido: v => raiz.poner("camaraCalidad", v) } }
                Fila { altoMinimo: 58; etiqueta: "Sonido"; ultima: true
                    Segmentado { opciones: [{ texto: "De la cámara", valor: "camara" }, { texto: "Micrófono USB", valor: "usb" }, { texto: "Sin sonido", valor: "no" }]
                        valor: raiz.a.camaraSonido || "camara"
                        onElegido: v => { raiz.poner("camaraSonido", v); Camara.pararNivel(); Camara.empezarNivel(); } } }
            }
        }
    }
    Component {
        id: gLugar
        Tarjeta {
            height: colL.height + 14
            Column {
                id: colL; width: parent.width; spacing: 6
                TituloTarjeta { texto: "Lugar en la tarjeta" }
                Rectangle {
                    x: 20; width: parent.width - 40; height: 16; radius: 8; color: "#0a2414"; border { width: 2; color: Tema.bordeCromo }
                    Rectangle { x: 2; y: 2; height: parent.height - 4; radius: 6; width: Math.max(height, (parent.width - 4) * (Camara.lugar.porcentaje || 0))
                                gradient: Gradient { GradientStop { position: 0; color: Tema.acentoClaro } GradientStop { position: 1; color: Tema.acento } } }
                }
                Item {
                    x: 20; width: parent.width - 40; height: 30
                    Text { anchors.verticalCenter: parent.verticalCenter; color: Tema.tintaSuave
                           font { family: Tema.fuente600; pixelSize: 17 }
                           text: "Usado: " + (Camara.lugar.usadoGB || "0") + " GB en " + (Camara.lugar.videos || 0) + " videos" }
                    Text { color: Tema.acento; anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                           font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 20 }
                           text: "Entran " + (Camara.lugar.horas || 0) + " h más" }
                }
                BotonCromo { x: 20; width: parent.width - 40; height: 46; tamTexto: 16
                             texto: "Ver videos" + (Camara.lugar.videos ? " (" + Camara.lugar.videos + ")" : "") + "  ›"
                             onTocado: videos.abrir() }
                Fila { altoMinimo: 58; etiqueta: "Borrar los videos más viejos si falta lugar"; ultima: true
                    Llave { prendida: raiz.a.camaraBorrarViejos !== false; onCambiada: v => raiz.poner("camaraBorrarViejos", v) } }
            }
        }
    }

    Flickable {
        id: lista
        x: raiz.h ? 36 : 24; y: 92
        width: parent.width - 2 * x; height: parent.height - y
        contentHeight: (raiz.h ? Math.max(izq.height, der.height) : unica.height) + 40
        clip: true; boundsBehavior: Flickable.StopAtBounds
        Column {
            id: izq; visible: raiz.h; width: raiz.h ? 560 : 0; spacing: 16; topPadding: 4
            Loader { width: parent.width; sourceComponent: raiz.h ? gVista : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gMic : null }
        }
        Column {
            id: der; visible: raiz.h; x: 584; width: raiz.h ? lista.width - 590 : 0; spacing: 16; topPadding: 4
            Loader { width: parent.width; sourceComponent: raiz.h ? gGrabacion : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gLugar : null }
        }
        Column {
            id: unica; visible: !raiz.h; width: lista.width; spacing: 16; topPadding: 4
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gVista }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gGrabacion }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gMic }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gLugar }
        }
    }

    ListaVideos { id: videos }
}
