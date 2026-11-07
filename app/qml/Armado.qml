pragma Singleton
import QtQuick

// Armados de la pantalla de carrera. Cada widget: { t, x, y, w, h, forma?, extra?, barra?, modo?, ancho? }
// ancho: «Números − / +» (0,7 a 1,8; estira los números a lo ancho). La cuenta de fábrica, 1,2 como la app.
// modo (solo «cuenta»): "progresiva" (de fábrica, también si falta) o "regresiva".
// en fracciones de la pantalla (6.6 de la app). Redondos: w = diámetro como fracción del ancho.
// Se guardan en la configuración: «widgetsH» (horizontal) y «widgets» (vertical).
QtObject {
    // De fábrica, medidos en las muestras aprobadas (px de 1280×720 y 720×1280).
    // 07/11/2026 (Matías): se suma ± segundos (delta). Acostada va arriba a la derecha y la Sugerida baja al lado
    // de la velocidad (que la sugerida quede cerca de la medida). De pie, la Sugerida queda al lado del velocímetro
    // y ± segundos va abajo, al lado de la última vuelta.
    readonly property var pxH: [
        { t: "cuenta", x: 24, y: 56, w: 1232, h: 320, extra: true, barra: true, ancho: 1.2 },
        { t: "tipo", x: 50, y: 90, w: 210, h: 210, forma: "redondo" },
        { t: "delta", x: 1010, y: 96, w: 220, h: 200 },
        { t: "velocimetro", x: 24, y: 400, w: 290, h: 300, barra: true },
        { t: "necesaria", x: 334, y: 400, w: 250, h: 300 },
        { t: "ultima", x: 604, y: 400, w: 220, h: 300 },
        { t: "marcar", x: 844, y: 400, w: 412, h: 300 }
    ]
    readonly property var pxV: [
        { t: "velocimetro", forma: "redondo", x: 150, y: 72, w: 420, h: 420 },
        { t: "tipo", x: 20, y: 410, w: 190, h: 190, forma: "redondo" },
        { t: "necesaria", x: 500, y: 430, w: 200, h: 160 },
        { t: "cuenta", x: 20, y: 620, w: 680, h: 250, extra: true, barra: true, ancho: 1.2 },
        { t: "delta", x: 20, y: 890, w: 330, h: 120 },
        { t: "ultima", x: 370, y: 890, w: 330, h: 120 },
        { t: "marcar", x: 20, y: 1030, w: 680, h: 230 }
    ]
    // Sin MARCAR en pantalla (llave apagada, Matías 07/11/2026): el lugar libre muestra más datos (Faltan y Tiempo
    // ideal). Aprobados: docs/muestras/sin-marcar/ (acostada horizontal.png; de pie vertical.png, la propuesta 2).
    readonly property var pxHSinMarcar: [
        { t: "cuenta", x: 24, y: 56, w: 1232, h: 328, extra: true, barra: true, ancho: 1.2 },
        { t: "tipo", x: 48, y: 74, w: 186, h: 186, forma: "redondo" },
        { t: "ideal", x: 40, y: 274, w: 204, h: 92 },
        { t: "delta", x: 992, y: 68, w: 250, h: 300 },
        { t: "velocimetro", x: 24, y: 400, w: 330, h: 296, barra: true },
        { t: "necesaria", x: 370, y: 400, w: 330, h: 296 },
        { t: "faltan", x: 716, y: 400, w: 540, h: 140 },
        { t: "ultima", x: 716, y: 556, w: 540, h: 140 }
    ]
    readonly property var pxVSinMarcar: [
        { t: "velocimetro", x: 20, y: 66, w: 380, h: 380, forma: "redondo" },
        { t: "tipo", x: 463, y: 66, w: 190, h: 190, forma: "redondo" },
        { t: "necesaria", x: 416, y: 262, w: 284, h: 184 },
        { t: "cuenta", x: 20, y: 468, w: 680, h: 290, extra: false, barra: false, ancho: 1.1 },
        { t: "delta", x: 20, y: 774, w: 330, h: 270 },
        { t: "faltan", x: 366, y: 774, w: 334, h: 270 },
        { t: "ideal", x: 20, y: 1060, w: 330, h: 200 },
        { t: "ultima", x: 366, y: 1060, w: 334, h: 200 }
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
    function fabrica(horizontal, sinMarcar) {
        if (sinMarcar) return horizontal ? aFracciones(pxHSinMarcar, 1280, 720) : aFracciones(pxVSinMarcar, 720, 1280);
        return horizontal ? aFracciones(pxH, 1280, 720) : aFracciones(pxV, 720, 1280);
    }
    // Con «MARCAR en pantalla» apagado se usa la pantalla sin MARCAR (la aprobada, o la que se acomodó con el
    // editor estando apagada), guardada aparte: la pantalla con MARCAR del usuario queda intacta (Matías, 07/11/2026)
    function sinMarcar(ajustes) { return ajustes.marcarPantalla === false || Contexto.prueba.indexOf("sin-marcar") >= 0; }
    function clave(horizontal, ajustes) {
        return (horizontal ? "widgetsH" : "widgets") + (sinMarcar(ajustes) ? "SinMarcar" : "");
    }
    function cargar(horizontal, ajustes) {
        const sm = sinMarcar(ajustes);
        // Ojo: la lista guardada llega de C++ como lista de Qt, no como Array de JavaScript
        // (Array.isArray da false): se recorre por índice. Antes se ignoraba y siempre quedaba la de fábrica.
        const g = ajustes[clave(horizontal, ajustes)];
        if (!g || typeof g !== "object" || !(g.length > 0)) return fabrica(horizontal, sm);
        const l = [];
        for (let i = 0; i < g.length; i++) if (g[i] && g[i].t && !(sm && g[i].t === "marcar")) l.push(Object.assign({}, g[i]));
        return l.length ? l : fabrica(horizontal, sm);
    }

}
