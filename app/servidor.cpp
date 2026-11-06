#include "servidor.h"

#include "control_remoto.h"
#include "datos.h"
#include "drive.h"
#include "reportes.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr qint64 kPedazo = 256 * 1024;   // de a cuánto se manda un video (y cuánto puede esperar en el socket)

QByteArray respuesta(int codigo, const QByteArray& tipo, const QByteArray& cuerpo, const QByteArray& extra = {}) {
    const QByteArray estado = codigo == 200 ? "200 OK" : codigo == 404 ? "404 Not Found" : codigo == 416 ? "416 Range Not Satisfiable" : "400 Bad Request";
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
                if (s->property("atendido").toBool()) return;
                const QList<QByteArray> linea = pedido.left(pedido.indexOf("\r\n")).split(' ');
                // POST /drive: el archivo del cliente de Google (chico: hasta 64 KB). Se espera a que llegue entero.
                if (linea.size() >= 2 && linea[0] == "POST" && QUrl(QString::fromUtf8(linea[1])).path() == QLatin1String("/drive")) {
                    const int fin = pedido.indexOf("\r\n\r\n");
                    qint64 largo = -1;
                    for (const QByteArray& c : pedido.left(fin).split('\n'))
                        if (c.trimmed().toLower().startsWith("content-length:")) largo = c.trimmed().mid(15).trimmed().toLongLong();
                    if (largo < 0 || largo > 65536) {
                        s->setProperty("atendido", true);
                        s->write(respuesta(400, "text/plain; charset=utf-8", "Archivo muy grande"));
                        s->disconnectFromHost();
                        return;
                    }
                    if (pedido.size() - fin - 4 < largo) return;   // falta el resto del archivo
                    s->setProperty("atendido", true);
                    s->write(respuesta(200, "text/html; charset=utf-8", recibirDrive(pedido.mid(fin + 4, largo))));
                    s->disconnectFromHost();
                    return;
                }
                s->setProperty("atendido", true);
                if (linea.size() >= 2 && linea[0] == "GET" && linea[1].startsWith("/video/")) {
                    const QUrl url(QString::fromUtf8(linea[1]));
                    QByteArray rangoPedido;   // la cabecera «Range:» (el navegador la manda para adelantar el video)
                    for (const QByteArray& c : pedido.left(pedido.indexOf("\r\n\r\n")).split('\n'))
                        if (c.trimmed().toLower().startsWith("range:")) rangoPedido = c.trimmed().mid(6).trimmed();
                    enviarVideo(s, url.path(QUrl::FullyDecoded).mid(7), rangoPedido, url.query().contains(QLatin1String("descargar")));
                    return;
                }
                // POST solo para los botones del celu (/control/accion): todo va en la dirección, sin cuerpo
                const bool postControl = linea.size() >= 2 && linea[0] == "POST" && QUrl(QString::fromUtf8(linea[1])).path() == QLatin1String("/control/accion");
                s->write(linea.size() >= 2 && (linea[0] == "GET" || postControl) ? responder(linea[1]) : respuesta(400, "text/plain", "Solo se puede mirar"));
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
        // Para la carrera: se corta también lo que se estaba mandando (un video ocupa la tarjeta y la wifi)
        for (QTcpSocket* s : tcp_.findChildren<QTcpSocket*>()) s->abort();
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
                                : n == QLatin1String("icono.png") ? QStringLiteral(":/recursos/img/betometro-icono-512.png")
                                : n == QLatin1String("icono-redondo.png") ? QStringLiteral(":/recursos/img/betometro-icono-redondo-512.png")
                                : QStringLiteral(":/recursos/fuentes/") + n;
        if (n.contains(QLatin1String("..")) || n.contains('/')) return respuesta(404, "text/plain", "No está");
        QFile f(archivo);
        if (!f.open(QIODevice::ReadOnly)) return respuesta(404, "text/plain", "No está");
        return respuesta(200, n.endsWith(QLatin1String(".png")) ? "image/png" : "font/woff2", f.readAll(), "Cache-Control: max-age=86400\r\n");
    }
    // Para el acceso directo del celu: nombre, ícono (el de la app) y colores. Chrome no deja «instalar»
    // una página sin https; el acceso directo sí, y con esto queda con el ícono de Betómetro.
    if (ruta == QLatin1String("/manifest.webmanifest"))
        return respuesta(200, "application/manifest+json",
                         "{\"name\":\"Betómetro\",\"short_name\":\"Betómetro\",\"start_url\":\"/\",\"display\":\"standalone\","
                         "\"background_color\":\"#0d2a18\",\"theme_color\":\"#0d2a18\",\"icons\":["
                         "{\"src\":\"/r/icono.png\",\"sizes\":\"512x512\",\"type\":\"image/png\",\"purpose\":\"any\"},"
                         "{\"src\":\"/r/icono-redondo.png\",\"sizes\":\"512x512\",\"type\":\"image/png\",\"purpose\":\"maskable\"}]}");
    if (ruta == QLatin1String("/favicon.ico")) {
        QFile f(QStringLiteral(":/recursos/img/betometro-icono-512.png"));
        if (f.open(QIODevice::ReadOnly)) return respuesta(200, "image/png", f.readAll(), "Cache-Control: max-age=86400\r\n");
    }
    // El celu como control remoto del modo tele: la página, cómo está la tele y los botones
    if (ruta == QLatin1String("/control")) return respuesta(200, "text/html; charset=utf-8", paginaControl());
    if (ruta == QLatin1String("/control/estado"))
        return respuesta(200, "application/json; charset=utf-8", control_ ? control_->estadoJson() : QByteArray("{\"activa\":false,\"carreras\":[]}"));
    if (ruta == QLatin1String("/control/accion")) {
        const QUrlQuery q(url);
        const QString problema = control_ ? control_->hacer(q.queryItemValue(QStringLiteral("tipo"), QUrl::FullyDecoded),
                                                            q.queryItemValue(QStringLiteral("valor"), QUrl::FullyDecoded))
                                          : QStringLiteral("La tele no está en uso");
        QJsonObject r{{QStringLiteral("ok"), problema.isEmpty()}};
        if (!problema.isEmpty()) r[QStringLiteral("error")] = problema;
        return respuesta(200, "application/json; charset=utf-8", QJsonDocument(r).toJson(QJsonDocument::Compact));
    }
    if (ruta == QLatin1String("/drive")) return respuesta(200, "text/html; charset=utf-8", paginaDrive());
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

