// Códigos QR para la pantalla: Image { source: "image://qr/http://betometro.local" }
#pragma once

#include <QQuickImageProvider>

class ProveedorQr : public QQuickImageProvider {
public:
    ProveedorQr() : QQuickImageProvider(QQuickImageProvider::Image) {}
    QImage requestImage(const QString& texto, QSize* tam, const QSize& pedido) override;
};
