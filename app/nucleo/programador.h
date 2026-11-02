// Programador de voz: decide qué decir y cuándo, mirando el estado del motor cada 15 ms.
// Misma lógica que `programador` de la app 2.1.1 (docs/app/lo-aprendido.md, sección 3).
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "motor.h"

namespace beto {

struct AjustesVoz {
    int cuentaN = 20;            // segundos que cuenta (0 = sin cuenta)
    bool enVoz = true;           // modoCuenta 'voz' (si no, pitidos)
    double anticipoMs = 40;      // solo en voz
    bool pitidoCero = false;     // pitido largo en vez de «cero»
    double pitidoCadaS = 0;      // tic cada X s (0 = apagado)
};

// Un sonido a reproducir
struct Sonido {
    enum class Tipo { Numero, Cero, PitidoCorto, PitidoLargo, Tic, Frase };
    Tipo tipo;
    int numero = 0;              // Numero: 1..60
    std::string frase;           // Frase: «Vuelta 3, medida», «Carrera completa»…
    bool operator==(const Sonido& o) const { return tipo == o.tipo && numero == o.numero && frase == o.frase; }
};

class ProgramadorVoz {
public:
    void reiniciarVuelta();      // resetVueltaAudio(): al empezar cada vuelta
    // Llamar cada ~15 ms con la carrera en curso. Devuelve lo que hay que reproducir ahora.
    std::vector<Sonido> tic(const Estado& e, const AjustesVoz& a);

private:
    std::optional<int> ultimoK_;
    bool pitoCero_ = false;
    long ultPitidoCada_ = 0;
};

// Anuncio al empezar la vuelta n (si avisoInicio). Vacío si la carrera ya terminó (n > total).
std::optional<std::string> anuncioVuelta(int n, bool medida, int vueltasTotal);

}  // namespace beto
