import QtQuick
import Betometro

// Pantalla 5: Práctica libre. Muestras #s5 y #v5.
// Los valores quedan guardados en la configuración para la próxima práctica (como la app).
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    property int tiempoIdealMs: Datos.cfg("tiempoIdealMs")
    property int distanciaM: Datos.cfg("distanciaM")
    property string patron: Datos.cfg("patron")
    property string campo: ""

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "PRÁCTICA LIBRE"; derechaRayas: raiz.h ? 90 : 20; onVolver: Navegacion.ir("inicio") }

    Tarjeta {
        x: raiz.h ? 40 : 24; y: 110
        width: raiz.h ? 600 : parent.width - 48
        height: col.height + 10
        Column {
            id: col
            width: parent.width
            TituloTarjeta { texto: "Vuelta de práctica" }
            Fila {
                etiqueta: "Tiempo ideal"
                Campo { texto: Formato.tiempo(raiz.tiempoIdealMs); editando: raiz.campo === "tiempo"
                        onTocado: { raiz.campo = "tiempo"; teclado.abrir("tiempo", "Tiempo ideal por vuelta (minutos, segundos y décimas)", Formato.msADigitos(raiz.tiempoIdealMs), 6); } }
            }
            Fila {
                etiqueta: "Distancia (m)"
                Campo { texto: Formato.miles(raiz.distanciaM); editando: raiz.campo === "distancia"
                        onTocado: { raiz.campo = "distancia"; teclado.abrir("numero", "Distancia por vuelta, en metros", raiz.distanciaM, 7); } }
            }
            Fila {
                etiqueta: "Vueltas medidas"
                Segmentado {
                    opciones: [{ texto: "Impares", valor: "alt1" }, { texto: "Pares", valor: "alt2" }, { texto: "Todas", valor: "todas" }]
                    valor: raiz.patron
                    onElegido: v => { raiz.patron = v; Datos.ponerCfg("patron", v); }
                }
            }
            Fila {
                etiqueta: "Velocidad promedio"; colorEtiqueta: Tema.tintaSuave; ultima: true
                Digito { tam: 30; text: Formato.velocidad(raiz.distanciaM, raiz.tiempoIdealMs) }
            }
        }
    }
    Sub {
        x: raiz.h ? 44 : 28; y: raiz.h ? 540 : 480
        width: raiz.h ? 590 : parent.width - 56
        wrapMode: Text.WordWrap; lineHeight: 1.6; tam: 14
        text: "Se guarda en Corridas como práctica cuando largás. Estos valores quedan para la próxima práctica."
    }
    BotonAmarillo {
        x: raiz.h ? 680 : 24; y: raiz.h ? 110 : 600
        width: raiz.h ? 560 : parent.width - 48; height: raiz.h ? 400 : 640
        texto: "LARGAR"; tamTexto: raiz.h ? 90 : 110
        onTocado: Navegacion.ir("carrera", { practica: true })
    }

    Teclado {
        id: teclado
        onAceptado: v => {
            if (raiz.campo === "tiempo") {
                if (v >= 5000) { raiz.tiempoIdealMs = v; Datos.ponerCfg("tiempoIdealMs", v); }
                else Navegacion.avisar("El tiempo ideal tiene que ser de al menos 5 segundos");
            } else if (raiz.campo === "distancia") {
                raiz.distanciaM = Math.max(0, Math.min(1000000, v)); Datos.ponerCfg("distanciaM", raiz.distanciaM);
            }
        }
        onVisibleChanged: if (!visible) raiz.campo = ""
    }
}
