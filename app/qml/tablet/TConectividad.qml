import QtQuick
import Betometro

// Conectividad en Modo Tablet: estado y QR grandes a la izquierda; redes en filas grandes a la derecha.
// Misma lógica que Conectividad.qml (tocar una red abre la pantalla de la clave; el QR se agranda).
TPantalla {
    id: raiz
    titulo: "Wifi e informes"
    readonly property var a: Datos.ajustes
    property bool qrGrande: false
    readonly property string url: "http://" + Wifi.direccion + (Servidor.puerto !== 80 ? ":" + Servidor.puerto : "")

    Column {
        id: izq
        width: raiz.h ? 560 : raiz.area.width
        spacing: 14
        TFila { height: 110; etiqueta: Wifi.conectando ? "Conectando…" : Wifi.propia ? "Wifi propia" : Wifi.conectada ? Wifi.conectada : "Sin conexión"
                detalle: Wifi.propia ? "«" + Wifi.nombrePropia + "» · clave " + Wifi.clavePropia : Wifi.conectada ? ["Sin señal", "Débil", "Regular", "Buena", "Excelente"][Wifi.senal] : ""
            IconoWifi { width: 60; height: 46; nivel: Wifi.conectada ? Math.max(1, Wifi.senal) : 0 } }
        Rectangle {   // QR para el celular
            width: parent.width; height: raiz.h ? 230 : 250
            radius: Math.max(14, Tema.radio * 1.5)
            border { width: 3; color: Tema.bordeCromo }
            gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
            Rectangle {
                x: 20; anchors.verticalCenter: parent.verticalCenter
                width: parent.height - 40; height: width; radius: 16; color: "#fff"
                Image { anchors.centerIn: parent; width: parent.width - 20; height: width; smooth: false; source: "image://qr/" + encodeURIComponent(raiz.url) }
            }
            Column {
                x: parent.height; anchors.verticalCenter: parent.verticalCenter; width: parent.width - x - 16; spacing: 8
                Text { text: "Informes en el celu"; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                Text { width: parent.width; elide: Text.ElideRight; text: raiz.url.replace("http://", ""); color: Tema.acento
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 } }
                Text { text: "Tocá para agrandar"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 21 } }
            }
            MouseArea { anchors.fill: parent; onClicked: raiz.qrGrande = true }
        }
        TFila { etiqueta: "Informes en la red"
            TLlave { prendida: raiz.a.informesRed !== false; onCambiada: v => Datos.ponerCfg("informesRed", v) } }
        TFila { etiqueta: "Wifi propia"; detalle: "Si no hay red, arma «" + Wifi.nombrePropia + "»"
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
            TBoton { width: parent.width; height: 92; tipo: "acento"; icono: ""; tamNombre: 30
                     nombre: Wifi.buscando ? "Buscando redes…" : "Buscar redes"; activo: !Wifi.buscando; onTocado: Wifi.buscar() }
            Repeater {
                model: Wifi.guardadas
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
                model: Wifi.cerca
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
