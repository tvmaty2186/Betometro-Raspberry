import QtQuick
import Betometro

// Pantalla 3: Ficha de la carrera (datos editables + resumen + LARGAR). Muestras #s3 y #v3.
// Sirve también para una carrera nueva (id vacío).
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    property var c: Datos.carrera(Navegacion.parametros.id || "")
    // Lo que se está editando (se guarda con Guardar o al largar)
    property string nombre: c.nombre
    property string fecha: c.fecha
    property int distanciaM: c.distanciaM
    property int tiempoIdealMs: c.tiempoIdealMs
    property string patron: c.patron
    property string campo: ""        // campo abierto en el teclado
    property string error: ""
    readonly property bool nueva: !c.id
    readonly property bool fijos: c.tieneVueltas    // con vueltas no se cambian tiempo, distancia ni patrón
    readonly property bool cambiada: nombre !== c.nombre || fecha !== c.fecha
                                     || distanciaM !== c.distanciaM || tiempoIdealMs !== c.tiempoIdealMs || patron !== c.patron

    function datos() {
        return { id: c.id || "", nombre: nombre, fecha: fecha,
                 distanciaM: distanciaM, tiempoIdealMs: tiempoIdealMs, patron: patron };
    }
    function guardar() {
        const r = Datos.guardarCarrera(datos());
        if (!r.ok) { error = r.error; return ""; }
        error = "";
        c = Datos.carrera(r.id);
        Navegacion.parametros = { id: r.id };
        return r.id;
    }
    function idealCorto(ms) { const t = Formato.tiempo(ms); return t.endsWith(",0") ? t.slice(0, -2) : t; }

    Fondo { anchors.fill: parent }
    CabeceraPantalla {
        titulo: raiz.nueva ? "NUEVA CARRERA" : raiz.nombre.toUpperCase()
        derechaRayas: raiz.h ? 90 : -200
        onVolver: Navegacion.ir("carreras")
    }

    // Resumen grande
    Tarjeta {
        id: resumen
        oscura: true
        x: raiz.h ? 680 : 24; y: raiz.h ? 110 : 100
        width: raiz.h ? 560 : parent.width - 48; height: raiz.h ? 150 : 140
        // Corrida: error total, promedio y mejor (como el detalle de la app)
        Row {
            visible: c.medidas > 0
            anchors.centerIn: parent
            spacing: raiz.h ? 44 : 54
            Repeater {
                model: [{ t: "Error total", v: c.errorTotal }, { t: "Promedio", v: c.promedio }, { t: "Mejor", v: c.mejor }]
                Column {
                    required property var modelData
                    required property int index
                    Sub { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.t }
                    Digito {
                        anchors.horizontalCenter: parent.horizontalCenter; tam: raiz.h ? 56 : 52; text: modelData.v
                        color: index === 0 ? (c.nivel === "ok" ? Tema.bien : c.nivel === "medio" ? Tema.acento : c.nivel === "mal" ? Tema.alerta : Tema.numero) : Tema.numero
                    }
                }
            }
        }
        Row {
            visible: c.medidas === 0
            anchors.centerIn: parent
            spacing: raiz.h ? 50 : 60
            Column {
                Sub { anchors.horizontalCenter: parent.horizontalCenter; text: "Ideal" }
                Digito { anchors.horizontalCenter: parent.horizontalCenter; tam: raiz.h ? 72 : 64; text: raiz.idealCorto(raiz.tiempoIdealMs) }
            }
            Column {
                Sub { anchors.horizontalCenter: parent.horizontalCenter; text: "Distancia" }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    Digito { tam: raiz.h ? 72 : 64; text: Formato.km(raiz.distanciaM) }
                    Digito { tam: raiz.h ? 30 : 26; text: " km"; anchors.baseline: parent.children[0].baseline }
                }
            }
        }
    }

    // Datos editables
    Tarjeta {
        id: tarjetaDatos
        x: raiz.h ? 40 : 24; y: raiz.h ? 110 : 270
        width: raiz.h ? 600 : parent.width - 48
        height: datosCol.height + 10
        Column {
            id: datosCol
            width: parent.width
            TituloTarjeta { texto: "Datos de la carrera" }
            Fila {
                etiqueta: "Nombre"
                Campo { anchoMinimo: 260; texto: raiz.nombre || "–"; editando: raiz.campo === "nombre"
                        onTocado: { raiz.campo = "nombre"; teclado.abrir("texto", "Nombre de la carrera", raiz.nombre, 80); } }
            }
            Fila {
                etiqueta: "Fecha"
                Campo { texto: Qt.formatDate(Formato.desdeIso(raiz.fecha), "dd/MM/yy"); editando: raiz.campo === "fecha"
                        onTocado: { raiz.campo = "fecha"; teclado.abrir("fecha", "Fecha de la carrera", Formato.desdeIso(raiz.fecha)); } }
            }
            Fila {
                etiqueta: "Distancia (m)"
                Campo { texto: Formato.miles(raiz.distanciaM); activo: !raiz.fijos; editando: raiz.campo === "distancia"
                        onTocado: { raiz.campo = "distancia"; teclado.abrir("numero", "Distancia por vuelta, en metros", raiz.distanciaM, 7); } }
            }
            Fila {
                etiqueta: "Tiempo ideal"
                Campo { texto: Formato.tiempo(raiz.tiempoIdealMs); activo: !raiz.fijos; editando: raiz.campo === "tiempo"
                        onTocado: { raiz.campo = "tiempo"; teclado.abrir("tiempo", "Tiempo ideal por vuelta (minutos, segundos y décimas)", Formato.msADigitos(raiz.tiempoIdealMs), 6); } }
            }
            Fila {
                etiqueta: "Vueltas medidas"
                Segmentado {
                    activo: !raiz.fijos
                    opciones: [{ texto: "Impares", valor: "alt1" }, { texto: "Pares", valor: "alt2" }, { texto: "Todas", valor: "todas" }]
                    valor: raiz.patron
                    onElegido: v => raiz.patron = v
                }
            }
            Fila {
                etiqueta: "Velocidad promedio"; colorEtiqueta: Tema.tintaSuave; ultima: true
                Digito { tam: 30; text: Formato.velocidad(raiz.distanciaM, raiz.tiempoIdealMs) }
            }
            Text {
                visible: raiz.fijos && !raiz.h
                x: 22; width: parent.width - 44; wrapMode: Text.WordWrap; bottomPadding: 8
                text: "Esta carrera ya tiene vueltas corridas: el tiempo, la distancia y las vueltas medidas no se pueden cambiar."
                color: Tema.tintaSuave; font { family: Tema.fuente; italic: Tema.cursiva; pixelSize: 15 }
            }
        }
    }

    // Tipo de cada vuelta
    Row {
        x: raiz.h ? 684 : 28; y: raiz.h ? 286 : Math.max(900, tarjetaDatos.y + tarjetaDatos.height + 18)
        spacing: 8
        Sub { text: "Vueltas:"; anchors.verticalCenter: parent.verticalCenter }
        Repeater {
            model: raiz.h ? 4 : 3
            Etiqueta {
                required property int index
                readonly property bool med: raiz.patron === "alt1" ? index % 2 === 0 : raiz.patron === "alt2" ? index % 2 === 1 : true
                medida: med
                texto: "V" + (index + 1) + (index < 3 ? (med ? " MEDIDA" : " LIBRE") : "")
            }
        }
        Sub { text: "…"; anchors.verticalCenter: parent.verticalCenter }
    }

    // LARGAR
    BotonAmarillo {
        id: largar
        x: raiz.h ? 680 : 24; y: raiz.h ? 342 : 950
        width: raiz.h ? 560 : parent.width - 48; height: raiz.h ? 150 : 170
        texto: c.finalizada ? "FINALIZADA" : "LARGAR"
        tamTexto: raiz.h ? 60 : 70
        activo: raiz.nueva || c.puedeLargar
        subtexto: !raiz.nueva && !c.puedeLargar ? c.motivoNoLargar : ""
        onTocado: {
            const id = raiz.cambiada || raiz.nueva ? raiz.guardar() : c.id;
            if (id && Datos.carrera(id).puedeLargar) Navegacion.ir("carrera", { id: id });
        }
    }
    Text {
        visible: raiz.error !== ""
        anchors { horizontalCenter: largar.horizontalCenter; bottom: largar.top; bottomMargin: 6 }
        text: raiz.error; color: Tema.alerta
        font { family: Tema.fuente600; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 17 }
    }

    Row {
        x: largar.x; y: raiz.h ? 528 : 1145
        width: largar.width
        spacing: 16
        BotonCromo {
            width: (parent.width - 16) / 2; height: raiz.h ? 60 : 64; tamTexto: 18
            texto: c.tieneVueltas && !raiz.cambiada ? "Ver informe" : "Guardar"
            onTocado: {
                if (c.tieneVueltas && !raiz.cambiada) { Navegacion.ir("informe", { id: c.id }); return; }
                if (raiz.guardar()) Navegacion.avisar("Carrera guardada");
            }
        }
        DobleToque {
            width: (parent.width - 16) / 2; height: raiz.h ? 60 : 64; tamTexto: 18
            activo: !raiz.nueva
            onConfirmado: { Datos.borrarCarrera(c.id); Navegacion.ir("carreras"); }
        }
    }
    Text {   // en horizontal el aviso va debajo de los botones (en la tarjeta no entra)
        visible: raiz.fijos && raiz.h
        x: largar.x + 4; y: 608; width: largar.width - 8; wrapMode: Text.WordWrap
        text: "Esta carrera ya tiene vueltas corridas: el tiempo, la distancia y las vueltas medidas no se pueden cambiar."
        color: Tema.tintaSuave; font { family: Tema.fuente; italic: Tema.cursiva; pixelSize: 16 }
    }

    Teclado {
        id: teclado
        onAceptado: v => {
            if (raiz.campo === "nombre") raiz.nombre = String(v).trim();
            else if (raiz.campo === "fecha") raiz.fecha = Formato.fechaIso(v);
            else if (raiz.campo === "distancia") raiz.distanciaM = Math.max(0, Math.min(1000000, v));
            else if (raiz.campo === "tiempo") { if (v >= 5000) raiz.tiempoIdealMs = v; else Navegacion.avisar("El tiempo ideal tiene que ser de al menos 5 segundos"); }
        }
        onVisibleChanged: if (!visible) raiz.campo = ""
    }
    Component.onCompleted: {   // solo para capturas
        if (Contexto.prueba === "teclado-texto") { campo = "nombre"; teclado.abrir("texto", "Nombre de la carrera", nombre, 80); }
        else if (Contexto.prueba === "teclado-tiempo") { campo = "tiempo"; teclado.abrir("tiempo", "Tiempo ideal por vuelta (minutos, segundos y décimas)", Formato.msADigitos(tiempoIdealMs), 6); }
        else if (Contexto.prueba === "teclado-fecha") { campo = "fecha"; teclado.abrir("fecha", "Fecha de la carrera", Formato.desdeIso(fecha)); }
    }
}
