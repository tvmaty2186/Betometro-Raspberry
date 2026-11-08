.pragma library
// Comparar una vuelta contra la vuelta modelo (modo «Comp.» del informe, 08/11/2026). Lo usan el gráfico, el mapa y
// la pantalla completa. Cada vuelta se lleva a su largo (L): la posición es la fracción de la vuelta (0..1).

// Serie de una vuelta (inf.vueltas[i]: m = [t, d, kmh, acel, adel, x, y, ok], L, realMs): fracción → segundos,
// velocidad y punto del mapa. Empieza en 0 s y termina en el tiempo del pulsador: la diferencia total es la de la vuelta.
function serie(v) {
    if (!v || !v.m || v.m.length < 5) return null;
    const L = v.L || 1, f = [0], t = [0], k = [], xy = [];
    for (const p of v.m) {
        if (!p[7]) continue;
        const fr = Math.min(1, Math.max(0, p[1] / L));
        if (fr <= f[f.length - 1] || p[0] <= t[t.length - 1]) continue;
        f.push(fr); t.push(p[0]); k.push([fr, p[2]]); xy.push([fr, p[5], p[6]]);
    }
    f.push(1); t.push(Math.max(t[t.length - 1], v.realMs / 1000));
    return { f: f, t: t, k: k, xy: xy, real: v.realMs };
}

// Segundos de vuelta en la fracción fr
function tEn(s, fr) {
    let a = 0, b = s.f.length - 1;
    while (b - a > 1) { const m = (a + b) >> 1; if (s.f[m] <= fr) a = m; else b = m; }
    const df = s.f[b] - s.f[a];
    return s.t[a] + (df > 0 ? (fr - s.f[a]) / df * (s.t[b] - s.t[a]) : 0);
}

// Positivo = la vuelta va atrasada contra la modelo en ese punto
function delta(sA, sM, fr) { return tEn(sA, fr) - tEn(sM, fr); }

// Color de un tramo del mapa: rojo si ahí perdió tiempo, verde si ganó, blanco si fue parejo
function colorTramo(sA, sM, f1, f2, rojo, verde) {
    if (f2 <= f1) return "rgba(255,255,255,0.55)";
    const v = (delta(sA, sM, f2) - delta(sA, sM, f1)) / (f2 - f1) / 100;   // segundos cada 1 % de vuelta
    return v > 0.01 ? rojo : v < -0.01 ? verde : "rgba(255,255,255,0.55)";
}
