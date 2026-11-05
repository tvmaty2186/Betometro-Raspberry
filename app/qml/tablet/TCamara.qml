import QtQuick
import Betometro

// Cámara en Modo Tablet. Muestra: docs/muestras/tablet/camara-h.png
// Vista previa grande (solo para apuntarla) y botones grandes. Cada botón con opciones las va pasando al tocarlo.
TPantalla {
    id: raiz
    titulo: "Cámara"
    volverA: Navegacion.parametros && Navegacion.parametros.desde === "ajustes" ? "ajustes" : "inicio"
    readonly property var a: Datos.ajustes
    function poner(k, v) { Datos.ponerCfg(k, v); }
    function siguiente(lista, actual) { const i = lista.indexOf(actual); return lista[(i + 1) % lista.length]; }
    Component.onCompleted: { Camara.detectar(); Camara.empezarVista(); Camara.empezarNivel(); }
    Component.onDestruction: { Camara.pararVista(); Camara.pararNivel(); }

    // ---- Vista previa
    Rectangle {
        id: vista
        width: raiz.h ? raiz.area.width - 478 - 18 : raiz.area.width
        height: raiz.h ? raiz.area.height : width * 9 / 16 + 20
        radius: Math.max(14, Tema.radio * 1.5)
        color: "#05110a"; clip: true
        border { width: 3; color: Tema.bordeCromo }
        Image {
            anchors { fill: parent; margins: 3 }
            source: Camara.vista; cache: false; asynchronous: false
            fillMode: Image.PreserveAspectCrop
            rotation: raiz.a.camaraGirar ? 180 : 0
            visible: Camara.viendo && Camara.vista.toString() !== ""
        }
        Column {
            visible: !Camara.viendo
            anchors.centerIn: parent; spacing: 22
            Text { anchors.horizontalCenter: parent.horizontalCenter; color: Tema.tinta
                   text: Camara.grabando ? "Grabando…" : Camara.hay ? "Prendiendo la cámara…" : (Camara.problema || "No se encontró la cámara")
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
            Rectangle {
                visible: !Camara.hay
                anchors.horizontalCenter: parent.horizontalCenter
                width: 260; height: 76; radius: 22; color: Tema.acento
                Row { anchors.centerIn: parent; spacing: 12
                    TIcono { width: 36; height: 36; nombre: "buscar"; color: Tema.acentoTinta; anchors.verticalCenter: parent.verticalCenter }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: "Buscar"; color: Tema.acentoTinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 } } }
                MouseArea { anchors.fill: parent; onClicked: { Camara.detectar(); Camara.empezarVista(); Camara.empezarNivel(); } }
            }
        }
        Rectangle {   // estado arriba
            x: 16; y: 14; height: 50; radius: 25; width: est.implicitWidth + 50
            color: Qt.rgba(0, 0, 0, 0.6); border { width: 2; color: "#c9ced3" }
            Rectangle { x: 18; anchors.verticalCenter: parent.verticalCenter; width: 16; height: 16; radius: 8; color: Camara.hay ? Tema.bien : Tema.alerta }
            Text { id: est; x: 42; anchors.verticalCenter: parent.verticalCenter; color: "#fff"
                   text: Camara.hay ? Camara.nombre.split(" · ").slice(-2).join(" · ") : "Sin cámara"
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22 } }
        }
        Rectangle {   // lugar abajo
            x: 16; anchors.bottom: parent.bottom; anchors.bottomMargin: 14; height: 50; radius: 25; width: lug.implicitWidth + 36
            color: Qt.rgba(0, 0, 0, 0.6); border { width: 2; color: "#c9ced3" }
            Text { id: lug; anchors.centerIn: parent; color: "#fff"; text: "Entran " + (Camara.lugar.horas || 0) + " h más"
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22 } }
        }
        Text { visible: !!Camara.falla; anchors { right: parent.right; rightMargin: 16; bottom: parent.bottom; bottomMargin: 26 }
               text: Camara.falla; color: Tema.alerta; font { family: Tema.fuente800; pixelSize: 22 } }
    }

    // ---- Botones
    Grid {
        x: raiz.h ? vista.width + 18 : 0
        y: raiz.h ? 0 : vista.height + 18
        width: raiz.h ? 478 : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - y
        columns: 2; spacing: 16
        readonly property real an: (width - spacing) / 2
        readonly property real al: (height - 2 * spacing) / 3
        TBoton { width: parent.an; height: parent.al; tamIcono: 60; icono: "rec"; nombre: "Grabar"
                 tipo: raiz.a.camara ? "acento" : ""; dato: raiz.a.camara ? "Prendida" : "Apagada"
                 onTocado: raiz.poner("camara", !raiz.a.camara) }
        TBoton { width: parent.an; height: parent.al; tamIcono: 60; icono: "bandera"; nombre: "Empieza"
                 dato: ({ abrir: "Al abrir", largada: "En la largada", mano: "A mano" })[raiz.a.camaraInicio || "abrir"]
                 onTocado: raiz.poner("camaraInicio", raiz.siguiente(["abrir", "largada", "mano"], raiz.a.camaraInicio || "abrir")) }
        TBoton { width: parent.an; height: parent.al; tamIcono: 60; icono: "pantalla"; nombre: "Calidad"; dato: raiz.a.camaraCalidad || "720p"
                 onTocado: raiz.poner("camaraCalidad", (raiz.a.camaraCalidad || "720p") === "720p" ? "1080p" : "720p") }
        TBoton { width: parent.an; height: parent.al; tamIcono: 60; icono: "mic"; nombre: "Sonido"
                 dato: ({ camara: "De la cámara", usb: "Micrófono USB", no: "Sin sonido" })[raiz.a.camaraSonido || "camara"]
                 onTocado: { raiz.poner("camaraSonido", raiz.siguiente(["camara", "usb", "no"], raiz.a.camaraSonido || "camara")); Camara.pararNivel(); Camara.empezarNivel(); } }
        TBoton { width: parent.an; height: parent.al; tamIcono: 60; icono: "crono"; nombre: Camara.grabando ? "Grabando…" : "Probar 10 s"
                 activo: Camara.hay && !Camara.grabando; onTocado: Camara.probarGrabacion() }
        TBoton { width: parent.an; height: parent.al; tamIcono: 60; icono: "girar"; nombre: "Girar 180°"; dato: raiz.a.camaraGirar ? "Girada" : ""
                 onTocado: raiz.poner("camaraGirar", !raiz.a.camaraGirar) }
    }
}
