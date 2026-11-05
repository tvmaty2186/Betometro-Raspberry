import QtQuick
import Betometro

// Pantalla 8: Ajustes. Muestras #s8 y #v8. Todo se guarda al instante.
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    readonly property var a: Datos.ajustes
    property string campo: ""
    readonly property var temas: [{ v: "avispon", t: "Avispón Verde" }, { v: "rally", t: "Rally clásico" }, { v: "sport", t: "Deportivo moderno" }, { v: "cabina", t: "Cabina de avión" }]
    function nombreTema(v) { for (const t of temas) if (t.v === v) return t.t; return v; }
    function poner(clave, valor) { Datos.ponerCfg(clave, valor); }
    function editar(clave, modo, titulo, valor, max) { campo = clave; teclado.abrir(modo, titulo, valor, max); }

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "AJUSTES"; derechaRayas: raiz.h ? 90 : 30; onVolver: Navegacion.ir("inicio") }

    // ---- Grupos ----
    Component {
        id: gAuto
        Tarjeta {
            height: 150
            TituloTarjeta { id: t1; texto: "Mi auto" }
            Row {
                x: 22; anchors.top: t1.bottom; anchors.topMargin: 8
                spacing: 20
                Numero101 { d: 78; numero: raiz.a.numeroAuto || "101"
                    MouseArea { anchors.fill: parent; onClicked: raiz.editar("numeroAuto", "texto", "Número del auto (hasta 4)", raiz.a.numeroAuto, 4) } }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Sub { text: "Número y nombre (tocá para cambiar)" }
                    Text { text: raiz.a.nombreAuto || "–"; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 26 }
                        MouseArea { anchors.fill: parent; anchors.margins: -10; onClicked: raiz.editar("nombreAuto", "texto", "Nombre del auto", raiz.a.nombreAuto, 30) } }
                }
            }
        }
    }
    Component {
        id: gTema
        Tarjeta {
            height: colT.height + 10
            Column {
                id: colT; width: parent.width
                TituloTarjeta { texto: "Tema y pantalla" }
                Fila { etiqueta: "Tema"
                    Campo { texto: raiz.nombreTema(raiz.a.tema); onTocado: {
                        let i = raiz.temas.findIndex(t => t.v === raiz.a.tema);
                        const v = raiz.temas[(i + 1) % raiz.temas.length].v;
                        raiz.poner("tema", v); Contexto.tema = v; } } }
                Fila { etiqueta: "Orientación"
                    Segmentado { opciones: [{ texto: "Vertical", valor: "vertical" }, { texto: "Horizontal", valor: "horizontal" }]
                        valor: Contexto.horizontal ? "horizontal" : "vertical"
                        onElegido: v => { raiz.poner("orientacion", v); Contexto.horizontal = v === "horizontal"; } } }
                Fila { etiqueta: "Modo Tablet (botones grandes)"
                    Llave { prendida: !!raiz.a.modoTablet; onCambiada: v => raiz.poner("modoTablet", v) } }
                Fila { etiqueta: "Modo sol"
                    Segmentado { opciones: [{ texto: "Sí", valor: "si" }, { texto: "No", valor: "no" }]
                        valor: raiz.a.modoSol; onElegido: v => raiz.poner("modoSol", v) } }
                Fila { etiqueta: "Brillo"
                    Deslizador { width: raiz.h ? 220 : 260; minimo: 10; valor: brillo.v
                        property QtObject brillo: QtObject { property real v: raiz.a.brillo }
                        onMovido: x => { brillo.v = x; Sistema.brillo(x); }   // se ve mientras se arrastra
                        onSoltado: x => raiz.poner("brillo", x) } }
                Fila { etiqueta: "Tipo de vuelta"; ultima: true
                    Segmentado { opciones: [{ texto: "Fondo", valor: "fondo" }, { texto: "Marco", valor: "marco" }]
                        valor: raiz.a.indicador; onElegido: v => raiz.poner("indicador", v) } }
            }
        }
    }
    Component {
        id: gVoz
        Tarjeta {
            height: colV.height + 10
            Column {
                id: colV; width: parent.width
                TituloTarjeta { texto: "Voz y sonido" }
                Fila { etiqueta: "Volumen auriculares"
                    Row { spacing: 12
                        Deslizador { width: raiz.h ? 170 : 220; valor: vol.v; anchors.verticalCenter: parent.verticalCenter
                            property QtObject vol: QtObject { property real v: raiz.a.volumen }
                            onMovido: x => { vol.v = x; Audio.ponerVolumen(x); }; onSoltado: x => { raiz.poner("volumen", x); Audio.probar(); } }
                        BotonCromo { width: 100; height: 42; texto: "▶ Probar"; tamTexto: 15; onTocado: Audio.lista ? Audio.probar() : Navegacion.avisar("Faltan los audios de la voz") } } }
                Fila { etiqueta: "Voz"
                    Segmentado { opciones: [{ texto: "Elena", valor: "elena" }, { texto: "Tomás", valor: "tomas" }]
                        valor: raiz.a.voz || "elena"; onElegido: v => { raiz.poner("voz", v); Qt.callLater(Audio.probar); } } }
                Fila { etiqueta: "Segundos que cuenta en voz"
                    Campo { texto: raiz.a.cuentaN; onTocado: raiz.editar("cuentaN", "numero", "Segundos que cuenta en voz (0 a 60, 0 = sin cuenta)", raiz.a.cuentaN, 2) } }
                Fila { etiqueta: "Cómo contar"
                    Segmentado { opciones: [{ texto: "Voz", valor: "voz" }, { texto: "Pitidos", valor: "pitidos" }]
                        valor: raiz.a.modoCuenta; onElegido: v => raiz.poner("modoCuenta", v) } }
                Fila { etiqueta: "Pitido largo al llegar a cero"
                    Llave { prendida: !!raiz.a.pitidoCero; onCambiada: v => raiz.poner("pitidoCero", v) } }
                Fila { etiqueta: "Clic al marcar"
                    Llave { prendida: !!raiz.a.clicMarca; onCambiada: v => raiz.poner("clicMarca", v) } }
                Fila { etiqueta: "Anunciar cada vuelta"
                    Llave { prendida: !!raiz.a.avisoInicio; onCambiada: v => raiz.poner("avisoInicio", v) } }
                Fila { etiqueta: "Pitido cada (s, 0 = no)"
                    Campo { texto: raiz.a.pitidoCadaS; onTocado: raiz.editar("pitidoCadaS", "numero", "Pitido cada cuántos segundos (0 = apagado)", raiz.a.pitidoCadaS, 3) } }
                Fila { etiqueta: "Anticipar la voz (ms)"; ultima: true
                    Campo { texto: raiz.a.anticipoMs; onTocado: raiz.editar("anticipoMs", "numero", "Anticipar la voz, en milésimas (0 a 1500)", raiz.a.anticipoMs, 4) } }
            }
        }
    }
    Component {
        id: gVelocidad
        Tarjeta {
            height: colG.height + 10
            Column {
                id: colG; width: parent.width
                TituloTarjeta { texto: "Velocidad y marcas" }
                Fila { etiqueta: "Mostrar velocidad y adelanto"
                    Llave { prendida: !!raiz.a.velActiva; onCambiada: v => raiz.poner("velActiva", v) } }
                Fila { etiqueta: "Descartar error mayor a (m)"
                    Campo { texto: raiz.a.umbralPrecisionM; onTocado: raiz.editar("umbralPrecisionM", "numero", "Descartar lecturas del GPS con error mayor a (metros, 1 a 200)", raiz.a.umbralPrecisionM, 3) } }
                Fila { etiqueta: "Módulo GPS (al reiniciar)"
                    Segmentado { opciones: [{ texto: "Auto", valor: "auto" }, { texto: "M8N", valor: "m8" }, { texto: "7M", valor: "7" }, { texto: "6M", valor: "6" }]
                        valor: raiz.a.gpsModulo || "auto"; onElegido: v => raiz.poner("gpsModulo", v) } }
                Fila { etiqueta: "Calibración automática"
                    Llave { prendida: !!raiz.a.calibrar; onCambiada: v => raiz.poner("calibrar", v) } }
                Fila { etiqueta: "Ignorar 2.ª marca durante"; ultima: true
                    Row { spacing: 10
                        BotonCromo { width: 46; height: 46; texto: "−"; tamTexto: 26; anchors.verticalCenter: parent.verticalCenter
                            onTocado: raiz.poner("antirreboteMs", Math.max(0, raiz.a.antirreboteMs - 500)) }
                        Campo { texto: String(raiz.a.antirreboteMs / 1000).replace(".", ",") + " s"; activo: false; opacity: 1 }
                        BotonCromo { width: 46; height: 46; texto: "+"; tamTexto: 26; anchors.verticalCenter: parent.verticalCenter
                            onTocado: raiz.poner("antirreboteMs", Math.min(60000, raiz.a.antirreboteMs + 500)) } } }
            }
        }
    }
    Component {
        id: gSistema
        Tarjeta {
            height: colS.height + 10
            Column {
                id: colS; width: parent.width
                TituloTarjeta { texto: "Conexión y sistema" }
                Fila { etiqueta: "Conectividad"; tocable: true; onTocada: Navegacion.ir("conectividad")
                    Text { text: (Wifi.conectada || "Sin conexión") + "  ›"; color: Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } } }
                Fila { etiqueta: "Cámara y micrófono"; tocable: true; onTocada: Navegacion.ir("camara")
                    Text { text: (!raiz.a.camara ? "Apagada" : Camara.hay ? "Lista" : "Sin cámara") + "  ›"
                           color: raiz.a.camara && !Camara.hay ? Tema.alerta : Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } } }
                Fila { etiqueta: "Google Drive"; tocable: true; onTocada: Navegacion.ir("drive")
                    Text { text: (Drive.estado === "conectado" ? (Drive.subiendo ? "Subiendo" : Drive.error ? "Con un problema" : "Conectado") : "Sin conectar") + "  ›"
                           color: Drive.error ? Tema.alerta : Tema.tinta; font { family: Tema.fuente600; pixelSize: 20 } } }
                Fila { etiqueta: "Versión " + Contexto.version
                    BotonAmarillo { visible: !!Contexto.versionNueva; width: 210; height: 48; tamTexto: 18; texto: "Actualizar a " + Contexto.versionNueva
                        onTocado: Navegacion.ir("actualizar") }
                    BotonCromo { visible: !Contexto.versionNueva; width: 210; height: 48; tamTexto: 17; texto: "Buscar actualización"
                        onTocado: Navegacion.ir("actualizar") } }
                Fila { etiqueta: "Apagar el equipo"; ultima: true
                    DobleToque { width: 170; height: 46; textoNormal: "Apagar"; onConfirmado: Sistema.apagar() } }
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
            id: izq; visible: raiz.h
            width: raiz.h ? 590 : 0; spacing: 18; topPadding: 6
            Loader { width: parent.width; sourceComponent: raiz.h ? gAuto : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gTema : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gVelocidad : null }
        }
        Column {
            id: der; visible: raiz.h
            x: 620; width: raiz.h ? 580 : 0; spacing: 18; topPadding: 6
            Loader { width: parent.width; sourceComponent: raiz.h ? gVoz : null }
            Loader { width: parent.width; sourceComponent: raiz.h ? gSistema : null }
        }
        Column {
            id: unica; visible: !raiz.h
            width: lista.width; spacing: 16; topPadding: 4
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gAuto }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gTema }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gVoz }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gSistema }
            Loader { width: parent.width; sourceComponent: raiz.h ? null : gVelocidad }
        }
    }

    Teclado {
        id: teclado
        onAceptado: v => {
            const lim = { cuentaN: [0, 60], pitidoCadaS: [0, 600], anticipoMs: [0, 1500], umbralPrecisionM: [1, 200] };
            if (lim[raiz.campo]) raiz.poner(raiz.campo, Math.max(lim[raiz.campo][0], Math.min(lim[raiz.campo][1], v)));
            else if (raiz.campo === "numeroAuto") raiz.poner("numeroAuto", String(v).trim().slice(0, 4) || "101");
            else if (raiz.campo === "nombreAuto") raiz.poner("nombreAuto", String(v).trim());
        }
        onVisibleChanged: if (!visible) raiz.campo = ""
    }
}
