import QtQuick
import Betometro

// Tabla del informe: todas las vueltas (tipo MEDIDA/LIBRE que se toca y cambia, tiempo, diferencia, velocidad o
// dónde empieza en el video), «Unir V5 con V6» al mantener apretada una vuelta 3 s (app 2.4.1), el menú ⋮
// (restablecer la carrera original, compartir) y el error total al pie.
// Marca virtual por GPS (Matías, 08/11/2026, muestra docs/muestras/informe-gps/a.png): al lado del pulsador manual,
// el tiempo y el error del pulsador virtual, solo de referencia; ★ en la vuelta modelo (la más cerca del ideal),
// y abajo el error total de cada uno y cuánto tarde (o temprano) se marcó en promedio.
// Lo que se elige acá (fila, menú, confirmaciones) vive acá; los cambios a la carrera salen como señales.
Tarjeta { datos: true;
    id: caja
    property bool h: true
    property bool vv: false                 // se está viendo el video: la columna de la derecha es «▸ hora del video»
    property var c: ({})                    // Datos.carrera(id): errorTotal, promedio, mejor, modificada
    property var vueltas: []
    property var inf: ({})                  // Datos.informe(id): largadas
    property var gps: []                    // Datos.marcasGps(id): una por vuelta
    readonly property bool conGps: gps.some(m => m && m.hay)
    // Vueltas donde se pasó la línea a mitad de camino sin marcar (¿faltó una marca?): solo un dato
    readonly property var sinMarca: gps.map((m, i) => m && m.cruceSinMarcaMs >= 0 ? i : -1).filter(i => i >= 0)
    readonly property color celeste: Tema.sol || Tema.datosClaros ? "#0b5394" : "#7fd0ff"
    // La vuelta modelo: la medida más cerca del ideal
    readonly property int modelo: {
        let k = -1;
        for (let i = 0; i < vueltas.length; ++i) {
            const v = vueltas[i];
            if (!v.medida || v.difMs === undefined || v.interrumpida) continue;
            if (k < 0 || Math.abs(v.difMs) < Math.abs(vueltas[k].difMs)) k = i;
        }
        return k;
    }
    function seg(ms, dec) { return (Math.abs(ms) / 1000).toFixed(dec).replace(".", ",") + " s"; }
    property int lap: -1                    // la vuelta elegida (se resalta y queda a la vista)
    property Item reproductor: null         // para «▸ hora del video» de cada vuelta
    property int fila: -1                   // vuelta elegida para «Unir» (manteniéndola apretada 3 s)
    property bool confirmarUnir: false
    property bool menuAbierto: false
    property bool confirmarRest: false
    signal verVuelta(int indice)            // se tocó o mantuvo una vuelta: si tiene recorrido, pasa a ser la elegida
    signal irVideo(int indice, real t)
    signal cambiarMedida(int indice)
    signal unir(int indice)                 // ya confirmado: juntar esa vuelta con la siguiente
    signal restablecer()                    // ya confirmado
    signal compartir()
    signal verClima()                       // el clima guardado en la carrera (Clima → A carrera)
    onFilaChanged: confirmarUnir = false
    Timer { id: tUnirVuelve; interval: 3000; onTriggered: caja.confirmarUnir = false }
    Timer { id: tRestVuelve; interval: 3000; onTriggered: caja.confirmarRest = false }
    function nombreV(v) { return "V" + v.n + (inf.largadas > 1 ? " L" + v.largada : ""); }
    function colorDif(ms) { const s = Math.abs(ms / 1000); return s <= 0.2 ? Tema.bien : s <= 0.5 ? Tema.acento : Tema.alerta; }
    // Momento del video en que empieza cada vuelta
    function inicioVideo(i) { const m = reproductor ? reproductor.momento(i, 0) : null; return m ? Formato.horaVideo(m.ms) : ""; }
    // Una marca de más parte una vuelta en dos: se juntan desde acá
    function puedeUnir(i) {
        if (i < 0 || i + 1 >= vueltas.length) return false;
        const a = vueltas[i], b = vueltas[i + 1];
        return a.largada === b.largada && !a.interrumpida && !b.interrumpida;
    }
    function tocarUnir() {
        if (!confirmarUnir) { confirmarUnir = true; tUnirVuelve.restart(); return; }
        unir(fila);
        confirmarUnir = false;
    }
    function tocarRestablecer() {
        if (!confirmarRest) { confirmarRest = true; tRestVuelve.restart(); return; }
        confirmarRest = false; menuAbierto = false;
        restablecer();
    }
    function cerrarMenu() { menuAbierto = false; confirmarRest = false; }
    function limpiar() { fila = -1; confirmarUnir = false; menuAbierto = false; confirmarRest = false; }

    z: caja.menuAbierto ? 20 : 0   // con el menú abierto, la tarjeta queda arriba de la capa que lo cierra
    oscura: true
    x: caja.h ? 648 : 20; y: caja.h ? 362 : 900
    width: caja.h ? 608 : parent.width - 40; height: caja.h ? 338 : 360
    Sub { x: 22; y: 16; text: "Todas las vueltas"; tam: 12 }
    // Con el menú abierto, tocar en otra parte de la tarjeta lo cierra
    MouseArea { anchors.fill: parent; z: 9; visible: caja.menuAbierto; onClicked: caja.cerrarMenu() }
    // Menú ⋮ (app 2.4.1): restablecer la carrera original y compartir
    Item {
        id: mas3
        anchors { right: parent.right; rightMargin: 10; top: parent.top; topMargin: 4 }
        width: 44; height: 36
        Rectangle { anchors.fill: parent; radius: 18; color: caja.menuAbierto ? Qt.rgba(1, 1, 1, 0.12) : "transparent" }
        Text { anchors.centerIn: parent; text: "⋮"; color: Tema.tinta; font { pixelSize: 26; weight: Font.Black } }
        MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: { caja.menuAbierto = !caja.menuAbierto; caja.confirmarRest = false; } }
    }
    Rectangle {
        visible: caja.menuAbierto
        z: 10
        anchors { right: mas3.right; top: mas3.bottom; topMargin: 4 }
        width: 330; height: colMenu.height + 12; radius: 12
        color: "#22262b"; border { width: 1; color: Qt.rgba(1, 1, 1, 0.18) }
        Column {
            id: colMenu; x: 6; y: 6; width: parent.width - 12
            Rectangle {
                width: parent.width; height: 52; radius: 8
                readonly property bool puede: !!caja.c.modificada
                color: caja.confirmarRest ? Tema.alerta : (mr.pressed ? Qt.rgba(1, 1, 1, 0.08) : "transparent")
                opacity: puede ? 1 : 0.35
                Text { x: 12; anchors.verticalCenter: parent.verticalCenter; width: parent.width - 24; elide: Text.ElideRight
                       text: caja.confirmarRest ? "¿Seguro? Se pierden los cambios" : "↺ Restablecer la carrera original"
                       color: caja.confirmarRest ? "#fff" : Tema.tinta; font { family: Tema.fuente600; pixelSize: 17 } }
                MouseArea { id: mr; anchors.fill: parent; enabled: parent.puede; onClicked: caja.tocarRestablecer() }
            }
            Rectangle {
                width: parent.width; height: 52; radius: 8
                color: mc.pressed ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                Text { x: 12; anchors.verticalCenter: parent.verticalCenter; text: "⤴ Compartir informe"; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 17 } }
                MouseArea { id: mc; anchors.fill: parent; onClicked: { caja.menuAbierto = false; caja.compartir(); } }
            }
            Rectangle {
                visible: !!caja.c.climaAdjunto
                width: parent.width; height: visible ? 52 : 0; radius: 8
                color: mcl.pressed ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                Text { x: 12; anchors.verticalCenter: parent.verticalCenter; text: "Ver el clima guardado"; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 17 } }
                MouseArea { id: mcl; anchors.fill: parent; onClicked: { caja.menuAbierto = false; caja.verClima(); } }
            }
        }
    }
    // «Unir V5 con V6»: barra amarilla, aparece al mantener apretada una vuelta 3 s
    BotonAmarillo {
        id: barraUnir
        visible: caja.puedeUnir(caja.fila)
        x: 22; y: 40; width: parent.width - 44; height: 40; tamTexto: 16
        texto: !visible ? "" : caja.confirmarUnir ? "¿Seguro? Tocá de nuevo"
               : "Unir " + caja.nombreV(caja.vueltas[caja.fila]) + " con " + caja.nombreV(caja.vueltas[caja.fila + 1])
        onTocado: caja.tocarUnir()
    }
    // Encabezado: PULSADOR MANUAL y PULSADOR VIRTUAL (GPS), cada uno con TIEMPO y ERROR
    Item {
        visible: caja.conGps
        x: 28; y: barraUnir.visible ? 90 : 40; width: parent.width - 56; height: 34
        readonly property real u: (width - 12) / 10
        Rectangle { x: parent.u * 6.2 + 6; width: parent.u * 3; height: parent.height; color: Qt.rgba(41 / 255, 182 / 255, 246 / 255, 0.12); radius: 6 }
        Text { x: parent.u * 3.2 + 6; y: 0; width: parent.u * 3; horizontalAlignment: Text.AlignHCenter; text: "PULSADOR MANUAL"; color: Tema.tinta
               font { family: Tema.fuente600; pixelSize: 11; letterSpacing: 1.2 } }
        Rectangle { x: parent.u * 3.2 + 12; y: 15; width: parent.u * 3 - 12; height: 1.5; color: Qt.rgba(1, 1, 1, 0.35) }
        Text { x: parent.u * 6.2 + 6; y: 0; width: parent.u * 3; horizontalAlignment: Text.AlignHCenter; text: "PULSADOR VIRTUAL (GPS)"; color: caja.celeste
               font { family: Tema.fuente600; pixelSize: 11; letterSpacing: 1.2 } }
        Rectangle { x: parent.u * 6.2 + 12; y: 15; width: parent.u * 3 - 12; height: 1.5; color: caja.celeste; opacity: 0.5 }
        Repeater {
            model: [[3.2, "TIEMPO"], [4.8, "ERROR"], [6.3, "TIEMPO"], [7.9, "ERROR"], [9.35, "KM/H"]]
            Text { required property var modelData; x: modelData[0] * parent.u + 6; y: 19; text: modelData[1]; color: Tema.tintaSuave
                   font { family: Tema.fuente600; pixelSize: 10; letterSpacing: 1.2 } }
        }
    }
    Flickable {
        id: tabla
        anchors { fill: parent; topMargin: (barraUnir.visible ? 90 : 40) + (caja.conGps ? 34 : 0); leftMargin: 22; rightMargin: 22; bottomMargin: 12 }
        contentHeight: colT.height; clip: true; boundsBehavior: Flickable.StopAtBounds
        // Con muchas vueltas, la vuelta elegida (o la del video) queda a la vista
        function verFila(i) {
            if (i < 0 || moving) return;
            if (i * 40 < contentY) contentY = i * 40;
            else if (i * 40 + 40 > contentY + height) contentY = Math.min(contentHeight - height, i * 40 + 40 - height);
        }
        onContentHeightChanged: verFila(caja.lap)
        Connections { target: caja; function onLapChanged() { tabla.verFila(caja.lap); } }
        Column {
            id: colT; width: tabla.width
            Repeater {
                model: caja.vueltas
                Item {
                    required property var modelData
                    required property int index
                    width: colT.width; height: 40
                    Rectangle { anchors.fill: parent; radius: 8; color: Qt.rgba(244 / 255, 197 / 255, 49 / 255, index === caja.fila ? 0.32 : 0.18); visible: index === (caja.fila >= 0 ? caja.fila : caja.lap) }
                    Rectangle {   // se llena mientras se mantiene apretada
                        id: llenado; height: parent.height; radius: 8; width: 0
                        color: Qt.rgba(244 / 255, 197 / 255, 49 / 255, 0.25)
                        NumberAnimation on width { id: animLlenado; running: false; from: 0; to: colT.width; duration: 3000 }
                    }
                    MouseArea {
                        anchors.fill: parent
                        pressAndHoldInterval: 3000
                        onPressed: animLlenado.restart()
                        onReleased: { animLlenado.stop(); llenado.width = 0; }
                        onCanceled: { animLlenado.stop(); llenado.width = 0; }
                        onClicked: { caja.fila = -1; caja.verVuelta(index); caja.irVideo(index, 0); }
                        onPressAndHold: { caja.fila = index; caja.verVuelta(index); }
                    }
                    Row {
                        id: filaV
                        anchors.verticalCenter: parent.verticalCenter; x: 6
                        readonly property real u: (colT.width - 12) / 10
                        readonly property var g: caja.conGps ? caja.gps[index] : null
                        // Cada columna centrada en la fila: la velocidad (letra más chica) quedaba más arriba (08-15)
                        Text { anchors.verticalCenter: parent.verticalCenter; width: parent.u * (caja.conGps ? 1.2 : 1.6); textFormat: Text.StyledText
                               text: caja.nombreV(modelData) + (index === caja.modelo && caja.conGps ? "<font color='" + Tema.acento + "'> ★</font>" : "")
                                     + (caja.sinMarca.indexOf(index) >= 0 ? "<font color='" + Tema.warn + "'> ⚠</font>" : "")
                               color: Tema.numero; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 11; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: caja.conGps ? 22 : 24 } }
                        // Etiqueta MEDIDA / LIBRE: se toca y cambia (la libre no suma error)
                        Item { anchors.verticalCenter: parent.verticalCenter; width: parent.u * (caja.conGps ? 2.0 : 3); height: 30
                            Rectangle {
                                id: etiq
                                readonly property color col: modelData.interrumpida ? "#8a929b" : modelData.medida ? Tema.rojo : Tema.bien
                                anchors.verticalCenter: parent.verticalCenter
                                width: tEtiq.implicitWidth + (caja.conGps ? 14 : 22); height: caja.conGps ? 24 : 28; radius: height / 2
                                color: Qt.rgba(col.r, col.g, col.b, 0.18); border { width: 1.5; color: col }
                                Text { id: tEtiq; anchors.centerIn: parent; color: etiq.col
                                       text: modelData.interrumpida ? "INTERRUMPIDA" : modelData.medida ? "MEDIDA" : "LIBRE"
                                       font { family: Tema.fuente800; weight: Font.ExtraBold; pixelSize: caja.conGps ? 13 : 16; letterSpacing: 1 } }
                                MouseArea { anchors.fill: parent; anchors.margins: -6; enabled: !modelData.interrumpida
                                            onClicked: caja.cambiarMedida(index) }
                            } }
                        Text { anchors.verticalCenter: parent.verticalCenter; width: parent.u * (caja.conGps ? 1.6 : 2.2); text: Formato.tiempo(modelData.realMs); color: Tema.numero; rightPadding: 10; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 11; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: caja.conGps ? 22 : 24 } }
                        Text { anchors.verticalCenter: parent.verticalCenter; width: parent.u * (caja.conGps ? 1.4 : 2); text: modelData.difMs === undefined ? "–" : Formato.dif(modelData.difMs)
                               color: modelData.difMs === undefined ? Tema.tintaSuave : caja.colorDif(modelData.difMs); rightPadding: 8; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 11
                               font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: caja.conGps ? 22 : 24 } }
                        // Pulsador virtual (GPS): solo referencia
                        Rectangle {
                            visible: caja.conGps; width: parent.u * 3; height: 40; color: Qt.rgba(41 / 255, 182 / 255, 246 / 255, 0.12)
                            readonly property bool ok: !!filaV.g && filaV.g.hay
                            Text { x: 4; anchors.verticalCenter: parent.verticalCenter; width: parent.width * 0.53
                                   text: parent.ok && !filaV.g.dudosa ? Formato.tiempo(filaV.g.realMs) : "—"; color: caja.celeste; rightPadding: 8; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 11
                                   font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 22 } }
                            Text { x: parent.width * 0.53; anchors.verticalCenter: parent.verticalCenter; width: parent.width * 0.47 - 4; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 11
                                   text: !parent.ok ? "sin datos" : filaV.g.dudosa ? "dudosa" : Formato.dif(filaV.g.difMs)
                                   color: parent.ok && !filaV.g.dudosa ? caja.celeste : Tema.tintaSuave
                                   font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: parent.ok && !filaV.g.dudosa ? 22 : 15 } }
                        }
                        // Sin la columna «▸ hora del video»: con el video también va la velocidad (Matías, 09/11/2026)
                        Text { anchors.verticalCenter: parent.verticalCenter; width: parent.u * (caja.conGps ? 0.8 : 1.2); horizontalAlignment: Text.AlignRight
                               text: modelData.velPromKmh !== null && modelData.velPromKmh !== undefined ? String(modelData.velPromKmh).replace(".", ",") : "–"
                               color: Tema.tintaSuave; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 11; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: caja.conGps ? 18 : 20 } }
                    }
                }
            }
            Item {
                width: colT.width; height: (caja.conGps ? 92 : 64) + caja.sinMarca.length * 24
                readonly property var med: caja.vueltas.filter(v => v.medida && v.difMs !== undefined)
                visible: med.length > 0
                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.2) }
                Sub { x: 6; y: 12; tam: 13; text: "Error total · " + parent.med.length + (parent.med.length === 1 ? " medida" : " medidas") }
                Sub { x: 6; y: 40; tam: 12; text: "promedio " + (caja.c.promedio || "–") + " · mejor " + (caja.c.mejor || "–") }
                Text { anchors { right: parent.right; rightMargin: 6; top: parent.top; topMargin: 6 }
                       text: caja.c.errorTotal || ""; color: Tema.acento; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 26 } }
                // Pulsador virtual: error total (sin las dudosas) y cuánto tarde o temprano se marcó en promedio
                Text {
                    visible: caja.conGps; x: 6; y: 64; width: parent.width - 12; elide: Text.ElideRight; textFormat: Text.StyledText
                    readonly property var buenas: caja.gps.filter(m => m && m.hay && !m.dudosa)
                    readonly property var medidas: caja.gps.filter((m, i) => m && m.hay && !m.dudosa && caja.vueltas[i] && caja.vueltas[i].medida)
                    readonly property real total: medidas.reduce((a, m) => a + Math.abs(m.difMs), 0)
                    readonly property real prom: buenas.length ? buenas.reduce((a, m) => a + m.pulsadorMenosGpsMs, 0) / buenas.length : 0
                    readonly property bool hayDudosas: caja.gps.some(m => m && m.hay && m.dudosa)
                    text: "Virtual: error total <font color='" + caja.celeste + "'><b>" + caja.seg(total, 1) + "</b></font>"
                          + (hayDudosas ? " (sin las dudosas)" : "")
                          + (buenas.length ? " · marcaste en promedio <font color='" + Tema.acento + "'><b>" + caja.seg(prom, 2)
                             + (Math.abs(prom) < 5 ? "" : prom > 0 ? " tarde" : " temprano") + "</b></font>" : "")
                    color: Tema.tintaSuave; font { family: Tema.fuente600; italic: true; pixelSize: 15 }
                }
                // ⚠ Pasó la línea sin marcar
                Column {
                    x: 6; y: caja.conGps ? 90 : 64; width: parent.width - 12
                    Repeater {
                        model: caja.sinMarca
                        Text {
                            required property int modelData
                            width: parent.width; elide: Text.ElideRight
                            text: "⚠ " + caja.nombreV(caja.vueltas[modelData]) + ": pasaste la línea a los " + Formato.tiempo(caja.gps[modelData].cruceSinMarcaMs) + " sin marcar (¿faltó una marca?)"
                            color: Tema.warn; font { family: Tema.fuente600; italic: true; pixelSize: 15 }
                        }
                    }
                }
            }
        }
    }
}
