// Compara una captura nueva con su referencia (auditoría 09, P2). Un píxel «cambió» si algún canal difiere en
// más de 24 (de 255). Imprime el porcentaje de píxeles que cambiaron y dónde (el rectángulo que los abarca), y
// si se pide, deja una imagen de la diferencia: la referencia atenuada con lo que cambió en rojo.
// Uso: comparar_capturas referencia.png nueva.png [--tolerancia 1.0] [--diferencia salida.png]
// Sale con 0 si la diferencia es menor o igual a la tolerancia (en %), 1 si cambió, 2 si falta algo.
#include <QColor>
#include <QImage>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "Uso: comparar_capturas referencia.png nueva.png [--tolerancia 1.0] [--diferencia salida.png]\n"); return 2; }
    double tolerancia = 1.0;
    const char* diferencia = nullptr;
    for (int i = 3; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--tolerancia") && i + 1 < argc) tolerancia = std::atof(argv[++i]);
        else if (!std::strcmp(argv[i], "--diferencia") && i + 1 < argc) diferencia = argv[++i];
    }
    QImage ref(QString::fromLocal8Bit(argv[1])), nueva(QString::fromLocal8Bit(argv[2]));
    if (ref.isNull()) { std::printf("FALTA la referencia %s\n", argv[1]); return 2; }
    if (nueva.isNull()) { std::printf("FALTA la captura %s\n", argv[2]); return 2; }
    if (ref.size() != nueva.size()) {
        std::printf("CAMBIO el tamaño: %dx%d antes, %dx%d ahora\n", ref.width(), ref.height(), nueva.width(), nueva.height());
        return 1;
    }
    ref = ref.convertToFormat(QImage::Format_RGB32);
    nueva = nueva.convertToFormat(QImage::Format_RGB32);
    QImage dif;
    if (diferencia) {
        dif = ref.copy();
        for (int y = 0; y < dif.height(); ++y) {
            auto* fila = reinterpret_cast<QRgb*>(dif.scanLine(y));
            for (int x = 0; x < dif.width(); ++x) {   // atenuada: para que lo rojo resalte
                const QRgb p = fila[x];
                fila[x] = qRgb(qRed(p) / 3 + 100, qGreen(p) / 3 + 100, qBlue(p) / 3 + 100);
            }
        }
    }
    long cambiados = 0;
    int x0 = ref.width(), y0 = ref.height(), x1 = -1, y1 = -1;
    for (int y = 0; y < ref.height(); ++y) {
        const auto* a = reinterpret_cast<const QRgb*>(ref.constScanLine(y));
        const auto* b = reinterpret_cast<const QRgb*>(nueva.constScanLine(y));
        auto* d = diferencia ? reinterpret_cast<QRgb*>(dif.scanLine(y)) : nullptr;
        for (int x = 0; x < ref.width(); ++x) {
            if (std::abs(qRed(a[x]) - qRed(b[x])) <= 24 && std::abs(qGreen(a[x]) - qGreen(b[x])) <= 24 && std::abs(qBlue(a[x]) - qBlue(b[x])) <= 24) continue;
            ++cambiados;
            if (x < x0) x0 = x;
            if (x > x1) x1 = x;
            if (y < y0) y0 = y;
            if (y > y1) y1 = y;
            if (d) d[x] = qRgb(230, 20, 20);
        }
    }
    const double pct = 100.0 * static_cast<double>(cambiados) / (static_cast<double>(ref.width()) * ref.height());
    if (cambiados == 0) { std::printf("IGUAL 0,000 %%\n"); return 0; }
    const bool ok = pct <= tolerancia;
    if (diferencia && !ok && !dif.save(QString::fromLocal8Bit(diferencia))) std::fprintf(stderr, "No pude guardar %s\n", diferencia);
    char num[32];
    std::snprintf(num, sizeof num, "%.3f", pct);
    if (char* punto = std::strchr(num, '.')) *punto = ',';   // en castellano: 0,186 %
    std::printf("%s %s %% x%d..%d y%d..%d\n", ok ? "IGUAL" : "CAMBIO", num, x0, x1, y0, y1);
    return ok ? 0 : 1;
}
