import QtQuick
import Betometro

// Pantalla 13: Conectividad. Muestras aprobadas docs/muestras/conectividad.html (#c1 y #c2).
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var a: Datos.ajustes
    readonly property string url: "http://" + Wifi.direccion + (Servidor.puerto !== 80 ? ":" + Servidor.puerto : "")

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "CONECTIVIDAD"; derechaRayas: raiz.h ? 90 : 30; onVolver: Navegacion.ir("ajustes") }

    Component {
        id: gEstado
        Tarjeta {
            height: colE.height + 10
            Column {
                id: colE; width: parent.width
                TituloTarjeta { texto: "Estado" }
                Row {
                    x: 22; spacing: 12; topPadding: 6; bottomPadding: 4
                    Rectangle { width: 14; height: 14; radius: 7; anchors.verticalCenter: parent.verticalCenter
                                color: Wifi.conectada ? Tema.bien : (Wifi.conectando ? Tema.acento : Tema.alerta) }
                    Text { color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 }
                           text: Wifi.conectando ? "Conectando…" : Wifi.conectada ? "Conectado a «" + Wifi.conectada + "»" : "Sin conexión" }
                }
                Fila { altoMinimo: 58; etiqueta: "Señal"; colorEtiqueta: Tema.tintaSuave
                    Row { spacing: 12
                        Senal { nivel: Wifi.senal; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: ["Sin señal", "Débil", "Regular", "Buena", "Excelente"][Wifi.senal]; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } } } }
                Fila { altoMinimo: 58; etiqueta: "Hora"; colorEtiqueta: Tema.tintaSuave; ultima: true
                    Text { text: Wifi.horaInternet ? "Puesta por internet ✓" : "Del GPS"; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } } }
            }
        }
    }
    Component {
        id: gRed
        Tarjeta {
            height: colR.height + 10
            Column {
                id: colR; width: parent.width
                TituloTarjeta { texto: "Betómetro en la red" }
                Row {
                    x: 22; spacing: 20; topPadding: 10; bottomPadding: 18
                    Rectangle {
                        width: 132; height: 132; radius: 14; color: "#fff"
                        Image { anchors.centerIn: parent; width: 120; height: 120; smooth: false; source: "image://qr/" + encodeURIComponent(raiz.url) }
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: colR.width - 132 - 64
                        Text { width: parent.width; wrapMode: Text.WordWrap; text: "Desde el celu o la compu, en la misma wifi:"
                               color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 18 } }
                        Text { text: Wifi.direccion; color: Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 34 } }
                        Text { visible: raiz.h; width: parent.width; wrapMode: Text.WordWrap; text: "Ahí están todas las carreras y sus informes."
                               color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 16 } }
                    }
                }
                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.12) }
                Fila { altoMinimo: 58; etiqueta: "Mostrar informes en la red"
                    Llave { prendida: raiz.a.informesRed !== false; onCambiada: v => Datos.ponerCfg("informesRed", v) } }
                Fila { altoMinimo: 58; etiqueta: "Wifi propia si no hay red" + (raiz.h ? "   «Betometro-" + (raiz.a.numeroAuto || "101") + "»" : ""); ultima: true
                    Llave { prendida: raiz.a.wifiPropia !== false; onCambiada: v => Datos.ponerCfg("wifiPropia", v) } }
            }
        }
    }
    Component {
        id: gGuardadas
        Tarjeta {
            height: colG.height + 10
            Column {
                id: colG; width: parent.width
                TituloTarjeta { texto: "Redes guardadas" }
                Repeater {
                    model: Wifi.guardadas
                    Fila { altoMinimo: 58;
                        required property var modelData
                        required property int index
                        ultima: index === Wifi.guardadas.length - 1
                        resaltada: modelData.conectada
                        izquierda: Row {
                            spacing: 14
                            Senal { nivel: modelData.senal; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: modelData.nombre; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } }
                        }
                        Row { spacing: 14
                            Text { anchors.verticalCenter: parent.verticalCenter
                                   text: modelData.conectada ? "conectada" : modelData.cerca ? "cerca" : "no está cerca"
                                   color: modelData.conectada ? Tema.acento : Tema.tintaSuave
                                   font { family: modelData.conectada ? Tema.fuente800 : Tema.fuente600; pixelSize: 18 } }
                            BotonCromo { width: 110; height: 40; tamTexto: 15; texto: "Olvidar"; onTocado: Wifi.olvidar(modelData.nombre) } }
                    }
                }
                Sub { visible: Wifi.guardadas.length === 0; x: 22; text: "Ninguna todavía"; tam: 13; bottomPadding: 14 }
            }
        }
    }
    Component {
        id: gCerca
        Tarjeta {
            height: colC.height + 10
            Column {
                id: colC; width: parent.width
                TituloTarjeta { texto: "Redes cerca" }
                Repeater {
                    model: Wifi.cerca
                    Fila { altoMinimo: 58;
                        required property var modelData
                        tocable: true
                        onTocada: Navegacion.ir("wifi", { red: modelData.nombre })
                        izquierda: Row {
                            spacing: 14
                            Senal { nivel: modelData.senal; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: modelData.nombre; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } }
                        }
                        Candado { visible: modelData.clave }
                        Text { visible: !modelData.clave; text: "abierta"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 18 } }
                    }
                }
                Item {
                    width: parent.width; height: 84
                    BotonAmarillo { anchors.centerIn: parent; width: raiz.h ? 260 : 300; height: raiz.h ? 52 : 58; tamTexto: 19
                        texto: Wifi.buscando ? "BUSCANDO…" : "↻ BUSCAR REDES"; onTocado: Wifi.buscar() }
                }
            }
        }
    }

    Flickable {
        id: lista
        x: raiz.h ? 40 : 24; y: 96
        width: parent.width - 2 * x; height: parent.height - y
        contentHeight: (raiz.h ? Math.max(izq.height, der.height) : unica.height) + 40
        clip: true; boundsBehavior: Flickable.StopAtBounds
        Column {
            id: izq; visible: raiz.h; width: raiz.h ? 590 : 0; spacing: 18; topPadding: 6
            Loader { width: parent.width; sourceComponent: raiz.h ? gEstado : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gRed : null }
        }
        Column {
            id: der; visible: raiz.h; x: 620; width: raiz.h ? 580 : 0; spacing: 18; topPadding: 6
            Loader { width: parent.width; sourceComponent: raiz.h ? gGuardadas : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gCerca : null }
            Sub { width: parent.width; wrapMode: Text.WordWrap; text: "Tocás una red → se abre la pantalla de la clave con el teclado"; tam: 11; leftPadding: 6 }
        }
        Column {
            id: unica; visible: !raiz.h; width: lista.width; spacing: 18; topPadding: 6
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gEstado }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gRed }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gGuardadas }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gCerca }
        }
    }
}
