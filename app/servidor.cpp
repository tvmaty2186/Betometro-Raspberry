#include "servidor.h"

#include "datos.h"

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
        filas += QStringLiteral("<a class=c href=\"/informe/%1\"><b>%2</b><span>%3%4</span></a>"
                                "<a class=d href=\"/informe/%1?descargar=1\">Descargar</a>")
                     .arg(id, esc(c.value("nombre").toString()), esc(c.value("fechaLarga").toString()),
                          err.isEmpty() ? QString() : QStringLiteral(" · error ") + esc(err));
        n++;
    }
    if (!n) filas = QStringLiteral("<p class=v>Todavía no hay carreras corridas.</p>");
    const QString html = QStringLiteral(
        "<!doctype html><html lang=es><head><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\">"
        "<title>Betómetro</title><style>"
        "body{margin:0;font-family:system-ui,sans-serif;background:#0b2a17;color:#f7f7f2}"
        "header{background:linear-gradient(#1d5c33,#0f3b20);padding:18px 16px;border-bottom:4px solid #f4c531}"
        "h1{margin:0;font-size:26px;font-style:italic;letter-spacing:.02em}header p{margin:4px 0 0;color:#d6e6d2;font-size:13px}"
        "main{padding:12px 16px 40px;max-width:720px;margin:auto}"
        ".c{display:block;text-decoration:none;color:inherit;background:#143d24;border:1px solid #2b6b41;border-radius:12px 12px 0 0;padding:12px 14px;margin-top:12px}"
        ".c b{display:block;font-size:18px}.c span{color:#d6e6d2;font-size:13px}"
        ".d{display:block;text-align:center;text-decoration:none;background:#f4c531;color:#111;font-weight:800;border-radius:0 0 12px 12px;padding:10px}"
        ".z{display:block;text-align:center;text-decoration:none;margin-top:22px;border:2px solid #f4c531;color:#f4c531;font-weight:800;border-radius:12px;padding:12px}"
        ".v{color:#d6e6d2}</style></head><body><header><h1>BETÓMETRO</h1><p>Regularidad para el navegante · %1 carreras</p></header>"
        "<main>%2%3<p class=v style=\"font-size:12px;margin-top:22px\">Tocá una carrera para ver el informe. «Descargar» lo guarda en el teléfono; "
        "desde ahí se puede mandar a Drive o por WhatsApp.</p></main></body></html>")
                             .arg(n)
                             .arg(filas, n ? QStringLiteral("<a class=z href=\"/todos.zip\">Descargar todos (.zip)</a>") : QString());
    return html.toUtf8();
}
