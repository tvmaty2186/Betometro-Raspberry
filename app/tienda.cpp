#include "tienda.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSaveFile>
#include <QVersionNumber>

namespace {
const char* kCatalogo = "https://raw.githubusercontent.com/tvmaty2186/Betometro-tienda/main/tienda.json";
const char* kBase = "https://raw.githubusercontent.com/tvmaty2186/Betometro-tienda/main/";
}

Tienda::Tienda(const QString& carpetaDatos, QObject* padre) : QObject(padre), archivo_(carpetaDatos + QStringLiteral("/tienda.json")) {
    QFile f(archivo_);
    if (f.open(QIODevice::ReadOnly)) leer(f.readAll());   // lo último que se bajó
}

bool Tienda::leer(const QByteArray& json) {
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    if (o.value(QStringLiteral("formato")).toInt() != 1) return false;
    const QRegularExpression idValido(QStringLiteral("^[a-z0-9-]{1,32}$"));
    QVariantList l;
    for (const auto& v : o.value(QStringLiteral("herramientas")).toArray()) {
        const QJsonObject h = v.toObject();
        const QString id = h.value(QStringLiteral("id")).toString();
        if (!idValido.match(id).hasMatch()) continue;
        // Solo las que esta versión de Betómetro puede abrir
        const QString minimo = h.value(QStringLiteral("betometroMinimo")).toString();
        if (!minimo.isEmpty() && QVersionNumber::fromString(QStringLiteral(BETO_VERSION)) < QVersionNumber::fromString(minimo)) continue;
        const QString icono = h.value(QStringLiteral("icono")).toString();
        l.append(QVariantMap{{"id", id},
                             {"nombre", h.value(QStringLiteral("nombre")).toString().left(30)},
                             {"descripcion", h.value(QStringLiteral("descripcion")).toString().left(80)},
                             {"version", h.value(QStringLiteral("version")).toString()},
                             {"icono", icono.isEmpty() || icono.contains(QLatin1String("..")) ? QString() : QLatin1String(kBase) + icono}});
    }
    herramientas_ = l;
    emit cambio();
    return true;
}

void Tienda::buscar() {
    if (enVuelo_ || qEnvironmentVariableIsSet("BETO_FIJO")) return;   // capturas: sin internet
    enVuelo_ = true;
    estado_ = QStringLiteral("buscando");
    emit cambio();
    QNetworkRequest q{QUrl(QString::fromLatin1(kCatalogo))};
    q.setRawHeader("User-Agent", "Betometro/" BETO_VERSION);
    q.setTransferTimeout(15000);
    QNetworkReply* r = red_.get(q);
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        enVuelo_ = false;
        const QByteArray d = r->readAll();
        if (r->error() != QNetworkReply::NoError || d.size() > 256 * 1024 || !leer(d)) {
            estado_ = QStringLiteral("sinred");
            emit cambio();
            return;
        }
        QSaveFile f(archivo_);   // para verla sin internet
        if (f.open(QIODevice::WriteOnly)) { f.write(d); f.commit(); }
        estado_ = QStringLiteral("listo");
        emit cambio();
    });
}
