import QtQuick
import Betometro

// Práctica libre en Modo Tablet. Muestra: docs/muestras/tablet/practica-h.png
// − / + grandes (dejándolos apretados siguen); tocando el número se escribe con el teclado.
TPantalla {
    id: raiz
    titulo: "Práctica"
    property int tiempoIdealMs: Datos.cfg("tiempoIdealMs")
    property int distanciaM: Datos.cfg("distanciaM")
    property string patron: Datos.cfg("patron")
    property string campo: ""
    function ponerTiempo(v) { tiempoIdealMs = Math.max(5000, Math.min(5999000, v)); Datos.ponerCfg("tiempoIdealMs", tiempoIdealMs); }
    // Casilleros del tiempo: qué se edita («m»/«s») y lo escrito, de izquierda a derecha
    property string casillero: ""
    Component.onCompleted: if (Contexto.prueba === "casillero") { tocarCasillero("s"); escribir("3"); }   // solo para capturas
    property string escrito: ""
    function tocarCasillero(que) { casillero = que; escrito = ""; }
    function escribir(d) {
        if (escrito.length >= 2) return;
        const n = escrito + d;
        if (casillero === "s" && parseInt(n) > 59) { Navegacion.avisar("Los segundos van de 0 a 59"); return; }
        escrito = n;
    }
    function aceptarCasillero() {
        const m = Math.floor(tiempoIdealMs / 60000), s = Math.floor(tiempoIdealMs / 1000) % 60;
        const v = escrito === "" ? -1 : parseInt(escrito);
        const nuevo = ((casillero === "m" && v >= 0 ? v : m) * 60 + (casillero === "s" && v >= 0 ? v : s)) * 1000;   // décimas en 0
        casillero = ""; escrito = "";
        if (nuevo >= 5000) ponerTiempo(nuevo); else Navegacion.avisar("El tiempo ideal tiene que ser de al menos 5 segundos");
    }
    function ponerDist(v) { distanciaM = Math.max(0, Math.min(1000000, v)); Datos.ponerCfg("distanciaM", distanciaM); }

    Column {
        id: col
        width: raiz.h ? 560 : raiz.area.width
        spacing: 16
        // Tiempo ideal por casilleros (muestra aprobada 1.0.29: docs/muestras/1.0.29/1-tiempo-casilleros.png)
        Rectangle {
            width: parent.width; height: 150
            radius: Math.max(12, Tema.radio * 1.3)
            border { width: 3; color: Tema.bordeCromo }
            gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
            Text { x: 26; y: 12; text: "TIEMPO IDEAL"; color: Tema.tintaSuave
                   font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 21; letterSpacing: 2 } }
            Text { anchors.right: parent.right; anchors.rightMargin: 24; y: 6
                   text: raiz.casillero === "" ? Formato.tiempo(raiz.tiempoIdealMs)
                         : raiz.casillero === "m" ? (raiz.escrito || "_") + ":" + String(Math.floor(raiz.tiempoIdealMs / 1000) % 60).padStart(2, "0") + ":0"
                         : Math.floor(raiz.tiempoIdealMs / 60000) + ":" + (raiz.escrito + "__").slice(0, 2) + ":0"
                   color: Tema.numero; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 36 } }
            TCasilleros {
                x: 24; anchors.bottom: parent.bottom; anchors.bottomMargin: 14
                ms: raiz.tiempoIdealMs; editando: raiz.casillero; escrito: raiz.escrito
                onCambiar: v => raiz.ponerTiempo(v)
                onEditar: que => raiz.tocarCasillero(que)
            }
        }
        TDato { width: parent.width; etiqueta: "Distancia"; texto: Formato.miles(raiz.distanciaM); unidad: "m"
                onMenos: raiz.ponerDist(raiz.distanciaM - 50); onMas: raiz.ponerDist(raiz.distanciaM + 50)
                onTocado: { raiz.campo = "distancia"; teclado.abrir("numero", "Distancia por vuelta, en metros", raiz.distanciaM, 7); } }
        TFila { etiqueta: "Medidas"; height: 110
            TOpciones { opciones: [{ texto: "Impares", valor: "alt1" }, { texto: "Pares", valor: "alt2" }, { texto: "Todas", valor: "todas" }]
                        valor: raiz.patron; onElegido: v => { raiz.patron = v; Datos.ponerCfg("patron", v); } } }
        Text { leftPadding: 8; text: "Promedio " + Formato.velocidad(raiz.distanciaM, raiz.tiempoIdealMs); color: Tema.acento
               font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 27 } }
        // Recorrido libre (Matías, 07/11/2026): sin vueltas, sin tiempo, sin distancia; graba todo
        TFila { etiqueta: "Recorrido libre"; detalle: "Sin vueltas, sin tiempo, sin distancia"; height: 110
                tocable: true; onTocada: Navegacion.ir("carrera", { recorrido: true })
            TIcono { width: 64; height: 64; nombre: "ruta"; color: Tema.acento } }
    }
    TBoton {
        x: raiz.h ? col.width + 22 : 0
        y: raiz.h ? 0 : col.height + 16
        width: raiz.h ? raiz.area.width - x : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - y
        tipo: "acento"; icono: "bandera"; tamIcono: 120; nombre: "IR A LARGAR"; tamNombre: raiz.h ? 84 : 96   // el LARGAR de verdad está en el crono (07-1.2)
        onTocado: Navegacion.ir("carrera", { practica: true })
    }
    TTecladoNum {   // en el lugar de IR A LARGAR mientras se escribe un casillero
        visible: raiz.casillero !== ""
        x: raiz.h ? col.width + 22 : 0
        y: raiz.h ? 0 : col.height + 16
        width: raiz.h ? raiz.area.width - x : raiz.area.width
        height: raiz.h ? raiz.area.height : raiz.area.height - y
        titulo: raiz.casillero === "m" ? "Escribí los minutos" : "Escribí los segundos"
        ayuda: raiz.casillero === "m" ? "de izquierda a derecha" : "de 0 a 59 · de izquierda a derecha"
        onDigito: d => raiz.escribir(d)
        onBorrar: raiz.escrito = raiz.escrito.slice(0, -1)
        onListo: raiz.aceptarCasillero()
    }
    Teclado {
        id: teclado
        parent: raiz   // encima de toda la pantalla, no solo del área
        onAceptado: v => {
            if (raiz.campo === "tiempo") { if (v >= 5000) raiz.ponerTiempo(v); else Navegacion.avisar("El tiempo ideal tiene que ser de al menos 5 segundos"); }
            else if (raiz.campo === "distancia") raiz.ponerDist(v);
        }
        onVisibleChanged: if (!visible) raiz.campo = ""
    }
}
