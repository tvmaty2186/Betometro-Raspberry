#include "drive_logica.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace drive {

QString nombreCarpeta(const QString& fechaIso, const QString& nombre) {
    QString n;
    for (QChar c : nombre.simplified()) n += (c == '/' || c == '\\' || c.category() == QChar::Other_Control) ? QChar('-') : c;
    n = n.trimmed();
    if (n.isEmpty()) n = QStringLiteral("Carrera");
    const QString f = fechaIso.left(10);
    const bool fechaBien = f.size() == 10 && f[4] == '-' && f[7] == '-';
    return fechaBien ? f + ' ' + n : n;
}

bool siguienteByte(const QByteArray& range, qint64& siguiente) {
    const QByteArray r = range.trimmed();
    if (r.isEmpty()) { siguiente = 0; return true; }
    if (!r.toLower().startsWith("bytes=")) return false;
    const QByteArray t = r.mid(6);
    const int guion = t.indexOf('-');
    if (guion <= 0) return false;
    bool okA = false, okB = false;
    const qint64 a = t.left(guion).trimmed().toLongLong(&okA), b = t.mid(guion + 1).trimmed().toLongLong(&okB);
    if (!okA || !okB || a != 0 || b < a) return false;
    siguiente = b + 1;
    return true;
}

bool wifiParaVideos(const QString& conectada, bool propia, const QStringList& elegidas) {
    return !propia && !conectada.isEmpty() && elegidas.contains(conectada);
}

QString revisarCliente(const QByteArray& json, QJsonObject* limpio) {
    QJsonParseError e{};
    const QJsonDocument d = QJsonDocument::fromJson(json, &e);
    if (e.error != QJsonParseError::NoError || !d.isObject()) return QStringLiteral("Ese archivo no es el JSON de Google");
    QJsonObject o = d.object();
    if (o.value("installed").isObject()) o = o.value("installed").toObject();
    else if (o.value("web").isObject()) o = o.value("web").toObject();
    const QString id = o.value("client_id").toString().trimmed(), secreto = o.value("client_secret").toString().trimmed();
    if (id.isEmpty() || secreto.isEmpty()) return QStringLiteral("Al archivo le falta client_id o client_secret");
    if (!id.endsWith(QLatin1String(".apps.googleusercontent.com"))) return QStringLiteral("El client_id no es de Google");
    if (limpio) *limpio = QJsonObject{{"client_id", id}, {"client_secret", secreto}};
    return {};
}

bool leerRespuesta(const QByteArray& crudo, int& codigo, QByteArray& range, QByteArray& cuerpo) {
    QByteArray resto = crudo;
    for (;;) {
        if (!resto.startsWith("HTTP/")) return false;
        const int fin = resto.indexOf("\r\n\r\n");
        if (fin < 0) return false;
        const QList<QByteArray> lineas = resto.left(fin).split('\n');
        const QList<QByteArray> estado = lineas.value(0).trimmed().split(' ');
        bool ok = false;
        codigo = estado.value(1).toInt(&ok);
        if (!ok) return false;
        resto = resto.mid(fin + 4);
        if (codigo == 100) continue;   // «100 Continue»: viene otra respuesta atrás
        range.clear();
        for (int i = 1; i < lineas.size(); i++) {
            const QByteArray l = lineas[i].trimmed();
            if (l.toLower().startsWith("range:")) range = l.mid(6).trimmed();
        }
        cuerpo = resto;
        return true;
    }
}

QString textoBytes(qint64 b) {
    if (b >= 1000LL * 1048576) return QString::number(b / 1073741824.0, 'f', 1).replace('.', ',') + QStringLiteral(" GB");
    return QString::number(qMax<qint64>(1, qRound64(b / 1048576.0))) + QStringLiteral(" MB");
}

QString errorEnPalabras(int http, const QByteArray& cuerpo) {
    const QString t = QString::fromUtf8(cuerpo);
    if (t.contains(QLatin1String("storageQuotaExceeded")) || t.contains(QLatin1String("quotaExceeded")))
        return QStringLiteral("Drive lleno");
    if (t.contains(QLatin1String("invalid_grant")) || http == 401) return QStringLiteral("Se venció el permiso: tocá Conectar");
    if (t.contains(QLatin1String("invalid_client")) || t.contains(QLatin1String("unauthorized_client")))
        return QStringLiteral("El cliente de Google no sirve: volvé a subir el JSON");
    if (t.contains(QLatin1String("accessNotConfigured")) || t.contains(QLatin1String("SERVICE_DISABLED")))
        return QStringLiteral("Falta activar la API de Drive en Google Cloud");
    if (http == 403 && (t.contains(QLatin1String("rateLimit")) || t.contains(QLatin1String("userRateLimit"))))
        return QStringLiteral("Google pidió esperar un rato");
    if (http == 429) return QStringLiteral("Google pidió esperar un rato");
    if (http >= 500) return QStringLiteral("Google no responde: se prueba más tarde");
    if (http == 0) return QStringLiteral("Sin internet");
    return QStringLiteral("Error de Google (%1)").arg(http);
}

Pendientes pendientes(const QJsonObject& estado, const QList<CarreraLocal>& carreras, const Opciones& op, const QJsonObject& estadoBase) {
    Pendientes p;
    QList<Tarea> videos;
    for (const CarreraLocal& c : carreras) {
        const QJsonObject e = estado.value(c.id).toObject();
        if (op.informes && !c.versionInforme.isEmpty()) {
            const QJsonObject inf = e.value("informe").toObject();
            if (inf.value("version").toString() != c.versionInforme || inf.value("id").toString().isEmpty())
                p.tareas << Tarea{Tarea::Informe, c.id, c.nombre, c.carpeta, {}, 0, inf.value("id").toString(), {}};
        }
        if (!op.videos) continue;
        const QJsonObject subidos = e.value("videos").toObject();
        for (const VideoLocal& v : c.videos) {
            if (!subidos.value(v.archivo).toObject().value("id").toString().isEmpty()) continue;
            if (!op.videosPermitidos) { p.videosEsperando++; p.bytesEsperando += v.bytes; continue; }
            videos << Tarea{Tarea::Video, c.id, c.nombre, c.carpeta, v.archivo, v.bytes, {}, {}};
        }
    }
    // La copia de la base va con los informes (chica), antes que los videos, solo cuando cambió
    if (op.informes && !op.baseVersion.isEmpty() &&
        (estadoBase.value("version").toString() != op.baseVersion || estadoBase.value("id").toString().isEmpty()))
        p.tareas << Tarea{Tarea::Base, {}, QStringLiteral("Betómetro"), {}, QStringLiteral("betometro.json"), 0,
                          estadoBase.value("id").toString(), op.baseVersion};
    p.tareas += videos;
    return p;
}

}  // namespace drive
