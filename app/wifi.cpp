#include "wifi.h"

Wifi::Wifi(QObject* padre) : QObject(padre) {
    // Simulado: lo que se vería en casa
    redes_ = {{"Casa", 3, true}, {"Vecino_2.4G", 4, true}, {"Fibertel WiFi 552", 2, true}, {"Invitados", 1, false}};
    guardadas_ = {"Casa", "Celu de Matías"};
    conectada_ = "Casa";
}

int Wifi::senal() const {
    for (const auto& r : redes_) if (r.nombre == conectada_) return r.senal;
    return 0;
}

QVariantList Wifi::cerca() const {
    QVariantList l;
    for (const auto& r : redes_)
        if (!guardadas_.contains(r.nombre)) l << QVariantMap{{"nombre", r.nombre}, {"senal", r.senal}, {"clave", r.clave}};
    return l;
}

QVariantList Wifi::guardadas() const {
    QVariantList l;
    for (const auto& g : guardadas_) {
        int s = 0;
        for (const auto& r : redes_) if (r.nombre == g) s = r.senal;
        l << QVariantMap{{"nombre", g}, {"senal", s}, {"cerca", s > 0}, {"conectada", g == conectada_}};
    }
    return l;
}

bool Wifi::tieneClave(const QString& red) const {
    for (const auto& r : redes_) if (r.nombre == red) return r.clave;
    return true;
}

void Wifi::buscar() {
    buscando_ = true;
    emit cambio();
    QTimer::singleShot(1200, this, [this] { buscando_ = false; emit cambio(); });
}

void Wifi::conectar(const QString& red, const QString& clave) {
    error_.clear();
    if (tieneClave(red) && clave.size() < 8) {
        error_ = QStringLiteral("La clave tiene que tener al menos 8 letras o números");
        emit cambio();
        return;
    }
    conectando_ = true;
    emit cambio();
    QTimer::singleShot(1500, this, [this, red] {
        conectando_ = false;
        conectada_ = red;
        if (!guardadas_.contains(red)) guardadas_.prepend(red);
        emit cambio();
        emit conectado(red);
    });
}

void Wifi::olvidar(const QString& red) {
    guardadas_.removeAll(red);
    if (conectada_ == red) conectada_.clear();
    emit cambio();
}
