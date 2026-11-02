#include "json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace beto {

Json::Json(const Json& o) : tipo_(o.tipo_), b_(o.b_), n_(o.n_), s_(o.s_) {
    if (o.l_) l_ = std::make_unique<Lista>(*o.l_);
    if (o.o_) o_ = std::make_unique<Objeto>(*o.o_);
}

Json& Json::operator=(const Json& o) {
    if (this != &o) { Json copia(o); *this = std::move(copia); }
    return *this;
}

int64_t Json::aEntero(int64_t def) const {
    if (!esNumero() || !std::isfinite(n_)) return def;
    return static_cast<int64_t>(std::llround(n_));
}

const Json::Lista& Json::elementos() const {
    static const Lista vacia;
    return esLista() ? *l_ : vacia;
}

Json::Lista& Json::elementos() {
    if (!esLista()) *this = Json(Lista{});
    return *l_;
}

void Json::agregar(Json v) { elementos().push_back(std::move(v)); }

size_t Json::tamano() const {
    if (esLista()) return l_->size();
    if (esObjeto()) return o_->size();
    return 0;
}

const Json::Objeto& Json::pares() const {
    static const Objeto vacio;
    return esObjeto() ? *o_ : vacio;
}

bool Json::tiene(const std::string& clave) const {
    for (const auto& p : pares()) if (p.first == clave) return true;
    return false;
}

const Json& Json::operator[](const std::string& clave) const {
    static const Json nulo;
    for (const auto& p : pares()) if (p.first == clave) return p.second;
    return nulo;
}

Json& Json::operator[](const std::string& clave) {
    if (!esObjeto()) *this = Json(Objeto{});
    for (auto& p : *o_) if (p.first == clave) return p.second;
    o_->emplace_back(clave, Json());
    return o_->back().second;
}

void Json::sacar(const std::string& clave) {
    if (!esObjeto()) return;
    for (auto it = o_->begin(); it != o_->end(); ++it)
        if (it->first == clave) { o_->erase(it); return; }
}

bool Json::operator==(const Json& x) const {
    if (tipo_ != x.tipo_) return false;
    switch (tipo_) {
        case Tipo::Nulo: return true;
        case Tipo::Bool: return b_ == x.b_;
        case Tipo::Numero: return n_ == x.n_;
        case Tipo::Texto: return s_ == x.s_;
        case Tipo::Lista: return *l_ == *x.l_;
        case Tipo::Objeto: {
            if (o_->size() != x.o_->size()) return false;
            for (const auto& p : *o_) {
                if (!x.tiene(p.first) || x[p.first] != p.second) return false;
            }
            return true;
        }
    }
    return false;
}

// ---------- Escribir ----------

std::string numeroComoJs(double x) {
    // Number.prototype.toString de JavaScript
    if (!std::isfinite(x)) return "null";            // JSON.stringify(NaN) → null
    if (x == 0) return "0";                           // también -0
    std::string signo = x < 0 ? "-" : "";
    double a = std::fabs(x);
    char b[40];
    for (int p = 0; p <= 16; ++p) {                   // menos dígitos que vuelven al mismo número
        std::snprintf(b, sizeof b, "%.*e", p, a);
        if (std::strtod(b, nullptr) == a) break;
    }
    std::string t = b;                                // d.ddddde±XX
    size_t e = t.find('e');
    int exp = std::atoi(t.c_str() + e + 1);
    std::string dig;
    for (size_t i = 0; i < e; ++i) if (t[i] != '.') dig += t[i];
    while (dig.size() > 1 && dig.back() == '0') dig.pop_back();
    int k = static_cast<int>(dig.size()), n = exp + 1;   // valor = 0.dig × 10^n
    std::string r;
    if (k <= n && n <= 21) r = dig + std::string(static_cast<size_t>(n - k), '0');
    else if (0 < n && n <= 21) r = dig.substr(0, static_cast<size_t>(n)) + "." + dig.substr(static_cast<size_t>(n));
    else if (-6 < n && n <= 0) r = "0." + std::string(static_cast<size_t>(-n), '0') + dig;
    else {
        std::string m = k == 1 ? dig : dig.substr(0, 1) + "." + dig.substr(1);
        r = m + "e" + (n - 1 >= 0 ? "+" : "-") + std::to_string(std::abs(n - 1));
    }
    return signo + r;
}

static void escribirTexto(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char b[8];
                    std::snprintf(b, sizeof b, "\\u%04x", c);
                    out += b;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
}

