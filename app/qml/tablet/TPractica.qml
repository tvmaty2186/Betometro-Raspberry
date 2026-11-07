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
    function ponerDist(v) { distanciaM = Math.max(0, Math.min(1000000, v)); Datos.ponerCfg("distanciaM", distanciaM); }

    Column {
        id: col
        width: raiz.h ? 560 : raiz.area.width
        spacing: 16
        TDato { width: parent.width; etiqueta: "Tiempo ideal"; texto: Formato.tiempo(raiz.tiempoIdealMs)
                onMenos: raiz.ponerTiempo(raiz.tiempoIdealMs - 1000); onMas: raiz.ponerTiempo(raiz.tiempoIdealMs + 1000)
                onTocado: { raiz.campo = "tiempo"; teclado.abrir("tiempo", "Tiempo ideal por vuelta", Formato.msADigitos(raiz.tiempoIdealMs), 6); } }
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
