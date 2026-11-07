// Los videos de la cámara: los de cada carrera («videos» en extra) y los archivos de la carpeta videos/.
#include "datos.h"
#include "datos_comun.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

#include <algorithm>
#include <map>

using namespace beto;

bool Datos::agregarVideo(const QString& id, const QVariantMap& v) {
    Carrera* c = base_.buscar(s(id));
    if (!c) return false;
    Json j = Json::objeto();
    j["archivo"] = s(v.value("archivo").toString());
    j["largada"] = static_cast<int64_t>(v.value("largada").toInt());
    j["desdeLargadaMs"] = static_cast<int64_t>(v.value("desdeLargadaMs").toLongLong());
    j["girar"] = v.value("girar").toBool();
    j["espejo"] = v.value("espejo").toBool();
    if (!c->extra["videos"].esLista()) c->extra["videos"] = Json::lista();
    c->extra["videos"].agregar(std::move(j));
    guardar(id);
    emit carrerasChanged();
    return true;
}

QVariantList Datos::videos(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c || !c->extra["videos"].esLista()) return {};
    return QJsonDocument::fromJson(QByteArray::fromStdString(c->extra["videos"].escribir())).toVariant().toList();
}

bool Datos::nombreVideoValido(const QString& a) {
    if (a.isEmpty() || a.startsWith('.') || a.contains('/') || a.contains('\\') || a.contains(QLatin1String(".."))) return false;
    return a.endsWith(QLatin1String(".mkv"), Qt::CaseInsensitive) || a.endsWith(QLatin1String(".mp4"), Qt::CaseInsensitive);
}

static QString tamTexto(qint64 b) {
    if (b >= 1000LL * 1048576) return QString::number(b / 1073741824.0, 'f', 1).replace('.', ',') + QStringLiteral(" GB");
    return QString::number(std::max<qint64>(1, qRound64(b / 1048576.0))) + QStringLiteral(" MB");
}

QVariantList Datos::videosGuardados() const {
    // De qué carrera es cada archivo (lo dice «videos» de cada carrera)
    std::map<QString, const Carrera*> deCarrera;
    for (const auto& c : base_.carreras)
        if (c.extra["videos"].esLista())
            for (const auto& v : c.extra["videos"].elementos()) deCarrera[q(v["archivo"].aTexto())] = &c;
    QVariantList r;
    const QFileInfoList l = QDir(carpetaVideos()).entryInfoList({QStringLiteral("*.mkv"), QStringLiteral("*.mp4")}, QDir::Files, QDir::Time);
    for (const QFileInfo& f : l) {
        QVariantMap m{{"archivo", f.fileName()}, {"bytes", f.size()}, {"tam", tamTexto(f.size())},
                      {"cuando", f.lastModified().toMSecsSinceEpoch()},
                      {"hora", f.lastModified().toString(QStringLiteral("dd/MM/yy HH:mm"))},
                      {"prueba", f.fileName().startsWith(QLatin1String("prueba-"))}};
        auto it = deCarrera.find(f.fileName());
        if (it != deCarrera.end()) {
            const QDate fc = QDate::fromString(q(it->second->fecha), Qt::ISODate);
            m["carreraId"] = q(it->second->id);
            m["carrera"] = q(it->second->nombre);
            m["fecha"] = fc.isValid() ? fc.toString(QStringLiteral("dd/MM/yy")) : QString();
        }
        r << m;
    }
    return r;
}

bool Datos::olvidarVideo(const QString& archivo) {
    bool cambio = false;
    for (auto& c : base_.carreras) {
        if (!c.extra["videos"].esLista()) continue;
        auto& l = c.extra["videos"].elementos();
        const auto n = l.size();
        l.erase(std::remove_if(l.begin(), l.end(), [&](const Json& v) { return q(v["archivo"].aTexto()) == archivo; }), l.end());
        if (l.size() == n) continue;
        if (l.empty()) c.extra.sacar("videos");   // sin videos: el informe no muestra «Video»
        guardar(q(c.id));
        cambio = true;
    }
    if (cambio) emit carrerasChanged();
    return cambio;
}

bool Datos::renombrarVideo(const QString& viejo, const QString& nuevo) {
    bool cambio = false;
    for (auto& c : base_.carreras) {
        if (!c.extra["videos"].esLista()) continue;
        bool esta = false;
        for (auto& v : c.extra["videos"].elementos())
            if (q(v["archivo"].aTexto()) == viejo) { v["archivo"] = s(nuevo); esta = true; }
        if (esta) { guardar(q(c.id)); cambio = true; }
    }
    if (cambio) emit carrerasChanged();
    return cambio;
}

bool Datos::borrarVideo(const QString& archivo) {
    if (!nombreVideoValido(archivo)) return false;
    const QString ruta = carpetaVideos() + '/' + archivo;
    const bool habia = QFileInfo::exists(ruta);
    if (habia && !QFile::remove(ruta)) return false;
    const bool enCarrera = olvidarVideo(archivo);
    return habia || enCarrera;
}
