import QtQuick
import Betometro

// Teclado numérico grande para los casilleros del tiempo (muestra aprobada 1.0.29): se abre en el lugar de
// IR A LARGAR, así se ve lo que se escribe sin tapar nada. 1-9, borrar, 0 y ✓.
Rectangle {
    id: raiz
    property string titulo: ""
    property string ayuda: ""
    signal digito(string d)
    signal borrar()
    signal listo()
    radius: Math.max(14, Tema.radio * 1.7)
    border { width: 3; color: Tema.bordeCromo }
    gradient: Gradient { GradientStop { position: 0; color: Qt.lighter(Tema.caja1, 1.15) } GradientStop { position: 1; color: Tema.caja2 } }
    MouseArea { anchors.fill: parent }   // no deja pasar los toques al botón de abajo
    Text { x: 26; y: 18; text: raiz.titulo; color: Tema.tinta; font { family: Tema.fuente800; italic: Tema.cursiva; pixelSize: 24 } }
    Text { anchors.right: parent.right; anchors.rightMargin: 26; y: 22; text: raiz.ayuda; color: Tema.acento; font { family: Tema.fuente600; pixelSize: 19 } }
    Grid {
        id: g
        x: 20; y: 70
        width: raiz.width - 40; height: raiz.height - 90
        columns: 3; spacing: 14
        readonly property real an: (width - 2 * spacing) / 3
        readonly property real al: (height - 3 * spacing) / 4
        Repeater {
            model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "⌫", "0", "✓"]
            Rectangle {
                id: tecla
                width: g.an; height: g.al; radius: 16
                readonly property bool ok: modelData === "✓"
                border { width: 2; color: ok ? Tema.acentoClaro : Tema.bordeCromo }
                gradient: Gradient {
                    GradientStop { position: 0; color: tecla.ok ? Tema.acentoClaro : Qt.lighter(Tema.caja1, 1.25) }
                    GradientStop { position: 1; color: tecla.ok ? Tema.acentoOscuro : Tema.caja2 }
                }
                Rectangle { anchors.fill: parent; radius: parent.radius; color: "#ffffff"; opacity: z.pressed ? 0.2 : 0 }
                TIcono { visible: parent.ok || modelData === "⌫"; anchors.centerIn: parent; width: 52; height: 52
                         nombre: parent.ok ? "tilde" : "borrar"; color: parent.ok ? Tema.acentoTinta : Tema.tinta }
                Text { visible: !parent.ok && modelData !== "⌫"; anchors.centerIn: parent; text: modelData; color: Tema.tinta
                       font { family: Tema.fuenteNum; weight: Font.Bold; pixelSize: 56 } }
                MouseArea { id: z; anchors.fill: parent
                    onClicked: modelData === "✓" ? raiz.listo() : modelData === "⌫" ? raiz.borrar() : raiz.digito(modelData) }
            }
        }
    }
}
