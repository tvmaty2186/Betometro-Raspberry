import QtQuick
import Betometro

// Clima en Modo Tablet. Muestra: docs/muestras/tablet/clima-h.png
// Arriba, grande: ahora (o el día tocado). Abajo, los próximos días en botones; tocar uno lo muestra arriba.
// Mismos datos y lugares que la pantalla normal (Pronostico.qml): GPS, localidad buscada o la de una carrera.
TPantalla {
    id: raiz
    titulo: "Clima"
    readonly property var par: Navegacion.parametros || ({})
    readonly property bool deCarrera: par.lat !== undefined && par.lat !== null
    volverA: deCarrera ? "carreras" : "inicio"
    property bool usarGps: !deCarrera && Datos.ajustes.climaModo !== "lugar"
    property var lugar: deCarrera ? { nombre: par.nombre, lat: par.lat, lon: par.lon } : (Datos.ajustes.climaLugar || null)
    property string diaElegido: par.fecha || ""
    readonly property bool hayLugar: usarGps ? Clima.hayGps : !!(lugar && lugar.lat !== undefined)
    readonly property real lat: usarGps ? Clima.gpsLat : (lugar && lugar.lat !== undefined ? lugar.lat : 0)
    readonly property real lon: usarGps ? Clima.gpsLon : (lugar && lugar.lon !== undefined ? lugar.lon : 0)
    readonly property string nombre: usarGps ? (Clima.nombreGps || "Tu ubicación") : (lugar && lugar.nombre ? lugar.nombre : "")
    readonly property var p: Clima.version >= 0 && hayLugar ? Clima.pronostico(lat, lon) : ({})
    readonly property var d: diaElegido && hayLugar && Clima.version >= 0 ? Clima.dia(lat, lon, diaElegido) : null
    readonly property var fechasCarrera: { const l = []; for (const c of Datos.proximas) l.push(c.fecha); return l; }
    function nombreDia(iso) {
        const f = Formato.desdeIso(iso);
        if (f.toDateString() === new Date().toDateString()) return "Hoy";
        return ["Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"][f.getDay()] + " " + f.getDate();
    }
    function elegirLugar(l) {
        Datos.ponerCfg("climaLugar", { nombre: l.nombre, detalle: l.detalle || "", lat: l.lat, lon: l.lon });
        Datos.ponerCfg("climaModo", "lugar");
        lugar = { nombre: l.nombre, lat: l.lat, lon: l.lon }; usarGps = false; diaElegido = "";
    }

    // ---- Arriba: ahora o el día elegido
    Rectangle {
        id: arriba
        width: raiz.area.width; height: raiz.h ? 250 : 470
        radius: Math.max(14, Tema.radio * 1.5)
        border { width: 3; color: Tema.bordeCromo }
        gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
        readonly property var a: raiz.d ? raiz.d : (raiz.p.actual || {})
        readonly property bool hay: raiz.p.ok === true && (!raiz.d || raiz.d.disponible)
        Text {
            visible: !arriba.hay
            anchors { left: parent.left; leftMargin: 30; top: parent.top; topMargin: 30; right: raiz.h ? botones.left : parent.right; rightMargin: 20 }
            wrapMode: Text.WordWrap; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 }
            text: !raiz.hayLugar ? (raiz.usarGps ? "Esperando el GPS.\nO buscá una localidad." : "Buscá una localidad.")
                  : raiz.d && !raiz.d.disponible ? "Ese día todavía no hay pronóstico.\nDesde el " + raiz.d.desde + "."
                  : raiz.p.cargando ? "Bajando el pronóstico…" : (raiz.p.error || "Sin pronóstico (hace falta wifi)")
        }
        Grid {
            visible: arriba.hay
            x: 26; width: raiz.h ? botones.x - 40 : parent.width - 52
            y: raiz.h ? (parent.height - height) / 2 : 22
            columns: raiz.h ? 3 : 2
            verticalItemAlignment: Grid.AlignVCenter
            spacing: 26
            IconoClima { width: raiz.h ? 170 : 150; height: width; codigo: arriba.a.codigo || 0 }
            Text { text: raiz.d ? arriba.a.max + "°" : (arriba.a.temp !== undefined ? arriba.a.temp + "°" : "")
                   color: Tema.numero; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 150 } }
            Column {
                spacing: 8
                Text { text: raiz.nombre + (raiz.d ? " · " + raiz.nombreDia(raiz.diaElegido) : ""); color: Tema.tinta
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 30 } }
                Text { visible: !!raiz.d; text: "Mínima " + arriba.a.min + "°"; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                Text { text: "Lluvia " + (arriba.a.lluvia || 0) + " %"; color: "#5fb4ff"; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                Text { text: "Viento " + (arriba.a.viento || 0) + " · ráfagas " + (arriba.a.rafagas || 0); color: Tema.tinta
                       font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
            }
        }
        Grid {
            id: botones
            columns: raiz.h ? 1 : 2
            x: raiz.h ? parent.width - width - 24 : (parent.width - width) / 2
            y: raiz.h ? (parent.height - height) / 2 : parent.height - height - 20
            spacing: 16
            component Opcion: Rectangle {
                property string texto
                property string icono
                property bool prendida
                signal tocada()
                width: 220; height: 72; radius: 22
                color: prendida ? Tema.acento : Qt.rgba(0, 0, 0, 0.4)
                border { width: 3; color: prendida ? Tema.acento : Tema.bordeCromo }
                Row { anchors.centerIn: parent; spacing: 10
                    TIcono { width: 34; height: 34; nombre: parent.parent.icono; color: parent.parent.prendida ? Tema.acentoTinta : Tema.tinta; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: Math.min(implicitWidth, 150); elide: Text.ElideRight; text: parent.parent.texto; anchors.verticalCenter: parent.verticalCenter
                           color: parent.parent.prendida ? Tema.acentoTinta : Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 25 } } }
                MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: parent.tocada() }
            }
            Opcion { texto: "GPS"; icono: "gps"; prendida: raiz.usarGps
                     onTocada: { Datos.ponerCfg("climaModo", "gps"); raiz.usarGps = true; raiz.diaElegido = ""; } }
            Opcion { texto: !raiz.usarGps && raiz.lugar && raiz.lugar.nombre ? raiz.lugar.nombre : "Buscar"; icono: "buscar"; prendida: !raiz.usarGps
                     onTocada: buscador.abrir("") }
        }
    }

    // ---- Abajo: los próximos días (tocar uno lo muestra arriba; otra vez, vuelve a ahora)
    Grid {
        y: arriba.height + 18
        width: raiz.area.width; height: raiz.area.height - y
        columns: raiz.h ? 5 : 3
        spacing: 16
        readonly property int filas: raiz.h ? 1 : 2
        readonly property real an: (width - (columns - 1) * spacing) / columns
        readonly property real al: (height - (filas - 1) * spacing) / filas
        Repeater {
            model: (raiz.p.dias || []).slice(0, raiz.h ? 5 : 6)
            Rectangle {
                required property var modelData
                readonly property bool sel: raiz.diaElegido === modelData.fecha
                readonly property bool carrera: raiz.fechasCarrera.indexOf(modelData.fecha) >= 0
                width: parent.an; height: parent.al
                radius: Math.max(14, Tema.radio * 1.5)
                border { width: 3; color: sel ? Tema.acento : Tema.bordeCromo }
                gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.25) } GradientStop { position: 1; color: Tema.caja2 } }
                Column {
                    anchors.centerIn: parent; spacing: 4
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: raiz.nombreDia(modelData.fecha); color: Tema.tinta
                           font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 28 } }
                    Text { visible: parent.parent.carrera; anchors.horizontalCenter: parent.horizontalCenter; text: "CARRERA"; color: Tema.acento
                           font { family: Tema.fuente800; italic: true; pixelSize: 15; letterSpacing: 2 } }
                    IconoClima { anchors.horizontalCenter: parent.horizontalCenter; width: 74; height: 74; codigo: modelData.codigo || 0 }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.max + "°"; color: Tema.numero
                           font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 44 } }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.min + "°"; color: Tema.tintaSuave
                           font { family: Tema.fuente600; pixelSize: 23 } }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: (modelData.lluvia || 0) + "%"; color: "#5fb4ff"
                           font { family: Tema.fuente600; pixelSize: 23 } }
                }
                MouseArea { anchors.fill: parent; onClicked: raiz.diaElegido = parent.sel ? "" : modelData.fecha }
            }
        }
    }
    BuscadorLugar { id: buscador; parent: raiz; onElegido: l => raiz.elegirLugar(l) }
}
