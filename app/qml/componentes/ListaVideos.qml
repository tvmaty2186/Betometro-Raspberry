import QtQuick
import Betometro

// Lista de los videos guardados (Ajustes → Cámara → «Ver videos»), del más nuevo al más viejo.
// Cada uno: carrera y fecha (o «Prueba» / «Sin carrera»), tamaño y «Borrar».
// Borrar pide confirmar: el primer toque lo pone rojo «¿Borrar?»; otro toque antes de 3 s lo borra.
// Nunca se borra mientras graba. Borra solo el video: la carrera y su informe quedan.
// grande: botones y letras del Modo Tablet.
Rectangle {
    id: raiz
    property bool grande: false
    property string confirmando: ""          // archivo que espera el segundo toque («__pruebas__» para los de prueba)
    readonly property bool h: Contexto.horizontal
    readonly property var lista: Camara.videos
    readonly property int pruebas: lista.filter(v => v.prueba).length
    readonly property real k: grande ? 1.45 : 1   // escala de letras y alturas
    function abrir() { confirmando = ""; visible = true; }
    function cerrar() { confirmando = ""; visible = false; }
    function tocarBorrar(archivo) {
        if (Camara.grabando) return;
        if (confirmando !== archivo) { confirmando = archivo; vuelve.restart(); return; }
        confirmando = "";
        if (archivo === "__pruebas__") Camara.borrarPruebas();
        else if (!Camara.borrarVideo(archivo)) Navegacion.avisar("No se pudo borrar el video");
    }
    Timer { id: vuelve; interval: 3000; onTriggered: raiz.confirmando = "" }

    visible: false
    anchors.fill: parent; z: 80
    color: Qt.rgba(0, 0, 0, 0.6)
    MouseArea { anchors.fill: parent; onClicked: raiz.cerrar() }   // tocar afuera cierra

    Tarjeta {
        id: caja
        oscura: true
        anchors.centerIn: parent
        width: Math.min(parent.width - (raiz.h ? 60 : 32), raiz.grande ? 1100 : 900)
        height: parent.height - (raiz.h ? 40 : 120)
        MouseArea { anchors.fill: parent }   // que tocar adentro no cierre

        TituloTarjeta { id: titulo; texto: "Videos guardados" }
        BotonCromo {
            anchors { right: parent.right; rightMargin: 20; top: parent.top; topMargin: 14 }
            width: 130 * raiz.k; height: 46 * raiz.k; tamTexto: 18 * raiz.k; texto: "Cerrar"
            onTocado: raiz.cerrar()
        }
        Text {
            id: resumen
            x: 22; anchors.top: titulo.bottom; anchors.topMargin: 6 * raiz.k
            width: parent.width - 44; wrapMode: Text.WordWrap
            color: Camara.grabando ? Tema.alerta : Tema.tintaSuave
            font { family: Tema.fuente600; pixelSize: 17 * raiz.k }
            text: Camara.grabando ? "Está grabando: para borrar, esperá a que termine"
                  : raiz.lista.length ? "Usado: " + (Camara.lugar.usadoGB || "0") + " GB en " + raiz.lista.length + (raiz.lista.length === 1 ? " video" : " videos")
                                        + " · entran " + (Camara.lugar.horas || 0) + " h más"
                  : "No hay videos guardados"
        }

        Flickable {
            id: desliza
            x: 20; width: parent.width - 40
            anchors { top: resumen.bottom; topMargin: 12 * raiz.k; bottom: pie.top; bottomMargin: 10 }
            contentHeight: col.height
            clip: true; boundsBehavior: Flickable.StopAtBounds
            Column {
                id: col; width: parent.width; spacing: 8 * raiz.k
                Repeater {
                    model: raiz.lista
                    Rectangle {
                        id: fila
                        required property var modelData
                        readonly property bool pide: raiz.confirmando === modelData.archivo
                        width: col.width; height: 70 * raiz.k; radius: Math.max(8, Tema.radio - 2)
                        color: Qt.rgba(1, 1, 1, 0.06)
                        border { width: 1.5; color: Tema.bordeCromoOscuro }
                        Column {
                            anchors { left: parent.left; leftMargin: 16; right: boton.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
                            spacing: 2
                            Text {
                                width: parent.width; elide: Text.ElideRight; color: Tema.tinta
                                font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 22 * raiz.k }
                                text: fila.modelData.carrera ? fila.modelData.carrera + (fila.modelData.fecha ? " · " + fila.modelData.fecha : "")
                                      : fila.modelData.prueba ? "Prueba" : "Sin carrera"
                            }
                            Text {
                                width: parent.width; elide: Text.ElideRight
                                color: fila.modelData.grabando ? Tema.alerta : Tema.tintaSuave
                                font { family: Tema.fuente600; pixelSize: 16 * raiz.k }
                                text: (fila.modelData.grabando ? "● Grabando ahora · " : "Grabado " + fila.modelData.hora + " · ") + fila.modelData.tam
                            }
                        }
                        BotonBorrar {
                            id: boton
                            anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                            pide: fila.pide
                            activo: !Camara.grabando
                            onTocado: raiz.tocarBorrar(fila.modelData.archivo)
                        }
                    }
                }
            }
        }

        // Abajo: borrar todas las pruebas juntas
        Item {
            id: pie
            x: 20; width: parent.width - 40
            anchors { bottom: parent.bottom; bottomMargin: 16 }
            height: raiz.pruebas > 0 ? 54 * raiz.k : 0
            visible: raiz.pruebas > 0
            BotonBorrar {
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                width: Math.min(parent.width, 380 * raiz.k)
                texto: "Borrar los de prueba (" + raiz.pruebas + ")"
                pide: raiz.confirmando === "__pruebas__"
                activo: !Camara.grabando
                onTocado: raiz.tocarBorrar("__pruebas__")
            }
        }
    }

    // Botón «Borrar» que se pone rojo «¿Borrar?» para confirmar
    component BotonBorrar: Rectangle {
        id: b
        property string texto: "Borrar"
        property bool pide: false
        property bool activo: true
        signal tocado()
        width: (raiz.grande ? 190 : 130); height: 48 * raiz.k
        radius: Math.max(6, Tema.radio - 4)
        opacity: activo ? 1 : 0.45
        color: pide ? "#c62a1f" : (zona.pressed ? Qt.lighter(Tema.caja1, 1.4) : Qt.lighter(Tema.caja1, 1.15))
        border { width: 2; color: pide ? "#ffb3aa" : Tema.bordeCromo }
        Text {
            anchors.centerIn: parent
            text: b.pide ? "¿Borrar?" : b.texto
            color: b.pide ? "#ffffff" : Tema.tinta
            font { family: Tema.fuente800; italic: Tema.cursiva; weight: Font.ExtraBold; pixelSize: 18 * raiz.k }
        }
        MouseArea { id: zona; anchors.fill: parent; enabled: b.activo; onClicked: b.tocado() }
    }
}
