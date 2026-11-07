// La configuración (cfg = fábrica + lo guardado): leerla desde QML y cambiarla.
#include "datos.h"
#include "datos_comun.h"

#if __has_include(<QJSValue>)   // en el programa (las pruebas no tienen QML)
#include <QJSValue>
#define BETO_CON_QJS 1
#endif

#include <QJsonDocument>

using namespace beto;

static QVariant aVariant(const Json& j) {
    switch (j.tipo()) {
        case Json::Tipo::Bool: return j.aBool();
        case Json::Tipo::Numero: return j.aNumero();
        case Json::Tipo::Texto: return q(j.aTexto());
        case Json::Tipo::Lista:
        case Json::Tipo::Objeto: return QJsonDocument::fromJson(QByteArray::fromStdString(j.escribir())).toVariant();
        default: return {};
    }
}

QVariant Datos::cfg(const QString& clave) const { return aVariant(cfg_[s(clave)]); }

QVariantMap Datos::ajustes() const {
    QVariantMap m;
    for (const auto& p : cfg_.pares()) m[q(p.first)] = aVariant(p.second);
    return m;
}

void Datos::ponerCfg(const QString& clave, const QVariant& valorQml) {
    // Un objeto de JavaScript puede llegar como QJSValue: se pasa a mapa/lista (si no, se guardaba como
    // el texto «[object Object]» y no andaban las llaves de «Datos sobre el video»)
#ifdef BETO_CON_QJS
    const QVariant valor = valorQml.typeId() == qMetaTypeId<QJSValue>() ? valorQml.value<QJSValue>().toVariant() : valorQml;
#else
    const QVariant& valor = valorQml;
#endif
    Json v;
    switch (valor.typeId()) {
        case QMetaType::Bool: v = valor.toBool(); break;
        case QMetaType::Int: case QMetaType::LongLong: case QMetaType::Double: v = valor.toDouble(); break;
        case QMetaType::QVariantList: case QMetaType::QVariantMap: case QMetaType::QJsonArray: case QMetaType::QJsonValue:
            v = Json::leer(QJsonDocument::fromVariant(valor).toJson(QJsonDocument::Compact).toStdString());
            break;
        default:
            if (valor.canConvert<QVariantList>() && !valor.canConvert<QString>()) {
                v = Json::leer(QJsonDocument::fromVariant(valor.toList()).toJson(QJsonDocument::Compact).toStdString());
            } else {
                v = s(valor.toString());
            }
    }
    base_.config[s(clave)] = v;
    cfg_ = armarConfig(base_.config);
    guardarConfigLuego();
    emit cfgChanged();
}
