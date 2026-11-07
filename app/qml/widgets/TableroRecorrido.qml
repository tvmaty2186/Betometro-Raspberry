import QtQuick
import Betometro

// Recorrido libre en marcha (muestra docs/muestras/recorrido/2-en-marcha.png): tiempo total, distancia,
// velocidad, máxima, promedio y MARCAR (deja una marca, no corta nada). Sin vueltas, sin sugerida, sin voz.
Item {
    id: raiz
    property var est: ({})
    readonly property bool h: width > height
    signal marcar()
    function efectoMarcar() { boton.efecto(); }
    readonly property bool corre: !!est.enCurso
    readonly property real km: (est.distM || 0) / 1000
    // [x, y, ancho, alto] en horizontal (1280×720) y de pie (720×1280)
    function lugar(hz, v) { const r = raiz.h ? hz : v; return { x: r[0], y: r[1], w: r[2], hh: r[3] }; }
    component Dato: WNumero {
        property var l
        x: l.x; y: l.y; width: l.w; height: l.hh
    }
    Dato { l: raiz.lugar([22, 72, 760, 300], [20, 72, 680, 290]); etiqueta: "Tiempo"; numero: Formato.hms(raiz.est.transMs || 0)
           extra: raiz.corre && raiz.est.paradoMs >= 1000 ? "parado " + Formato.hms(raiz.est.paradoMs).replace(/^0:/, "") : "" }
    Dato { l: raiz.lugar([800, 72, 458, 300], [20, 380, 680, 220]); etiqueta: "Distancia km"
           numero: raiz.km < 100 ? Formato.coma(raiz.km, 2) : Formato.coma(raiz.km, 1) }
    Dato { l: raiz.lugar([22, 392, 360, 306], [20, 618, 330, 250]); etiqueta: "Velocidad km/h"
           numero: raiz.est.vActKmh !== undefined && raiz.est.vActKmh !== null ? String(Math.round(raiz.est.vActKmh)) : "–" }
    Dato { l: raiz.lugar([400, 392, 190, 145], [370, 618, 330, 117]); etiqueta: "Máxima"
           numero: raiz.corre ? String(Math.round(raiz.est.velMaxVuelta || 0)) : "–" }
    Dato { l: raiz.lugar([400, 553, 190, 145], [370, 751, 330, 117]); etiqueta: "Promedio"
           numero: raiz.corre && raiz.est.promKmh > 0 ? Formato.coma(raiz.est.promKmh, 1) : "–" }
    WMarcar {
        id: boton
        readonly property var l: raiz.lugar([608, 392, 650, 306], [20, 888, 680, 372])
        x: l.x; y: l.y; width: l.w; height: l.hh
        est: raiz.est
        subtitulo: "DEJA LA MARCA " + ((raiz.est.marcas || 0) + 1) + " · NO CORTA NADA"
        onMarcar: raiz.marcar()
    }
}
