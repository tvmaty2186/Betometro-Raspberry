import QtQuick
import Betometro

// Ficha de la carrera en Modo Tablet: los datos en filas grandes (− / + o tocando para escribir) y LARGAR enorme.
// Misma lógica que Ficha.qml (guardar, largar, borrar con doble toque, informe si ya tiene vueltas).
TPantalla {
    id: raiz
    property var c: Datos.carrera(Navegacion.parametros.id || "")
    readonly property bool nueva: !c.id
    readonly property bool fijos: c.tieneVueltas
    titulo: nueva ? "Nueva carrera" : (nombre || "Carrera")
    volverA: "carreras"
    property string nombre: c.nombre
    property string fecha: c.fecha
    property int distanciaM: c.distanciaM
    property int tiempoIdealMs: c.tiempoIdealMs
    property string patron: c.patron
    property string lugar: c.lugar || ""
    property var lugarLat: c.lugarLat
    property var lugarLon: c.lugarLon
    property string campo: ""
    property string error: ""
    property bool borrarArmado: false
    readonly property bool cambiada: nombre !== c.nombre || fecha !== c.fecha || distanciaM !== c.distanciaM
                                     || tiempoIdealMs !== c.tiempoIdealMs || patron !== c.patron || lugar !== (c.lugar || "")
    function datos() {
        return { id: c.id || "", nombre: nombre, fecha: fecha, distanciaM: distanciaM, tiempoIdealMs: tiempoIdealMs, patron: patron,
                 lugar: lugar, lugarLat: lugarLat === undefined ? null : lugarLat, lugarLon: lugarLon === undefined ? null : lugarLon };
    }
    function guardar() {
        const r = Datos.guardarCarrera(datos());
        if (!r.ok) { error = r.error; Navegacion.avisar(r.error); return ""; }
        error = ""; c = Datos.carrera(r.id); Navegacion.parametros = { id: r.id };
        return r.id;
    }
    function moverDia(n) { const f = Formato.desdeIso(fecha); f.setDate(f.getDate() + n); fecha = Formato.fechaIso(f); }
    Timer { id: desarmar; interval: 3000; onTriggered: raiz.borrarArmado = false }

    Flickable {
        id: izq
        width: raiz.h ? raiz.area.width - 420 - 18 : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - 330
        contentHeight: col.height + 10
        clip: true; boundsBehavior: Flickable.StopAtBounds
        Column {
            id: col; width: parent.width; spacing: 10
            TFila { height: raiz.h ? 90 : 96; etiqueta: "Nombre"; detalle: raiz.nombre || "Tocá para escribir"; tocable: true
                    onTocada: { raiz.campo = "nombre"; teclado.abrir("texto", "Nombre de la carrera", raiz.nombre, 80); }
                TIcono { width: 40; height: 40; nombre: "bandera" } }
            TFila { height: raiz.h ? 90 : 96; etiqueta: "Localidad"; detalle: raiz.lugar || "Tocá para elegir"; tocable: true; onTocada: buscador.abrir("")
                TIcono { width: 40; height: 40; nombre: "lugar" } }
            TFila { height: raiz.h ? 90 : 96; etiqueta: "Fecha"
                TMasMenos { texto: Qt.formatDate(Formato.desdeIso(raiz.fecha), "dd/MM/yy"); anchoTexto: 190
                            onMenos: raiz.moverDia(-1); onMas: raiz.moverDia(1)
                            onTocado: { raiz.campo = "fecha"; teclado.abrir("fecha", "Fecha de la carrera", Formato.desdeIso(raiz.fecha)); } } }
            TFila { height: raiz.h ? 90 : 96; etiqueta: "Tiempo ideal"; opacity: raiz.fijos ? 0.5 : 1; enabled: !raiz.fijos
                TMasMenos { texto: Formato.tiempo(raiz.tiempoIdealMs); anchoTexto: 190
                            onMenos: raiz.tiempoIdealMs = Math.max(5000, raiz.tiempoIdealMs - 1000); onMas: raiz.tiempoIdealMs = raiz.tiempoIdealMs + 1000
                            onTocado: { raiz.campo = "tiempo"; teclado.abrir("tiempo", "Tiempo ideal por vuelta", Formato.msADigitos(raiz.tiempoIdealMs), 6); } } }
            TFila { height: raiz.h ? 90 : 96; etiqueta: "Distancia"; opacity: raiz.fijos ? 0.5 : 1; enabled: !raiz.fijos
                TMasMenos { texto: Formato.miles(raiz.distanciaM) + " m"; anchoTexto: 190
                            onMenos: raiz.distanciaM = Math.max(0, raiz.distanciaM - 50); onMas: raiz.distanciaM = Math.min(1000000, raiz.distanciaM + 50)
                            onTocado: { raiz.campo = "distancia"; teclado.abrir("numero", "Distancia por vuelta, en metros", raiz.distanciaM, 7); } } }
            TFila { height: raiz.h ? 90 : 96; etiqueta: "Medidas"; opacity: raiz.fijos ? 0.5 : 1; enabled: !raiz.fijos
                TOpciones { opciones: [{ texto: "Impares", valor: "alt1" }, { texto: "Pares", valor: "alt2" }, { texto: "Todas", valor: "todas" }]
                            valor: raiz.patron; onElegido: v => raiz.patron = v } }
        }
    }

    // Derecha (acostada) o abajo (parada): LARGAR, guardar / informe y borrar
    Item {
        x: raiz.h ? izq.width + 18 : 0
        y: raiz.h ? 0 : izq.height + 16
        width: raiz.h ? 420 : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - y
        TBoton {
            id: largar
            width: parent.width; height: parent.height - 136
            tipo: "acento"; icono: raiz.h ? "bandera" : ""; tamIcono: 90
            nombre: c.finalizada ? "FINALIZADA" : "LARGAR"; tamNombre: 70
            dato: !raiz.nueva && !c.puedeLargar ? c.motivoNoLargar : "Promedio " + Formato.velocidad(raiz.distanciaM, raiz.tiempoIdealMs)
            activo: raiz.nueva || c.puedeLargar
            onTocado: { const id = raiz.cambiada || raiz.nueva ? raiz.guardar() : c.id; if (id && Datos.carrera(id).puedeLargar) Navegacion.ir("carrera", { id: id }); }
        }
        Row {
            anchors.bottom: parent.bottom; spacing: 16
            TBoton { width: (largar.width - 16) / 2; height: 120; tamNombre: 27; icono: ""
                     nombre: c.tieneVueltas && !raiz.cambiada ? "Ver informe" : "Guardar"
                     onTocado: { if (c.tieneVueltas && !raiz.cambiada) { Navegacion.ir("informe", { id: c.id }); return; }
                                 if (raiz.guardar()) Navegacion.avisar("Carrera guardada"); } }
            TBoton { width: (largar.width - 16) / 2; height: 120; tamNombre: 27; icono: ""; tipo: raiz.borrarArmado ? "peligro" : ""
                     nombre: raiz.borrarArmado ? "Tocá otra vez" : "Borrar"; activo: !raiz.nueva
                     onTocado: { if (raiz.borrarArmado) { Datos.borrarCarrera(c.id); Navegacion.ir("carreras"); } else { raiz.borrarArmado = true; desarmar.restart(); } } }
        }
    }

    BuscadorLugar { id: buscador; parent: raiz
        onElegido: l => { raiz.lugar = l.nombre + (l.detalle ? ", " + l.detalle.split(",")[0] : ""); raiz.lugarLat = l.lat; raiz.lugarLon = l.lon; } }
    Teclado {
        id: teclado
        parent: raiz
        onAceptado: v => {
            if (raiz.campo === "nombre") raiz.nombre = String(v).trim();
            else if (raiz.campo === "fecha") raiz.fecha = Formato.fechaIso(v);
            else if (raiz.campo === "distancia") raiz.distanciaM = Math.max(0, Math.min(1000000, v));
            else if (raiz.campo === "tiempo") { if (v >= 5000) raiz.tiempoIdealMs = v; else Navegacion.avisar("El tiempo ideal tiene que ser de al menos 5 segundos"); }
        }
        onVisibleChanged: if (!visible) raiz.campo = ""
    }
}
