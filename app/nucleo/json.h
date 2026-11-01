// JSON chico y propio: alcanza para la base de Betómetro (mismo formato que la app).
// Los objetos guardan el orden de las claves, como JavaScript.
#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace beto {

class Json {
public:
    enum class Tipo { Nulo, Bool, Numero, Texto, Lista, Objeto };
    using Lista = std::vector<Json>;
    using Par = std::pair<std::string, Json>;
    using Objeto = std::vector<Par>;

    Json() = default;                                   // null
    Json(std::nullptr_t) {}
    Json(bool b) : tipo_(Tipo::Bool), b_(b) {}
    Json(int n) : tipo_(Tipo::Numero), n_(n) {}
    Json(int64_t n) : tipo_(Tipo::Numero), n_(static_cast<double>(n)) {}
    Json(double n) : tipo_(Tipo::Numero), n_(n) {}
    Json(const char* s) : tipo_(Tipo::Texto), s_(s) {}
    Json(std::string s) : tipo_(Tipo::Texto), s_(std::move(s)) {}
    Json(Lista l) : tipo_(Tipo::Lista), l_(std::make_unique<Lista>(std::move(l))) {}
    Json(Objeto o) : tipo_(Tipo::Objeto), o_(std::make_unique<Objeto>(std::move(o))) {}

    Json(const Json& otro);
    Json& operator=(const Json& otro);
    Json(Json&&) noexcept = default;
    Json& operator=(Json&&) noexcept = default;

    static Json lista() { return Json(Lista{}); }
    static Json objeto() { return Json(Objeto{}); }

    Tipo tipo() const { return tipo_; }
    bool esNulo() const { return tipo_ == Tipo::Nulo; }
    bool esBool() const { return tipo_ == Tipo::Bool; }
    bool esNumero() const { return tipo_ == Tipo::Numero; }
    bool esTexto() const { return tipo_ == Tipo::Texto; }
    bool esLista() const { return tipo_ == Tipo::Lista; }
    bool esObjeto() const { return tipo_ == Tipo::Objeto; }

    // Lectura con valor por defecto si no es del tipo pedido
    bool aBool(bool def = false) const { return esBool() ? b_ : def; }
    double aNumero(double def = 0) const { return esNumero() ? n_ : def; }
    int64_t aEntero(int64_t def = 0) const;
    std::string aTexto(const std::string& def = "") const { return esTexto() ? s_ : def; }

    // Listas
    const Lista& elementos() const;
    Lista& elementos();
    void agregar(Json v);
    size_t tamano() const;

    // Objetos
    const Objeto& pares() const;
    bool tiene(const std::string& clave) const;
    const Json& operator[](const std::string& clave) const;   // null si no está
    Json& operator[](const std::string& clave);               // la crea si no está
    void poner(const std::string& clave, Json v) { (*this)[clave] = std::move(v); }
    void sacar(const std::string& clave);

    bool operator==(const Json& otro) const;
    bool operator!=(const Json& otro) const { return !(*this == otro); }

    // Texto: sangria < 0 → todo en una línea (como JSON.stringify(x))
    std::string escribir(int sangria = -1) const;
    static Json leer(const std::string& texto);   // tira ErrorJson si está mal

private:
    void escribirEn(std::string& out, int sangria, int nivel) const;

    Tipo tipo_ = Tipo::Nulo;
    bool b_ = false;
    double n_ = 0;
    std::string s_;
    std::unique_ptr<Lista> l_;     // copia profunda al copiar
    std::unique_ptr<Objeto> o_;
};

struct ErrorJson : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Números como JavaScript: el texto más corto que vuelve al mismo número; enteros sin «.0».
std::string numeroComoJs(double x);

}  // namespace beto
