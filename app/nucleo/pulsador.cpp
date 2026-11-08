#include "pulsador.h"

namespace beto {

void FiltroPulsador::flanco(double t, bool apretado) {
    // Primero resolver lo que venció antes de este flanco. Si eso confirma una marca, no se tira: queda pendiente.
    if (auto m = evaluar(t)) pendientes_.push_back(*m);
    bajo_ = apretado;
    tUltimo_ = t;
    switch (estado_) {
        case Estado::Suelto:
            if (apretado) { estado_ = Estado::Candidato; t0_ = t; }
            break;
        case Estado::Candidato:
            // rebote o ruido: se sigue esperando; la hora de la marca queda en el primer flanco
            break;
        case Estado::Apretado:
            if (!apretado) estado_ = Estado::Soltando;
            break;
        case Estado::Soltando:
            if (apretado) estado_ = Estado::Apretado;   // rebote al soltar: sigue apretado, sin marca nueva
            break;
    }
}

std::optional<double> FiltroPulsador::revisar(double ahora) {
    if (!pendientes_.empty()) {   // las que se confirmaron en un flanco: primero esas, en orden
        const double m = pendientes_.front();
        pendientes_.erase(pendientes_.begin());
        return m;
    }
    return evaluar(ahora);
}

std::optional<double> FiltroPulsador::evaluar(double ahora) {
    switch (estado_) {
        case Estado::Candidato:
            if (bajo_ && ahora - tUltimo_ >= confirmarMs_) {   // quedó apretado lo suficiente: vale
                estado_ = Estado::Apretado;
                return t0_;
            }
            if (!bajo_ && ahora - tUltimo_ >= reboteMs_) {     // se soltó y no volvió: era ruido
                estado_ = Estado::Suelto;
                ++descartados_;
            }
            break;
        case Estado::Soltando:
            if (!bajo_ && ahora - tUltimo_ >= soltarMs_) estado_ = Estado::Suelto;   // rearmado
            break;
        default:
            break;
    }
    return std::nullopt;
}

std::optional<double> FiltroPulsador::proximoVencimiento() const {
    if (!pendientes_.empty()) return tUltimo_;   // hay una marca para entregar ya
    switch (estado_) {
        case Estado::Candidato: return tUltimo_ + (bajo_ ? confirmarMs_ : reboteMs_);
        case Estado::Soltando: return tUltimo_ + soltarMs_;
        default: return std::nullopt;
    }
}

}  // namespace beto
