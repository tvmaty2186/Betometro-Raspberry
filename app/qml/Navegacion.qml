pragma Singleton
import QtQuick

// Navegación entre pantallas. Cada pantalla decide a dónde vuelve
// (reglas de la app, docs/app/lo-aprendido.md 5.0).
QtObject {
    property string actual: Contexto.pantallaInicial
    property var parametros: ({ id: Contexto.idInicial })
    signal aviso(string texto)
    function ir(nombre, params) {
        parametros = params || {};
        actual = nombre;
    }
    function avisar(texto) { aviso(texto); }
}
