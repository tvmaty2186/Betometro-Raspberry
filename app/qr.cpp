#include "qr.h"

#include <QPainter>
#include <QUrl>

#include <optional>
#include <stdexcept>

#include "terceros/qrcodegen/qrcodegen.hpp"

QImage ProveedorQr::requestImage(const QString& texto, QSize* tam, const QSize& pedido) {
    const QString t = QUrl::fromPercentEncoding(texto.toUtf8());
    // Más de ~2.300 bytes no entran en un QR: encodeText tira data_too_long, y acá (hilo de imágenes de Qt)
    // una excepción cerraba el programa. Queda un cuadrado blanco (11A-2.6)
    std::optional<qrcodegen::QrCode> hecho;
    try {
        hecho = qrcodegen::QrCode::encodeText(t.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
    } catch (const std::exception& e) {
        qWarning("QR: no se pudo armar (%s, %lld bytes)", e.what(), static_cast<long long>(t.toUtf8().size()));
        QImage vacio(pedido.width() > 0 ? pedido.width() : 200, pedido.height() > 0 ? pedido.height() : 200, QImage::Format_RGB32);
        vacio.fill(Qt::white);
        if (tam) *tam = vacio.size();
        return vacio;
    }
    const qrcodegen::QrCode& qr = *hecho;
    const int n = qr.getSize(), borde = 2, total = n + 2 * borde;
    const int lado = pedido.width() > 0 ? pedido.width() : total * 8;
    const int k = std::max(1, lado / total);
    QImage img(total * k, total * k, QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
            if (qr.getModule(x, y)) p.fillRect((x + borde) * k, (y + borde) * k, k, k, Qt::black);
    if (tam) *tam = img.size();
    return img;
}
