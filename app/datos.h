// Puente entre la base (núcleo: modelo + almacén) y la pantalla (QML).
// Las reglas son las de la app: docs/app/lo-aprendido.md, secciones 5.2 a 5.5.
#pragma once

#include <QDate>
#include <QObject>
#include <QVariant>

#include "almacen.h"
#include "modelo.h"

class Datos : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList proximas READ proximas NOTIFY carrerasChanged)
    Q_PROPERTY(QVariantList corridas READ corridas NOTIFY carrerasChanged)
    Q_PROPERTY(bool horaValida READ horaValida NOTIFY horaChanged)

public:
    explicit Datos(const QString& carpeta, QObject* padre = nullptr);

    QVariantList proximas() const;
    QVariantList corridas() const;
    bool horaValida() const { return horaValida_; }
    void setHoraValida(bool v) { if (v != horaValida_) { horaValida_ = v; emit horaChanged(); emit carrerasChanged(); } }

    // Una carrera lista para mostrar (o una nueva con los valores de la última práctica si id está vacío)
    Q_INVOKABLE QVariantMap carrera(const QString& id) const;
    // Guarda (crea o cambia). Devuelve {ok, error, id}
    Q_INVOKABLE QVariantMap guardarCarrera(const QVariantMap& c);
    Q_INVOKABLE bool borrarCarrera(const QString& id);

    // Configuración (cfg = fábrica + lo guardado)
    Q_INVOKABLE QVariant cfg(const QString& clave) const;
    Q_INVOKABLE void ponerCfg(const QString& clave, const QVariant& valor);

    // Carga carreras de ejemplo (para capturas y pruebas)
    void cargarMuestra();

    const beto::Base& base() const { return base_; }

signals:
    void carrerasChanged();
    void cfgChanged();
    void horaChanged();
    void error(const QString& texto);

private:
    QVariantMap resumen(const beto::Carrera& c) const;
    bool finalizada(const beto::Carrera& c) const;
    QDate hoy() const { return QDate::currentDate(); }
    void guardar();

    beto::Almacen almacen_;
    beto::Base base_;
    beto::Json cfg_;
    bool horaValida_ = true;
};
