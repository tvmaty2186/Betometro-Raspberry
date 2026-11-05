import QtQuick
import Betometro

// Pantalla 2: Carreras (PRÓXIMAS y CORRIDAS). Muestras #s2 y #v2.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    property var borrando: null

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "CARRERAS"; onVolver: Navegacion.ir("inicio") }

    BotonAmarillo {
        x: parent.width - (raiz.h ? 40 : 24) - width
        y: raiz.h ? 110 : 26
        width: raiz.h ? 200 : 130; height: raiz.h ? 64 : 52
        texto: "+ Nueva"; tamTexto: raiz.h ? 22 : 20
        onTocado: Navegacion.ir("ficha", { id: "" })
    }

    Flickable {
        id: lista
        x: raiz.h ? 40 : 24; y: 96
        width: parent.width - x - (raiz.h ? 280 : 24); height: parent.height - y
        contentHeight: col.height + 40
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        Column {
            id: col
            width: lista.width
            spacing: 14
            topPadding: 4

            Text {
                visible: Datos.proximas.length === 0 && Datos.corridas.length === 0
                width: parent.width; wrapMode: Text.WordWrap
                text: "Todavía no hay carreras. Tocá + Nueva para cargar la primera."
                color: Tema.tintaSuave; font { family: Tema.fuente; italic: Tema.cursiva; pixelSize: 22 }
                topPadding: 40
            }

            BandaSeccion { texto: "PRÓXIMAS"; visible: Datos.proximas.length > 0 }
            Repeater {
                model: Datos.proximas
                Tarjeta {
                    id: tp
                    required property var modelData
                    width: col.width
                    height: raiz.h ? 92 : 172
                    Row {
                        x: 20; y: raiz.h ? (parent.height - height) / 2 : 16
                        spacing: raiz.h ? 20 : 16
                        FechaRedonda { dia: tp.modelData.dia; mes: tp.modelData.mes; anchors.verticalCenter: parent.verticalCenter }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            width: tp.width - 62 - 60 - (raiz.h ? 210 : 0) - 116
                            Text { width: parent.width; elide: Text.ElideRight; text: tp.modelData.nombre; color: Tema.tinta
                                   font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 28 } }
                            Sub { width: parent.width; elide: Text.ElideRight
                                  text: tp.modelData.linea + (tp.modelData.lugar ? "  ·  " + tp.modelData.lugar.split(",")[0] : "") }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        pressAndHoldInterval: 600
                        onClicked: Navegacion.ir("ficha", { id: tp.modelData.id })
                        onPressAndHold: raiz.borrando = tp.modelData
                    }
                    // Clima del día de la carrera en su localidad (Matías, 05/11/2026). Tocarlo: ese día hora por hora.
                    Rectangle {
                        id: botonClima
                        readonly property bool conLugar: tp.modelData.lugarLat !== undefined && tp.modelData.lugarLat !== null
                        readonly property var d: conLugar && Clima.version >= 0 ? Clima.dia(tp.modelData.lugarLat, tp.modelData.lugarLon, tp.modelData.fecha) : null
                        width: 104; height: raiz.h ? 58 : 62; radius: height / 2
                        x: raiz.h ? parent.width - 20 - 170 - 14 - width : parent.width - 20 - width
                        y: raiz.h ? (parent.height - height) / 2 : 16
                        opacity: d && d.disponible ? 1 : 0.6
                        border { width: 2; color: Tema.bordeCromo }
                        gradient: Gradient { GradientStop { position: 0; color: "#2b6b43" } GradientStop { position: 1; color: "#0b2a17" } }
                        Row {
                            visible: !!(botonClima.d && botonClima.d.disponible)
                            anchors.centerIn: parent; spacing: 4
                            IconoClima { width: 32; height: 32; anchors.verticalCenter: parent.verticalCenter; codigo: botonClima.d ? botonClima.d.codigo || 0 : 0 }
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                Text { text: botonClima.d ? botonClima.d.max + "°" : ""; color: Tema.tinta; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 21 } }
                                Text { text: botonClima.d ? "lluvia " + botonClima.d.lluvia + "%" : ""; color: "#5fb4ff"; font { family: Tema.fuente600; pixelSize: 11 } }
                            }
                        }
                        Column {
                            visible: !(botonClima.d && botonClima.d.disponible)
                            anchors.centerIn: parent
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Clima"; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 17 } }
                            Text { anchors.horizontalCenter: parent.horizontalCenter
                                   text: !botonClima.conLugar ? "sin localidad" : botonClima.d && botonClima.d.desde ? "desde " + botonClima.d.desde : "cargando…"
                                   color: Tema.tintaSuave; font { family: Tema.fuente600; italic: true; pixelSize: 11 } }
                        }
                        MouseArea {
                            anchors.fill: parent; anchors.margins: -6
                            onClicked: botonClima.conLugar
                                       ? Navegacion.ir("clima", { nombre: tp.modelData.lugar.split(",")[0], lat: tp.modelData.lugarLat, lon: tp.modelData.lugarLon, fecha: tp.modelData.fecha })
                                       : Navegacion.ir("ficha", { id: tp.modelData.id })
                        }
                    }
                    BotonAmarillo {
                        x: raiz.h ? parent.width - 20 - width : 18
                        y: raiz.h ? (parent.height - height) / 2 : parent.height - 16 - height
                        width: raiz.h ? 170 : parent.width - 36; height: raiz.h ? 56 : 60
                        texto: "LARGAR"
                        activo: tp.modelData.puedeLargar
                        onTocado: Navegacion.ir("carrera", { id: tp.modelData.id })
                    }
                }
            }

            BandaSeccion { texto: "CORRIDAS"; visible: Datos.corridas.length > 0; height: 56; Component.onCompleted: {} }
            Repeater {
                model: Datos.corridas
                Tarjeta {
                    id: tc
                    required property var modelData
                    width: col.width; height: 92
                    Row {
                        x: 20; anchors.verticalCenter: parent.verticalCenter
                        spacing: raiz.h ? 20 : 16
                        FechaRedonda { dia: tc.modelData.dia; mes: tc.modelData.mes; anchors.verticalCenter: parent.verticalCenter }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            width: tc.width - 62 - 60 - (raiz.h ? 210 : 170)
                            Text { width: parent.width; elide: Text.ElideRight; text: tc.modelData.nombre; color: Tema.tinta
                                   font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 28 } }
                            Sub { width: parent.width; elide: Text.ElideRight; text: tc.modelData.linea }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        pressAndHoldInterval: 600
                        onClicked: Navegacion.ir("ficha", { id: tc.modelData.id })
                        onPressAndHold: raiz.borrando = tc.modelData
                    }
                    BotonCromo {
                        x: parent.width - 20 - width; anchors.verticalCenter: parent.verticalCenter
                        width: raiz.h ? 170 : 130; height: raiz.h ? 56 : 52
                        texto: "Informe"
                        activo: tc.modelData.medidas > 0 || tc.modelData.tieneVueltas
                        onTocado: Navegacion.ir("informe", { id: tc.modelData.id })
                    }
                }
            }
            Sub {
                visible: Datos.proximas.length + Datos.corridas.length > 0
                text: "Mantené apretada una carrera para borrarla"; tam: 12; topPadding: 10
            }
        }
    }

    // ¿Borrar «nombre»?
    Rectangle {
        visible: raiz.borrando !== null
        anchors.fill: parent; z: 80
        color: Qt.rgba(0, 0, 0, 0.6)
        MouseArea { anchors.fill: parent; onClicked: raiz.borrando = null }
        Tarjeta {
            oscura: true
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 620); height: 230
            Text {
                x: 28; y: 28; width: parent.width - 56; wrapMode: Text.WordWrap
                text: raiz.borrando ? "¿Borrar «" + raiz.borrando.nombre + "»?" : ""
                color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 28 }
            }
            Sub { x: 28; y: 110; text: "Se borran también todas sus vueltas"; tam: 13 }
            Row {
                anchors { right: parent.right; bottom: parent.bottom; margins: 26 }
                spacing: 18
                BotonCromo { width: 170; height: 58; texto: "Cancelar"; tamTexto: 18; onTocado: raiz.borrando = null }
                BotonAmarillo { width: 170; height: 58; texto: "BORRAR"; onTocado: { Datos.borrarCarrera(raiz.borrando.id); raiz.borrando = null; } }
            }
        }
    }
}