bool Servidor::rango(const QByteArray& pedido, qint64 tam, qint64& desde, qint64& hasta) {
    // Solo un tramo: «bytes=a-b», «bytes=a-» o «bytes=-n» (los últimos n). Con varios tramos se toma el primero.
    QByteArray r = pedido.trimmed();
    if (!r.toLower().startsWith("bytes=") || tam <= 0) return false;
    r = r.mid(6);
    if (r.contains(',')) r = r.left(r.indexOf(','));
    const int guion = r.indexOf('-');
    if (guion < 0) return false;
    const QByteArray a = r.left(guion).trimmed(), b = r.mid(guion + 1).trimmed();
    bool okA = true, okB = true;
    if (a.isEmpty()) {
        const qint64 n = b.toLongLong(&okB);
        if (!okB || n <= 0) return false;
        desde = qMax<qint64>(0, tam - n);
        hasta = tam - 1;
        return true;
    }
    desde = a.toLongLong(&okA);
    hasta = b.isEmpty() ? tam - 1 : b.toLongLong(&okB);
    if (!okA || !okB || desde < 0 || desde >= tam || hasta < desde) return false;
    hasta = qMin(hasta, tam - 1);
    return true;
}

void Servidor::enviarVideo(QTcpSocket* s, const QString& nombre, const QByteArray& rangoPedido, bool descargar) {
    const auto chau = [s](const QByteArray& r) { s->write(r); s->disconnectFromHost(); };
    // Solo un nombre de archivo de la carpeta de videos: nada de «..» ni carpetas
    const bool mkv = nombre.endsWith(QLatin1String(".mkv"), Qt::CaseInsensitive), mp4 = nombre.endsWith(QLatin1String(".mp4"), Qt::CaseInsensitive);
    if (nombre.isEmpty() || nombre.contains(QLatin1String("..")) || nombre.contains('/') || nombre.contains('\\') || (!mkv && !mp4))
        return chau(respuesta(404, "text/plain; charset=utf-8", "No está ese video"));
    auto* f = new QFile(datos_->carpetaVideos() + '/' + nombre, s);   // se borra con el socket
    if (!QFileInfo(f->fileName()).isFile() || !f->open(QIODevice::ReadOnly))
        return chau(respuesta(404, "text/plain; charset=utf-8", "No está ese video"));
    const qint64 tam = f->size();
    qint64 desde = 0, hasta = tam - 1;
    const bool parcial = !rangoPedido.isEmpty();
    if (parcial && !rango(rangoPedido, tam, desde, hasta))
        return chau(respuesta(416, "text/plain", "", "Content-Range: bytes */" + QByteArray::number(tam) + "\r\n"));
    QByteArray cab = parcial ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";
    cab += "Content-Type: " + QByteArray(mkv ? "video/x-matroska" : "video/mp4") + "\r\nContent-Length: " +
           QByteArray::number(qMax<qint64>(0, hasta - desde + 1)) + "\r\nAccept-Ranges: bytes\r\nCache-Control: no-store\r\nConnection: close\r\n";
    if (parcial) cab += "Content-Range: bytes " + QByteArray::number(desde) + '-' + QByteArray::number(hasta) + '/' + QByteArray::number(tam) + "\r\n";
    if (descargar) cab += adjunto(nombre);
    s->write(cab + "\r\n");
    if (tam == 0) return s->disconnectFromHost();
    f->seek(desde);
    s->setProperty("falta", hasta - desde + 1);
    // De a pedazos: cuando el socket ya mandó casi todo, se lee el siguiente. Así en memoria hay a lo sumo ~2 pedazos.
    const auto seguir = [s, f] {
        qint64 falta = s->property("falta").toLongLong();
        while (falta > 0 && s->bytesToWrite() < kPedazo && s->state() == QAbstractSocket::ConnectedState) {
            const QByteArray b = f->read(qMin(kPedazo, falta));
            if (b.isEmpty()) { falta = 0; s->abort(); return; }   // el archivo se achicó: se corta
            s->write(b);
            falta -= b.size();
        }
        s->setProperty("falta", falta);
        if (falta == 0 && !s->property("cerrando").toBool()) { s->setProperty("cerrando", true); s->disconnectFromHost(); }
    };
    connect(s, &QTcpSocket::bytesWritten, s, seguir);
    seguir();
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
        "<title>Betómetro · Avispón Verde</title>"
        "<link rel=manifest href=/manifest.webmanifest><link rel=icon href=/r/icono.png><link rel=apple-touch-icon href=/r/icono.png>"
        "<meta name=theme-color content=\"#0d2a18\"><meta name=apple-mobile-web-app-title content=\"Betómetro\"><style>"
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
        "<p class=\"v pie\"><a href=\"/drive\">Google Drive</a> · <a href=\"/reportes?descargar=1\">Reporte de errores</a> · Avispón Verde #101</p></main></body></html>")
                             .arg(n)
                             .arg(filas, n ? QStringLiteral("<a class=z href=\"/todos.zip\">Descargar todos (.zip)</a>") : QString());
    return html.toUtf8();
}

