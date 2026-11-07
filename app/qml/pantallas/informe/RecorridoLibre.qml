import QtQuick
import Betometro

// Recorrido libre en el informe: el resumen (tiempo, distancia, promedio, máxima, parado, aceleración máxima)
// y la lista de marcas (muestra docs/muestras/recorrido/3-informe.png). Va donde iría la tabla de vueltas.
Tarjeta {
    id: caja
    property bool h: true
    property var inf: ({})           // Datos.informe(id): resumen
    property var marcas: []
    property bool conVideo: false    // tocar una marca lleva el video ahí
    signal irMarca(var mk)
    readonly property var r: inf.resumen || ({})

    oscura: true
    x: caja.h ? 648 : 20; y: caja.h ? 362 : 900
    width: caja.h ? 608 : parent.width - 40; height: caja.h ? 338 : 360
    Grid {
        id: gridRes
        x: 22; y: 14; width: parent.width - 44
        columns: 3; rowSpacing: 2
        Repeater {
            model: [
                { e: "Tiempo", v: Formato.hms(caja.r.tiempoMs || 0, true) },
                { e: "Distancia", v: Formato.coma((caja.r.distM || 0) / 1000, 1) + " km" },
                { e: "Promedio", v: Formato.coma(caja.r.promKmh, 1) + " km/h" },
                { e: "Máxima", v: Formato.coma(caja.r.maxKmh, 0) + " km/h" },
                { e: "Parado", v: Formato.hms(caja.r.paradoMs || 0, true).replace(/^0:/, "") },
                { e: "Acel. máx.", v: Formato.coma(caja.r.acelMax, 1) + " m/s²" }
            ]
            Column {
                required property var modelData
                width: gridRes.width / 3
                Sub { text: modelData.e; tam: 11 }
                Text { text: modelData.v; color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: caja.h ? 30 : 28 } }
            }
        }
    }
    Sub { x: 22; y: gridRes.y + gridRes.height + 12; tam: 12
          text: caja.marcas.length ? "Marcas" + (caja.conVideo ? " · tocá una y el video va ahí" : "") : "Sin marcas" }
    Flickable {
        id: listaMarcas
        x: 22; y: gridRes.y + gridRes.height + 38; width: parent.width - 44; height: parent.height - y - 12
        contentHeight: colMarcas.height; clip: true; boundsBehavior: Flickable.StopAtBounds
        Column {
            id: colMarcas; width: parent.width
            Repeater {
                model: caja.marcas
                Item {
                    required property var modelData
                    width: colMarcas.width; height: 40
                    Row {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 18
                        Rectangle { width: 30; height: 30; radius: 15; color: Tema.amarillo; anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: modelData.n; color: "#111111"; font { family: Tema.fuente800; pixelSize: 17 } } }
                        Text { width: 110; text: Formato.hms(modelData.ms, true); color: Tema.tinta; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 25 } }
                        Text { width: 120; text: Formato.coma((modelData.m || 0) / 1000, 1) + " km"; color: Tema.tinta; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 25 } }
                        Text { text: Math.round(modelData.kmh || 0) + " km/h"; color: Tema.acento; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 22 } }
                    }
                    MouseArea { anchors.fill: parent; enabled: caja.conVideo; onClicked: caja.irMarca(modelData) }
                }
            }
        }
    }
}
