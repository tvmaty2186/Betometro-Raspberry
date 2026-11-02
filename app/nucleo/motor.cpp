#include "motor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace beto {

bool esMedida(const std::string& patron, int n) {
    if (patron == "alt1") return n % 2 == 1;
    if (patron == "alt2") return n % 2 == 0;
    return true;
}

double redondearJs(double x) {
    // Math.round: el entero más cercano; en el empate, hacia +infinito
    if (!std::isfinite(x)) return x;
    double r = std::floor(x);
    if (x - r >= 0.5) r += 1;
    return r;
}

double fijarDecimalesJs(double x, int dec) {
    // +x.toFixed(dec): redondeo exacto del número decimal; en el empate exacto, lejos del cero
    if (!std::isfinite(x) || std::fabs(x) >= 1e21) return x;
    double k = 2 * std::pow(10.0, dec);
    double r = x * k;
    if (std::fma(x, k, -r) == 0 && r == std::trunc(r) && std::fmod(std::fabs(r), 2.0) == 1.0) {
        double m = std::pow(10.0, dec);
        double n = (std::fabs(r) + 1) / 2;           // n / 10^dec, el mayor de los dos
        return std::copysign(n / m, x);
    }
    char b[400];
    std::snprintf(b, sizeof b, "%.*f", dec, x);      // glibc redondea exacto los que no empatan
    return std::strtod(b, nullptr);
}

Motor::Motor(DatosLargada datos, Oyente oyente, RelojPared relojPared)
    : datos_(std::move(datos)), oyente_(std::move(oyente)), relojPared_(std::move(relojPared)) {
    factor_ = datos_.factorGps != 0 && std::isfinite(datos_.factorGps) ? datos_.factorGps : 1;
    if (!relojPared_) {
        relojPared_ = [] {
            using namespace std::chrono;
            return static_cast<int64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
        };
    }
}

void Motor::emitir(Evento e, const DatosEvento& d) {
    if (oyente_) oyente_(e, d);
}

double Motor::distCruda(double ahora) const {
    if (!(enCurso_ && ult_)) return dCruda_;
    return dCruda_ + ult_->v * std::min(2.0, std::max(0.0, (ahora - ult_->t) / 1000));
}

void Motor::nuevaVuelta(double ahora) {
    lapStart_ = ahora;
    dCruda_ = 0;
    velMaxMs_ = ult_ ? ult_->v : 0;
    vNecCongelada_.reset();
    if (ult_) ult_ = Ult{ahora, ult_->v};   // la distancia de la vuelta nueva arranca justo en la marca
}

bool Motor::muestra(const Muestra& m) {
    if (m.precisionM > datos_.umbralPrecisionM) {
        DatosEvento d;
        d.muestra = &m;
        emitir(Evento::Descartada, d);
        return false;
    }
    if (enCurso_ && ult_) {
        double dt = (m.t - ult_->t) / 1000;
        if (dt > 0 && dt < 10) dCruda_ += (ult_->v + m.velMs) / 2 * dt;   // trapecio; un hueco de 10 s o más no suma
    }
    ult_ = Ult{m.t, m.velMs};
    if (enCurso_) velMaxMs_ = std::max(velMaxMs_, m.velMs);
    return true;
}

void Motor::completarDistancia(VueltaMotor& v, double real, double cruda) const {
    v.distM = redondearJs(cruda * factor_);
    v.velPromKmh = fijarDecimalesJs(cruda * factor_ / (real / 1000) * 3.6, 1);
    v.velMaxKmh = fijarDecimalesJs(velMaxMs_ * 3.6, 1);
}

