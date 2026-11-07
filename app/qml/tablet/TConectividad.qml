import QtQuick
import Betometro

// Conectividad en Modo Tablet: estado y QR grandes a la izquierda; redes en filas grandes a la derecha.
// Misma lógica que Conectividad.qml (tocar una red abre la pantalla de la clave; el QR se agranda).
TPantalla {
    id: raiz
    titulo: "Wifi e informes"
    readonly property var a: Datos.ajustes
    property bool qrGrande: false
    Component.onCompleted: if (Contexto.prueba === "red-propia") Wifi.armarPropia()   // captura
    // Con la red propia: primero el QR que conecta el celu a esa wifi, después el de los informes
    property bool paso1: true
    readonly property bool verWifi: Wifi.propia && paso1   // el QR de la wifi solo con la red propia
    readonly property string qrWifi: "WIFI:T:WPA;S:" + Wifi.nombrePropia + ";P:" + Wifi.clavePropia + ";;"
    readonly property string url: "http://" + Wifi.direccion + (Servidor.puerto !== 80 ? ":" + Servidor.puerto : "")

    Column {
        id: izq
        width: raiz.h ? 560 : raiz.area.width
        spacing: 14
        TFila { height: 110; etiqueta: Wifi.conectando ? "Conectando…" : Wifi.propia ? "Red propia · " + Wifi.conectadosPropia + (Wifi.conectadosPropia === 1 ? " celu" : " celus") : Wifi.armandoPropia ? "Armando la red propia…" : Wifi.conectada ? Wifi.conectada : "Sin conexión"
                detalle: Wifi.propia ? "«" + Wifi.nombrePropia + "» · clave " + Wifi.clavePropia
                         : Wifi.errorPropia ? Wifi.errorPropia : Wifi.conectada ? ["Sin señal", "Débil", "Regular", "Buena", "Excelente"][Wifi.senal] : ""
            IconoWifi { visible: !Wifi.propia; width: 60; height: 46; nivel: Wifi.conectada ? Math.max(1, Wifi.senal) : 0 }
            TIcono { visible: Wifi.propia; width: 52; height: 52; nombre: "red"; color: Tema.acento } }
        // Sin wifi: armar la red propia ya, sin esperar los 90 s
        TBoton { visible: Wifi.redes && !Wifi.propia && !Wifi.conectada && !Wifi.conectando; width: parent.width; height: 84; tipo: "acento"; tamNombre: 26
                 nombre: Wifi.armandoPropia ? "Armando la red propia…" : "Armar la red propia ahora"; activo: !Wifi.armandoPropia; onTocado: Wifi.armarPropia() }
        Rectangle {   // QR para el celular
            width: parent.width; height: raiz.h ? 230 : 250
            radius: Math.max(14, Tema.radio * 1.5)
            border { width: 3; color: Tema.bordeCromo }
            gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
            Rectangle {
                x: 20; anchors.verticalCenter: parent.verticalCenter
                width: parent.height - 40; height: width; radius: 16; color: "#fff"
                Image { anchors.centerIn: parent; width: parent.width - 20; height: width; smooth: false
                        source: "image://qr/" + encodeURIComponent(raiz.verWifi ? raiz.qrWifi : raiz.url) }
            }
            Column {
                x: parent.height; anchors.verticalCenter: parent.verticalCenter; width: parent.width - x - 16; spacing: 8
                Text { text: raiz.verWifi ? "1 · Conectá el celu" : (Wifi.propia ? "2 · " : "") + "Informes en el celu"; color: Tema.tinta
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                // Si no entra (Cabina: «betometro.local:80…») se achica hasta 22 en vez de cortarse (08-11)
                Text { width: parent.width; elide: Text.ElideRight; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 22
                       text: raiz.verWifi ? Wifi.nombrePropia + " · " + Wifi.clavePropia : raiz.url.replace("http://", ""); color: Tema.acento
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 } }
                Text { text: Wifi.propia ? (raiz.verWifi ? "Tocá: QR de los informes" : "Tocá: QR de la wifi") : "Tocá para agrandar"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 21 } }
            }
            MouseArea { anchors.fill: parent; onClicked: { if (Wifi.propia) raiz.paso1 = !raiz.paso1; else raiz.qrGrande = true; } }
        }
        TFila { etiqueta: "Informes en la red"
            TLlave { prendida: raiz.a.informesRed !== false; onCambiada: v => Datos.ponerCfg("informesRed", v) } }
        TFila { visible: Wifi.redes; etiqueta: "Wifi propia"; detalle: "Sin red, arma «" + Wifi.nombrePropia + "»"   // corto: en Cabina no entraba (08-11)
            TLlave { prendida: raiz.a.wifiPropia !== false; onCambiada: v => Datos.ponerCfg("wifiPropia", v) } }
    }

    Flickable {
        x: raiz.h ? izq.width + 18 : 0
        y: raiz.h ? 0 : izq.height + 14
        width: raiz.h ? raiz.area.width - x : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - y
        contentHeight: der.height + 10
        clip: true; boundsBehavior: Flickable.StopAtBounds
        Column {
            id: der; width: parent.width; spacing: 12
            // Matías, 07/11/2026: apagarlo deja solo la red propia (sin conectarse a ninguna wifi)
            TFila { etiqueta: "Conectarse a redes wifi"
                    detalle: Wifi.redes ? "A tus redes guardadas" : "Apagado: solo la red propia, sin internet"
                TLlave { prendida: Wifi.redes; onCambiada: v => Datos.ponerCfg("wifiRedes", v) } }
            TFila { visible: !Wifi.redes; etiqueta: "Sin internet"
                    detalle: "Sin actualizaciones, clima ni Drive" }
            TBoton { visible: Wifi.redes; width: parent.width; height: 92; tipo: "acento"; icono: ""; tamNombre: 30
                     nombre: Wifi.buscando ? "Buscando redes…" : "Buscar redes"; activo: !Wifi.buscando; onTocado: Wifi.buscar() }
            Repeater {
                model: Wifi.redes ? Wifi.guardadas : []
                TFila {
                    id: fg
                    required property var modelData
                    etiqueta: modelData.nombre
                    detalle: modelData.conectada ? "Conectada" : modelData.cerca ? "Guardada · cerca" : "Guardada · no está cerca"
                    tocable: !modelData.conectada && modelData.cerca; onTocada: Wifi.conectar(modelData.nombre, "")
                    Rectangle { width: 140; height: 62; radius: 18; color: Qt.rgba(0, 0, 0, 0.4); border { width: 3; color: Tema.bordeCromo }
                        Text { anchors.centerIn: parent; text: "Olvidar"; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 23 } }
                        MouseArea { anchors.fill: parent; onClicked: Wifi.olvidar(fg.modelData.nombre) } }
                }
            }
            Repeater {
                model: Wifi.redes ? Wifi.cerca : []
                TFila {
                    required property var modelData
                    etiqueta: modelData.nombre; detalle: modelData.clave ? "Con clave" : "Abierta"
                    tocable: true; onTocada: Navegacion.ir("wifi", { red: modelData.nombre })
                    Senal { nivel: modelData.senal; grosor: 8; paso: 7 }
                }
            }
        }
    }

    // QR en grande
    Rectangle {
        parent: raiz
        anchors.fill: parent; visible: raiz.qrGrande; z: 100
        color: Qt.rgba(0, 0, 0, 0.88)
        MouseArea { anchors.fill: parent; onClicked: raiz.qrGrande = false }
        Column {
            anchors.centerIn: parent; spacing: 18
            Rectangle {
                readonly property real lado: Math.min(raiz.width, raiz.height) - 150
                anchors.horizontalCenter: parent.horizontalCenter
                width: lado; height: lado; radius: 22; color: "#fff"
                Image { anchors.centerIn: parent; width: parent.lado - 40; height: width; smooth: false
                        source: raiz.qrGrande ? "image://qr/" + encodeURIComponent(raiz.url) : "" }
            }
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: raiz.url.replace("http://", "")
                   color: Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 34 } }
        }
    }
}
