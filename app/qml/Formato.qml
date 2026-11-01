pragma Singleton
import QtQuick

// Formatos de números y tiempos como la app.
QtObject {
    // m:ss,d
    function tiempo(ms) {
        const dec = Math.floor(ms / 100), d = dec % 10, s = Math.floor(dec / 10);
        return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0") + "," + d;
    }
    // Dígitos tecleados → ms: el último es la décima, los dos anteriores los segundos, el resto minutos.
    // Los segundos de más se reacomodan (0:90,0 → 1:30,0), como la app.
    function digitosAMs(t) {
        const s = String(t).replace(/\D/g, "");
        if (s === "") return 0;
        const d = parseInt(s.slice(-1)), ss = s.length > 1 ? parseInt(s.slice(-3, -1)) : 0, m = s.length > 3 ? parseInt(s.slice(0, -3)) : 0;
        return (m * 60 + ss) * 1000 + d * 100;
    }
    // ms → dígitos para seguir editando
    function msADigitos(ms) {
        const dec = Math.floor(ms / 100), d = dec % 10, s = Math.floor(dec / 10);
        const m = Math.floor(s / 60);
        return (m > 0 ? String(m) + String(s % 60).padStart(2, "0") : String(s % 60)) + String(d);
    }
    // 3200 → «3.200»
    function miles(n) { return String(Math.round(n)).replace(/\B(?=(\d{3})+(?!\d))/g, "."); }
    function velocidad(distanciaM, tiempoMs) {
        if (!(distanciaM > 0 && tiempoMs > 0)) return "–";
        return (Math.round(distanciaM / (tiempoMs / 1000) * 3.6 * 10) / 10).toFixed(1).replace(".", ",") + " km/h";
    }
    // 3200 → «3,2»
    function km(m) { return (Math.round(m / 100) / 10).toString().replace(".", ","); }
    function fechaIso(f) { return Qt.formatDate(f, "yyyy-MM-dd"); }
    function desdeIso(s) { const p = s.split("-"); return new Date(+p[0], +p[1] - 1, +p[2]); }
    function moverFecha(f, que, n) {
        const d = new Date(f.getFullYear(), f.getMonth(), f.getDate());
        if (que === "d") d.setDate(d.getDate() + n);
        else if (que === "m") d.setMonth(d.getMonth() + n);
        else d.setFullYear(d.getFullYear() + n);
        return d;
    }
}
