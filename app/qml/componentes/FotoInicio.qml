import QtQuick
import Betometro

// La foto fija del inicio, recortada como «cover» y fundida con el fondo
// (horizontal: hacia la derecha y hacia abajo; vertical: solo hacia abajo).
Canvas {
    id: lienzo
    property real focoY: 0.18          // background-position: center 18%
    property real fundeDerecha: 0.75   // 1 = no funde
    property real fundeAbajo: 0.70
    // La foto propia (Personalizar) ya viene recortada a la medida del marco; si no hay, la de fábrica
    // El Avispón Verde siempre con su foto original (Matías, 08/11/2026: es de su auto)
    readonly property string propia: Tema.avispon ? "" : (Contexto.horizontal ? Contexto.fotoH : Contexto.fotoV)
    readonly property string archivo: propia || "qrc:/recursos/img/inicio.jpg"
    // Cada vez que se guarda desde el celu la foto cambia de dirección (?v=N). El Canvas guarda cada imagen que
    // carga y no la suelta: ~12 MB por guardado, y la Pi se ponía lenta (Matías, 08/11/2026). Se suelta la anterior.
    property string cargada: ""
    function cargar() {
        if (cargada && cargada !== archivo) unloadImage(cargada);
        cargada = archivo; loadImage(archivo);
    }
    onArchivoChanged: { cargar(); requestPaint(); }
    Component.onCompleted: cargar()
    Component.onDestruction: if (cargada) unloadImage(cargada)
    onImageLoaded: requestPaint()
    onPaint: {
        const c = getContext("2d"), W = width, H = height;
        c.reset();
        if (!isImageLoaded(archivo)) return;
        if (propia) c.drawImage(archivo, 0, 0, W, H);   // ya tiene la medida del marco
        else {
            const iw = 1200, ih = 800;   // medidas de inicio.jpg (antes se copiaba la foto entera, 3,8 MB, solo para leerlas) (03-2.3)
            const e = Math.max(W / iw, H / ih), dw = iw * e, dh = ih * e;
            c.drawImage(archivo, (W - dw) / 2, (H - dh) * focoY, dw, dh);
        }
        c.globalCompositeOperation = "destination-in";
        if (fundeDerecha < 1) {
            const g = c.createLinearGradient(0, 0, W, 0);
            g.addColorStop(0, "rgba(0,0,0,1)"); g.addColorStop(fundeDerecha, "rgba(0,0,0,1)"); g.addColorStop(1, "rgba(0,0,0,0)");
            c.fillStyle = g; c.fillRect(0, 0, W, H);
        }
        const g2 = c.createLinearGradient(0, 0, 0, H);
        g2.addColorStop(0, "rgba(0,0,0,1)"); g2.addColorStop(fundeAbajo, "rgba(0,0,0,1)"); g2.addColorStop(1, "rgba(0,0,0,0)");
        c.fillStyle = g2; c.fillRect(0, 0, W, H);
    }
}
