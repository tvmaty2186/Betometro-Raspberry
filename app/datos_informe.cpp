// El informe de una carrera: datos para dibujar, HTML para compartir y los cambios que se hacen desde él.
#include "marca_gps.h"
#include "datos.h"
#include "datos_comun.h"
#include "informe.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>

using namespace beto;

QVariantMap Datos::informe(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c) return {};
    const std::string j = datosInforme(*c, cfg_["umbralPrecisionM"].aNumero(20)).escribir();
    return QJsonDocument::fromJson(QByteArray::fromStdString(j)).toVariant().toMap();
}

QVariantList Datos::marcasGps(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c) return {};
    QVariantList r;
    for (const MarcaGps& m : beto::marcasGps(*c))
        r << QVariantMap{{"hay", m.hay}, {"dudosa", m.dudosa}, {"realMs", static_cast<qlonglong>(m.realMs)},
                         {"difMs", static_cast<qlonglong>(m.difMs)}, {"pulsadorMenosGpsMs", static_cast<qlonglong>(m.pulsadorMenosGpsMs)}};
    return r;
}

QString Datos::nombreInforme(const QString& id) const {
    const Carrera* c = buscar(id);
    return c ? q(beto::nombreInforme(*c)) : QString();
}

QByteArray Datos::informeHtml(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c) return {};
    QFile p(QStringLiteral(":/recursos/informe-plantilla.html"));
    if (!p.open(QIODevice::ReadOnly)) return {};
    return QByteArray::fromStdString(beto::informeHtml(*c, cfg_["umbralPrecisionM"].aNumero(20), p.readAll().toStdString()));
}

QString Datos::guardarInformeHtml(const QString& id) const {
    const Carrera* c = buscar(id);
    if (!c) return {};
    const QByteArray html = informeHtml(id);
    if (html.isEmpty()) return {};
    QDir().mkpath(carpeta_ + "/informes");
    const QString ruta = carpeta_ + "/informes/" + q(beto::nombreInforme(*c));
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    f.write(html);
    return ruta;
}

bool Datos::cambiarMedida(const QString& id, int indice) {
    Carrera* c = base_.buscar(s(id));
    if (!c || indice < 0 || indice >= static_cast<int>(c->vueltas.size())) return false;
    Vuelta& v = c->vueltas[static_cast<size_t>(indice)];
    if (v.interrumpida) return false;
    c->guardarOriginal();
    v.medida = !v.medida;
    guardar(id);
    emit carrerasChanged();
    return true;
}

bool Datos::unirConSiguiente(const QString& id, int indice) {
    Carrera* c = base_.buscar(s(id));
    if (!c || indice < 0) return false;
    Carrera antes = *c;
    if (!c->unirConSiguiente(static_cast<size_t>(indice))) return false;
    if (!antes.modificada()) { antes.guardarOriginal(); c->extra = antes.extra; }
    guardar(id);
    emit carrerasChanged();
    return true;
}

bool Datos::restablecer(const QString& id) {
    Carrera* c = base_.buscar(s(id));
    if (!c || !c->restablecer()) return false;
    guardar(id);
    emit carrerasChanged();
    return true;
}

bool Datos::modificada(const QString& id) const {
    for (const auto& c : base_.carreras)
        if (q(c.id) == id) return c.modificada();
    return false;
}
