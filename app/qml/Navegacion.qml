pragma Singleton
import QtQuick

// Navegación entre pantallas con «Volver» (reglas de la app, docs/app/lo-aprendido.md 5.0).
QtObject {
    property string actual: Contexto.pantallaInicial
    property var pila: []
    function ir(nombre) {
        pila = pila.concat([actual]);
        actual = nombre;
    }
    function volver() {
        if (pila.length === 0) { actual = "inicio"; return; }
        actual = pila[pila.length - 1];
        pila = pila.slice(0, -1);
    }
}
