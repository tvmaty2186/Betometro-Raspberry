import QtQuick
import Betometro

// Pantalla 9: clave de una red. Muestras #s9 y #v9 (lista, clave y teclado siempre a la vista).
Item {
    id: raiz
    readonly property bool h: Contexto.horizontal
    property string red: Navegacion.parametros.red || (Contexto.prueba === "red-vecino" ? "Vecino_2.4G" : "")
    property string clave: Contexto.prueba === "red-vecino" ? "avispon101" : ""
    property bool ver: false
    property bool mayuscula: false
    property bool numeros: false

    function tecla(k) {
        if (k === "⌫") { clave = clave.slice(0, -1); return; }
        if (k === "✓") { conectar(); return; }
        if (k === "⇧") { mayuscula = !mayuscula; return; }
        if (k === "123" || k === "abc") { numeros = k === "123"; return; }
        if (k === "espacio") k = " ";
        if (clave.length < 63) clave += mayuscula ? k.toUpperCase() : k;
        mayuscula = false;
    }
    function conectar() { if (red) Wifi.conectar(red, clave); }
    Connections {
        target: Wifi
        function onConectado(r) { Navegacion.avisar("Conectado a «" + r + "»"); Navegacion.ir("conectividad"); }
    }

    Fondo { anchors.fill: parent }
    CabeceraPantalla { titulo: "WIFI"; derechaRayas: raiz.h ? 90 : 30; onVolver: Navegacion.ir("conectividad") }

    Tarjeta {
        id: lista
        x: raiz.h ? 40 : 24; y: raiz.h ? 96 : 100
        width: raiz.h ? 520 : parent.width - 48; height: colL.height + 4
        Column {
            id: colL; width: parent.width
            Repeater {
                model: Wifi.cerca.concat(Wifi.guardadas.filter(g => g.cerca && !g.conectada).map(g => ({ nombre: g.nombre, senal: g.senal, clave: true })))
                Fila {
                    required property var modelData
                    implicitHeight: 56
                    resaltada: modelData.nombre === raiz.red
                    tocable: true
                    onTocada: { raiz.red = modelData.nombre; raiz.clave = ""; }
                    izquierda: Row { spacing: 14
                        Senal { nivel: modelData.senal; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: modelData.nombre; color: Tema.blanco; font { family: Tema.fuente600; pixelSize: 20 } } }
                    Text { visible: modelData.nombre === raiz.red || !modelData.clave; text: modelData.nombre === raiz.red ? "elegida" : "abierta"
                           color: Tema.amarillo; font { family: Tema.fuente800; pixelSize: 18 } }
                    Candado { visible: modelData.nombre !== raiz.red && modelData.clave }
                }
            }
        }
    }

    Tarjeta {
        id: claveT
        x: raiz.h ? 590 : 24; y: raiz.h ? 96 : lista.y + lista.height + 24
        width: raiz.h ? 650 : parent.width - 48; height: 250
        Sub { x: 24; y: 22; text: Wifi.tieneClave(raiz.red) ? "Clave de «" + raiz.red + "»" : "«" + raiz.red + "» es abierta: no lleva clave"; tam: 14 }
        Rectangle {
            x: 24; y: 50; width: parent.width - 48; height: 64; radius: 14
            color: Qt.rgba(0, 0, 0, 0.35); border { width: 2; color: Tema.bordeCromo }
            Text {
                anchors { left: parent.left; leftMargin: 18; verticalCenter: parent.verticalCenter }
                text: raiz.ver ? raiz.clave : "•".repeat(raiz.clave.length)
                color: Tema.blanco; font { family: Tema.fuente600; pixelSize: 30 }
                Rectangle { anchors { left: parent.right; leftMargin: 2; verticalCenter: parent.verticalCenter } width: 3; height: 34; color: Tema.amarillo }
            }
        }
        Text { x: 24; y: 122; visible: Wifi.error !== ""; text: Wifi.error; color: "#ff8a7a"; font { family: Tema.fuente600; italic: true; pixelSize: 16 } }
        Row {
            x: 24; y: 154; spacing: 14
            BotonCromo { width: (claveT.width - 48 - 14) / 2; height: 58; tamTexto: 18; texto: raiz.ver ? "Ocultar clave" : "Mostrar clave"; onTocado: raiz.ver = !raiz.ver }
            BotonAmarillo { width: (claveT.width - 48 - 14) / 2; height: 58; texto: Wifi.conectando ? "CONECTANDO…" : "CONECTAR"; activo: !Wifi.conectando && raiz.red !== ""; onTocado: raiz.conectar() }
        }
    }

    Teclas {
        x: raiz.h ? 40 : 14; width: parent.width - 2 * x
        y: raiz.h ? 370 : claveT.y + claveT.height + 30
        height: parent.height - y - (raiz.h ? 24 : 30)
        mayuscula: raiz.mayuscula
        filas: raiz.numeros ? ["1234567890", "-/:;()&@\".", "#,?!'%+=_⌫", "abc espacio ✓"] : ["qwertyuiop", "asdfghjklñ", "⇧zxcvbnm⌫", "123 espacio ✓"]
        onTecla: k => raiz.tecla(k)
    }
}
