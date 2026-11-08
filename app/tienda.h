// Tienda (Matías, 08/11/2026): el catálogo de herramientas está en el repo público Betometro-tienda (tienda.json).
// Al abrir la Tienda se baja por internet; sin internet se muestra lo último que se bajó (<datos>/tienda.json).
// Por ahora solo se muestran: instalar y abrir viene después (cada herramienta se abre aparte de Betómetro).
#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>

class Tienda : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList herramientas READ herramientas NOTIFY cambio)   // [{id, nombre, descripcion, version, icono}]
    Q_PROPERTY(QString estado READ estado NOTIFY cambio)                     // "", buscando, listo, sinred
public:
    explicit Tienda(const QString& carpetaDatos, QObject* padre = nullptr);
    QVariantList herramientas() const { return herramientas_; }
    QString estado() const { return estado_; }
    Q_INVOKABLE void buscar();
    // Lee un catálogo (público para probarlo): false si no es del formato 1
    bool leer(const QByteArray& json);

signals:
    void cambio();

private:
    QString archivo_;
    QNetworkAccessManager red_;
    QVariantList herramientas_;
    QString estado_;
    bool enVuelo_ = false;
};
