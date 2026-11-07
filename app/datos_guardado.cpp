// El guardado: un archivo por carrera (1.0.29), juntando los cambios 0,3 s y escribiendo en un hilo aparte.
#include "datos.h"
#include "datos_comun.h"

#include <algorithm>
#include <sys/resource.h>
#include <unistd.h>

using namespace beto;

void Datos::guardarLuego(const QString& id) {
    luego_.insert(id);
    if (!guardarLuego_.isActive()) guardarLuego_.start();
}

void Datos::guardarConfigLuego() {
    configLuego_ = true;
    if (!guardarLuego_.isActive()) guardarLuego_.start();
}

void Datos::guardarPendientes() {
    guardarLuego_.stop();
    const std::set<QString> ids = std::move(luego_);
    luego_.clear();
    for (const QString& id : ids) guardar(id);
    if (configLuego_) guardarConfig();
}

void Datos::encolar(Tarea t) {
    {
        std::lock_guard<std::mutex> l(m_);
        // Si la misma carrera (o la config) ya estaba esperando, se escribe solo la más nueva
        pendientes_.erase(std::remove_if(pendientes_.begin(), pendientes_.end(), [&](const Tarea& p) {
            return p.tipo == Tarea::Config ? t.tipo == Tarea::Config : t.tipo != Tarea::Config && p.id == t.id;
        }), pendientes_.end());
        pendientes_.push_back(std::move(t));
    }
    cv_.notify_all();
}

void Datos::guardar(const QString& id) {
    luego_.erase(id);
    const Carrera* c = buscar(id);
    if (!c) return;
    Tarea t;
    t.tipo = Tarea::Carrera;
    t.id = c->id;
    t.carrera = std::make_shared<beto::Carrera>(*c);   // la copia de una carrera es rápida; escribir en la SD no
    encolar(std::move(t));
}

void Datos::guardarConfig() {
    configLuego_ = false;
    Tarea t;
    t.tipo = Tarea::Config;
    t.config = std::make_shared<beto::Json>(base_.config);
    encolar(std::move(t));
}

void Datos::borrar(const QString& id) {
    luego_.erase(id);
    Tarea t;
    t.tipo = Tarea::Borrar;
    t.id = s(id);
    encolar(std::move(t));
}

void Datos::guardarYa() {
    guardarPendientes();
    std::unique_lock<std::mutex> l(m_);
    listo_.wait(l, [this] { return pendientes_.empty() && !escribiendo_; });
}

void Datos::hiloGuardar() {
    // Este hilo, con menos prioridad: primero GPS, cronómetro, pulsador y pantalla
    setpriority(PRIO_PROCESS, static_cast<id_t>(::gettid()), 10);
    std::unique_lock<std::mutex> l(m_);
    for (;;) {
        cv_.wait(l, [this] { return !pendientes_.empty() || terminar_; });
        if (pendientes_.empty()) break;   // terminar y no hay nada pendiente
        std::vector<Tarea> tareas = std::move(pendientes_);
        pendientes_.clear();
        escribiendo_ = true;
        l.unlock();
        bool ok = true;
        std::string err;
        for (Tarea& t : tareas) {
            std::string e;
            const bool bien = t.tipo == Tarea::Config ? almacen_.guardarConfig(*t.config, &e)
                              : t.tipo == Tarea::Borrar ? almacen_.borrarCarrera(t.id, &e)
                                                        : almacen_.guardarCarrera(*t.carrera, &e);
            if (!bien) { ok = false; if (err.empty()) err = e; }
        }
        tareas.clear();
        l.lock();
        escribiendo_ = false;
        // Si no se puede guardar (tarjeta llena o dañada) queda en el reporte y la pantalla lo avisa (05-1.2)
        QMetaObject::invokeMethod(this, [this, ok, err] {
            if (!ok) { qWarning("No se pudo guardar la base: %s", err.c_str()); emit error(q("No se pudo guardar: " + err)); }
            if (ok != guardaBien_) { guardaBien_ = ok; emit guardaBienChanged(); }
        }, Qt::QueuedConnection);
        listo_.notify_all();
    }
    listo_.notify_all();
}
