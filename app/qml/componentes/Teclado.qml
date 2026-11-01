import QtQuick
import Betometro

// Teclado en pantalla para cambiar un campo. Se abre encima de la pantalla.
//   modo "texto":  letras con ñ (como la muestra del wifi)
//   modo "numero": números enteros
//   modo "tiempo": se escriben los dígitos y queda m:ss,d (90 s pasa a 1:30,0, como la app)
//   modo "fecha":  día, mes y año con − y +
Rectangle {
    id: raiz
    property string modo: "texto"
    property string titulo: ""
    property string valor: ""      // texto, número o dígitos del tiempo
    property date fecha: new Date()
    property int maximo: 80        // largo máximo del texto / dígitos
    property bool mayuscula: true
    property bool numeros: false     // texto: tecla «123» muestra números y signos
    property bool reemplazar: false  // la primera tecla borra lo que había (números y tiempo)
    signal aceptado(var valor)

    function abrir(m, t, v, max) {
        modo = m; titulo = t; maximo = max || 80;
        if (m === "fecha") fecha = v; else valor = String(v);
        mayuscula = m === "texto" && valor.length === 0;
        numeros = false;
        reemplazar = m === "numero" || m === "tiempo";
        visible = true;
    }
    function cerrar() { visible = false; }
    function aceptar() {
        if (modo === "numero") aceptado(valor === "" ? 0 : parseInt(valor));
        else if (modo === "tiempo") aceptado(Formato.digitosAMs(valor));
        else if (modo === "fecha") aceptado(fecha);
        else aceptado(valor);
        cerrar();
    }
    function tecla(k) {
        if (k === "⌫") { reemplazar = false; valor = valor.slice(0, -1); return; }
        if (k === "✓") { aceptar(); return; }
        if (k === "⇧") { mayuscula = !mayuscula; return; }
        if (k === "123" || k === "abc") { numeros = k === "123"; return; }
        if (reemplazar) { valor = ""; reemplazar = false; }
        if (k === "espacio") k = " ";
        if (valor.length >= maximo) return;
        if (modo === "numero" && valor === "0") valor = "";
        valor += mayuscula ? k.toUpperCase() : k;
        if (mayuscula && modo === "texto") mayuscula = false;
    }

    visible: false
    anchors.fill: parent
    z: 90
    color: Qt.rgba(0, 0, 0, 0.6)
    MouseArea { anchors.fill: parent; onClicked: raiz.cerrar() }

    readonly property bool h: Contexto.horizontal
    readonly property var filasTexto: numeros ? ["1234567890", "-/:;()&@\".", "#,?!'%+=_⌫", "abc espacio ✓"]
                                              : ["qwertyuiop", "asdfghjklñ", "⇧zxcvbnm⌫", "123 espacio ✓"]
    readonly property var filasNumero: ["123", "456", "789", "⌫0✓"]

    Tarjeta {
        id: panel
        oscura: true
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: raiz.h ? 30 : 16 }
        height: raiz.modo === "texto" ? (raiz.h ? 440 : 520) : (raiz.modo === "fecha" ? 300 : (raiz.h ? 520 : 600))
        MouseArea { anchors.fill: parent }   // que tocar el panel no lo cierre

        Sub { id: tit; x: 24; y: 18; text: raiz.titulo; tam: 14 }
        Rectangle {
            id: visor
            visible: raiz.modo !== "fecha"
            x: 20; y: 42; width: parent.width - 40 - (raiz.h ? 0 : 0); height: 64; radius: 14
            color: Qt.rgba(0, 0, 0, 0.35); border { width: 2; color: Tema.acento }
            Text {
                anchors { left: parent.left; leftMargin: 18; verticalCenter: parent.verticalCenter }
                text: raiz.modo === "tiempo" ? Formato.tiempo(Formato.digitosAMs(raiz.valor)) : raiz.valor
                color: Tema.tinta
                font { family: raiz.modo === "texto" ? Tema.fuente : Tema.fuenteNum; weight: Font.DemiBold; pixelSize: 32 }
                Rectangle { anchors { left: parent.right; leftMargin: 2; verticalCenter: parent.verticalCenter } width: 3; height: 34; color: Tema.acento }
            }
        }

        // Letras o números
        Teclas {
            visible: raiz.modo !== "fecha"
            anchors { left: parent.left; right: parent.right; top: visor.bottom; bottom: parent.bottom; margins: 20 }
            filas: raiz.modo === "texto" ? raiz.filasTexto : raiz.filasNumero
            mayuscula: raiz.mayuscula
            grande: raiz.modo !== "texto"
            onTecla: k => raiz.tecla(k)
        }

        // Fecha: día, mes, año
        Row {
            visible: raiz.modo === "fecha"
            anchors { horizontalCenter: parent.horizontalCenter; top: tit.bottom; topMargin: 24 }
            spacing: 26
            Repeater {
                model: [{ t: "Día", k: "d" }, { t: "Mes", k: "m" }, { t: "Año", k: "a" }]
                Column {
                    required property var modelData
                    spacing: 8
                    Sub { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.t; tam: 13 }
                    Row {
                        spacing: 10
                        BotonCromo { width: 64; height: 64; texto: "−"; tamTexto: 30; onTocado: raiz.fecha = Formato.moverFecha(raiz.fecha, modelData.k, -1) }
                        Digito {
                            width: modelData.k === "a" ? 110 : 76; horizontalAlignment: Text.AlignHCenter; tam: 52
                            text: modelData.k === "d" ? Qt.formatDate(raiz.fecha, "dd") : modelData.k === "m" ? Qt.formatDate(raiz.fecha, "MM") : Qt.formatDate(raiz.fecha, "yyyy")
                        }
                        BotonCromo { width: 64; height: 64; texto: "+"; tamTexto: 30; onTocado: raiz.fecha = Formato.moverFecha(raiz.fecha, modelData.k, 1) }
                    }
                }
            }
        }
        BotonAmarillo {
            visible: raiz.modo === "fecha"
            anchors { right: parent.right; bottom: parent.bottom; margins: 22 }
            width: 200; height: 60; texto: "LISTO"; onTocado: raiz.aceptar()
        }
    }
}
