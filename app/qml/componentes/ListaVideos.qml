import QtQuick
import Betometro
import Betometro.Video

// Lista de los videos guardados (Ajustes → Cámara → «Ver videos»), del más nuevo al más viejo.
// Cada uno: carrera y fecha (o «Prueba» / «Sin carrera»), tamaño y «Borrar».
// Borrar pide confirmar: el primer toque lo pone rojo «¿Borrar?»; otro toque antes de 3 s lo borra.
// Nunca se borra mientras graba. Borra solo el video: la carrera y su informe quedan.
// grande: botones y letras del Modo Tablet.
Rectangle {
    id: raiz
    property bool grande: false
    property string confirmando: ""          // archivo que espera el segundo toque («__pruebas__» para los de prueba)
    readonly property bool h: Contexto.horizontal
    readonly property var lista: Camara.videos
    readonly property int pruebas: lista.filter(v => v.prueba).length
    readonly property real k: grande ? 1.45 : 1   // escala de letras y alturas
    function abrir() { confirmando = ""; visible = true; }
    function cerrar() { confirmando = ""; if (ver.visible) ver.cerrar(); visible = false; }
    function verPrimero() { if (lista.length) ver.abrir(lista[0]); }   // capturas
    function tocarBorrar(archivo) {
        if (Camara.grabando) return;
        if (confirmando !== archivo) { confirmando = archivo; vuelve.restart(); return; }
        confirmando = "";
        if (archivo === "__pruebas__") Camara.borrarPruebas();
        else if (!Camara.borrarVideo(archivo)) Navegacion.avisar("No se pudo borrar el video");
    }
    Timer { id: vuelve; interval: 3000; onTriggered: raiz.confirmando = "" }

    visible: false
    anchors.fill: parent; z: 80
    color: Qt.rgba(0, 0, 0, 0.6)
    MouseArea { anchors.fill: parent; onClicked: raiz.cerrar() }   // tocar afuera cierra

    Tarjeta {
        id: caja
        oscura: true
        anchors.centerIn: parent
        width: Math.min(parent.width - (raiz.h ? 60 : 32), raiz.grande ? 1100 : 900)
        height: parent.height - (raiz.h ? 40 : 120)
        MouseArea { anchors.fill: parent }   // que tocar adentro no cierre

        Row {   // como TituloTarjeta, más grande en el Modo Tablet
            id: titulo
            leftPadding: 22; topPadding: 16 * raiz.k; bottomPadding: 6; spacing: 12
            Banda { width: 30 * raiz.k; height: 14 * raiz.k; anchors.verticalCenter: parent.verticalCenter }
            Text { text: "Videos guardados"; color: Tema.tinta
                   font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 24 * raiz.k } }
        }
        BotonCromo {
            anchors { right: parent.right; rightMargin: 20; top: parent.top; topMargin: 14 }
            width: 130 * raiz.k; height: 46 * raiz.k; tamTexto: 18 * raiz.k; texto: "Cerrar"
            onTocado: raiz.cerrar()
        }
        Text {
            id: resumen
            x: 22; anchors.top: titulo.bottom; anchors.topMargin: 6 * raiz.k
            width: parent.width - 44; wrapMode: Text.WordWrap
            color: Camara.grabando ? Tema.alerta : Tema.tintaSuave
            font { family: Tema.fuente600; pixelSize: 17 * raiz.k }
            text: Camara.grabando ? "Está grabando: para borrar, esperá a que termine"
                  : raiz.lista.length ? "Usado: " + (Camara.lugar.usadoGB || "0") + " GB en " + raiz.lista.length + (raiz.lista.length === 1 ? " video" : " videos")
                                        + " · entran " + (Camara.lugar.horas || 0) + " h más"
                  : "No hay videos guardados"
        }

        Flickable {
            id: desliza
            x: 20; width: parent.width - 40
            anchors { top: resumen.bottom; topMargin: 12 * raiz.k; bottom: pie.top; bottomMargin: 10 }
            contentHeight: col.height
            clip: true; boundsBehavior: Flickable.StopAtBounds
            Column {
                id: col; width: parent.width; spacing: 8 * raiz.k
                Repeater {
                    model: raiz.lista
                    Rectangle {
                        id: fila
                        required property var modelData
                        readonly property bool pide: raiz.confirmando === modelData.archivo
                        width: col.width; height: 70 * raiz.k; radius: Math.max(8, Tema.radio - 2)
                        color: Qt.rgba(1, 1, 1, 0.06)
                        border { width: 1.5; color: Tema.bordeCromoOscuro }
                        Column {
                            anchors { left: parent.left; leftMargin: 16; right: botonVer.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
                            spacing: 2
                            Text {
                                width: parent.width; elide: Text.ElideRight; color: Tema.tinta
                                font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 22 * raiz.k }
                                text: fila.modelData.carrera ? fila.modelData.carrera + (fila.modelData.fecha ? " · " + fila.modelData.fecha : "")
                                      : fila.modelData.prueba ? "Prueba" : "Sin carrera"
                            }
                            Text {
                                width: parent.width; elide: Text.ElideRight
                                color: fila.modelData.grabando ? Tema.alerta : Tema.tintaSuave
                                font { family: Tema.fuente600; pixelSize: 16 * raiz.k }
                                text: (fila.modelData.grabando ? "● Grabando ahora · " : "Grabado " + fila.modelData.hora + " · ") + fila.modelData.tam
                            }
                        }
                        // ▶ Ver: para mirar cómo quedó (por ejemplo los «Probar 10 s»). Matías, 06/11/2026
                        BotonBorrar {
                            id: botonVer
                            anchors { right: boton.left; rightMargin: 10; verticalCenter: parent.verticalCenter }
                            width: (raiz.grande ? 150 : 110)
                            texto: "▶ Ver"
                            activo: !fila.modelData.grabando
                            onTocado: { raiz.confirmando = ""; ver.abrir(fila.modelData); }
                        }
                        BotonBorrar {
                            id: boton
                            anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                            pide: fila.pide
                            activo: !Camara.grabando
                            onTocado: raiz.tocarBorrar(fila.modelData.archivo)
                        }
                    }
                }
            }
        }

        // Abajo: borrar todas las pruebas juntas
        Item {
            id: pie
            x: 20; width: parent.width - 40
            anchors { bottom: parent.bottom; bottomMargin: 16 }
            height: raiz.pruebas > 0 ? 54 * raiz.k : 0
            visible: raiz.pruebas > 0
            BotonBorrar {
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                width: Math.min(parent.width, 380 * raiz.k)
                texto: "Borrar los de prueba (" + raiz.pruebas + ")"
                pide: raiz.confirmando === "__pruebas__"
                activo: !Camara.grabando
                onTocado: raiz.tocarBorrar("__pruebas__")
            }
        }
    }

    // Mirar un video: a pantalla completa, tocar = pausa/sigue, barra para ir a cualquier parte
    Rectangle {
        id: ver
        property var video: null
        function abrir(v) {
            video = v;
            // Girar y espejo: los que se guardaron con el video de la carrera; si no (pruebas), los de ahora
            let g = !!Datos.ajustes.camaraGirar, e = !!Datos.ajustes.camaraEspejo;
            if (v.carreraId) for (const x of Datos.videos(v.carreraId)) if (x.archivo === v.archivo) { g = !!x.girar; e = !!x.espejo; }
            girar = g; espejo = e;
            visible = true;
            reproductor.play();
        }
        function cerrar() { reproductor.stop(); visible = false; video = null; }
        property bool girar: false
        property bool espejo: false
        visible: false
        anchors.fill: parent; z: 10
        color: "#000000"
        VideoGst {
            id: reproductor
            source: ver.video ? Camara.urlVideo(ver.video.archivo) : ""
            videoOutput: imagen
        }
        ImagenVideo {
            id: imagen
            anchors.fill: parent
            rotation: ver.girar ? 180 : 0
            transform: Scale { origin.x: imagen.width / 2; xScale: ver.espejo ? -1 : 1 }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: reproductor.playbackState === 1 ? reproductor.pause() : reproductor.play()
        }
        // Arriba: qué video y Cerrar
        Rectangle {
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: 76; color: Qt.rgba(0, 0, 0, 0.55)
            Text {
                anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter; right: cerrarVer.left; rightMargin: 12 }
                elide: Text.ElideRight; color: Tema.tinta
                font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 28 }
                text: !ver.video ? "" : (ver.video.carrera || (ver.video.prueba ? "Prueba" : "Sin carrera")) + " · " + ver.video.hora
            }
            BotonBorrar { id: cerrarVer; anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                          texto: "Cerrar"; onTocado: ver.cerrar() }
        }
        // Abajo: play/pausa, tiempo y barra
        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 84; color: Qt.rgba(0, 0, 0, 0.55)
            Text {
                id: icono; x: 24; anchors.verticalCenter: parent.verticalCenter
                text: reproductor.playbackState === 1 ? "❚❚" : "▶"; color: Tema.tinta
                font { family: Tema.fuente800; pixelSize: 34 }
                MouseArea { anchors.fill: parent; anchors.margins: -16; onClicked: reproductor.playbackState === 1 ? reproductor.pause() : reproductor.play() }
            }
            Text {
                id: tiempo; anchors { left: icono.right; leftMargin: 20; verticalCenter: parent.verticalCenter }
                color: Tema.tinta; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 26 }
                text: Formato.horaVideo(reproductor.position) + " / " + Formato.horaVideo(reproductor.duration)
            }
            Item {
                id: linea
                anchors { left: tiempo.right; leftMargin: 24; right: parent.right; rightMargin: 28; verticalCenter: parent.verticalCenter }
                height: 44
                Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 8; radius: 4; color: Qt.rgba(1, 1, 1, 0.25) }
                Rectangle { anchors.verticalCenter: parent.verticalCenter; height: 8; radius: 4; color: Tema.acento
                            width: reproductor.duration > 0 ? parent.width * reproductor.position / reproductor.duration : 0 }
                MouseArea {
                    anchors.fill: parent
                    function ir(mx) { if (reproductor.duration > 0) reproductor.position = Math.max(0, Math.min(1, mx / width)) * reproductor.duration; }
                    onPressed: m => ir(m.x)
                    onPositionChanged: m => ir(m.x)
                }
            }
        }
    }

    // Botón «Borrar» que se pone rojo «¿Borrar?» para confirmar
    component BotonBorrar: Rectangle {
        id: b
        property string texto: "Borrar"
        property bool pide: false
        property bool activo: true
        signal tocado()
        width: (raiz.grande ? 190 : 130); height: 48 * raiz.k
        radius: Math.max(6, Tema.radio - 4)
        opacity: activo ? 1 : 0.45
        color: pide ? "#c62a1f" : (zona.pressed ? Qt.lighter(Tema.caja1, 1.4) : Qt.lighter(Tema.caja1, 1.15))
        border { width: 2; color: pide ? "#ffb3aa" : Tema.bordeCromo }
        Text {
            anchors.centerIn: parent
            text: b.pide ? "¿Borrar?" : b.texto
            color: b.pide ? "#ffffff" : Tema.tinta
            font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 18 * raiz.k }
        }
        MouseArea { id: zona; anchors.fill: parent; enabled: b.activo; onClicked: b.tocado() }
    }
}
