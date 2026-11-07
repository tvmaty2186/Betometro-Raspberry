import QtQuick
import Betometro

// Ventana para confirmar que se borra una carrera: Cancelar / Borrar. La misma en la lista de Carreras y en la
// ficha (07-3.5: antes la ficha pedía «Tocá otra vez»). carrera: la carrera (null: no se ve).
Rectangle {
    id: raiz
    property var carrera: null
    signal cancelar()
    signal borrar()
    visible: carrera !== null && carrera !== undefined
    anchors.fill: parent; z: 80
    color: Qt.rgba(0, 0, 0, 0.7)
    MouseArea { anchors.fill: parent; onClicked: raiz.cancelar() }
    Column {
        anchors.centerIn: parent; spacing: 30
        width: parent.width - 60
        Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
               text: raiz.carrera ? "¿Borrar «" + raiz.carrera.nombre + "»?" : ""
               color: "#ffffff"; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 38 } }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter; spacing: 24
            TBoton { width: 260; height: 120; nombre: "Cancelar"; onTocado: raiz.cancelar() }
            TBoton { width: 260; height: 120; tipo: "peligro"; nombre: "Borrar"
                     dato: raiz.carrera && Datos.videos(raiz.carrera.id).length ? "vueltas y video" : "y sus vueltas"
                     onTocado: raiz.borrar() }
        }
    }
}
