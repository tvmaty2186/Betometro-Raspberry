import QtQuick
import Betometro

// Video de la carrera en el informe (E7): el reproductor a medias (arriba a la izquierda) o en pantalla completa
// (doble toque) y la tarjeta «En este momento». Ocupa toda la pantalla; el
// reproductor se comparte con el informe (`reproductor`) para la tele, la tabla y el mapa.
Item {
    id: vid
    property bool h: true
    property bool vv: false          // se está viendo el video
    property bool completo: false    // pantalla completa
    property var videos: []          // Datos.videos(id)
    property var inf: ({})           // Datos.informe(id)
    property real sug: 0             // velocidad sugerida (km/h)
    property real dist: 0
    property alias reproductor: reproductor
    signal dobleToque()
    anchors.fill: parent
    z: vid.completo ? 50 : 1
    // Video a medias: arriba a la izquierda; abajo, «En este momento»
    Tarjeta { datos: true
        id: cajaVideo
        visible: vid.vv
        oscura: true
        x: vid.h ? 24 : 20; y: vid.h ? 110 : 180
        width: vid.h ? 600 : parent.width - 40; height: 400
    }
    Tarjeta { datos: true
        visible: vid.vv
        oscura: true
        x: vid.h ? 24 : 370; y: vid.h ? 530 : 600
        width: vid.h ? 600 : 330; height: vid.h ? 170 : 280
        Sub { x: 18; y: 16; text: "En este momento"; tam: 12 }
        Grid {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 18 }
            columns: vid.h ? 3 : 1; rowSpacing: 4
            // Tres datos fijos con el texto enlazado: el Repeater recibía una lista nueva 10 veces por segundo (03-1.2)
            component DatoAhora: Item {
                property string e
                property string v
                property string u
                property color c
                width: vid.h ? (cajaVideo.width - 36) / 3 : 294; height: vid.h ? 96 : 64
                Sub { text: parent.e; tam: 11; anchors.top: vid.h ? parent.top : undefined; anchors.verticalCenter: vid.h ? undefined : parent.verticalCenter }
                Row {
                    anchors { bottom: parent.bottom; right: vid.h ? undefined : parent.right }
                    spacing: 4
                    Text { id: num; text: parent.parent.v; color: parent.parent.c; font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: vid.h ? 52 : 44 } }
                    Text { anchors.baseline: num.baseline; text: parent.parent.u; color: Tema.tintaSuave
                           font { family: Tema.fuenteNum; italic: Tema.cursiva; weight: Font.Bold; pixelSize: 20 } }
                }
            }
            DatoAhora { e: "Velocidad"; v: reproductor.e.lap >= 0 && reproductor.e.ok ? String(Math.round(reproductor.e.kmh)) : "–"; u: "km/h"; c: Tema.numero }
            DatoAhora { e: "Sugerida"; v: vid.sug > 0 ? vid.sug.toFixed(1).replace(".", ",") : "–"; u: ""; c: Tema.tinta }
            DatoAhora { e: "Adelanto"; v: reproductor.e.lap >= 0 && vid.dist > 0 ? Formato.dif(reproductor.e.ade * 1000) : "–"; u: ""
                        c: reproductor.e.lap < 0 || Math.abs(reproductor.e.ade) < 1 ? Tema.bien : Math.abs(reproductor.e.ade) < 3 ? Tema.acento : Tema.rojo }
        }
    }
    Reproductor {
        id: reproductor
        visible: vid.vv
        z: vid.completo ? 50 : 1
        activo: vid.vv
        completo: vid.completo
        inf: vid.inf
        videos: vid.videos
        x: vid.completo ? 0 : cajaVideo.x + 3; y: vid.completo ? 0 : cajaVideo.y + 3
        width: vid.completo ? vid.width : cajaVideo.width - 6; height: vid.completo ? vid.height : cajaVideo.height - 6
        onDobleToque: vid.dobleToque()
    }
}
