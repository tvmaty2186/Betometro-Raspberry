pragma Singleton
import QtQuick

// Armados de la pantalla de carrera. Cada widget: { t, x, y, w, h, forma?, extra?, barra?, modo?, ancho? }
// ancho: «Números − / +» (0,7 a 1,8; estira los números a lo ancho). La cuenta de fábrica, 1,2 como la app.
// modo (solo «cuenta»): "progresiva" (de fábrica, también si falta) o "regresiva".
// en fracciones de la pantalla (6.6 de la app). Redondos: w = diámetro como fracción del ancho.
// Se guardan en la configuración: «widgetsH» (horizontal) y «widgets» (vertical).
QtObject {
    // De fábrica, medidos en las muestras aprobadas (px de 1280×720 y 720×1280)
    readonly property var pxH: [
        { t: "cuenta", x: 24, y: 56, w: 1232, h: 320, extra: true, barra: true, ancho: 1.2 },
        { t: "tipo", x: 50, y: 90, w: 210, h: 210, forma: "redondo" },
        { t: "necesaria", x: 1010, y: 96, w: 220, h: 200 },
        { t: "velocimetro", x: 24, y: 400, w: 330, h: 300, barra: true },
        { t: "ultima", x: 378, y: 400, w: 270, h: 300 },
        { t: "marcar", x: 672, y: 400, w: 584, h: 300 }
    ]
    readonly property var pxV: [
        { t: "velocimetro", forma: "redondo", x: 150, y: 72, w: 420, h: 420 },
        { t: "tipo", x: 20, y: 410, w: 190, h: 190, forma: "redondo" },
        { t: "necesaria", x: 500, y: 430, w: 200, h: 160 },
        { t: "cuenta", x: 20, y: 620, w: 680, h: 270, extra: true, barra: true, ancho: 1.2 },
        { t: "ultima", x: 20, y: 910, w: 680, h: 96 },
        { t: "marcar", x: 20, y: 1026, w: 680, h: 234 }
    ]
    readonly property var tipos: [
        { t: "cuenta", nombre: "Cronómetro de la vuelta", vel: false },
        { t: "tipo", nombre: "Vuelta y tipo", vel: false },
        { t: "delta", nombre: "Adelantado / atrasado", vel: true },
        { t: "velocimetro", nombre: "Velocímetro", vel: true },
        { t: "necesaria", nombre: "Velocidad sugerida", vel: true },
        { t: "faltan", nombre: "Metros que faltan", vel: true },
        { t: "transcurrido", nombre: "Tiempo transcurrido", vel: false },
        { t: "ideal", nombre: "Tiempo ideal", vel: false },
        { t: "proxima", nombre: "Próxima vuelta", vel: false },
        { t: "ultima", nombre: "Última vuelta", vel: false },
        { t: "marcar", nombre: "Botón MARCAR", vel: false }
    ]
    function nombre(t) { for (const x of tipos) if (x.t === t) return x.nombre; return t; }
    function necesitaGps(t) { for (const x of tipos) if (x.t === t) return x.vel; return false; }
    function aFracciones(lista, W, H) {
        return lista.map(w => Object.assign({}, w, { x: w.x / W, y: w.y / H, w: w.w / W, h: w.h / H }));
    }
    function fabrica(horizontal) { return horizontal ? aFracciones(pxH, 1280, 720) : aFracciones(pxV, 720, 1280); }
    function cargar(horizontal, ajustes) {
        const g = ajustes[horizontal ? "widgetsH" : "widgets"];
        return Array.isArray(g) && g.length ? g.map(w => Object.assign({}, w)) : fabrica(horizontal);
    }
}
