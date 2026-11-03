#include "servidor.h"

#include "datos.h"
#include "reportes.h"

#include <QFile>
#include <QTcpSocket>
#include <QUrl>

namespace {
QByteArray respuesta(int codigo, const QByteArray& tipo, const QByteArray& cuerpo, const QByteArray& extra = {}) {
    const QByteArray estado = codigo == 200 ? "200 OK" : codigo == 404 ? "404 Not Found" : "400 Bad Request";
    return "HTTP/1.1 " + estado + "\r\nContent-Type: " + tipo + "\r\nContent-Length: " + QByteArray::number(cuerpo.size()) +
           "\r\nCache-Control: no-store\r\nConnection: close\r\n" + extra + "\r\n" + cuerpo;
}
QByteArray adjunto(const QString& nombre) {   // nombre con acentos: filename* en UTF-8
    QString ascii;
    for (QChar c : nombre) ascii += (c.unicode() < 128 && c != '"') ? c : QChar('_');
    return "Content-Disposition: attachment; filename=\"" + ascii.toLatin1() + "\"; filename*=UTF-8''" +
           QUrl::toPercentEncoding(nombre) + "\r\n";
}
QString esc(QString s) {
    return s.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;").replace('"', "&quot;");
}
quint32 crc32(const QByteArray& d) {
    static quint32 tabla[256];
    static bool lista = false;
    if (!lista) {
        for (quint32 i = 0; i < 256; i++) {
            quint32 c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            tabla[i] = c;
        }
        lista = true;
    }
    quint32 c = 0xFFFFFFFFu;
    for (unsigned char b : d) c = tabla[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
void le16(QByteArray& o, quint16 v) { o.append(char(v & 0xFF)); o.append(char(v >> 8)); }
void le32(QByteArray& o, quint32 v) { for (int i = 0; i < 4; i++) o.append(char((v >> (8 * i)) & 0xFF)); }
}  // namespace

QByteArray Servidor::zip(const QList<QPair<QString, QByteArray>>& archivos) {
    QByteArray o, central;
    quint16 n = 0;
    for (const auto& [nombre, datos] : archivos) {
        const QByteArray nb = nombre.toUtf8();
        const quint32 crc = crc32(datos), pos = static_cast<quint32>(o.size()), tam = static_cast<quint32>(datos.size());
        le32(o, 0x04034b50); le16(o, 20); le16(o, 0x0800); le16(o, 0); le16(o, 0); le16(o, 0x21);   // UTF-8, guardado, fecha 1980-01-01
        le32(o, crc); le32(o, tam); le32(o, tam); le16(o, static_cast<quint16>(nb.size())); le16(o, 0);
        o += nb; o += datos;
        le32(central, 0x02014b50); le16(central, 20); le16(central, 20); le16(central, 0x0800); le16(central, 0);
        le16(central, 0); le16(central, 0x21); le32(central, crc); le32(central, tam); le32(central, tam);
        le16(central, static_cast<quint16>(nb.size())); le16(central, 0); le16(central, 0); le16(central, 0); le16(central, 0);
        le32(central, 0); le32(central, pos); central += nb;
        n++;
    }
    const quint32 inicio = static_cast<quint32>(o.size());
    o += central;
    le32(o, 0x06054b50); le16(o, 0); le16(o, 0); le16(o, n); le16(o, n);
    le32(o, static_cast<quint32>(central.size())); le32(o, inicio); le16(o, 0);
    return o;
}

Servidor::Servidor(Datos* datos, quint16 puerto, QObject* padre) : QObject(padre), datos_(datos), puerto_(puerto) {
    connect(&tcp_, &QTcpServer::newConnection, this, [this] {
        while (QTcpSocket* s = tcp_.nextPendingConnection()) {
            connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
            connect(s, &QTcpSocket::readyRead, s, [this, s] {
                const QByteArray pedido = s->property("pedido").toByteArray() + s->readAll();
                s->setProperty("pedido", pedido);
                if (!pedido.contains("\r\n\r\n") && pedido.size() < 8192) return;   // todavía no llegó entero
                const QList<QByteArray> linea = pedido.left(pedido.indexOf("\r\n")).split(' ');
                s->write(linea.size() >= 2 && linea[0] == "GET" ? responder(linea[1]) : respuesta(400, "text/plain", "Solo se puede mirar"));
                s->disconnectFromHost();
            });
        }
    });
    prender(true);
}

void Servidor::prender(bool si) {
    if (si == activo()) return;
    if (si) {
        if (!tcp_.listen(QHostAddress::Any, puerto_)) qWarning("Betómetro en la red: no pude abrir el puerto %d", puerto_);
    } else {
        tcp_.close();
    }
    emit cambio();
}

QByteArray Servidor::responder(const QByteArray& rutaCruda) const {
    const QUrl url(QString::fromUtf8(rutaCruda));
    const QString ruta = url.path();
    const bool descargar = url.query().contains(QLatin1String("descargar"));
    if (ruta == QLatin1String("/") || ruta == QLatin1String("/index.html")) return respuesta(200, "text/html; charset=utf-8", pagina());
    if (ruta.startsWith(QLatin1String("/informe/"))) {
        const QString id = ruta.mid(9);
        const QByteArray html = datos_->informeHtml(id);
        if (html.isEmpty()) return respuesta(404, "text/plain; charset=utf-8", "No está esa carrera");
        return respuesta(200, "text/html; charset=utf-8", html, descargar ? adjunto(datos_->nombreInforme(id)) : QByteArray());
    }
    if (ruta.startsWith(QLatin1String("/r/"))) {   // león y tipografías para la página (de los recursos del programa)
        const QString n = ruta.mid(3);
        const QString archivo = n == QLatin1String("leon.png") ? QStringLiteral(":/recursos/img/leon.png")
                                : QStringLiteral(":/recursos/fuentes/") + n;
        if (n.contains(QLatin1String("..")) || n.contains('/')) return respuesta(404, "text/plain", "No está");
        QFile f(archivo);
        if (!f.open(QIODevice::ReadOnly)) return respuesta(404, "text/plain", "No está");
        return respuesta(200, n.endsWith(QLatin1String(".png")) ? "image/png" : "font/woff2", f.readAll(), "Cache-Control: max-age=86400\r\n");
    }
    if (ruta == QLatin1String("/reportes")) {   // errores anotados (para mandarlos y mejorar Betómetro)
        const QByteArray t = reportes_ ? reportes_->texto() : QByteArray();
        return respuesta(200, "text/plain; charset=utf-8", t.isEmpty() ? QByteArray("Sin errores anotados.\n") : t,
                         descargar ? adjunto(QStringLiteral("betometro-errores.txt")) : QByteArray());
    }
    if (ruta == QLatin1String("/todos.zip")) {
        QList<QPair<QString, QByteArray>> archivos;
        for (const QVariant& v : datos_->corridas() + datos_->proximas()) {
            const QVariantMap c = v.toMap();
            if (!c.value("tieneVueltas").toBool()) continue;
            archivos << qMakePair(datos_->nombreInforme(c.value("id").toString()), datos_->informeHtml(c.value("id").toString()));
        }
        return respuesta(200, "application/zip", zip(archivos), adjunto(QStringLiteral("Betometro informes.zip")));
    }
    return respuesta(404, "text/plain; charset=utf-8", "No está");
}

QByteArray Servidor::pagina() const {
    QString filas;
    int n = 0;
    for (const QVariant& v : datos_->corridas() + datos_->proximas()) {
        const QVariantMap c = v.toMap();
        if (!c.value("tieneVueltas").toBool()) continue;
        const QString id = QString::fromUtf8(QUrl::toPercentEncoding(c.value("id").toString()));
        const QString err = c.value("errorTotal").toString();
        filas += QStringLiteral("<div class=c><a class=info href=\"/informe/%1\"><span class=dia>%5<small>%6</small></span>"
                                "<span class=txt><b>%2</b><span>%3</span></span>%4</a>"
                                "<div class=bot><a class=ver href=\"/informe/%1\">Ver informe</a><a class=bajar href=\"/informe/%1?descargar=1\">Descargar</a></div></div>")
                     .arg(id, esc(c.value("nombre").toString()), esc(c.value("fechaLarga").toString()),
                          err.isEmpty() ? QString() : QStringLiteral("<span class=err>%1</span>").arg(esc(err)),
                          esc(c.value("dia").toString()), esc(c.value("mes").toString()));
        n++;
    }
    if (!n) filas = QStringLiteral("<p class=v>Todavía no hay carreras corridas.</p>");
    const QString html = QStringLiteral(
        "<!doctype html><html lang=es><head><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\">"
        "<title>Betómetro · Avispón Verde</title><style>"
        "@font-face{font-family:Barlow;font-weight:600;src:url(/r/barlow-latin-600-normal.woff2)}"
        "@font-face{font-family:Barlow;font-weight:600;font-style:italic;src:url(/r/barlow-latin-600-italic.woff2)}"
        "@font-face{font-family:Barlow;font-weight:800;font-style:italic;src:url(/r/barlow-latin-800-italic.woff2)}"
        "@font-face{font-family:'Barlow Condensed';font-weight:700;src:url(/r/barlow-condensed-latin-700-normal.woff2)}"
        "*{box-sizing:border-box}body{margin:0;font-family:Barlow,system-ui,sans-serif;font-weight:600;color:#f7f7f2;min-height:100vh;"
        "background:linear-gradient(180deg,rgba(11,42,23,0) 0,#0b2a17 260px),repeating-conic-gradient(#ffffff09 0 25%,transparent 0 50%) 0 0/22px 22px,"
        "radial-gradient(140% 70% at 50% 0%,#2f7a49,#0b2a17 70%);background-color:#0b2a17}"
        "main{max-width:620px;margin:auto;padding:0 16px 40px}"
        ".cab{position:relative;display:flex;align-items:center;gap:10px;padding:16px 0 12px;margin-bottom:6px;border-bottom:3px solid #f4c531;overflow:hidden}"
        ".cab img{height:50px;filter:drop-shadow(0 2px 2px #0008)}.cab .bar{width:2px;height:42px;background:linear-gradient(#fff,#9aa3ad);transform:skewX(-18deg);opacity:.8}"
        ".marca b{display:block;font:800 italic 28px/1 Barlow;background:linear-gradient(180deg,#fff 0%,#d9dde1 45%,#8d959c 55%,#e9ecef 100%);"
        "-webkit-background-clip:text;background-clip:text;color:transparent;filter:drop-shadow(0 2px 1px #000a)}"
        ".marca span{display:block;font:600 9px/1.4 Barlow;letter-spacing:.2em;color:#b9cdb8;margin-top:3px}"
        ".rayas{position:absolute;right:-6px;top:-10px;bottom:-4px;width:74px;background:linear-gradient(105deg,transparent 0 30%,#f7f7f2 30% 46%,transparent 46% 58%,#f4c531 58% 74%,transparent 74%)}"
        "h2{font:800 italic 13px Barlow;letter-spacing:.18em;color:#b9cdb8;margin:16px 2px 4px}"
        ".c{position:relative;margin-top:14px;border-radius:18px;overflow:hidden;background:radial-gradient(120% 90% at 50% 0%,#1f4a2f,#08190e);"
        "box-shadow:0 0 0 2px #cfd4d8,0 0 0 3px #4b5258,0 10px 18px #0008,inset 0 1px 0 #ffffff4d}"
        ".info{display:flex;align-items:center;gap:12px;padding:12px 14px;text-decoration:none;color:inherit}"
        ".dia{flex:none;width:52px;height:52px;border-radius:50%;display:flex;flex-direction:column;align-items:center;justify-content:center;"
        "background:linear-gradient(#ffe07a,#f4c531 50%,#d9a20f);color:#14240f;font:700 22px/1 'Barlow Condensed';box-shadow:0 0 0 2px #fff,0 3px 6px #0008}"
        ".dia small{font:800 italic 9px Barlow;letter-spacing:.1em}"
        ".txt{flex:1;min-width:0}.txt b{display:block;font:800 italic 21px/1.1 Barlow;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}"
        ".txt span{display:block;color:#b9cdb8;font-size:12px;font-style:italic;margin-top:2px}"
        ".err{flex:none;font:700 italic 24px 'Barlow Condensed';color:#f4c531}"
        ".bot{display:flex;border-top:1px solid #ffffff22}.bot a{flex:1;text-align:center;text-decoration:none;padding:12px;font:800 italic 15px Barlow}"
        ".ver{color:#f7f7f2}.bajar{background:linear-gradient(#ffe07a,#f4c531 50%,#d9a20f);color:#14240f}"
        ".z{display:block;text-align:center;text-decoration:none;margin-top:22px;border:2px solid #f4c531;color:#f4c531;font:800 italic 16px Barlow;border-radius:14px;padding:13px}"
        ".v{color:#b9cdb8;font-style:italic}.pie{font-size:12px;margin-top:22px}.pie a{color:#b9cdb8}</style></head><body><main>"
        "<div class=cab><img alt=\"\" src=\"/r/leon.png\"><div class=bar></div><div class=marca><b>BETÓMETRO</b><span>REGULARIDAD PARA EL NAVEGANTE</span></div><div class=rayas></div></div>"
        "<h2>CARRERAS · %1</h2>%2%3"
        "<p class=\"v pie\">Tocá una carrera para ver el informe. «Descargar» lo guarda en el teléfono; desde ahí se puede mandar a Drive o por WhatsApp.</p>"
        "<p class=\"v pie\"><a href=\"/reportes?descargar=1\">Reporte de errores</a> · Avispón Verde #101</p></main></body></html>")
                             .arg(n)
                             .arg(filas, n ? QStringLiteral("<a class=z href=\"/todos.zip\">Descargar todos (.zip)</a>") : QString());
    return html.toUtf8();
}
