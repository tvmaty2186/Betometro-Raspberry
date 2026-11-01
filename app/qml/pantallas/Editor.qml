import QtQuick
import Betometro

// Pantalla 7: editor de la pantalla de carrera. Muestras #s7 y #v7.
// Se edita el armado de la orientación elegida; «Guardar» lo deja para las carreras.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property bool hInicial: Datos.ajustes.orientacion !== "vertical"
    property var armado: Armado.cargar(h, Datos.ajustes)
    property bool cambiado: false
    property bool panel: false
    property bool lista: false
    readonly property var w: tablero.elegido >= 0 ? armado[tablero.elegido] : null
    Component.onCompleted: if (Contexto.prueba === "elegir") { tablero.elegido = armado.findIndex(x => x.t === "velocimetro"); panel = true; }

    function poner(a) { armado = a; cambiado = true; }
    function cambiarElegido(campos) {
        const a = armado.slice();
        a[tablero.elegido] = Object.assign({}, a[tablero.elegido], campos);
        poner(a);
    }
    function salir() { Contexto.horizontal = hInicial; Navegacion.ir("inicio"); }
    function orientacion(hor) {
        if (cambiado) { Navegacion.avisar("Primero guardá o volvé a fábrica"); return; }
        Contexto.horizontal = hor; tablero.elegido = -1; armado = Armado.cargar(hor, Datos.ajustes);
    }
    // Datos de muestra para ver los widgets llenos mientras se edita
    readonly property var muestra: ({ enCurso: true, n: 3, medida: true, transMs: 87700, restanteMs: 152300, proxMedida: false,
                                      sinGps: false, vActKmh: 78, vBaseKmh: 48, vNecKmh: 75, congelada: false, restanteM: 2310,
                                      deltaS: 1.2, velMaxVuelta: 81, vueltasTotal: 10, tiempoIdealMs: 240000, distanciaM: 3200 })

    Fondo { anchors.fill: parent }

    // Tablero achicado al 88 % desde abajo, como la muestra
    Item {
        anchors.fill: parent
        transform: Scale { origin.x: raiz.width / 2; origin.y: raiz.height; xScale: 0.88; yScale: 0.88 }
        Tablero {
            id: tablero
            anchors.fill: parent
            editando: true
            armado: raiz.armado
            est: raiz.muestra
            onCambio: a => raiz.poner(a)
        }
    }

    // Barra de arriba
    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: raiz.h ? 58 : 124; color: Qt.rgba(0, 0, 0, 0.6); z: 5
        Row {
            x: 20; y: raiz.h ? 9 : 12; spacing: 16
            Volver { width: 40; height: 40; anchors.verticalCenter: parent.verticalCenter
                     onTocado: { if (raiz.cambiado) Navegacion.avisar("Se salió sin guardar"); raiz.salir(); } }
            Text { text: "Pantalla de carrera"; color: Tema.blanco; anchors.verticalCenter: parent.verticalCenter
                   font { family: Tema.fuente800; italic: true; pixelSize: 22 } }
            Segmentado { anchors.verticalCenter: parent.verticalCenter; visible: raiz.h
                         opciones: [{ texto: "Vertical", valor: false }, { texto: "Horizontal", valor: true }]
                         valor: raiz.h; onElegido: v => raiz.orientacion(v) }
        }
        Segmentado { visible: !raiz.h; x: parent.width - width - 18; y: 12
                     opciones: [{ texto: "Vertical", valor: false }, { texto: "Horizontal", valor: true }]
                     valor: raiz.h; onElegido: v => raiz.orientacion(v) }
        Row {
            x: raiz.h ? parent.width - width - 20 : 16; y: raiz.h ? 8 : 68
            spacing: 12
            readonly property real ancho: raiz.h ? 150 : (raiz.width - 32 - 24) / 3
            BotonCromo { width: parent.ancho; height: 42; texto: "+ Widget"; onTocado: { raiz.lista = true; raiz.panel = false; } }
            DobleToque { width: parent.ancho; height: 42; textoNormal: "De fábrica"
                         onConfirmado: { raiz.poner(Armado.fabrica(raiz.h)); tablero.elegido = -1; } }
            BotonAmarillo { width: raiz.h ? 140 : parent.ancho; height: 42; tamTexto: 18; texto: "Guardar"
                onTocado: { Datos.ponerCfg(raiz.h ? "widgetsH" : "widgets", raiz.armado); raiz.cambiado = false; Navegacion.avisar("Pantalla guardada"); } }
        }
    }
    Rectangle {
        visible: tablero.elegido < 0 && !raiz.lista
        z: 6
        anchors.horizontalCenter: parent.horizontalCenter
        y: raiz.h ? 300 : 134
        width: ayuda.implicitWidth + 32; height: 40; radius: 12; color: Qt.rgba(0, 0, 0, 0.78)
        Text { id: ayuda; anchors.centerIn: parent; color: Tema.blanco; font { family: Tema.fuente600; italic: true; pixelSize: 17 }
               text: "Mantené apretado para mover · tocá para cambiar tamaño · ✎ opciones" }
    }

    // ✎ opciones del widget elegido
    Rectangle {
        visible: tablero.elegido >= 0
        z: 6
        width: 58; height: 58; radius: 29
        x: parent.width - width - 18; y: raiz.h ? 70 : 140
        color: Tema.amarillo; border { width: 2; color: "#000" }
        Text { anchors.centerIn: parent; text: "✎"; color: "#000"; font { pixelSize: 28; bold: true } }
        MouseArea { anchors.fill: parent; onClicked: raiz.panel = !raiz.panel }
    }
    Tarjeta {
        visible: raiz.panel && raiz.w !== null
        z: 7; oscura: true
        width: 360; height: colP.height + 20
        x: parent.width - width - 18; y: raiz.h ? 140 : 210
        Column {
            id: colP; x: 18; y: 14; width: parent.width - 36; spacing: 10
            Sub { text: "Widget: " + (raiz.w ? Armado.nombre(raiz.w.t) : ""); tam: 13 }
            BotonCromo { width: parent.width; height: 46; tamTexto: 17
                visible: !!raiz.w && (raiz.w.t === "velocimetro" || raiz.w.t === "tipo")
                texto: raiz.w && raiz.w.forma === "redondo" ? "Cuadrado" : "Redondo"
                onTocado: {
                    const W = tablero.width, H = tablero.height, p = tablero.px(raiz.w);
                    if (raiz.w.forma === "redondo") raiz.cambiarElegido({ forma: undefined, h: p.w / H });
                    else { const d = Math.min(p.w, p.h); raiz.cambiarElegido({ forma: "redondo", w: d / W, h: d / H }); }
                } }
            BotonCromo { width: parent.width; height: 46; tamTexto: 17
                visible: !!raiz.w && ["cuenta", "necesaria", "velocimetro", "faltan", "ultima"].indexOf(raiz.w.t) >= 0
                texto: raiz.w && raiz.w.extra ? "Sin dato extra" : "Dato extra"
                onTocado: raiz.cambiarElegido({ extra: !raiz.w.extra }) }
            BotonCromo { width: parent.width; height: 46; tamTexto: 17
                visible: !!raiz.w && (raiz.w.t === "cuenta" || (raiz.w.t === "velocimetro" && raiz.w.forma !== "redondo"))
                texto: raiz.w && raiz.w.barra !== false ? "Sin barra ±" : "Barra ±"
                onTocado: raiz.cambiarElegido({ barra: raiz.w.barra === false }) }
            BotonCromo { width: parent.width; height: 46; tamTexto: 17
                texto: raiz.w && raiz.w.t === "marcar" ? "MARCAR no se quita" : "✕ Quitar"
                activo: !!raiz.w && raiz.w.t !== "marcar"
                onTocado: { const a = raiz.armado.slice(); a.splice(tablero.elegido, 1); tablero.elegido = -1; raiz.panel = false; raiz.poner(a); } }
        }
    }
    // + Widget: la lista de tipos
    Rectangle {
        visible: raiz.lista
        anchors.fill: parent; z: 8; color: Qt.rgba(0, 0, 0, 0.6)
        MouseArea { anchors.fill: parent; onClicked: raiz.lista = false }
        Tarjeta {
            oscura: true
            anchors.centerIn: parent
            width: Math.min(parent.width - 40, 560); height: colL.height + 30
            Column {
                id: colL; x: 20; y: 16; width: parent.width - 40; spacing: 8
                TituloTarjeta { texto: "Agregar widget"; leftPadding: 0 }
                Repeater {
                    model: Armado.tipos.filter(t => t.t !== "marcar" || !raiz.armado.some(w => w.t === "marcar"))
                    BotonCromo {
                        required property var modelData
                        width: colL.width; height: 46; tamTexto: 17; texto: modelData.nombre
                        onTocado: {
                            const W = tablero.width, H = tablero.height;
                            const n = modelData.t === "velocimetro" ? { t: "velocimetro", forma: "redondo", x: 0.3, y: 0.3, w: 220 / W, h: 220 / H }
                                    : { t: modelData.t, x: (W - 240) / 2 / W, y: (H - 90) / 2 / H, w: 240 / W, h: 90 / H };
                            const a = raiz.armado.concat([n]);
                            raiz.poner(a); tablero.elegido = a.length - 1; raiz.lista = false;
                        }
                    }
                }
            }
        }
    }
}
