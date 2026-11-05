import QtQuick
import Betometro

// Clima (pedido de Matías, 05/11/2026; muestras aprobadas el mismo día).
// Lugar: la ubicación del GPS, una localidad buscada (queda guardada en «climaLugar») o la de una carrera
// (desde la lista de carreras: Navegacion.parametros = {nombre, lat, lon, fecha}).
// Izquierda: ahora y las próximas horas (o el día elegido, hora por hora). Derecha: 7 días.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var par: Navegacion.parametros || ({})
    readonly property bool deCarrera: par.lat !== undefined && par.lat !== null
    property bool usarGps: !deCarrera && Datos.ajustes.climaModo !== "lugar"
    property var lugar: deCarrera ? { nombre: par.nombre, lat: par.lat, lon: par.lon } : (Datos.ajustes.climaLugar || null)
    property string diaElegido: par.fecha || ""

    readonly property bool hayLugar: usarGps ? Clima.hayGps : !!(lugar && lugar.lat !== undefined)
    readonly property real lat: usarGps ? Clima.gpsLat : (lugar ? lugar.lat : 0)
    readonly property real lon: usarGps ? Clima.gpsLon : (lugar ? lugar.lon : 0)
    readonly property string nombre: usarGps ? (Clima.nombreGps || "Tu ubicación (GPS)") : (lugar ? lugar.nombre : "")
    readonly property var p: Clima.version >= 0 && hayLugar ? Clima.pronostico(lat, lon) : ({})
    readonly property var diaSel: diaElegido && hayLugar && Clima.version >= 0 ? Clima.dia(lat, lon, diaElegido) : null
    readonly property var horas: diaElegido ? (Clima.version >= 0 && hayLugar ? Clima.horasDe(lat, lon, diaElegido) : []) : (p.horas || [])
    // Días con carrera (para la 🏁)
    readonly property var fechasCarrera: { const l = []; for (const c of Datos.proximas) l.push(c.fecha); return l; }
    // Rango de temperaturas de la semana (para las barras)
    readonly property var rango: { let a = 99, b = -99; for (const d of (p.dias || [])) if (d.disponible) { a = Math.min(a, d.min); b = Math.max(b, d.max); } return [a, b]; }

    function nombreDia(iso, corto) {
        const f = Formato.desdeIso(iso), hoy = new Date();
        if (f.toDateString() === hoy.toDateString()) return "Hoy";
        const d = ["Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"][f.getDay()];
        return d + " " + f.getDate() + (corto ? "" : "/" + (f.getMonth() + 1));
    }
    function elegirLugar(l) {
        Datos.ponerCfg("climaLugar", { nombre: l.nombre, detalle: l.detalle || "", lat: l.lat, lon: l.lon });
        Datos.ponerCfg("climaModo", "lugar");
        raiz.lugar = { nombre: l.nombre, lat: l.lat, lon: l.lon };
        raiz.usarGps = false; raiz.diaElegido = "";
    }

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "CLIMA"; onVolver: Navegacion.ir(raiz.deCarrera ? "carreras" : "inicio") }

    // Elegir el lugar: GPS o buscar una localidad
    Row {
        x: raiz.h ? 250 : 24; y: raiz.h ? 30 : 96
        spacing: 10
        component Opcion: Rectangle {
            property string texto
            property bool prendida
            signal tocada()
            height: 44; radius: 22; width: tOp.implicitWidth + 36
            color: prendida ? Tema.acento : Qt.rgba(0, 0, 0, 0.5)
            border { width: 1.5; color: prendida ? Tema.acento : Tema.bordeCromo }
            Text { id: tOp; anchors.centerIn: parent; text: parent.texto
                   color: parent.prendida ? Tema.acentoTinta : Tema.tinta; font { family: Tema.fuente600; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 19 } }
            MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: parent.tocada() }
        }
        Opcion { texto: "Ubicación del GPS"; prendida: raiz.usarGps
                 onTocada: { Datos.ponerCfg("climaModo", "gps"); raiz.usarGps = true; raiz.diaElegido = ""; } }
        Opcion { texto: !raiz.usarGps && raiz.lugar ? raiz.lugar.nombre : "Buscar localidad…"; prendida: !raiz.usarGps
                 onTocada: buscador.abrir("") }
    }

    // ---------------------------------------------------------------- izquierda: ahora / día elegido
    Tarjeta {
        id: tAhora
        x: raiz.h ? 28 : 24; y: raiz.h ? 96 : 156
        width: raiz.h ? 520 : parent.width - 48; height: raiz.h ? 600 : 506
        Item {
            anchors { fill: parent; margins: 24 }
            Sub {
                width: parent.width; elide: Text.ElideRight; tam: 12
                text: raiz.nombre + (raiz.diaElegido ? " · " + raiz.nombreDia(raiz.diaElegido) : " · ahora")
            }
            // Sin lugar, cargando o lejos
            Text {
                visible: !raiz.hayLugar || (!raiz.p.ok && !raiz.diaSel) || (raiz.diaSel && !raiz.diaSel.disponible && !raiz.diaSel.cargando)
                anchors.centerIn: parent; width: parent.width - 20; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                text: !raiz.hayLugar ? (raiz.usarGps ? "Todavía no hay posición del GPS.\nBuscá una localidad arriba." : "Buscá una localidad arriba.")
                      : raiz.diaSel && !raiz.diaSel.disponible ? "El pronóstico para ese día todavía no está.\nVa a estar desde el " + raiz.diaSel.desde + "."
                      : raiz.p.cargando ? "Bajando el pronóstico…" : (raiz.p.error || "Sin pronóstico todavía")
                color: Tema.tinta; font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 22 }
            }
            // Ahora (o el día elegido): ícono, temperatura y datos
            Row {
                id: filaAhora
                visible: raiz.p.ok && (!raiz.diaSel || raiz.diaSel.disponible)
                y: 36; spacing: 20
                readonly property var a: raiz.diaSel ? raiz.diaSel : (raiz.p.actual || {})
                IconoClima { anchors.verticalCenter: parent.verticalCenter; width: 104; height: 104; codigo: filaAhora.a.codigo || 0 }
                Digito { anchors.verticalCenter: parent.verticalCenter; tam: raiz.diaSel ? 74 : 104
                         text: raiz.diaSel ? filaAhora.a.min + "° / " + filaAhora.a.max + "°" : (filaAhora.a.temp !== undefined ? filaAhora.a.temp + "°" : "") }
                Column {
                    anchors.verticalCenter: parent.verticalCenter; spacing: 4
                    Text { text: "Lluvia " + (filaAhora.a.lluvia || 0) + " % · " + Number(filaAhora.a.mm || 0).toFixed(1).replace(".", ",") + " mm"
                           color: "#5fb4ff"; font { family: Tema.fuente600; pixelSize: 20 } }
                    Text { text: "Viento " + (filaAhora.a.viento || 0) + " km/h"; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } }
                    Text { text: "Ráfagas " + (filaAhora.a.rafagas || 0) + " km/h"; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } }
                }
            }
            // Hora por hora (cada 2 h)
            Sub { visible: filaAhora.visible; y: 168; tam: 12; text: raiz.diaSel ? "Hora por hora" : "Próximas horas" }
            Grid {
                id: grilla
                visible: filaAhora.visible
                y: 196; width: parent.width
                columns: 6; columnSpacing: 6; rowSpacing: 6
                readonly property real ancho: (width - 5 * columnSpacing) / 6
                Repeater {
                    model: raiz.horas.slice(0, 12)
                    Rectangle {
                        required property var modelData
                        width: grilla.ancho; height: raiz.h ? 150 : 108; radius: 12; color: Qt.rgba(0, 0, 0, 0.3)
                        Column {
                            anchors.centerIn: parent; spacing: raiz.h ? 4 : 1
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.hora + " h"; color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 15 } }
                            IconoClima { anchors.horizontalCenter: parent.horizontalCenter; width: raiz.h ? 40 : 34; height: width; codigo: modelData.codigo }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.temp + "°"; color: Tema.tinta; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 22 } }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.lluvia + "%"; color: "#5fb4ff"; font { family: Tema.fuente600; pixelSize: 14 } }
                        }
                    }
                }
            }
            Text {
                visible: raiz.p.ok === true
                anchors { left: parent.left; bottom: parent.bottom }
                text: "Actualizado " + (raiz.p.actualizado || "") + " · Open-Meteo" + (raiz.p.viejo ? " · sin wifi: es el último que bajó" : "")
                color: raiz.p.viejo ? Tema.acento : Tema.tintaSuave; font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 15 }
            }
        }
    }

    // ---------------------------------------------------------------- derecha: 7 días
    Tarjeta {
        x: raiz.h ? 572 : 24; y: raiz.h ? 96 : 680
        width: raiz.h ? 680 : parent.width - 48; height: raiz.h ? 600 : 572
        Item {
            anchors { fill: parent; margins: 22 }
            Sub { tam: 12; text: "Próximos 7 días" }
            Column {
                y: 30; width: parent.width
                Repeater {
                    model: raiz.p.dias || []
                    Item {
                        required property var modelData
                        readonly property bool sel: raiz.diaElegido === modelData.fecha
                        readonly property bool carrera: raiz.fechasCarrera.indexOf(modelData.fecha) >= 0
                        width: parent.width; height: 66
                        Rectangle { anchors.fill: parent; radius: 10; color: parent.sel ? Qt.rgba(244 / 255, 197 / 255, 49 / 255, 0.22) : "transparent" }
                        Rectangle { anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 1; color: Qt.rgba(1, 1, 1, 0.12) }
                        Column {
                            x: 10; anchors.verticalCenter: parent.verticalCenter
                            Text { text: raiz.nombreDia(modelData.fecha, true); color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } }
                            Text { visible: parent.parent.carrera; text: "CARRERA"   // ese día hay una carrera cargada
                                   color: Tema.acento; font { family: Tema.fuente800; italic: true; pixelSize: 12; letterSpacing: 1.5 } }
                        }
                        IconoClima { x: 140; anchors.verticalCenter: parent.verticalCenter; width: 42; height: 42; codigo: modelData.codigo || 0 }
                        // Barra de la mínima a la máxima, sobre el rango de la semana
                        Item {
                            id: zonaBarra
                            x: 200; width: parent.width - 200 - 190; height: 8; anchors.verticalCenter: parent.verticalCenter
                            Rectangle { anchors.fill: parent; radius: 4; color: Qt.rgba(1, 1, 1, 0.1) }
                            Rectangle {
                                readonly property real a: raiz.rango[1] > raiz.rango[0] ? (modelData.min - raiz.rango[0]) / (raiz.rango[1] - raiz.rango[0]) : 0
                                readonly property real b: raiz.rango[1] > raiz.rango[0] ? (modelData.max - raiz.rango[0]) / (raiz.rango[1] - raiz.rango[0]) : 1
                                x: a * parent.width; width: Math.max(8, (b - a) * parent.width); height: 8; radius: 4
                                gradient: Gradient { orientation: Gradient.Horizontal
                                    GradientStop { position: 0; color: "#5fb4ff" } GradientStop { position: 0.6; color: "#f4c531" } GradientStop { position: 1; color: "#e8743b" } }
                            }
                        }
                        Text { anchors { right: parent.right; rightMargin: 110; verticalCenter: parent.verticalCenter }
                               text: (modelData.lluvia || 0) + "%"; color: "#5fb4ff"; font { family: Tema.fuente600; pixelSize: 19 } }
                        Text { anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                               textFormat: Text.StyledText
                               text: modelData.min + "° / <font color='" + Tema.acento + "'><b>" + modelData.max + "°</b></font>"
                               color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } }
                        MouseArea { anchors.fill: parent; onClicked: raiz.diaElegido = parent.sel ? "" : modelData.fecha }
                    }
                }
            }
            Text {
                anchors { left: parent.left; bottom: parent.bottom }
                width: parent.width; wrapMode: Text.WordWrap
                text: "Tocá un día para verlo hora por hora (otra vez: vuelve a ahora)"
                color: Tema.tintaSuave; font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: 15 }
            }
        }
    }

    BuscadorLugar { id: buscador; onElegido: l => raiz.elegirLugar(l) }
}
