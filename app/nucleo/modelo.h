// Modelo de datos: carrera, vuelta, traza y configuración.
// Mismo formato JSON que la app (docs/app/lo-aprendido.md, secciones 1 y 4).
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "json.h"
#include "motor.h"

namespace beto {

// ---------- Traza de la vuelta (`grabarTraza`) ----------
// Fila: [t s (1 dec), d m (1 dec, mín. 0), kmh (1 dec), lat (6 dec), lon (6 dec), prec m (entera)]
using FilaTraza = std::array<double, 6>;

class Traza {
public:
    static constexpr size_t kTope = 20000;      // se tira la más vieja
    static constexpr size_t kMinimo = 5;        // con menos filas no se guarda

    void empezar(double inicio) { inicio_ = inicio; filas_.clear(); }   // al empezar cada vuelta
    // `restanteM` es el del motor en el instante de la muestra. Graba también las descartadas.
    void agregar(const Muestra& m, double distanciaM, double restanteM);
    const std::vector<FilaTraza>& filas() const { return filas_; }

private:
    double inicio_ = 0;
    std::vector<FilaTraza> filas_;
};

// ---------- Vuelta guardada ----------
struct Vuelta {
    int largada = 1;
    int n = 1;
    bool medida = true;
    int64_t idealMs = 0;
    int64_t realMs = 0;
    std::optional<int64_t> difMs;           // positivo = llegó tarde; sin valor si es interrumpida
    std::string fuente = "pantalla";        // pantalla, teclado, pulsador; «terminar» en la interrumpida
    int64_t fin = 0;                        // hora de pared en ms
    std::optional<double> distM, velPromKmh, velMaxKmh, factorVuelta;
    bool interrumpida = false;              // solo Raspberry: se terminó a mano en medio de la vuelta
    std::vector<FilaTraza> traza;

    static Vuelta desdeMotor(const VueltaMotor& v, int largada, const Traza* traza = nullptr);
    Json aJson() const;
    static Vuelta desdeJson(const Json& j);
};

// ---------- Carrera ----------
struct Carrera {
    std::string id;
    std::string nombre;
    std::string fecha;                      // AAAA-MM-DD
    std::string notas;
    std::string lugar;                      // resto de versiones viejas: se guarda vacío
    int64_t tiempoIdealMs = 240000;
    int64_t distanciaM = 3200;
    std::string patron = "alt1";
    bool practica = false;
    std::string creada;                     // ISO
    int vueltasTotal = 0;                   // de versiones viejas; ya no se usa (sin límite de vueltas)
    std::vector<Vuelta> vueltas;
    Json extra = Json::objeto();            // campos que no conocemos: se conservan

    static constexpr size_t kMaxNombre = 80;
    static constexpr size_t kMaxNotas = 500;

    // Con vueltas ya corridas no se cambian tiempo ideal, distancia ni patrón
    bool parametrosFijos() const { return !vueltas.empty(); }
    int proximaLargada() const;             // máx(largada) + 1
    std::vector<const Vuelta*> vueltasDeLargada(int largada) const;
    // Solo Raspberry, desde el informe: junta la vuelta `i` con la siguiente (una marca de más
    // partió una vuelta en dos). Misma largada, ninguna interrumpida. Queda el tipo (medida/libre)
    // de la primera; las siguientes de esa largada se renumeran. Devuelve false si no se puede.
    bool unirConSiguiente(size_t i);

    Json aJson() const;
    static Carrera desdeJson(const Json& j);
};

// 'c' + ms en base 36 + 4 caracteres al azar (`nuevaId`)
std::string nuevaId(int64_t ahoraMs, uint32_t azar);

// ---------- Configuración ----------
// Valores de fábrica de la Raspberry: los de la app (`DEF`) menos lo que es solo de Android,
// más lo nuevo (orientación, brillo, volumen).
Json configFabrica();
// cfg = fábrica + lo guardado; los valores fuera de rango vuelven al límite (como `leerForm`).
Json armarConfig(const Json& guardado);

// Lo que el motor necesita para una largada (`nuevaCarrera`)
DatosLargada datosLargada(const Carrera& c, const Json& cfg);

// ---------- Base completa ----------
struct Base {
    int schemaVersion = 2;
    Json config = Json::objeto();           // lo guardado (sin mezclar con fábrica)
    std::vector<Carrera> carreras;

    Carrera* buscar(const std::string& id);
    Json aJson() const;
    static Base desdeJson(const Json& j);
};

}  // namespace beto
