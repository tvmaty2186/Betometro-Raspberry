import QtQuick
import QtMultimedia
import Betometro
import Betometro.Video

// Lo que se ve del video: la imagen, los datos encima, los chips a medias y la barra de abajo.
// El estado (reproductor, sincronía, ajustes de los datos) está en el Reproductor (`rep`). Hay una vista
// en la Pi (dentro del informe, se toca) y otra en la tele (modo tele, no se toca): las dos ubican los
// datos con las mismas fracciones guardadas (cfg `videoDatos`), así quedan iguales.
Item {
    id: vista
    property Item rep: null
    property bool completo: false        // pantalla completa (la tele siempre)
    property bool tocable: true          // la de la tele no se toca
    readonly property alias salida: salida
    readonly property var e: rep.e
    readonly property bool editando: tocable && rep.editando
    readonly property bool menu: tocable && rep.menuDatos
    function prendido(clave) { return rep.prendido(clave); }
    clip: true

    Rectangle { anchors.fill: parent; color: "#000000" }
    // No VideoOutput: en la Pi 3 queda negro (el chip gráfico no puede con su programa). Ver app/imagen_video.h
    ImagenVideo {
        id: salida
        anchors.fill: parent
        rotation: vista.rep.vid && vista.rep.vid.girar ? 180 : 0
        // espejo (como cámara de selfie): solo el video, los datos de encima no
        transform: Scale { origin.x: salida.width / 2; xScale: vista.rep.vid && vista.rep.vid.espejo ? -1 : 1 }
    }
    Text {
        anchors.centerIn: parent
        visible: vista.rep.falta || !vista.rep.vid
        text: vista.rep.vid ? "No está el archivo del video" : "Esta carrera no tiene video"
        color: Tema.tinta
        font { family: Tema.fuente600; italic: Tema.cursiva; pixelSize: vista.completo ? 30 : 22 }
    }

    // Un toque: anda / pausa. Doble toque: pantalla completa (otro doble toque vuelve).
    Timer { id: unToque; interval: 280; onTriggered: vista.rep.alternar() }
    TapHandler {
        enabled: vista.tocable && !vista.editando && !vista.menu
        onTapped: (punto, boton) => { if (tapCount === 1) unToque.restart(); }
        onDoubleTapped: { unToque.stop(); vista.rep.dobleToque(); }
    }

    // Botón grande de play en pausa (pantalla completa). Con el pulsador por cable aparece un ratito
    // (también a medias): play si quedó en pausa, pausa si arrancó.
    Timer { id: destello; interval: 800 }
    Connections { target: vista.rep; function onDestello() { destello.restart(); } }
    Rectangle {
        visible: !vista.editando && !vista.rep.falta && !!vista.rep.vid && (destello.running || (vista.completo && !vista.rep.andando))
        anchors.centerIn: parent
        width: 120 * vista.s; height: width; radius: width / 2
        color: Qt.rgba(0, 0, 0, 0.53); border { width: 3; color: "#ffffff" }
        Triangulo { visible: !vista.rep.andando; anchors.centerIn: parent; anchors.horizontalCenterOffset: 6 * vista.s; width: 44 * vista.s; height: 56 * vista.s }
        Row {
            visible: vista.rep.andando; anchors.centerIn: parent; spacing: 12 * vista.s
            Rectangle { width: 14 * vista.s; height: 50 * vista.s; color: "#ffffff" }
            Rectangle { width: 14 * vista.s; height: 50 * vista.s; color: "#ffffff" }
        }
    }

    // ---------- Datos encima (pantalla completa) ----------
    readonly property real s: Math.max(0.55, Math.min(width / 1280, height / 720))
    readonly property real margen: 22 * s
    readonly property real arriba: 18 * s
    readonly property real abajo: height - 84 * s     // arriba de la barra
    function xDe(clave, w) { return margen + rep.lugar(clave).x * Math.max(0, width - 2 * margen - w); }
    function yDe(clave, h) { return arriba + rep.lugar(clave).y * Math.max(0, abajo - arriba - h); }
    function guardarPos(clave, x, y, w, h) {
        const aw = width - 2 * margen - w, ah = abajo - arriba - h;
        const fx = aw > 0 ? (x - margen) / aw : 0, fy = ah > 0 ? (y - arriba) / ah : 0;
        rep.guardar(clave, { x: Math.round(Math.max(0, Math.min(1, fx)) * 1000) / 1000, y: Math.round(Math.max(0, Math.min(1, fy)) * 1000) / 1000 });
        guiaX = -1; guiaY = -1; pegaA = "";
    }
    // Imán: el dato se pega a los bordes y a los otros datos (bordes alineados o uno al lado del otro)
    property real guiaX: -1
    property real guiaY: -1
    property string pegaA: ""
    function imantar(item, nx, ny) {
        const w = item.width, h = item.height, u = 16 * s, sep = 10 * s;
        nx = Math.max(margen, Math.min(width - margen - w, nx));
        ny = Math.max(arriba, Math.min(abajo - h, ny));
        let bx = null, by = null;
        const px = (v, gx, quien) => { const dd = Math.abs(v - nx); if (dd < u && (!bx || dd < bx.dd)) bx = { v: v, dd: dd, g: gx, q: quien }; };
        const py = (v, gy, quien) => { const dd = Math.abs(v - ny); if (dd < u && (!by || dd < by.dd)) by = { v: v, dd: dd, g: gy, q: quien }; };
        px(margen, margen, ""); px(width - margen - w, width - margen, ""); px(width / 2 - w / 2, width / 2, "");
        py(arriba, arriba, ""); py(abajo - h, abajo, "");
        for (const o of datos) {
            if (o === item || !o.visible) continue;
            px(o.x, o.x, o.nombre); px(o.x + o.width - w, o.x + o.width, o.nombre);
            px(o.x + o.width + sep, o.x + o.width + sep, o.nombre); px(o.x - sep - w, o.x - sep, o.nombre);
            py(o.y, o.y, o.nombre); py(o.y + o.height - h, o.y + o.height, o.nombre);
            py(o.y + o.height + sep, o.y + o.height + sep, o.nombre); py(o.y - sep - h, o.y - sep, o.nombre);
        }
        guiaX = bx ? bx.g : -1; guiaY = by ? by.g : -1;
        pegaA = by && by.q ? by.q : bx && bx.q ? bx.q : "";
        return { x: bx ? bx.v : nx, y: by ? by.v : ny };
    }
    readonly property var datos: [dVuelta, dAdelanto, dTiempo, dDistancia, dVelocidad, dPista]

    // Tira finita de una línea
    component Tira: Rectangle {
        default property alias hijos: fila.data
        property real s: vista.s
        width: fila.implicitWidth + 36 * s; height: 52 * s; radius: height / 2
        color: Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.8)
        border { width: 2; color: Tema.bordeCromo }
        Row { id: fila; anchors.centerIn: parent; spacing: 10 * parent.s }
    }
    component Etiq: Text {
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        color: Tema.tintaSuave
        font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 14 * vista.s; letterSpacing: 2 * vista.s; capitalization: Font.AllUppercase }
    }
    component Num: Text {
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        color: Tema.tinta
        font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 38 * vista.s }
    }
    component Uni: Text {
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        color: Tema.tintaSuave
        font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 20 * vista.s }
    }
    component Triangulo: Canvas {
        property color color: "#ffffff"
        onWidthChanged: requestPaint()
        onColorChanged: requestPaint()
        onPaint: { const g = getContext("2d"); g.reset(); g.fillStyle = color; g.beginPath(); g.moveTo(0, 0); g.lineTo(width, height / 2); g.lineTo(0, height); g.closePath(); g.fill(); }
    }

    DatoVideo {
        id: dVuelta; reproductor: vista; clave: "vuelta"; nombre: "Vuelta"
        Rectangle {
            width: tv.implicitWidth + 40 * vista.s; height: 52 * vista.s; radius: height / 2
            color: Tema.acento; border { width: 2; color: Tema.acento }
            Text {
                id: tv; anchors.centerIn: parent
                text: vista.e.lap >= 0 ? vista.rep.nombreV(vista.e.v) + " · " + (vista.e.v.interrumpida ? "INTERRUMPIDA" : vista.e.v.medida ? "MEDIDA" : "LIBRE")
                      : vista.e.antes ? "ANTES DE LARGAR" : "TERMINADA"
                color: Tema.acentoTinta
                font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 22 * vista.s }
            }
        }
    }
    DatoVideo {
        id: dAdelanto; reproductor: vista; clave: "adelanto"; nombre: "Adelanto"
        Tira {
            // ade > 0: adelantado. Mismo signo y colores que la lectura del informe (+ adelantado, − atrasado; como la app)
            id: tAde
            readonly property real a: vista.e.ade
            readonly property bool enTiempo: Math.abs(a) < 0.2
            readonly property color col: Math.abs(a) < 1 ? Tema.bien : Math.abs(a) < 3 ? Tema.acento : "#d23b2c"
            Num { visible: !tAde.enTiempo; text: Formato.dif(tAde.a * 1000); color: tAde.col }
            Etiq { text: tAde.enTiempo ? "en tiempo" : tAde.a < 0 ? "atrasado" : "adelantado"; color: tAde.col }
        }
    }
    DatoVideo {
        id: dTiempo; reproductor: vista; clave: "tiempo"; nombre: "Vuelta"
        Tira { Etiq { text: "Vuelta" } Num { text: vista.e.lap >= 0 ? Formato.tiempo(vista.e.tMs) : "–" } }
    }
    DatoVideo {
        id: dDistancia; reproductor: vista; clave: "distancia"; nombre: "Distancia"
        Tira {
            Etiq { text: "Dist." }
            Num { text: vista.e.lap >= 0 ? Formato.miles(vista.e.d) : "–" }
            Uni { text: "/ " + Formato.miles(vista.e.L) + " m" }
        }
    }
    DatoVideo {
        id: dVelocidad; reproductor: vista; clave: "velocidad"; nombre: "Velocidad"
        Tira {
            Etiq { text: "Vel." }
            Num { text: vista.e.lap >= 0 && vista.e.ok ? String(Math.round(vista.e.kmh)) : "–" }
            Uni { text: "km/h" }
            Item {   // barrita contra la sugerida: el medio es ir justo
                visible: vista.rep.sug > 0
                width: 90 * vista.s; height: 17 * vista.s; anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 7 * vista.s; radius: height / 2
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: "#d23b2c" } GradientStop { position: 0.25; color: Tema.sugerida }
                        GradientStop { position: 0.5; color: Tema.bien } GradientStop { position: 0.75; color: Tema.sugerida }
                        GradientStop { position: 1; color: "#d23b2c" }
                    }
                }
                Rectangle {
                    width: 4 * vista.s; height: parent.height; radius: 2 * vista.s; color: "#ffffff"
                    x: parent.width * Math.max(0, Math.min(1, 0.5 + (vista.e.kmh - vista.rep.sug) / 20)) - width / 2
                }
            }
            Etiq { visible: vista.rep.sug > 0; text: "sug." }
            Uni { visible: vista.rep.sug > 0; text: vista.rep.sug.toFixed(1).replace(".", ","); color: Tema.sugerida; font.pixelSize: 18 * vista.s }
        }
    }
    DatoVideo {
        id: dPista; reproductor: vista; clave: "pista"; nombre: "Pista"
        Rectangle {
            width: 246 * vista.s; height: 166 * vista.s; radius: 18 * vista.s
            color: Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.8); border { width: 2; color: Tema.bordeCromo }
            Pista { anchors { fill: parent; margins: 10 * vista.s } inf: vista.rep.inf; punto: vista.e; grosor: 0.6 }
        }
    }

    // Guías del imán mientras se mueve un dato
    Rectangle { visible: vista.guiaX >= 0; x: vista.guiaX - 1; width: 2; height: parent.height; color: Tema.sugerida; opacity: 0.8; z: 4 }
    Rectangle { visible: vista.guiaY >= 0; y: vista.guiaY - 1; height: 2; width: parent.width; color: Tema.sugerida; opacity: 0.8; z: 4 }
    Text {
        readonly property Item m: vista.datos.find(o => o.moviendo) || null
        visible: !!m && vista.pegaA !== ""
        z: 6
        x: m ? Math.max(8, Math.min(vista.width - width - 8, m.x + m.width / 2 - width / 2)) : 0
        y: m ? m.y + m.height + 14 * vista.s : 0
        text: "se pega a «" + vista.pegaA + "»"
        color: Tema.sugerida; style: Text.Outline; styleColor: "#000000"
        font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 18 * vista.s }
    }

    // ---------- Chips a medias (dentro del informe) ----------
    Row {
        visible: !vista.completo && !!vista.rep.vid && !vista.rep.falta
        x: 14; y: 12; spacing: 8
        // Dos chips fijos con el texto enlazado: el Repeater recibía una lista nueva 10 veces por segundo y borraba y
        // creaba todo (03-1.2)
        component Chip: Rectangle {
            property string texto
            property bool primero
            width: tc.implicitWidth + 24; height: 32; radius: 16
            color: primero ? Tema.acento : Qt.rgba(0, 0, 0, 0.55); border { width: 1; color: primero ? Tema.acento : Tema.bordeCromo }
            Text { id: tc; anchors.centerIn: parent; text: parent.texto; color: parent.primero ? Tema.acentoTinta : Tema.tinta
                   font { family: Tema.fuente600; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 16 } }
        }
        Chip { primero: true
               texto: vista.e.lap >= 0 ? vista.rep.nombreV(vista.e.v) + " · " + (vista.e.v.medida ? "medida" : "libre") : (vista.e.antes ? "Antes de largar" : "Terminada") }
        Chip { visible: vista.e.lap >= 0; texto: vista.e.lap >= 0 ? Formato.tiempo(vista.e.tMs) + " de la vuelta" : "" }
    }

    // ---------- Barra de abajo: play/pausa, tiempo, línea con una rayita por vuelta, total, engranaje ----------
    Rectangle {
        id: barra
        visible: !!vista.rep.vid && !vista.rep.falta
        readonly property real b: vista.completo ? vista.s : 0.85
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: vista.completo ? 22 * vista.s : 12 }
        anchors.bottomMargin: vista.completo ? 18 * vista.s : 12
        height: 52 * b; radius: height / 2
        color: Qt.rgba(0, 0, 0, 0.67); border { width: 2; color: Tema.bordeCromo }
        z: 3
        Item {   // play / pausa
            id: bPlay
            x: 14 * barra.b; width: 34 * barra.b; height: parent.height
            Triangulo { visible: !vista.rep.andando; anchors.centerIn: parent; width: 20 * barra.b; height: 24 * barra.b }
            Row {
                visible: vista.rep.andando; anchors.centerIn: parent; spacing: 6 * barra.b
                Rectangle { width: 6 * barra.b; height: 22 * barra.b; color: "#ffffff" }
                Rectangle { width: 6 * barra.b; height: 22 * barra.b; color: "#ffffff" }
            }
            MouseArea { anchors.fill: parent; anchors.margins: -8; enabled: vista.tocable; onClicked: vista.rep.alternar() }
        }
        Text {
            id: tAhora
            x: bPlay.x + bPlay.width + 10 * barra.b; anchors.verticalCenter: parent.verticalCenter
            text: vista.rep.reloj(vista.rep.arrastre >= 0 ? vista.rep.arrastre : vista.rep.pos)
            color: "#ffffff"; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 26 * barra.b }
        }
        Text {
            id: tTotal
            anchors { right: bPantalla.visible ? bPantalla.left : bDatos.visible ? bDatos.left : parent.right
                      rightMargin: bPantalla.visible || bDatos.visible ? 12 * barra.b : 20 * barra.b; verticalCenter: parent.verticalCenter }
            text: vista.rep.reloj(vista.rep.total)
            color: "#ffffff"; opacity: 0.8; font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 26 * barra.b }
        }
        // Pantalla completa / achicar, a la derecha del tiempo (Matías, 06/11/2026: el doble toque a veces no
        // entraba). Las cuatro esquinas hacia afuera para agrandar, hacia adentro para achicar.
        Item {
            id: bPantalla
            visible: vista.tocable && !vista.editando
            anchors { right: bDatos.visible ? bDatos.left : parent.right; rightMargin: bDatos.visible ? 12 * barra.b : 16 * barra.b
                      verticalCenter: parent.verticalCenter }
            width: 42 * barra.b; height: width
            Canvas {
                id: iconoPantalla
                anchors.centerIn: parent; width: 28 * barra.b; height: width
                property bool adentro: vista.completo
                onAdentroChanged: requestPaint()
                onWidthChanged: requestPaint()
                onPaint: {
                    const c = getContext("2d"), w = width, k = w * 0.36, g = Math.max(2, w * 0.11);
                    c.reset(); c.strokeStyle = "#ffffff"; c.lineWidth = g; c.lineCap = "round"; c.lineJoin = "round";
                    const p = g / 2, q = w - g / 2;
                    // [esquina x, esquina y, dirección x, dirección y]
                    for (const e of [[p, p, 1, 1], [q, p, -1, 1], [p, q, 1, -1], [q, q, -1, -1]]) {
                        // agrandar: el codo en la esquina; achicar: el codo hacia el centro
                        const m = adentro ? w * 0.28 : k;   // achicar: codos más separados del centro
                        const x = adentro ? e[0] + e[2] * m : e[0], y = adentro ? e[1] + e[3] * m : e[1];
                        const sx = adentro ? -e[2] : e[2], sy = adentro ? -e[3] : e[3];
                        c.beginPath(); c.moveTo(x + sx * m, y); c.lineTo(x, y); c.lineTo(x, y + sy * m); c.stroke();
                    }
                }
            }
            MouseArea { anchors.fill: parent; anchors.margins: -8; onClicked: vista.rep.dobleToque() }
        }
        // Engranaje en pausa (pantalla completa): abre el menú de los datos
        Rectangle {
            id: bDatos
            visible: vista.tocable && vista.completo && !vista.rep.andando && !vista.editando
            anchors { right: parent.right; rightMargin: 18 * barra.b; verticalCenter: parent.verticalCenter }
            height: 42 * barra.b; width: height; radius: height / 2
            color: Tema.acento
            Engranaje { anchors.centerIn: parent; width: 28 * barra.b; height: width; color: Tema.acentoTinta }
            MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: vista.rep.menuDatos = !vista.rep.menuDatos }
        }
        // Línea de tiempo
        Item {
            id: linea
            anchors { left: tAhora.right; leftMargin: 16 * barra.b; right: tTotal.left; rightMargin: 16 * barra.b; verticalCenter: parent.verticalCenter }
            height: 24 * barra.b
            readonly property real f: Math.max(0, Math.min(1, (vista.rep.arrastre >= 0 ? vista.rep.arrastre : vista.rep.pos) / vista.rep.total))
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 8 * barra.b; radius: height / 2; color: Qt.rgba(1, 1, 1, 0.2) }
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width * linea.f; height: 8 * barra.b; radius: height / 2; color: Tema.acento }
            Repeater {
                model: vista.rep.rayitas(linea.width)
                Rectangle {
                    required property var modelData
                    visible: modelData.desde >= 0 && modelData.desde <= vista.rep.total
                    x: linea.width * modelData.desde / vista.rep.total - width / 2; width: 3 * barra.b; height: linea.height; color: "#ffffff"
                }
            }
            MouseArea {
                anchors { fill: parent; topMargin: -14; bottomMargin: -14 }
                enabled: vista.tocable
                preventStealing: true
                onPressed: (m) => vista.rep.arrastre = vista.rep.msEn(m.x, linea.width)
                onPositionChanged: (m) => vista.rep.arrastre = vista.rep.msEn(m.x, linea.width)
                onReleased: vista.rep.soltarArrastre()
                onCanceled: vista.rep.arrastre = -1
            }
        }
    }

    // ---------- Menú del engranaje (Matías, 06/11/2026): prender y apagar cada dato; «Mover los datos» deja
    // moverlos en la pantalla hasta «Guardar». Tocar afuera del menú lo cierra. ----------
    MouseArea {   // afuera del menú: lo cierra
        anchors.fill: parent; z: 19
        visible: vista.menu && vista.completo
        onClicked: vista.rep.menuDatos = false
    }
    Rectangle {
        visible: vista.menu && vista.completo
        z: 20
        width: Math.min(vista.width - 40, 616 * vista.s); height: colDatos.height + 36 * vista.s
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 8 * vista.s }
        radius: 18 * vista.s
        color: Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.97); border { width: 2; color: Tema.bordeCromo }
        MouseArea { anchors.fill: parent }   // los toques en el menú no lo cierran ni llegan al video
        Column {
            id: colDatos
            x: 24 * vista.s; y: 18 * vista.s; width: parent.width - 48 * vista.s
            Text { text: "Datos sobre el video"; color: Tema.tinta; height: 44 * vista.s
                   font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 24 * vista.s } }
            Repeater {
                model: vista.rep.lista
                Item {
                    id: filaDato
                    required property var modelData
                    required property int index
                    readonly property bool sePuede: !(modelData.clave === "adelanto" && !(vista.rep.distancia > 0))
                    width: colDatos.width; height: 54 * vista.s
                    Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.12); visible: index > 0 }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: modelData.nombre; color: Tema.tinta
                           font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 22 * vista.s } }
                    Llave {
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        scale: 1.4 * vista.s; transformOrigin: Item.Right
                        prendida: vista.rep.prendido(filaDato.modelData.clave)
                        opacity: filaDato.sePuede ? 1 : 0.4
                        enabled: false   // la fila entera es el botón (más fácil de tocar)
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: filaDato.sePuede
                        onClicked: vista.rep.guardar(filaDato.modelData.clave, { on: !vista.rep.prendido(filaDato.modelData.clave) })
                    }
                }
            }
            Item { width: 1; height: 10 * vista.s }
            Rectangle {   // mover los datos
                width: parent.width; height: 56 * vista.s; radius: height / 2
                color: zonaMover.pressed ? Qt.lighter(Tema.acento, 1.2) : Tema.acento
                Text { anchors.centerIn: parent; text: "✎  Mover los datos en la pantalla"; color: Tema.acentoTinta
                       font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 21 * vista.s } }
                MouseArea { id: zonaMover; anchors.fill: parent; onClicked: { vista.rep.menuDatos = false; vista.rep.editando = true; } }
            }
            Text {
                width: parent.width; wrapMode: Text.WordWrap; topPadding: 8 * vista.s
                text: "Tocá afuera para cerrar"
                color: Tema.tintaSuave; font { family: Tema.fuente600; pixelSize: 17 * vista.s }
            }
        }
    }
    // Moviendo los datos: aviso arriba y «Guardar»
    Rectangle {
        visible: vista.editando && vista.completo
        z: 20
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 12 * vista.s }
        height: 60 * vista.s; width: filaGuardar.implicitWidth + 30 * vista.s; radius: height / 2
        color: Qt.rgba(Tema.caja2.r, Tema.caja2.g, Tema.caja2.b, 0.95); border { width: 2; color: Tema.bordeCromo }
        Row {
            id: filaGuardar; anchors.centerIn: parent; spacing: 16 * vista.s
            Text { anchors.verticalCenter: parent.verticalCenter; text: "Mantené apretado un dato y arrastralo"; color: Tema.tinta
                   font { family: Tema.fuente600; pixelSize: 19 * vista.s } }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                height: 44 * vista.s; width: tGuardar.implicitWidth + 36 * vista.s; radius: height / 2; color: Tema.acento
                Text { id: tGuardar; anchors.centerIn: parent; text: "✓ Guardar"; color: Tema.acentoTinta
                       font { family: Tema.fuente800; italic: true; weight: Font.ExtraBold; pixelSize: 20 * vista.s } }
                MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: vista.rep.editando = false }
            }
        }
    }
}
