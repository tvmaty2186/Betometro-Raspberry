import QtQuick
import Betometro

// Carreras en Modo Tablet: filas grandes. Próximas con el clima del día y LARGAR; corridas con su informe.
// Mantener apretada una carrera para borrarla (como la pantalla normal).
TPantalla {
    id: raiz
    titulo: "Carreras"
    property var borrando: null
    Component.onCompleted: if (Contexto.prueba === "borrar" && Datos.corridas.length) borrando = Datos.corridas[0]   // captura de la ventana

    Flickable {
        anchors.fill: raiz.area
        contentHeight: col.height + 20
        clip: true; boundsBehavior: Flickable.StopAtBounds
        Column {
            id: col
            width: parent.width
            spacing: 14
            TFila { etiqueta: "+  Nueva carrera"; tocable: true; height: 90; onTocada: Navegacion.ir("ficha", { id: "" })
                TIcono { width: 44; height: 44; nombre: "bandera"; color: Tema.acento } }

            Text { visible: Datos.proximas.length > 0; text: "PRÓXIMAS"; color: Tema.acento; topPadding: 6
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 24; letterSpacing: 3 } }
            Repeater {
                model: Datos.proximas
                Rectangle {
                    id: fp
                    required property var modelData
                    readonly property bool conLugar: modelData.lugarLat !== undefined && modelData.lugarLat !== null
                    // Con clima adjuntado (desde la pantalla Clima), ese: se actualiza solo y sirve sin internet
                    readonly property var adj: modelData.climaAdjunto || null
                    readonly property var d: adj && adj.dia ? adj.dia
                                             : conLugar && Clima.version >= 0 ? Clima.dia(modelData.lugarLat, modelData.lugarLon, modelData.fecha) : null
                    width: col.width; height: raiz.h ? 130 : 250
                    radius: Math.max(14, Tema.radio * 1.5)
                    border { width: 3; color: Tema.bordeCromo }
                    gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.2) } GradientStop { position: 1; color: Tema.caja2 } }
                    MouseArea { anchors.fill: parent; pressAndHoldInterval: 600
                                onClicked: Navegacion.ir("ficha", { id: fp.modelData.id }); onPressAndHold: raiz.borrando = fp.modelData }
                    Row {
                        x: 20; y: raiz.h ? (fp.height - height) / 2 : 22; spacing: 20
                        FechaRedonda { dia: fp.modelData.dia; mes: fp.modelData.mes; anchors.verticalCenter: parent.verticalCenter; scale: 1.25 }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            width: fp.width - 140 - (raiz.h ? 470 : 0)
                            Text { width: parent.width; elide: Text.ElideRight; text: fp.modelData.nombre; color: Tema.tinta
                                   font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 34 } }
                            Text { width: parent.width; elide: Text.ElideRight; color: Tema.acento
                                   text: fp.modelData.lugar ? fp.modelData.lugar.split(",")[0] : "Sin localidad"
                                   font { family: Tema.fuente600; pixelSize: 23 } }
                        }
                    }
                    Row {
                        x: fp.width - width - 18; y: raiz.h ? (fp.height - height) / 2 : fp.height - height - 18
                        spacing: 14
                        Rectangle {   // clima del día de la carrera
                            width: 200; height: 96; radius: 22
                            opacity: fp.d && fp.d.disponible ? 1 : 0.65
                            color: Qt.rgba(0, 0, 0, 0.35); border { width: 3; color: Tema.bordeCromo }
                            Row { visible: !!(fp.d && fp.d.disponible); anchors.centerIn: parent; spacing: 8
                                IconoClima { width: 56; height: 56; anchors.verticalCenter: parent.verticalCenter; codigo: fp.d ? fp.d.codigo || 0 : 0 }
                                Column { anchors.verticalCenter: parent.verticalCenter
                                    Text { text: fp.d ? fp.d.max + "°" : ""; color: Tema.numero; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 36 } }
                                    Text { text: fp.d ? fp.d.lluvia + "%" : ""; color: Tema.lluvia; font { family: Tema.fuente600; pixelSize: 21 } }
                                    Text { visible: !!fp.adj; text: "guardado"; color: Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 15 } } } }
                            Column { visible: !(fp.d && fp.d.disponible); anchors.centerIn: parent
                                Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Clima"; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 25 } }
                                Text { anchors.horizontalCenter: parent.horizontalCenter; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 21 }
                                       text: fp.adj ? "guardado" : !fp.conLugar ? "sin localidad" : fp.d && fp.d.desde ? "desde " + fp.d.desde : "cargando…" } }
                            MouseArea { anchors.fill: parent; anchors.margins: -6
                                onClicked: fp.adj && fp.adj.horas ? horasAdjunto.abrir(fp.modelData, fp.adj)
                                         : fp.conLugar ? Navegacion.ir("clima", { nombre: fp.modelData.lugar.split(",")[0], lat: fp.modelData.lugarLat, lon: fp.modelData.lugarLon, fecha: fp.modelData.fecha })
                                                       : Navegacion.ir("ficha", { id: fp.modelData.id }) }
                        }
                        TBoton { width: raiz.h ? 220 : fp.width - 36 - 214; height: 96; tipo: "acento"; icono: ""; nombre: "IR A LARGAR"; tamNombre: 30
                                 dato: fp.modelData.puedeLargar ? "" : fp.modelData.motivoNoLargar   // gris con el motivo (07-2.2)
                                 activo: fp.modelData.puedeLargar; onTocado: Navegacion.ir("carrera", { id: fp.modelData.id }) }
                    }
                }
            }

            Text { visible: Datos.corridas.length > 0; text: "CORRIDAS"; color: Tema.acento; topPadding: 6
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 24; letterSpacing: 3 } }
            Repeater {
                model: Datos.corridas
                Rectangle {
                    id: fc
                    required property var modelData
                    width: col.width; height: 116
                    radius: Math.max(14, Tema.radio * 1.5)
                    border { width: 3; color: Tema.bordeCromo }
                    gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.2) } GradientStop { position: 1; color: Tema.caja2 } }
                    MouseArea { anchors.fill: parent; pressAndHoldInterval: 600
                                onClicked: Navegacion.ir("ficha", { id: fc.modelData.id }); onPressAndHold: raiz.borrando = fc.modelData }
                    Row {
                        x: 20; anchors.verticalCenter: parent.verticalCenter; spacing: 20
                        FechaRedonda { dia: fc.modelData.dia; mes: fc.modelData.mes; anchors.verticalCenter: parent.verticalCenter; scale: 1.2 }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            width: fc.width - 140 - 250
                            Text { width: parent.width; elide: Text.ElideRight; text: fc.modelData.nombre; color: Tema.tinta
                                   font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 32 } }
                            Text { width: parent.width; elide: Text.ElideRight; text: fc.modelData.linea; color: Tema.acento
                                   font { family: Tema.fuente600; pixelSize: 21 } }
                        }
                    }
                    TBoton { x: fc.width - width - 18; anchors.verticalCenter: parent.verticalCenter; width: 220; height: 84
                             icono: ""; nombre: "Informe"; tamNombre: 30
                             activo: fc.modelData.medidas > 0 || fc.modelData.tieneVueltas
                             onTocado: Navegacion.ir("informe", { id: fc.modelData.id }) }
                }
            }
            Text { visible: Datos.proximas.length + Datos.corridas.length === 0; topPadding: 30
                   text: "Todavía no hay carreras."; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 } }
        }
    }

    // ¿Borrar? La misma ventana que en la ficha (07-3.5)
    TBorrar { parent: raiz; carrera: raiz.borrando; onCancelar: raiz.borrando = null
              onBorrar: { Datos.borrarCarrera(raiz.borrando.id); raiz.borrando = null; } }
    // El clima adjuntado a una carrera, hora por hora (sirve sin internet)
    TClimaHoras {
        id: horasAdjunto
        parent: raiz; anchors.fill: parent
        h: raiz.h
        function abrir(c, a) {
            lugar = (a.nombre || "") + (a.actualizado ? " · actualizado " + Qt.formatDateTime(new Date(a.actualizado), "dd/MM HH:mm") : "");
            dia = a.dia; horas = a.horas || []; fecha = c.fecha;
        }
        onCerrar: fecha = ""
    }
}
