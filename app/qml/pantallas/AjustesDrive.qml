import QtQuick
import Betometro

// Ajustes → Google Drive (docs/google-drive.md). A la izquierda la cuenta (conectar con un código desde
// el celu) y cómo va la subida; a la derecha qué se sube y con qué wifi.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var a: Datos.ajustes
    readonly property var wifis: a.driveWifis || []
    // Página para subir el cliente de Google desde la compu
    readonly property string urlPagina: Wifi.direccion ? "http://" + Wifi.direccion + (Servidor.puerto !== 80 ? ":" + Servidor.puerto : "") + "/drive" : ""
    function poner(k, v) { Datos.ponerCfg(k, v); }
    function elegirWifi(nombre, si) {
        const l = raiz.wifis.filter(w => w !== nombre);
        if (si) l.push(nombre);
        poner("driveWifis", l);
    }

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "GOOGLE DRIVE"; derechaRayas: raiz.h ? 90 : 30; onVolver: Navegacion.ir("ajustes") }

    Component {
        id: gCuenta
        Tarjeta {
            height: colC.height + 18
            Column {
                id: colC; width: parent.width; spacing: 10
                TituloTarjeta { texto: "Tu cuenta" }

                // ---- Falta el cliente de Google: se sube desde la compu
                Row {
                    visible: Drive.estado === "falta"
                    x: 22; spacing: 20
                    Rectangle {
                        visible: raiz.urlPagina !== ""
                        width: 150; height: 150; radius: 14; color: "#fff"
                        Image { anchors.centerIn: parent; width: 136; height: 136; smooth: false
                                source: raiz.urlPagina ? "image://qr/" + encodeURIComponent(raiz.urlPagina) : "" }
                    }
                    Column {
                        width: colC.width - 44 - (raiz.urlPagina ? 170 : 0); spacing: 6
                        anchors.verticalCenter: parent.verticalCenter
                        Text { width: parent.width; wrapMode: Text.WordWrap; text: "Falta un paso: desde la compu abrí"
                               color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 24 } }
                        Text { width: parent.width; wrapMode: Text.WrapAnywhere; text: raiz.urlPagina || "Conectá el Betómetro a la wifi"
                               color: Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                        Text { width: parent.width; wrapMode: Text.WordWrap; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 17 }
                               text: "y subí el archivo de Google (los pasos están ahí y en la guía)" }
                    }
                }

                // ---- Sin conectar
                Column {
                    visible: Drive.estado === "desconectado" || Drive.estado === "pidiendo"
                    x: 22; width: parent.width - 44; spacing: 14
                    Text { width: parent.width; wrapMode: Text.WordWrap; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 }
                           text: "Conectá tu Google Drive y los informes y videos se suben solos a la carpeta «Betómetro»." }
                    BotonAmarillo { width: parent.width; height: 64; tamTexto: 24; activo: Drive.estado !== "pidiendo"
                                    texto: Drive.estado === "pidiendo" ? "Pidiendo el código…" : "Conectar"; onTocado: Drive.conectar() }
                }

                // ---- El código para el celu: grande
                Row {
                    visible: Drive.estado === "codigo"
                    x: 22; spacing: 20
                    Rectangle {
                        width: 150; height: 150; radius: 14; color: "#fff"
                        Image { anchors.centerIn: parent; width: 136; height: 136; smooth: false
                                source: Drive.estado === "codigo" ? "image://qr/" + encodeURIComponent(Drive.enlace) : "" }
                    }
                    Column {
                        width: colC.width - 44 - 170; spacing: 4
                        anchors.verticalCenter: parent.verticalCenter
                        Text { width: parent.width; wrapMode: Text.WordWrap; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 18 }
                               text: "En el celu abrí" }
                        Text { text: Drive.enlace.replace("https://", "").replace("www.", ""); color: Tema.acento
                               font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                        Text { color: Tema.tintaSuave; text: "y escribí este código:"; font { family: Tema.fuente600; pixelSize: 18 } }
                    }
                }
                Rectangle {
                    visible: Drive.estado === "codigo"
                    x: 22; width: parent.width - 44; height: 96; radius: 16
                    color: Qt.rgba(0, 0, 0, 0.35); border { width: 2; color: Tema.acento }
                    Text { anchors.centerIn: parent; text: Drive.codigo; color: Tema.tinta
                           font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 68; letterSpacing: 6 } }
                }
                Item {
                    visible: Drive.estado === "codigo"
                    x: 22; width: parent.width - 44; height: 46
                    Text { anchors.verticalCenter: parent.verticalCenter; width: parent.width - 170; wrapMode: Text.WordWrap
                           text: "Elegí tu cuenta y tocá «Continuar»"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 17 } }
                    BotonCromo { anchors.right: parent.right; width: 150; height: 46; tamTexto: 16; texto: "Cancelar"; onTocado: Drive.cancelar() }
                }

                // ---- Conectado
                Column {
                    visible: Drive.estado === "conectado"
                    x: 22; width: parent.width - 44; spacing: 12
                    Row {
                        spacing: 12
                        Rectangle { width: 14; height: 14; radius: 7; anchors.verticalCenter: parent.verticalCenter; color: Tema.bien }
                        Text { width: colC.width - 70; elide: Text.ElideMiddle; color: Tema.tinta
                               text: "Conectado como " + (Drive.cuenta || "…")
                               font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22 } }
                    }
                    Text { width: parent.width; wrapMode: Text.WordWrap; text: Drive.texto; visible: Drive.texto !== ""
                           color: Drive.listo ? Tema.bien : Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 24 } }
                    Rectangle {
                        visible: Drive.subiendo
                        width: parent.width; height: 16; radius: 8; color: "#0a2414"; border { width: 2; color: Tema.bordeCromo }
                        Rectangle { x: 2; y: 2; height: parent.height - 4; radius: 6; width: Math.max(height, (parent.width - 4) * Drive.progreso)
                                    gradient: Gradient { GradientStop { position: 0; color: Tema.acentoClaro } GradientStop { position: 1; color: Tema.acento } } }
                    }
                    Row {
                        spacing: 12
                        BotonCromo { width: (colC.width - 44 - 12) / 2; height: 48; tamTexto: 17; texto: "Subir ahora"
                                     activo: !Drive.subiendo; onTocado: Drive.revisar() }
                        DobleToque { width: (colC.width - 44 - 12) / 2; height: 48; textoNormal: "Desconectar"; onConfirmado: Drive.desconectar() }
                    }
                }

                // ---- El último problema, en palabras
                Text { visible: Drive.error !== ""; x: 22; width: parent.width - 44; wrapMode: Text.WordWrap
                       text: "⚠ " + Drive.error; color: Tema.alerta; font { family: Tema.fuente800; pixelSize: 20 } }
            }
        }
    }
    Component {
        id: gQue
        Tarjeta {
            height: colQ.height + 10
            Column {
                id: colQ; width: parent.width
                TituloTarjeta { texto: "Qué se sube" }
                Fila { altoMinimo: 62; etiqueta: "Subir informes"
                    Llave { prendida: raiz.a.driveInformes !== false; onCambiada: v => raiz.poner("driveInformes", v) } }
                Fila { altoMinimo: 62; etiqueta: "Subir videos"
                    Llave { prendida: raiz.a.driveVideos !== false; onCambiada: v => raiz.poner("driveVideos", v) } }
                Fila { altoMinimo: 62; etiqueta: "Borrar del equipo los videos ya subidos"; ultima: true
                    Llave { prendida: !!raiz.a.driveBorrarSubidos; onCambiada: v => raiz.poner("driveBorrarSubidos", v) } }
            }
        }
    }
    Component {
        id: gWifi
        Tarjeta {
            height: colW.height + 14
            Column {
                id: colW; width: parent.width
                TituloTarjeta { texto: "Videos: solo con estas wifi" }
                Text { x: 22; width: parent.width - 44; wrapMode: Text.WordWrap; bottomPadding: 6
                       color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 16 }
                       text: raiz.wifis.length ? "Una hora de video son unos 2 GB: con el celu se gastan los datos."
                                               : "Ninguna elegida: los videos no se suben. Elegí la wifi de casa." }
                Repeater {
                    model: Wifi.guardadas
                    Fila {
                        required property var modelData
                        required property int index
                        altoMinimo: 58; ultima: index === Wifi.guardadas.length - 1
                        etiqueta: modelData.nombre + (modelData.conectada ? "  ·  conectada" : "")
                        Llave { prendida: raiz.wifis.indexOf(modelData.nombre) >= 0; onCambiada: v => raiz.elegirWifi(modelData.nombre, v) }
                    }
                }
                Text { visible: !Wifi.guardadas.length; x: 22; topPadding: 6; bottomPadding: 6; text: "No hay wifi guardadas (Ajustes → Conectividad)"
                       color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 18 } }
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
            id: izq; visible: raiz.h; width: raiz.h ? 600 : 0; spacing: 16; topPadding: 4
            Loader { width: parent.width; sourceComponent: raiz.h ? gCuenta : null }
        }
        Column {
            id: der; visible: raiz.h; x: 624; width: raiz.h ? lista.width - 630 : 0; spacing: 16; topPadding: 4
            Loader { width: parent.width; sourceComponent: raiz.h ? gQue : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gWifi : null }
        }
        Column {
            id: unica; visible: !raiz.h; width: lista.width; spacing: 16; topPadding: 4
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gCuenta }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gQue }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gWifi }
        }
    }
}