void Json::escribirEn(std::string& out, int sangria, int nivel) const {
    auto salto = [&](int n) {
        if (sangria < 0) return;
        out += '\n';
        out.append(static_cast<size_t>(sangria * n), ' ');
    };
    switch (tipo_) {
        case Tipo::Nulo: out += "null"; break;
        case Tipo::Bool: out += b_ ? "true" : "false"; break;
        case Tipo::Numero: out += numeroComoJs(n_); break;
        case Tipo::Texto: escribirTexto(out, s_); break;
        case Tipo::Lista:
            if (l_->empty()) { out += "[]"; break; }
            out += '[';
            for (size_t i = 0; i < l_->size(); ++i) {
                if (i) out += ',';
                salto(nivel + 1);
                (*l_)[i].escribirEn(out, sangria, nivel + 1);
            }
            salto(nivel);
            out += ']';
            break;
        case Tipo::Objeto:
            if (o_->empty()) { out += "{}"; break; }
            out += '{';
            for (size_t i = 0; i < o_->size(); ++i) {
                if (i) out += ',';
                salto(nivel + 1);
                escribirTexto(out, (*o_)[i].first);
                out += sangria < 0 ? ":" : ": ";
                (*o_)[i].second.escribirEn(out, sangria, nivel + 1);
            }
            salto(nivel);
            out += '}';
            break;
    }
}

std::string Json::escribir(int sangria) const {
    std::string out;
    escribirEn(out, sangria, 0);
    return out;
}

// ---------- Leer ----------

namespace {

struct Lector {
    const std::string& s;
    size_t i = 0;

    [[noreturn]] void error(const char* que) const {
        throw ErrorJson(std::string("JSON mal formado (") + que + ") en la posición " + std::to_string(i));
    }
    void blancos() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i;
    }
    bool sigue(const char* palabra) {
        size_t n = std::char_traits<char>::length(palabra);
        if (s.compare(i, n, palabra) == 0) { i += n; return true; }
        return false;
    }
    static void utf8(std::string& out, uint32_t c) {
        if (c < 0x80) out += static_cast<char>(c);
        else if (c < 0x800) { out += static_cast<char>(0xC0 | (c >> 6)); out += static_cast<char>(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) {
            out += static_cast<char>(0xE0 | (c >> 12));
            out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (c >> 18));
            out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    uint32_t hex4() {
        if (i + 4 > s.size()) error("\\u corto");
        uint32_t v = 0;
        for (int k = 0; k < 4; ++k) {
            char c = s[i++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= static_cast<uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<uint32_t>(c - 'A' + 10);
            else error("\\u inválido");
        }
        return v;
    }
    std::string texto() {
        if (s[i] != '"') error("se esperaba texto");
        ++i;
        std::string out;
        while (true) {
            if (i >= s.size()) error("texto sin cerrar");
            char c = s[i++];
            if (c == '"') break;
            if (c != '\\') { out += c; continue; }
            if (i >= s.size()) error("escape sin cerrar");
            char e = s[i++];
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    uint32_t c1 = hex4();
                    if (c1 >= 0xD800 && c1 < 0xDC00 && sigue("\\u")) {
                        uint32_t c2 = hex4();
                        c1 = 0x10000 + ((c1 - 0xD800) << 10) + (c2 - 0xDC00);
                    }
                    utf8(out, c1);
                    break;
                }
                default: error("escape inválido");
            }
        }
        return out;
    }
    Json valor(int profundidad) {
        if (profundidad > 200) error("demasiado anidado");
        blancos();
        if (i >= s.size()) error("faltan datos");
        char c = s[i];
        if (c == '{') {
            ++i;
            Json o = Json::objeto();
            blancos();
            if (i < s.size() && s[i] == '}') { ++i; return o; }
            while (true) {
                blancos();
                std::string k = texto();
                blancos();
                if (i >= s.size() || s[i] != ':') error("se esperaba «:»");
                ++i;
                o[k] = valor(profundidad + 1);
                blancos();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == '}') { ++i; return o; }
                error("se esperaba «,» o «}»");
            }
        }
        if (c == '[') {
            ++i;
            Json l = Json::lista();
            blancos();
            if (i < s.size() && s[i] == ']') { ++i; return l; }
            while (true) {
                l.agregar(valor(profundidad + 1));
                blancos();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == ']') { ++i; return l; }
                error("se esperaba «,» o «]»");
            }
        }
        if (c == '"') return Json(texto());
        if (sigue("true")) return Json(true);
        if (sigue("false")) return Json(false);
        if (sigue("null")) return Json();
        if (c == '-' || (c >= '0' && c <= '9')) {
            const char* ini = s.c_str() + i;
            char* fin = nullptr;
            double n = std::strtod(ini, &fin);
            if (fin == ini) error("número inválido");
            i += static_cast<size_t>(fin - ini);
            return Json(n);
        }
        error("valor inesperado");
    }
};

}  // namespace

Json Json::leer(const std::string& texto) {
    Lector l{texto};
    Json v = l.valor(0);
    l.blancos();
    if (l.i != texto.size()) l.error("sobran datos");
    return v;
}

}  // namespace beto
