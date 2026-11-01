#include "programador.h"

#include <cmath>

namespace beto {

void ProgramadorVoz::reiniciarVuelta() {
    ultimoK_.reset();
    pitoCero_ = false;
    ultPitidoCada_ = 0;
}

std::vector<Sonido> ProgramadorVoz::tic(const Estado& e, const AjustesVoz& a) {
    std::vector<Sonido> r;
    if (!e.enCurso) return r;
    const int N = a.cuentaN;
    const double ant = a.enVoz ? a.anticipoMs : 0;
    // Pitido cada X segundos desde el inicio de la vuelta; se calla durante la cuenta
    if (a.pitidoCadaS > 0) {
        const long p = static_cast<long>(std::floor(e.transMs / (a.pitidoCadaS * 1000)));
        if (p > 0 && p != ultPitidoCada_ && e.restanteMs > N * 1000 + 500) {
            ultPitidoCada_ = p;
            r.push_back({Sonido::Tipo::Tic});
        }
    }
    // Cuenta regresiva: solo hacia abajo, adelantada por la demora de la voz
    if (N > 0) {
        const double kd = std::ceil((e.restanteMs - ant) / 1000);
        if (kd >= 1 && kd <= N) {
            const int k = static_cast<int>(kd);
            if ((!ultimoK_ || k < *ultimoK_) && (!ultimoK_ || k != *ultimoK_)) {
                ultimoK_ = k;
                if (a.enVoz) r.push_back({Sonido::Tipo::Numero, k});
                else r.push_back({Sonido::Tipo::PitidoCorto});
            }
        }
    }
    if (a.pitidoCero && !pitoCero_ && e.restanteMs <= 0) {   // sin anticipo
        pitoCero_ = true;
        r.push_back({Sonido::Tipo::PitidoLargo});
    }
    if (!a.pitidoCero && N > 0 && !pitoCero_ && ultimoK_ && e.restanteMs - ant <= 0) {
        pitoCero_ = true;
        if (a.enVoz) r.push_back({Sonido::Tipo::Cero});
        else r.push_back({Sonido::Tipo::PitidoCorto});
    }
    return r;
}

std::optional<std::string> anuncioVuelta(int n, bool medida, int vueltasTotal) {
    if (vueltasTotal > 0 && n > vueltasTotal) return std::nullopt;
    const std::string tipo = medida ? "medida" : "práctica";
    if (vueltasTotal > 0 && n == vueltasTotal) return "Última vuelta, " + tipo;
    return "Vuelta " + std::to_string(n) + ", " + tipo;
}

}  // namespace beto