// ---------- Google Drive (docs/google-drive.md) ----------
// Página para subir el JSON del cliente de Google desde la compu o el celu. Nunca muestra lo que hay adentro.

QByteArray Servidor::recibirDrive(const QByteArray& cuerpo) {
    if (!drive_) return paginaDrive(QStringLiteral("Google Drive no está en este equipo"));
    // Llega como formulario (multipart) o el archivo solo: el JSON va de la primera «{» a la última «}»
    const int a = cuerpo.indexOf('{'), b = cuerpo.lastIndexOf('}');
    if (a < 0 || b < a) return paginaDrive(QStringLiteral("No llegó ningún archivo. Elegí el JSON y tocá «Subir»."));
    const QString problema = drive_->guardarCliente(cuerpo.mid(a, b - a + 1));
    if (!problema.isEmpty()) return paginaDrive(problema);
    return paginaDrive(QStringLiteral("¡Listo! Ahora en el Betómetro: Ajustes → Google Drive → Conectar."), true);
}

QByteArray Servidor::paginaDrive(const QString& aviso, bool bien) const {
    QString estado;
    if (!drive_) estado = QStringLiteral("No disponible.");
    else if (drive_->estado() == QLatin1String("conectado"))
        estado = QStringLiteral("Conectado") + (drive_->cuenta().isEmpty() ? QString() : QStringLiteral(" como <b>%1</b>").arg(esc(drive_->cuenta()))) +
                 (drive_->texto().isEmpty() ? QString() : QStringLiteral(" · ") + esc(drive_->texto()));
    else if (drive_->tieneCliente()) estado = QStringLiteral("El cliente de Google ya está cargado ✓. Falta tocar «Conectar» en el Betómetro.");
    else estado = QStringLiteral("Falta el cliente de Google (paso 4 de la guía).");
    const QString html = QStringLiteral(
        "<!doctype html><html lang=es><head><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\">"
        "<title>Betómetro · Google Drive</title><style>"
        "@font-face{font-family:Barlow;font-weight:600;src:url(/r/barlow-latin-600-normal.woff2)}"
        "@font-face{font-family:Barlow;font-weight:800;font-style:italic;src:url(/r/barlow-latin-800-italic.woff2)}"
        "*{box-sizing:border-box}body{margin:0;font-family:Barlow,system-ui,sans-serif;font-weight:600;color:#f7f7f2;min-height:100vh;"
        "background:radial-gradient(140% 70% at 50% 0%,#2f7a49,#0b2a17 70%);background-color:#0b2a17}"
        "main{max-width:620px;margin:auto;padding:0 16px 40px}"
        ".cab{display:flex;align-items:center;gap:10px;padding:16px 0 12px;border-bottom:3px solid #f4c531}.cab img{height:50px}"
        ".cab b{font:800 italic 28px/1 Barlow}h2{font:800 italic 13px Barlow;letter-spacing:.18em;color:#b9cdb8;margin:18px 2px 6px}"
        ".c{margin-top:12px;border-radius:18px;padding:16px;background:radial-gradient(120% 90% at 50% 0%,#1f4a2f,#08190e);"
        "box-shadow:0 0 0 2px #cfd4d8,0 0 0 3px #4b5258,0 10px 18px #0008}"
        "ol{padding-left:22px;line-height:1.5}a{color:#f4c531}input[type=file]{width:100%;margin:10px 0;font:600 16px Barlow;color:#f7f7f2}"
        "button{width:100%;border:0;border-radius:14px;padding:14px;font:800 italic 18px Barlow;color:#14240f;"
        "background:linear-gradient(#ffe07a,#f4c531 50%,#d9a20f)}"
        ".aviso{border-radius:14px;padding:12px 14px;margin-top:12px;font-size:17px}.bien{background:#1f7a3a}.mal{background:#8f130b}"
        ".v{color:#b9cdb8;font-size:14px}</style></head><body><main>"
        "<div class=cab><img alt=\"\" src=\"/r/leon.png\"><b>GOOGLE DRIVE</b></div>%1"
        "<h2>ESTADO</h2><div class=c>%2</div>"
        "<h2>SUBIR EL CLIENTE DE GOOGLE</h2><div class=c>"
        "<p>Elegí el archivo <b>.json</b> que bajaste de Google Cloud (cliente «TVs y dispositivos de entrada limitada»).</p>"
        "<form method=post action=/drive enctype=multipart/form-data>"
        "<input type=file name=cliente accept=\".json,application/json\" required><button>Subir</button></form>"
        "<p class=v>Queda guardado solo en el Betómetro. Nunca lo mandes por chat ni lo subas a otro lado.</p></div>"
        "<h2>LOS PASOS</h2><div class=c><ol>"
        "<li>Crear el proyecto: <a href=\"https://console.cloud.google.com/projectcreate\">console.cloud.google.com/projectcreate</a></li>"
        "<li>Activar la API de Drive: <a href=\"https://console.cloud.google.com/apis/library/drive.googleapis.com\">abrir</a> → «Habilitar»</li>"
        "<li>Pantalla de consentimiento: <a href=\"https://console.cloud.google.com/auth/overview\">abrir</a> (nombre Betómetro, tu mail, «Externo») y después «Publicar app»</li>"
        "<li>Cliente: <a href=\"https://console.cloud.google.com/auth/clients/create\">abrir</a> → «TVs y dispositivos de entrada limitada» → bajar el JSON</li>"
        "<li>Subirlo acá arriba</li><li>En el Betómetro: Ajustes → Google Drive → Conectar</li></ol></div>"
        "<p class=v><a href=\"/\">‹ Volver a las carreras</a></p></main></body></html>")
                             .arg(aviso.isEmpty() ? QString() : QStringLiteral("<div class=\"aviso %1\">%2</div>").arg(bien ? "bien" : "mal", esc(aviso)), estado);
    return html.toUtf8();
}

