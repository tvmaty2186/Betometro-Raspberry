// Filtro del pulsador por cable (GPIO): marca al APRETAR, con antirrebote y antirruido.
//
// El pulsador va entre el GPIO17 (pin 11) y GND (pin 9), con resistencia hacia 3,3 V:
// suelto = nivel alto, apretado = nivel bajo. El sistema avisa cada cambio con la hora exacta
// del flanco (la toma el kernel en la interrupción), y este filtro decide:
//
// - La marca vale la hora del PRIMER flanco de bajada: el instante en que se apoyó el dedo.
//   Soltar no cuenta, aunque se tarde en soltar.
// - Antirruido: el botón tiene que quedar apretado al menos `confirmarMs` (10 ms) para valer.
//   Un pico de ruido (chispa, motor de arranque) dura microsegundos y se descarta.
// - Antirrebote al apretar: los rebotes del contacto (unos ms) no cambian la hora de la marca
//   ni dan marcas dobles.
// - Rearme: después de una marca, el botón tiene que quedar suelto al menos `soltarMs` (30 ms)
//   antes de aceptar otra. Mantenerlo apretado nunca da más de una marca.
// - Además, el motor tiene su antirrebote de la app (3 s desde el inicio de la vuelta).
// C++ puro, sin hardware: se prueba en la PC.
#pragma once

#include <optional>
#include <vector>

namespace beto {

class FiltroPulsador {
public:
    explicit FiltroPulsador(double confirmarMs = 10, double soltarMs = 30, double reboteMs = 20)
        : confirmarMs_(confirmarMs), soltarMs_(soltarMs), reboteMs_(reboteMs) {}

    // Un flanco: `apretado` = el nivel quedó bajo. `t` = hora del flanco (ms, reloj monotónico).
    void flanco(double t, bool apretado);
    // Llamar seguido (o cuando vence `proximoVencimiento`). Devuelve la hora de la marca si se confirmó una.
    std::optional<double> revisar(double ahora);
    // Hasta cuándo hay que esperar para la próxima decisión (para programar un temporizador)
    std::optional<double> proximoVencimiento() const;

    int descartados() const { return descartados_; }   // picos de ruido descartados (para el chequeo)

private:
    enum class Estado { Suelto, Candidato, Apretado, Soltando };
    Estado estado_ = Estado::Suelto;
    bool bajo_ = false;          // nivel actual (apretado)
    double t0_ = 0;              // primer flanco de bajada del intento: la hora de la marca
    double tUltimo_ = 0;         // último flanco
    double confirmarMs_, soltarMs_, reboteMs_;
    int descartados_ = 0;
    // Una marca que se confirmó adentro de flanco() (apretar y soltar llegaron juntos porque el programa estaba
    // ocupado): se guarda para que la devuelva el próximo revisar(). Antes se perdía (auditoría 12, 08/11/2026).
    std::vector<double> pendientes_;
    std::optional<double> evaluar(double ahora);   // decide con el estado (lo que antes hacía revisar)
};

}  // namespace beto
