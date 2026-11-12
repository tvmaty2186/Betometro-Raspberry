// Cuánto tarda cada parte del arranque de Betómetro (para hacerlo más rápido, 09/11/2026). Se anota siempre (es
// barato) y la Prueba completa lo suma al paso «Sistema»; con BETO_MEDIR_ARRANQUE=1 además se escribe al momento.
#pragma once

#include <QElapsedTimer>
#include <QFile>
#include <QStringList>
#include <unistd.h>

namespace arranque {

inline QElapsedTimer& reloj() { static QElapsedTimer r; if (!r.isValid()) r.start(); return r; }
inline QStringList& marcas() { static QStringList m; return m; }

// Segundos desde que prendió la Raspberry hasta que empezó este programa (campo 22 de /proc/self/stat)
inline double empezoA() {
    static double s = -1;
    if (s >= 0) return s;
    QFile f(QStringLiteral("/proc/self/stat"));
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray t = f.readAll();
        const QList<QByteArray> c = t.mid(t.lastIndexOf(')') + 2).split(' ');   // empieza en el campo 3
        if (c.size() > 19) s = c[19].toDouble() / sysconf(_SC_CLK_TCK);
    }
    return s;
}

inline void marcar(const char* que) {
    const qint64 ms = reloj().elapsed();
    marcas() << QStringLiteral("%1 %2 s").arg(QString::fromUtf8(que), QString::number(ms / 1000.0, 'f', 1).replace('.', ','));
    if (qEnvironmentVariableIsSet("BETO_MEDIR_ARRANQUE")) qInfo("arranque %6lld ms · %s", static_cast<long long>(ms), que);
}

// «Betómetro empezó a los 13,4 s · datos 0,2 s · … · primera imagen 8,1 s»
inline QString resumen() {
    const double e = empezoA();
    // Pasado el minuto no es el arranque: el programa se volvió a abrir (actualización o reinicio). Antes decía
    // «empezó a los 480,4 s de prender» y confundía (reporte del 12/11/2026)
    const QString cuando = e < 0 ? QString()
                           : e <= 60 ? QStringLiteral("Betómetro empezó a los %1 s de prender · ").arg(QString::number(e, 'f', 1).replace('.', ','))
                                     : QStringLiteral("Betómetro se volvió a abrir a los %1 min de prender (actualización o reinicio) · ").arg(qRound(e / 60.0));
    return cuando
           + marcas().join(QStringLiteral(" · "));
}

}  // namespace arranque
