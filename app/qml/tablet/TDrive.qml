import QtQuick
import Betometro

// Google Drive en Modo Tablet. Misma lógica que AjustesDrive.qml: a la izquierda la cuenta (código
// bien grande para escribirlo en el celu) y a la derecha filas grandes con qué se sube y con qué wifi.
TPantalla {
    id: raiz
    titulo: "Google Drive"
    volverA: "ajustes"
    readonly property var a: Datos.ajustes
    readonly property var wifis: a.driveWifis || []
    readonly property string urlPagina: Wifi.direccion ? "http://" + Wifi.direccion + (Servidor.puerto !== 80 ? ":" + Servidor.puerto : "") + "/drive" : ""
    function poner(k, v) { Datos.ponerCfg(k, v); }
    function elegirWifi(nombre, si) {
        const l = raiz.wifis.filter(w => w !== nombre);
        if (si) l.push(nombre);
        poner("driveWifis", l);
    }

    // ---- La cuenta
    Rectangle {
        id: cuenta
        width: raiz.h ? 600 : raiz.area.width
        height: raiz.h ? raiz.area.height : 470
        radius: Math.max(14, Tema.radio * 1.5)
        border { width: 3; color: Tema.bordeCromo }
        gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }

        // Falta el cliente de Google
        Column {
            visible: Drive.estado === "falta"
            anchors.centerIn: parent; width: parent.width - 48; spacing: 16
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; text: "Falta un paso: desde la compu abrí"
                   color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 32 } }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 18
                   text: raiz.urlPagina || "Conectá el Betómetro a la wifi"
                   color: Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 36 } }
            // El código que pide la página (que nadie de la wifi cambie el cliente sin ver esta pantalla)
            Text { visible: raiz.urlPagina !== ""; width: parent.width; horizontalAlignment: Text.AlignHCenter
                   text: "Código: <b>" + Servidor.codigoDrive + "</b>"; textFormat: Text.StyledText
                   color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30; letterSpacing: 2 } }
            Rectangle {
                visible: raiz.urlPagina !== ""
                anchors.horizontalCenter: parent.horizontalCenter; width: 230; height: 230; radius: 16; color: "#fff"
                Image { anchors.centerIn: parent; width: 210; height: 210; smooth: false
                        source: raiz.urlPagina ? "image://qr/" + encodeURIComponent(raiz.urlPagina) : "" }
            }
        }

        // Sin conectar
        Column {
            visible: Drive.estado === "desconectado" || Drive.estado === "pidiendo"
            anchors.centerIn: parent; width: parent.width - 48; spacing: 26
            TIcono { anchors.horizontalCenter: parent.horizontalCenter; width: 110; height: 110; nombre: "nube"; color: Tema.tinta }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: Tema.tinta
                   text: "Los informes y videos se suben solos a tu Drive"; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 } }
            TBoton { width: parent.width; height: 110; tipo: "acento"; tamNombre: 36; activo: Drive.estado !== "pidiendo"
                     nombre: Drive.estado === "pidiendo" ? "Pidiendo el código…" : "Conectar"; onTocado: Drive.conectar() }
        }

        // El código para el celu
        Column {
            visible: Drive.estado === "codigo"
            anchors.centerIn: parent; width: parent.width - 48; spacing: 14
            Row {
                anchors.horizontalCenter: parent.horizontalCenter; spacing: 22
                Rectangle {
                    width: 170; height: 170; radius: 16; color: "#fff"
                    Image { anchors.centerIn: parent; width: 154; height: 154; smooth: false
                            source: Drive.estado === "codigo" ? "image://qr/" + encodeURIComponent(Drive.enlace) : "" }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter; spacing: 4
                    Text { text: "En el celu abrí"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 24 } }
                    Text { text: Drive.enlace.replace("https://", "").replace("www.", ""); color: Tema.acento
                           font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 34 } }
                    Text { text: "y escribí:"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 24 } }
                }
            }
            Rectangle {
                width: parent.width; height: 130; radius: 20
                color: Qt.rgba(0, 0, 0, 0.35); border { width: 3; color: Tema.acento }
                Text { anchors.centerIn: parent; text: Drive.codigo; color: Tema.tinta
                       font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 92; letterSpacing: 8 } }
            }
            TBoton { width: parent.width; height: 84; tamNombre: 28; nombre: "Cancelar"; onTocado: Drive.cancelar() }
        }

        // Conectado
        Column {
            visible: Drive.estado === "conectado"
            anchors.centerIn: parent; width: parent.width - 48; spacing: 20
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; text: "Conectado como"; color: Tema.tintaSuave
                   font { family: Tema.fuente600; pixelSize: 24 } }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideMiddle; text: Drive.cuenta || "…"; color: Tema.tinta
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 32 } }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; visible: Drive.texto !== ""; text: Drive.texto
                   color: Drive.listo ? Tema.bien : Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 } }
            Rectangle {
                visible: Drive.subiendo
                width: parent.width; height: 26; radius: 13; color: Tema.barraFondo; border { width: 3; color: Tema.bordeCromo }
                Rectangle { x: 3; y: 3; height: parent.height - 6; radius: 10; width: Math.max(height, (parent.width - 6) * Drive.progreso)
                            gradient: Gradient { GradientStop { position: 0; color: Tema.acentoClaro } GradientStop { position: 1; color: Tema.acento } } }
            }
            Row {
                spacing: 16
                TBoton { width: (cuenta.width - 48 - 16) / 2; height: 96; tamNombre: 26; tamIcono: 40; icono: "nube"; nombre: "Subir ahora"
                         activo: !Drive.subiendo; onTocado: Drive.revisar() }
                TBoton { id: desc; width: (cuenta.width - 48 - 16) / 2; height: 96; tamNombre: 26; tipo: "peligro"
                         property bool armado: false
                         nombre: armado ? "Tocá otra vez" : "Desconectar"
                         onTocado: { if (armado) Drive.desconectar(); else { armado = true; desarmar.restart(); } }
                         Timer { id: desarmar; interval: 3000; onTriggered: desc.armado = false } }
            }
        }

        Text { visible: Drive.error !== ""; anchors { bottom: parent.bottom; bottomMargin: 18; horizontalCenter: parent.horizontalCenter }
               width: parent.width - 48; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
               text: "⚠ " + Drive.error; color: Tema.alerta; font { family: Tema.fuente800; pixelSize: 26 } }
    }

    // ---- Qué se sube
    Flickable {
        x: raiz.h ? cuenta.width + 18 : 0
        y: raiz.h ? 0 : cuenta.height + 18
        width: raiz.h ? raiz.area.width - x : raiz.area.width
        height: raiz.area.height - y
        contentHeight: filas.height
        clip: true; boundsBehavior: Flickable.StopAtBounds
        Column {
            id: filas; width: parent.width; spacing: 12
            TFila { etiqueta: "Subir informes"
                TLlave { prendida: raiz.a.driveInformes !== false; onCambiada: v => raiz.poner("driveInformes", v) } }
            TFila { etiqueta: "Subir videos"
                TLlave { prendida: raiz.a.driveVideos !== false; onCambiada: v => raiz.poner("driveVideos", v) } }
            TFila { etiqueta: "Borrar videos ya subidos"; detalle: "Del equipo, no de Drive"
                TLlave { prendida: !!raiz.a.driveBorrarSubidos; onCambiada: v => raiz.poner("driveBorrarSubidos", v) } }
            Text { width: parent.width; wrapMode: Text.WordWrap; topPadding: 8; color: Tema.tinta
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 26 }
                   text: raiz.wifis.length ? "Videos solo con estas wifi:" : "Videos: elegí la wifi de casa" }
            Repeater {
                model: Wifi.guardadas
                TFila {
                    required property var modelData
                    etiqueta: modelData.nombre; detalle: modelData.conectada ? "Conectada" : ""
                    TLlave { prendida: raiz.wifis.indexOf(modelData.nombre) >= 0; onCambiada: v => raiz.elegirWifi(modelData.nombre, v) }
                }
            }
            // Para cambiar el cliente de Google más adelante: la página y el código (auditoría 12)
            Text { visible: raiz.urlPagina !== "" && Drive.estado !== "falta"; width: parent.width; wrapMode: Text.WordWrap; topPadding: 8
                   color: Tema.tintaSuave; textFormat: Text.StyledText
                   font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 21 }
                   text: "Cambiar el cliente de Google: " + raiz.urlPagina + " · código <b>" + Servidor.codigoDrive + "</b>" }
        }
    }

    // Al pedir el código: el QR y el código a pantalla completa, bien grandes para el celu (Matías, 06/11/2026)
    Rectangle {
        parent: raiz; anchors.fill: parent; z: 200
        visible: Drive.estado === "codigo"
        color: Tema.fondo3
        MouseArea { anchors.fill: parent }   // que no se toque lo de atrás
        Flow {
            anchors.centerIn: parent
            width: raiz.h ? qr.width + 60 + datos.width : Math.max(qr.width, datos.width)
            spacing: raiz.h ? 60 : 40
            Rectangle {
                id: qr
                width: raiz.h ? 470 : 520; height: width; radius: 24; color: "#ffffff"
                Image { anchors.centerIn: parent; width: parent.width - 36; height: width; smooth: false
                        source: Drive.estado === "codigo" ? "image://qr/" + encodeURIComponent(Drive.enlace) : "" }
            }
            Column {
                id: datos
                width: raiz.h ? 640 : 640; spacing: 18
                Text { text: "Escaneá el QR con el celu, o abrí"; color: Tema.tintaSuave; width: parent.width; wrapMode: Text.WordWrap
                       font { family: Tema.fuente600; pixelSize: 30 } }
                Text { text: Drive.enlace.replace("https://", "").replace("www.", ""); color: Tema.acento
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 48 } }
                Text { text: "y escribí este código:"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 30 } }
                Rectangle {
                    width: parent.width; height: 150; radius: 24
                    color: Qt.rgba(0, 0, 0, 0.35); border { width: 4; color: Tema.acento }
                    Text { anchors.centerIn: parent; text: Drive.codigo; color: Tema.tinta
                           font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 104; letterSpacing: 10 } }
                }
                Text { text: "Elegí tu cuenta y tocá «Continuar». Esta pantalla se cierra sola."; color: Tema.tintaSuave
                       width: parent.width; wrapMode: Text.WordWrap; font { family: Tema.fuente600; pixelSize: 24 } }
                TBoton { width: parent.width; height: 84; tamNombre: 28; nombre: "Cancelar"; onTocado: Drive.cancelar() }
            }
        }
    }
}