ResultadoMarca Motor::marcar(double ahora, const std::string& fuente) {
    if (!enCurso_) {                                  // LARGADA (sin antirrebote)
        enCurso_ = true;
        lapNum_ = 1;
        nuevaVuelta(ahora);
        DatosEvento d;
        d.n = 1;
        d.medida = esMedida(datos_.patron, 1);
        d.inicio = ahora;
        emitir(Evento::Vuelta, d);
        return ResultadoMarca::Largada;
    }
    if (ahora - lapStart_ < datos_.antirreboteMs) {   // contado desde el inicio de la vuelta
        emitir(Evento::Ignorada, DatosEvento{});
        return ResultadoMarca::Ignorada;
    }
    double real = ahora - lapStart_, cruda = distCruda(ahora);
    VueltaMotor v;
    v.n = lapNum_;
    v.medida = esMedida(datos_.patron, lapNum_);
    v.idealMs = datos_.tiempoIdealMs;
    v.realMs = redondearJs(real);
    v.difMs = redondearJs(real - datos_.tiempoIdealMs);   // positivo = llegó tarde
    v.fuente = fuente.empty() ? "pantalla" : fuente;
    v.fin = relojPared_();
    if (cruda > 0) {
        completarDistancia(v, real, cruda);
        double D = datos_.distanciaM;
        if (datos_.calibrar && D > 0) {
            double f = D / cruda;
            if (f > 0.85 && f < 1.15) {               // fuera de eso: se marcó mal o se cortó camino
                factores_.push_back(f);
                double suma = 0;
                for (double x : factores_) suma += x;   // mismo orden que reduce() de la app
                factor_ = suma / static_cast<double>(factores_.size());
                datos_.factorGps = factor_;
                v.factorVuelta = fijarDecimalesJs(f, 4);
            }
        }
    }
    vueltas_.push_back(v);
    {
        DatosEvento d;
        d.vuelta = &vueltas_.back();
        emitir(Evento::Cerrada, d);
    }
    lapNum_ += 1;
    nuevaVuelta(ahora);
    DatosEvento d;
    d.n = lapNum_;
    d.medida = esMedida(datos_.patron, lapNum_);
    d.inicio = ahora;
    emitir(Evento::Vuelta, d);
    return ResultadoMarca::Marca;
}

void Motor::terminar(double ahora, bool aMano) {
    if (enCurso_ && aMano) {
        bool dentroDelTotal = datos_.vueltasTotal <= 0 || lapNum_ <= datos_.vueltasTotal;
        double real = ahora - lapStart_;
        if (dentroDelTotal && real >= datos_.antirreboteMs && real > 0) {
            VueltaMotor v;
            v.n = lapNum_;
            v.medida = esMedida(datos_.patron, lapNum_);
            v.idealMs = datos_.tiempoIdealMs;
            v.realMs = redondearJs(real);
            v.fuente = "terminar";
            v.fin = relojPared_();
            v.interrumpida = true;
            double cruda = distCruda(ahora);
            if (cruda > 0) completarDistancia(v, real, cruda);   // sin calibrar: no es una vuelta completa
            vueltas_.push_back(v);
            DatosEvento d;
            d.vuelta = &vueltas_.back();
            emitir(Evento::Interrumpida, d);
        }
    }
    enCurso_ = false;
    emitir(Evento::Terminada, DatosEvento{});
}

Estado Motor::estado(double ahora) {
    const double T = datos_.tiempoIdealMs, D = datos_.distanciaM;
    Estado e;
    e.sinGps = !ult_ || ahora - ult_->t > 3000;
    if (ult_ && !e.sinGps) e.vActKmh = ult_->v * 3.6;
    if (D > 0) e.vBaseKmh = D / (T / 1000) * 3.6;
    if (!enCurso_) {
        e.enCurso = false;
        e.n = 0;
        e.medida = esMedida(datos_.patron, 1);
        e.transMs = 0;
        e.restanteMs = T;
        e.proxMedida = esMedida(datos_.patron, 1);
        e.vNecKmh = e.vBaseKmh;
        e.restanteM = D;
        return e;
    }
    double trans = ahora - lapStart_, restanteMs = T - trans;
    double d = distCruda(ahora) * factor_, restanteM = D - d;
    if (D > 0 && !e.sinGps) {
        e.deltaS = (d - D * trans / T) / (D / (T / 1000));
        if (restanteM <= 0) {
            e.vNecKmh = 0.0;
        } else if (restanteMs <= datos_.congelarMs) {   // CONGELAR
            if (!vNecCongelada_ && restanteMs > 0) vNecCongelada_ = restanteM / (restanteMs / 1000) * 3.6;
            e.vNecKmh = vNecCongelada_;
        } else {
            e.vNecKmh = restanteM / (restanteMs / 1000) * 3.6;
        }
    }
    e.enCurso = true;
    e.n = lapNum_;
    e.medida = esMedida(datos_.patron, lapNum_);
    e.transMs = trans;
    e.restanteMs = restanteMs;
    e.proxMedida = esMedida(datos_.patron, lapNum_ + 1);
    e.congelada = vNecCongelada_.has_value();
    e.restanteM = restanteM;
    return e;
}

}  // namespace beto
