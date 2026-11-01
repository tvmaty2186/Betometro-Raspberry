#include "qr.h"

#include <QPainter>
#include <QUrl>

#include "terceros/qrcodegen/qrcodegen.hpp"

QImage ProveedorQr::requestImage(const QString& texto, QSize* tam, const QSize& pedido) {
    const QString t = QUrl::fromPercentEncoding(texto.toUtf8());
    const auto qr = qrcodegen::QrCode::encodeText(t.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
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
