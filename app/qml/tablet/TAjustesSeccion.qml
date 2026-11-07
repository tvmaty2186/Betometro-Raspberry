import QtQuick
import Betometro

// Una sección de Ajustes en Modo Tablet: filas grandes con − / +, interruptores y opciones.
// Navegacion.parametros.seccion: auto, pantalla, voz, velocidad. Muestra: docs/muestras/tablet/voz-h.png
TPantalla {
    id: raiz
    readonly property string sec: (Navegacion.parametros || {}).seccion || (Contexto.prueba.indexOf("tablet-") === 0 ? Contexto.prueba.slice(7) : "pantalla")
    titulo: ({ auto: "Mi auto", pantalla: "Pantalla y tema", voz: "Voz y sonido", velocidad: "Velocidad y GPS", pulsador: "Pulsador" })[sec]
    volverA: "ajustes"
    readonly property var a: Datos.ajustes
    property string campo: ""
    function poner(k, v) { Datos.ponerCfg(k, v); }
    function entre(v, a, b) { return Math.max(a, Math.min(b, v)); }

    Flickable {
        anchors.fill: raiz.area
        contentHeight: col.height + 20
        clip: true; boundsBehavior: Flickable.StopAtBounds
        Column {
            id: col
            width: parent.width
            spacing: 14

            // ---- Mi auto
            TFila { visible: raiz.sec === "auto"; etiqueta: "Número"; tocable: true
                    onTocada: { raiz.campo = "numeroAuto"; teclado.abrir("texto", "Número del auto (hasta 4)", raiz.a.numeroAuto, 4); }
                Numero101 { d: 76; numero: raiz.a.numeroAuto || "101" } }
            TFila { visible: raiz.sec === "auto"; etiqueta: "Nombre"; detalle: raiz.a.nombreAuto || "–"; tocable: true
                    onTocada: { raiz.campo = "nombreAuto"; teclado.abrir("texto", "Nombre del auto", raiz.a.nombreAuto, 30); }
                TIcono { width: 44; height: 44; nombre: "auto" } }

            // ---- Pantalla y tema
            TFila { visible: raiz.sec === "pantalla"; etiqueta: "Inicio con botones"; detalle: "Apagado: el inicio con la foto del auto"
                TLlave { prendida: !!raiz.a.modoTablet; onCambiada: v => raiz.poner("modoTablet", v) } }
            TFila { visible: raiz.sec === "pantalla"; etiqueta: "Tema"
                TOpciones { opciones: [{ texto: "Avispón", valor: "avispon" }, { texto: "Rally", valor: "rally" }, { texto: "Deportivo", valor: "sport" }, { texto: "Cabina", valor: "cabina" }]
                            valor: raiz.a.tema || "avispon"; onElegido: v => { raiz.poner("tema", v); Contexto.tema = v; } } }
            TFila { visible: raiz.sec === "pantalla"; etiqueta: "Orientación"
                TOpciones { opciones: [{ texto: "Acostada", valor: "horizontal" }, { texto: "Parada", valor: "vertical" }]
                            valor: Contexto.horizontal ? "horizontal" : "vertical"
                            onElegido: v => { raiz.poner("orientacion", v); Contexto.horizontal = v === "horizontal"; } } }
            TFila { visible: raiz.sec === "pantalla"; etiqueta: "Brillo"
                TMasMenos { texto: Math.round(raiz.a.brillo) + "%"
                            onMenos: { const v = raiz.entre(Math.round(raiz.a.brillo) - 10, 10, 100); Sistema.brillo(v); raiz.poner("brillo", v); }
                            onMas: { const v = raiz.entre(Math.round(raiz.a.brillo) + 10, 10, 100); Sistema.brillo(v); raiz.poner("brillo", v); } } }
            TFila { visible: raiz.sec === "pantalla"; etiqueta: "Modo sol"; detalle: "Fondo claro para pleno sol"
                TLlave { prendida: raiz.a.modoSol === "si"; onCambiada: v => raiz.poner("modoSol", v ? "si" : "no") } }
            TFila { visible: raiz.sec === "pantalla"; etiqueta: "Color de la vuelta"
                TOpciones { opciones: [{ texto: "Fondo", valor: "fondo" }, { texto: "Marco", valor: "marco" }]
                            valor: raiz.a.indicador; onElegido: v => raiz.poner("indicador", v) } }

            // ---- Voz y sonido
            TFila { visible: raiz.sec === "voz"; etiqueta: "Volumen"
                TMasMenos { texto: Math.round(raiz.a.volumen) + "%"
                            onMenos: { const v = raiz.entre(Math.round(raiz.a.volumen) - 10, 0, 100); Audio.ponerVolumen(v); raiz.poner("volumen", v); }
                            onMas: { const v = raiz.entre(Math.round(raiz.a.volumen) + 10, 0, 100); Audio.ponerVolumen(v); raiz.poner("volumen", v); } }
                Rectangle { width: 150; height: 66; radius: 20; color: Qt.rgba(0, 0, 0, 0.4); border { width: 3; color: Tema.bordeCromo }
                    Row { anchors.centerIn: parent; spacing: 8
                        TIcono { width: 32; height: 32; nombre: "voz"; anchors.verticalCenter: parent.verticalCenter }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "Probar"; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 25 } } }
                    MouseArea { anchors.fill: parent; onClicked: Audio.lista ? Audio.probar() : Navegacion.avisar("Faltan los audios de la voz") } } }
            TFila { visible: raiz.sec === "voz"; etiqueta: "Voz"
                TOpciones { opciones: [{ texto: "Elena", valor: "elena" }, { texto: "Tomás", valor: "tomas" }]
                            valor: raiz.a.voz || "elena"; onElegido: v => { raiz.poner("voz", v); Qt.callLater(Audio.probar); } } }
            TFila { visible: raiz.sec === "voz"; etiqueta: "Cuenta en voz"; detalle: "Segundos antes del final"
                TMasMenos { texto: raiz.a.cuentaN + " s"
                            onMenos: raiz.poner("cuentaN", raiz.entre(raiz.a.cuentaN - 1, 0, 60)); onMas: raiz.poner("cuentaN", raiz.entre(raiz.a.cuentaN + 1, 0, 60)) } }
            TFila { visible: raiz.sec === "voz"; etiqueta: "Cómo contar"
                TOpciones { opciones: [{ texto: "Voz", valor: "voz" }, { texto: "Pitidos", valor: "pitidos" }]
                            valor: raiz.a.modoCuenta; onElegido: v => raiz.poner("modoCuenta", v) } }
            TFila { visible: raiz.sec === "voz"; etiqueta: "Anunciar cada vuelta"
                TLlave { prendida: !!raiz.a.avisoInicio; onCambiada: v => raiz.poner("avisoInicio", v) } }
            TFila { visible: raiz.sec === "voz"; etiqueta: "Clic al marcar"
                TLlave { prendida: !!raiz.a.clicMarca; onCambiada: v => raiz.poner("clicMarca", v) } }
            TFila { visible: raiz.sec === "voz"; etiqueta: "Pitido largo en cero"
                TLlave { prendida: !!raiz.a.pitidoCero; onCambiada: v => raiz.poner("pitidoCero", v) } }
            TFila { visible: raiz.sec === "voz"; etiqueta: "Pitido cada"; detalle: "0 = apagado"
                TMasMenos { texto: raiz.a.pitidoCadaS + " s"
                            onMenos: raiz.poner("pitidoCadaS", raiz.entre(raiz.a.pitidoCadaS - 5, 0, 600)); onMas: raiz.poner("pitidoCadaS", raiz.entre(raiz.a.pitidoCadaS + 5, 0, 600)) } }
            TFila { visible: raiz.sec === "voz"; etiqueta: "Anticipar la voz"
                TMasMenos { texto: raiz.a.anticipoMs + " ms"; anchoTexto: 170
                            onMenos: raiz.poner("anticipoMs", raiz.entre(raiz.a.anticipoMs - 50, 0, 1500)); onMas: raiz.poner("anticipoMs", raiz.entre(raiz.a.anticipoMs + 50, 0, 1500)) } }

            // ---- Velocidad y GPS
            TFila { visible: raiz.sec === "velocidad"; etiqueta: "Velocidad y adelanto"; detalle: "Usar el GPS en la carrera"
                TLlave { prendida: raiz.a.velActiva !== false; onCambiada: v => raiz.poner("velActiva", v) } }
            TFila { visible: raiz.sec === "velocidad"; etiqueta: "Módulo GPS"; detalle: "Al reiniciar"
                TOpciones { opciones: [{ texto: "Auto", valor: "auto" }, { texto: "M8N", valor: "m8" }, { texto: "7M", valor: "7" }, { texto: "6M", valor: "6" }]
                            valor: raiz.a.gpsModulo || "auto"; onElegido: v => raiz.poner("gpsModulo", v) } }
            TFila { visible: raiz.sec === "velocidad"; etiqueta: "Error máximo"; detalle: "Lecturas peores se descartan"
                TMasMenos { texto: raiz.a.umbralPrecisionM + " m"
                            onMenos: raiz.poner("umbralPrecisionM", raiz.entre(raiz.a.umbralPrecisionM - 5, 1, 200)); onMas: raiz.poner("umbralPrecisionM", raiz.entre(raiz.a.umbralPrecisionM + 5, 1, 200)) } }
            TFila { visible: raiz.sec === "velocidad"; etiqueta: "Calibración automática"
                TLlave { prendida: !!raiz.a.calibrar; onCambiada: v => raiz.poner("calibrar", v) } }
            // ---- Pulsador (1.0.29). MARCAR en pantalla opcional (Matías, 07/11/2026): apagado, sin botón; larga y
            //      marca solo el pulsador, sin nada en pantalla como plan B.
            TFila { visible: raiz.sec === "pulsador"; etiqueta: "MARCAR en pantalla"
                    detalle: raiz.a.marcarPantalla !== false ? "Se marca con el botón o con el pulsador" : "Larga y marca solo el pulsador"
                TLlave { prendida: raiz.a.marcarPantalla !== false; onCambiada: v => raiz.poner("marcarPantalla", v) } }
            TFila { visible: raiz.sec === "pulsador"; etiqueta: "Probar el pulsador"; detalle: "En el Chequeo previo"; tocable: true
                    onTocada: Navegacion.ir("chequeo")
                TIcono { width: 40; height: 40; nombre: "check" } }
            TFila { visible: raiz.sec === "pulsador"; etiqueta: "Ignorar 2.ª marca"; detalle: "Durante"
                TMasMenos { texto: String(raiz.a.antirreboteMs / 1000).replace(".", ",") + " s"
                            onMenos: raiz.poner("antirreboteMs", Math.max(0, raiz.a.antirreboteMs - 500)); onMas: raiz.poner("antirreboteMs", Math.min(60000, raiz.a.antirreboteMs + 500)) } }
        }
    }
    Teclado {
        id: teclado
        parent: raiz
        onAceptado: v => {
            if (raiz.campo === "numeroAuto") raiz.poner("numeroAuto", String(v).trim().slice(0, 4) || "101");
            else if (raiz.campo === "nombreAuto") raiz.poner("nombreAuto", String(v).trim());
        }
        onVisibleChanged: if (!visible) raiz.campo = ""
    }
}
