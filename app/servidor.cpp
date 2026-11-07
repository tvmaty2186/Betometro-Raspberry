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
#include <QTimer>
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
// Las páginas del celu viven en recursos/web/ (index, drive, control), cargadas de los recursos del programa (01-1.3)
QString pagina_web(const char* nombre) {
    QFile f(QStringLiteral(":/recursos/web/") + QLatin1String(nombre) + QStringLiteral(".html"));
    if (!f.open(QIODevice::ReadOnly)) { qWarning("Betómetro en la red: falta la página %s", nombre); return {}; }
    return QString::fromUtf8(f.readAll());
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
            // Un pedido a medias no queda abierto para siempre: a los 30 s se corta (02-3). Los videos que se
            // están mandando tienen su propio ritmo: solo se corta si todavía no se atendió.
            QTimer::singleShot(30000, s, [s] { if (!s->property("atendido").toBool()) s->abort(); });
            connect(s, &QTcpSocket::readyRead, s, [this, s] {
                if (s->property("atendido").toBool()) { s->readAll(); return; }   // ya se contestó: no se junta de más
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
                s->write(linea.size() >= 2 && (linea[0] == "GET" || postControl) ? responder(linea[1], postControl) : respuesta(400, "text/plain", "Solo se puede mirar"));
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

QByteArray Servidor::responder(const QByteArray& rutaCruda, bool post) const {
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
        // Solo por POST: con GET, una página cualquiera abierta en el celu podía meter un <img src=…/control/accion?tipo=salir> (11A-2.4)
        if (!post) return respuesta(405, "text/plain; charset=utf-8", "Solo por POST");
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
        QByteArray t = reportes_ ? reportes_->texto() : QByteArray();
        if (t.isEmpty()) t = "Sin errores anotados.\n";
        if (reportes_ && !reportes_->problema().isEmpty()) t.prepend("OJO: " + reportes_->problema().toUtf8() + "\n\n");   // la llave vencida (11A-1.3)
        return respuesta(200, "text/plain; charset=utf-8", t,
                         descargar ? adjunto(QStringLiteral("betometro-errores.txt")) : QByteArray());
    }
    // Toda la base en un archivo, igual al de la app ({schemaVersion, config, carreras}): la copia para el celu
    if (ruta == QLatin1String("/betometro.json"))
        return respuesta(200, "application/json; charset=utf-8", datos_->exportar(), descargar ? adjunto(QStringLiteral("betometro.json")) : QByteArray());
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
    // La página está en recursos/web/index.html (01-1.3): %1 es la cantidad, %2 las filas y %3 el botón del zip
    const QString html = pagina_web("index").arg(n)
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
        // Sin el mail de la cuenta: lo ve cualquiera de la wifi (la cuenta se ve en la pantalla de la Pi) (11A-2.5)
        estado = QStringLiteral("Conectado") + (drive_->texto().isEmpty() ? QString() : QStringLiteral(" · ") + esc(drive_->texto()));
    else if (drive_->tieneCliente()) estado = QStringLiteral("El cliente de Google ya está cargado ✓. Falta tocar «Conectar» en el Betómetro.");
    else estado = QStringLiteral("Falta el cliente de Google (paso 4 de la guía).");
    // La página está en recursos/web/drive.html: %1 el aviso, %2 el estado
    const QString html = pagina_web("drive")
                             .arg(aviso.isEmpty() ? QString() : QStringLiteral("<div class=\"aviso %1\">%2</div>").arg(bien ? "bien" : "mal", esc(aviso)), estado);
    return html.toUtf8();
}

// ---------- El celu como control remoto del modo tele (docs/video.md) ----------
// Igual que el control de la Pi: ir a la vuelta, −10 s / play-pausa / +10 s, la línea de tiempo y
// «Carreras» (cambiar de carrera como capítulos). Pregunta cómo está la tele cada medio segundo.
QByteArray Servidor::paginaControl() const {
    return pagina_web("control").toUtf8();
}