// ---------- El celu como control remoto del modo tele (docs/video.md) ----------
// Igual que el control de la Pi: ir a la vuelta, −10 s / play-pausa / +10 s, la línea de tiempo y
// «Carreras» (cambiar de carrera como capítulos). Pregunta cómo está la tele cada medio segundo.
QByteArray Servidor::paginaControl() const {
    return QByteArray(R"HTML(<!doctype html><html lang=es><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Betómetro · Control de la tele</title><link rel=icon href=/r/icono.png><meta name=theme-color content="#0d2a18"><style>
@font-face{font-family:Barlow;font-weight:600;src:url(/r/barlow-latin-600-normal.woff2)}
@font-face{font-family:Barlow;font-weight:800;font-style:italic;src:url(/r/barlow-latin-800-italic.woff2)}
@font-face{font-family:'Barlow Condensed';font-weight:700;src:url(/r/barlow-condensed-latin-700-normal.woff2)}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html{-webkit-text-size-adjust:100%;text-size-adjust:100%}
body{margin:0;font-family:Barlow,system-ui,sans-serif;font-weight:600;color:#f7f7f2;min-height:100vh;user-select:none;-webkit-user-select:none;overflow-x:hidden;
background:linear-gradient(180deg,rgba(11,42,23,0) 0,#0b2a17 260px),repeating-conic-gradient(#ffffff09 0 25%,transparent 0 50%) 0 0/22px 22px,
radial-gradient(140% 70% at 50% 0%,#2f7a49,#0b2a17 70%);background-color:#0b2a17}
main{max-width:620px;margin:auto;padding:0 16px 30px}
.cab{display:flex;align-items:center;gap:10px;padding:14px 0 12px;border-bottom:3px solid #f4c531}
.cab img{height:44px;filter:drop-shadow(0 2px 2px #0008)}
.cab div{flex:1;min-width:0}.cab small{display:block;font:800 italic 12px Barlow;letter-spacing:.18em;color:#b9cdb8}
.cab b{display:block;font:800 italic 24px/1.1 Barlow;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.f{border:3px solid #cfd4d8;background:#08190ecc;color:#f7f7f2;border-radius:24px;font:800 italic 16px Barlow;letter-spacing:.12em;
text-transform:uppercase;padding:12px 16px;cursor:pointer;transition:transform .08s}
.f:active{transform:scale(.95)}.f:disabled{opacity:.35}.f.am{background:linear-gradient(#ffe07a,#f4c531 50%,#d9a20f);border-color:#f4c531;color:#14240f}
.arriba{display:flex;gap:10px;margin-top:14px}.arriba .f{flex:1}
h2{font:800 italic 13px Barlow;letter-spacing:.18em;color:#b9cdb8;margin:18px 2px 8px}
#vueltas{display:flex;gap:10px;overflow-x:auto;padding:4px 2px 8px;scrollbar-width:none;overscroll-behavior-x:contain}
#vueltas .f{flex:none;min-width:92px;padding:8px 10px;letter-spacing:0;text-transform:none;text-align:center;border-radius:20px}
#vueltas b{display:block;font:800 italic 22px Barlow}#vueltas span{display:block;font:700 20px 'Barlow Condensed';color:#b9cdb8}
#vueltas .am span{color:#14240f}
.bot{display:flex;align-items:center;justify-content:center;gap:min(16px,3vw);margin:26px 0 22px}
.salto{flex:none;width:min(96px,24vw);height:min(80px,20vw);padding:0;border-radius:30px;font:700 26px 'Barlow Condensed';letter-spacing:0}
#play{flex:none;width:min(132px,34vw);height:min(132px,34vw);border-radius:50%;padding:0;border-color:#fff6d0;display:flex;align-items:center;justify-content:center}
#play svg{width:42%;height:42%}
#linea{position:relative;height:52px;touch-action:none;margin:0 8px}
#linea .r{position:absolute;left:0;right:0;top:16px;height:20px;border-radius:10px;background:#ffffff33}
#linea .l{position:absolute;left:0;top:16px;height:20px;border-radius:10px;background:#f4c531}
#linea .m{position:absolute;top:6px;width:4px;height:40px;margin-left:-2px;border-radius:2px;background:#ffffff99}
#linea .p{position:absolute;top:4px;width:44px;height:44px;margin-left:-22px;border-radius:50%;background:#fff;box-shadow:0 2px 6px #0008}
#tiempo{font:700 34px 'Barlow Condensed';margin:6px 2px}
.v{color:#b9cdb8;font-style:italic;font-size:14px}
.apagada{margin-top:40px;text-align:center}.apagada img{height:120px;opacity:.9}
.apagada p{font:800 italic 22px/1.3 Barlow;margin:18px 6px}
#capa{position:fixed;inset:0;background:#000a;display:flex;align-items:flex-end;justify-content:center;z-index:5}
#capa[hidden],[hidden]{display:none!important}
.hoja{width:100%;max-width:620px;max-height:85vh;overflow:auto;background:radial-gradient(120% 90% at 50% 0%,#1f4a2f,#08190e);
border-radius:24px 24px 0 0;box-shadow:0 0 0 2px #cfd4d8;padding:16px 16px 24px}
.hoja .t{display:flex;align-items:center;justify-content:space-between}.hoja .t b{font:800 italic 24px Barlow}
.c{display:flex;align-items:center;gap:12px;width:100%;margin-top:12px;padding:12px 14px;border-radius:18px;text-align:left;
background:#08190ecc;border:2px solid #cfd4d8;color:#f7f7f2;font-family:Barlow;cursor:pointer}
.c.yo{border:3px solid #f4c531;background:#f4c53122}
.c .dia{flex:none;width:44px;height:44px;border-radius:50%;display:flex;align-items:center;justify-content:center;
background:linear-gradient(#ffe07a,#f4c531 50%,#d9a20f);color:#14240f;font:800 italic 18px Barlow}
.c .x{flex:1;min-width:0}.c .x b{display:block;font:800 italic 20px/1.15 Barlow;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.c .x span{display:block;color:#b9cdb8;font-size:13px;font-style:italic;margin-top:2px}
.c .ya{font:800 italic 12px Barlow;letter-spacing:.12em;color:#f4c531}
#aviso{position:fixed;left:16px;right:16px;bottom:20px;background:#8f130b;border-radius:14px;padding:12px 14px;font-size:16px;z-index:9}
</style></head><body><main>
<div class=cab><img alt="" src="/r/leon.png"><div><small id=arr>EN LA TELE</small><b id=nombre>Betómetro</b></div></div>
<div id=sinred class="v" hidden style="margin-top:10px">Sin conexión con el Betómetro. ¿Estás en la misma wifi?</div>
<section id=apagada class=apagada hidden><img alt="" src="/r/leon.png">
<p>La tele no está en uso: en la Pi, abrí un informe con video y tocá «Ver en la tele»</p>
<p class=v>Esta página se pone sola cuando la tele empieza.</p></section>
<section id=control hidden>
<div class=arriba><button class=f id=bCarreras>☰ Carreras</button><button class=f id=bSalir>Salir</button></div>
<h2>IR A LA VUELTA</h2><div id=vueltas></div>
<div class=bot><button class="f salto" id=bMenos>−10 s</button><button class="f am" id=play></button><button class="f salto" id=bMas>+10 s</button></div>
<div id=linea><div class=r></div><div class=l></div><div id=marcas></div><div class=p></div></div>
<div id=tiempo>0:00 / 0:00</div>
<p class=v>Escaneaste el código de la Pi: este celu también es control. Lo que toques acá pasa en la tele.</p>
</section></main>
<div id=capa hidden><div class=hoja><div class=t><b>Carreras</b><button class="f am" id=bListo>Listo</button></div>
<p class=v>Elegí una carrera: la tele la muestra desde la primera vuelta.</p><div id=lista></div></div></div>
<div id=aviso hidden></div>
<script>
const $=i=>document.getElementById(i);
let E={activa:false,carreras:[]},arr=-1,claveV='',claveL='',tAviso=0,tocaV=0,R=[];
const esc=s=>String(s==null?'':s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
function reloj(ms){const s=Math.max(0,Math.floor(ms/1000)),h=Math.floor(s/3600),m=Math.floor(s/60)%60,ss=String(s%60).padStart(2,'0');
return h>0?h+':'+String(m).padStart(2,'0')+':'+ss:m+':'+ss}
const PLAY='<svg viewBox="0 0 84 104"><path d="M8 0L84 52L8 104Z" fill="#111"/></svg>';
const PAUSA='<svg viewBox="0 0 100 104"><rect x="8" width="32" height="104" rx="7" fill="#111"/><rect x="60" width="32" height="104" rx="7" fill="#111"/></svg>';
function aviso(t){$('aviso').textContent=t;$('aviso').hidden=false;clearTimeout(tAviso);tAviso=setTimeout(()=>$('aviso').hidden=true,2500)}
async function estado(){
  try{const r=await fetch('/control/estado',{cache:'no-store'});E=await r.json();$('sinred').hidden=true}
  catch(e){$('sinred').hidden=false;return}
  pintar();
}
async function hacer(tipo,valor){
  try{const r=await fetch('/control/accion?tipo='+tipo+(valor===undefined?'':'&valor='+encodeURIComponent(valor)),{method:'POST'});
    const j=await r.json();if(!j.ok)aviso(j.error||'No se pudo')}
  catch(e){aviso('Sin conexión con el Betómetro')}
  estado();
}
function pintar(){
  $('apagada').hidden=E.activa;$('control').hidden=!E.activa;
  $('arr').textContent=E.activa?'EN LA TELE':'CONTROL DE LA TELE';
  $('nombre').textContent=E.activa?E.carrera:'Betómetro';
  if(!E.activa){$('capa').hidden=true;return}
  const vs=E.vueltas||[],k=E.carreraId+'|'+vs.map(v=>v.nombre+v.tiempo+v.video).join(',')+'|'+E.vuelta;
  if(k!==claveV){claveV=k;
    $('vueltas').innerHTML=vs.map((v,i)=>'<button class="f'+(i===E.vuelta?' am':'')+'" data-i="'+i+'"'+(v.video?'':' disabled')+'><b>'+esc(v.nombre)+'</b><span>'+esc(v.tiempo)+'</span></button>').join('');
    centrarV();
  }
  $('play').innerHTML=E.andando?PAUSA:PLAY;
  const t=E.total||1,ms=arr>=0?arr:E.pos,f=Math.max(0,Math.min(1,ms/t));
  document.querySelector('#linea .l').style.width=(f*100)+'%';
  document.querySelector('#linea .p').style.left=(f*100)+'%';
  const km=(E.marcas||[]).join(',')+'|'+t+'|'+L.clientWidth;
  if(km!==claveL){claveL=km;R=rayitas();$('marcas').innerHTML=R.filter(m=>m>0&&m<=t).map(m=>'<div class=m style="left:'+(m/t*100)+'%"></div>').join('')}
  $('tiempo').textContent=reloj(ms)+' / '+reloj(t);
  if(!$('capa').hidden)lista();
}
function lista(){
  $('lista').innerHTML=(E.carreras||[]).map(c=>'<button class="c'+(c.id===E.carreraId?' yo':'')+'" data-id="'+esc(c.id)+'"><span class=dia>'+esc(c.dia)+'</span>'+
    '<span class=x><b>'+esc(c.nombre)+'</b><span>'+esc(c.fecha)+' · '+c.vueltas+' vueltas'+(c.videos>1?' · '+c.videos+' videos':'')+'</span></span>'+
    (c.id===E.carreraId?'<span class=ya>EN LA TELE</span>':'')+'</button>').join('')||'<p class=v>No hay carreras con video.</p>';
}
// La vuelta de ahora a la vista (solo la fila, no la página); si recién la deslizaron, no
function centrarV(){const V=$('vueltas'),a=V.querySelector('.am');if(!a||Date.now()-tocaV<4000)return;
  V.scrollLeft=a.offsetLeft-V.offsetLeft-(V.clientWidth-a.offsetWidth)/2}
['touchstart','wheel','pointerdown'].forEach(n=>$('vueltas').addEventListener(n,()=>tocaV=Date.now(),{passive:true}));
$('vueltas').onclick=e=>{const b=e.target.closest('button');if(b&&!b.disabled)hacer('vuelta',b.dataset.i)};
$('bMenos').onclick=()=>hacer('saltar',-10000);$('bMas').onclick=()=>hacer('saltar',10000);
$('play').onclick=()=>hacer('alternar');$('bSalir').onclick=()=>hacer('salir');
$('bCarreras').onclick=()=>{lista();$('capa').hidden=false};$('bListo').onclick=()=>$('capa').hidden=true;
$('capa').onclick=e=>{if(e.target.id==='capa')$('capa').hidden=true};
$('lista').onclick=e=>{const b=e.target.closest('button');if(!b)return;$('capa').hidden=true;if(b.dataset.id!==E.carreraId)hacer('carrera',b.dataset.id)};
// Línea de tiempo: se arrastra; cerca de una rayita salta justo al empezar esa vuelta
const L=$('linea');
// Con muchas vueltas, una rayita cada 2, 5 o 10 vueltas, así no se pegan (y se puede arrastrar entre ellas)
function rayitas(){const ms=E.marcas||[],t=E.total||1,n=ms.length;if(n<2)return ms.slice();
  const sep=L.clientWidth*(ms[n-1]-ms[0])/(n-1)/t,paso=[1,2,5,10,20].find(p=>sep*p>=36)||Math.ceil(36/Math.max(sep,.01));
  return ms.filter((m,i)=>i%paso===0)}
function msEn(x){const r=L.getBoundingClientRect(),t=E.total||1;let ms=Math.max(0,Math.min(1,(x-r.left)/r.width))*t,c=null,d=14;
  for(const m of R){const dx=Math.abs((m-ms)/t*r.width);if(dx<d){d=dx;c=m}}return c===null?ms:Math.max(0,c)}
L.onpointerdown=e=>{L.setPointerCapture(e.pointerId);arr=msEn(e.clientX);pintar()};
L.onpointermove=e=>{if(arr>=0){arr=msEn(e.clientX);pintar()}};
L.onpointerup=()=>{if(arr<0)return;const ms=Math.round(arr);arr=-1;hacer('ir',ms)};
L.onpointercancel=()=>{arr=-1;pintar()};
estado();setInterval(estado,500);
</script></body></html>)HTML");
}
