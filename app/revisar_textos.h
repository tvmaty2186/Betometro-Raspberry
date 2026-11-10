// Revisor de textos (solo para pruebas, BETO_REVISAR_TEXTOS=1 con --captura): recorre la pantalla y avisa cada
// texto que no entra (más ancho que su lugar sin achicarse ni cortarse), que se sale de su cuadro o que se pisa con
// otro. Para repasar todos los temas con precisión (Matías, 10/11/2026: en Cabina los números se salían).
#pragma once

#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <cmath>
#include <vector>

namespace revisar {

struct Texto {
    QQuickItem* item;
    bool desliza = false;   // está en una lista que se desliza
    QRectF r;          // lo que ocupa de verdad, en coordenadas de la ventana
    QQuickItem* caja;  // el cuadro que lo contiene (tarjeta, botón, fila…)
    QString texto;
};

inline bool visible(QQuickItem* i) {
    for (QQuickItem* p = i; p; p = p->parentItem())
        if (!p->isVisible() || p->opacity() < 0.05) return false;
    return true;
}

inline QString clase(QQuickItem* i) { return QString::fromLatin1(i->metaObject()->className()).section(QLatin1String("_QML"), 0, 0); }

// Un cuadro: lo que tiene borde redondeado (tarjetas, botones, chips, filas) o recorta
inline bool esCaja(QQuickItem* i) {
    static const QStringList nombres{QStringLiteral("Tarjeta"), QStringLiteral("TBoton"), QStringLiteral("TFila"), QStringLiteral("Chip"),
                                     QStringLiteral("TOpciones"), QStringLiteral("TMasMenos"), QStringLiteral("BotonAmarillo"), QStringLiteral("BotonCromo")};
    if (i->clip()) return true;
    const QString c = clase(i);
    for (const QString& n : nombres) if (c == n) return true;
    return false;
}

inline QString limpio(QString t) {
    t.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    t.replace('\n', ' ');
    return t.left(40);
}

inline void juntar(QQuickItem* i, std::vector<Texto>& v, QStringList& avisos) {
    for (QQuickItem* h : i->childItems()) juntar(h, v, avisos);
    if (QString::fromLatin1(i->metaObject()->className()) != QLatin1String("QQuickText")) return;
    const QString texto = i->property("text").toString();
    if (texto.trimmed().isEmpty() || !visible(i) || i->width() <= 0) return;
    const double cw = i->property("contentWidth").toDouble(), ch = i->property("contentHeight").toDouble();
    const int elide = i->property("elide").toInt(), wrap = i->property("wrapMode").toInt(), fit = i->property("fontSizeMode").toInt();
    const int alH = i->property("horizontalAlignment").toInt();
    // 1) No entra en su lugar (y no se achica ni se corta)
    if (cw > i->width() + 2 && elide == 0 && wrap == 0 && fit == 0)
        avisos << QStringLiteral("NO ENTRA  %1 px de más · «%2»").arg(std::lround(cw - i->width())).arg(limpio(texto));
    // Lo que ocupa de verdad, según la alineación
    double x0 = 0;
    const double w = std::min(cw, std::max<double>(i->width(), cw));
    if (alH == Qt::AlignHCenter) x0 = (i->width() - w) / 2; else if (alH == Qt::AlignRight) x0 = i->width() - w;
    const double vh = i->height() > 0 ? std::min(ch, i->height() + 1000) : ch;
    double y0 = 0;
    const int alV = i->property("verticalAlignment").toInt();
    if (alV == Qt::AlignVCenter) y0 = (i->height() - vh) / 2; else if (alV == Qt::AlignBottom) y0 = i->height() - vh;
    const QRectF r = i->mapRectToScene(QRectF(x0, y0, w, vh));
    QQuickItem* caja = nullptr;
    for (QQuickItem* p = i->parentItem(); p; p = p->parentItem()) if (esCaja(p)) { caja = p; break; }
    // Dentro de una lista que se desliza: lo que queda fuera de la parte visible no se ve (no es un error)
    for (QQuickItem* p = i->parentItem(); p; p = p->parentItem())
        if (clase(p).contains(QLatin1String("Flickable")) || clase(p).contains(QLatin1String("ListView"))) {
            const QRectF vista = p->mapRectToScene(QRectF(0, 0, p->width(), p->height()));
            if (!vista.intersects(r)) return;
            if (caja == p) caja = nullptr;
            Texto t{i, true, r, caja, limpio(texto)};
            v.push_back(t);
            return;
        }
    v.push_back({i, false, r, caja, limpio(texto)});
}

// Devuelve los avisos (vacío = todo bien)
inline QStringList revisar(QQuickWindow* ventana) {
    QStringList avisos;
    if (!ventana) return avisos;
    std::vector<Texto> v;
    juntar(ventana->contentItem(), v, avisos);
    const QRectF pantalla(0, 0, ventana->width(), ventana->height());
    for (const Texto& t : v) {
        // 2) Se sale de la pantalla o de su cuadro
        if (t.desliza) continue;   // en una lista que se desliza, lo que no se ve se ve deslizando
        if (!pantalla.adjusted(-2, -2, 2, 2).contains(t.r))
            avisos << QStringLiteral("SALE DE LA PANTALLA · «%1»").arg(t.texto);
        else if (t.caja) {
            const QRectF c = t.caja->mapRectToScene(QRectF(0, 0, t.caja->width(), t.caja->height()));
            if (!c.adjusted(-2, -2, 2, 2).contains(t.r)) {
                const double sale = std::max({c.left() - t.r.left(), t.r.right() - c.right(), c.top() - t.r.top(), t.r.bottom() - c.bottom()});
                avisos << QStringLiteral("SALE DEL CUADRO (%1) %2 px · «%3»").arg(clase(t.caja)).arg(std::lround(sale)).arg(t.texto);
            }
        }
    }
    // 3) Dos textos que se pisan (más de 3 px en los dos sentidos)
    for (size_t a = 0; a < v.size(); ++a)
        for (size_t b = a + 1; b < v.size(); ++b) {
            // Solo dentro del mismo cuadro: un menú o una ventana encima tapa lo de abajo (no es pisarse)
            if (v[a].caja != v[b].caja) continue;
            const QRectF x = v[a].r.intersected(v[b].r);
            if (x.width() > 3 && x.height() > 3)
                avisos << QStringLiteral("SE PISAN  «%1» con «%2»").arg(v[a].texto, v[b].texto);
        }
    avisos.removeDuplicates();
    return avisos;
}

}  // namespace revisar
