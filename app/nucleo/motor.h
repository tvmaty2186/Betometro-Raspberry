// Motor de Betómetro: la misma lógica que `Motor` de la app 2.1.1
// (referencia/app-2.1.1/index.html, sección MOTOR; explicado en docs/app/lo-aprendido.md, sección 2).
//
// No tiene pantalla, ni audio, ni hardware. Todo el tiempo es en ms de un reloj monotónico,
// salvo `fin` de la vuelta (hora de pared).
//
// Lo propio de la Raspberry:
//   - Vuelta interrumpida: al terminar a mano, la vuelta en curso se guarda marcada así.
//   - Solo cronómetro: sin muestras de GPS la carrera anda igual, solo con el tiempo.
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace beto {

bool esMedida(const std::string& patron, int n);

// Redondeos iguales a JavaScript
double redondearJs(double x);                 // Math.round
double fijarDecimalesJs(double x, int dec);   // +x.toFixed(dec)

// Datos de la largada (`nuevaCarrera` de la app)
struct DatosLargada {
    double tiempoIdealMs = 240000;
    double distanciaM = 3200;
    std::string patron = "alt1";
    int vueltasTotal = 0;            // 0 = sin fin (práctica)
    double antirreboteMs = 3000;
    double umbralPrecisionM = 20;
    bool calibrar = true;
    double congelarMs = 20000;       // max(cuentaN, 3) * 1000
    double factorGps = 1;            // arranca en 1 en cada largada
};

// Muestra de velocidad del GPS
struct Muestra {
    double t = 0;                    // ms monotónico
    double velMs = 0;                // m/s
    double precisionM = 0;           // error horizontal estimado
    std::optional<double> lat, lon;
};

// Vuelta que cierra el motor (lo que emite 'cerrada' o 'interrumpida')
struct VueltaMotor {
    int n = 0;
    bool medida = true;
    double idealMs = 0;
    double realMs = 0;
    std::optional<double> difMs;     // sin valor en la vuelta interrumpida
    std::string fuente = "pantalla";
    int64_t fin = 0;                 // hora de pared en ms
    std::optional<double> distM, velPromKmh, velMaxKmh, factorVuelta;
    bool interrumpida = false;
};

// Lo que leen la pantalla y la voz (`estado(ahora)`)
struct Estado {
    bool enCurso = false;
    int n = 0;
    bool medida = true;
    double transMs = 0;
    double restanteMs = 0;           // puede ser negativo (vuelta pasada)
    bool proxMedida = true;
    bool sinGps = true;
    std::optional<double> vActKmh, vBaseKmh, vNecKmh;
    bool congelada = false;
    double restanteM = 0;
    std::optional<double> deltaS;    // positivo = adelantado
};

enum class Evento { Vuelta, Cerrada, Interrumpida, Ignorada, Descartada, Terminada };

struct DatosEvento {
    // 'vuelta'
    int n = 0;
    bool medida = true;
    double inicio = 0;
    // 'cerrada' / 'interrumpida'
    const VueltaMotor* vuelta = nullptr;
    // 'descartada'
    const Muestra* muestra = nullptr;
};

enum class ResultadoMarca { Largada, Marca, Ignorada };

class Motor {
public:
    using Oyente = std::function<void(Evento, const DatosEvento&)>;
    using RelojPared = std::function<int64_t()>;   // Date.now()

    Motor(DatosLargada datos, Oyente oyente = {}, RelojPared relojPared = {});

    bool enCurso() const { return enCurso_; }
    double factor() const { return factor_; }
    const DatosLargada& datos() const { return datos_; }
    const std::vector<VueltaMotor>& vueltas() const { return vueltas_; }

    // Cambios de Ajustes con la carrera en curso: se aplican enseguida
    void ponerAntirrebote(double ms) { datos_.antirreboteMs = ms; }
    void ponerUmbralPrecision(double m) { datos_.umbralPrecisionM = m; }
    void ponerCalibrar(bool si) { datos_.calibrar = si; }

    bool muestra(const Muestra& m);
    ResultadoMarca marcar(double ahora, const std::string& fuente = "pantalla");
    // Terminar. Si es a mano y la vuelta en curso tiene al menos el tiempo del antirrebote,
    // se guarda como vuelta interrumpida (no cuenta como completa, no calibra, sin error).
    void terminar(double ahora, bool aMano);
    Estado estado(double ahora);      // ojo: fija la sugerida congelada (igual que la app)

private:
    struct Ult { double t; double v; };
    double distCruda(double ahora) const;
    void nuevaVuelta(double ahora);
    void completarDistancia(VueltaMotor& v, double real, double cruda) const;
    void emitir(Evento e, const DatosEvento& d);

    DatosLargada datos_;
    Oyente oyente_;
    RelojPared relojPared_;
    std::vector<VueltaMotor> vueltas_;

    int lapNum_ = 0;
    double lapStart_ = 0;
    bool enCurso_ = false;
    double dCruda_ = 0;
    std::optional<Ult> ult_;          // última muestra válida
    double velMaxMs_ = 0;
    std::optional<double> vNecCongelada_;
    std::vector<double> factores_;
    double factor_ = 1;
};

}  // namespace beto
