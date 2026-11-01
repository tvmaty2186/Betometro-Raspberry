#include "neo.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace beto {

namespace {

uint32_t u4(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24); }
int32_t i4(const uint8_t* p) { return static_cast<int32_t>(u4(p)); }
uint16_t u2(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
void pon4(std::vector<uint8_t>& v, size_t i, uint32_t x) { for (int k = 0; k < 4; ++k) v[i + k] = static_cast<uint8_t>(x >> (8 * k)); }
void pon2(std::vector<uint8_t>& v, size_t i, uint16_t x) { v[i] = static_cast<uint8_t>(x); v[i + 1] = static_cast<uint8_t>(x >> 8); }

std::vector<uint8_t> marco(uint8_t clase, uint8_t id, const std::vector<uint8_t>& carga) {
    std::vector<uint8_t> m = {0xB5, 0x62, clase, id, static_cast<uint8_t>(carga.size()), static_cast<uint8_t>(carga.size() >> 8)};
    m.insert(m.end(), carga.begin(), carga.end());
    uint8_t a = 0, b = 0;
    for (size_t i = 2; i < m.size(); ++i) { a = static_cast<uint8_t>(a + m[i]); b = static_cast<uint8_t>(b + a); }
    m.push_back(a);
    m.push_back(b);
    return m;
}

uint8_t sumaNmea(const std::string& cuerpo) {
    uint8_t s = 0;
    for (char c : cuerpo) s ^= static_cast<uint8_t>(c);
    return s;
}

std::vector<std::string> partes(const std::string& s) {
    std::vector<std::string> r;
    std::string actual;
    for (char c : s) {
        if (c == ',') { r.push_back(actual); actual.clear(); } else actual += c;
    }
    r.push_back(actual);
    return r;
}

double coordenada(const std::string& v, const std::string& hemisferio) {
    if (v.size() < 4) return 0;
    const double x = std::atof(v.c_str());
    const double grados = std::floor(x / 100), minutos = x - grados * 100;
    const double g = grados + minutos / 60;
    return hemisferio == "S" || hemisferio == "W" ? -g : g;
}

}  // namespace

std::vector<LecturaGps> DecodificadorNeo::agregar(const uint8_t* datos, size_t n) {
    std::vector<LecturaGps> out;
    buf_.insert(buf_.end(), datos, datos + n);
    size_t i = 0;
    while (i < buf_.size()) {
        if (buf_[i] == 0xB5) {   // UBX
            if (buf_.size() - i < 8) break;
            if (buf_[i + 1] != 0x62) { ++i; continue; }
            const size_t largo = u2(&buf_[i + 4]);
            if (largo > 1024) { ++i; continue; }
            if (buf_.size() - i < 8 + largo) break;
            uint8_t a = 0, b = 0;
            for (size_t k = i + 2; k < i + 6 + largo; ++k) { a = static_cast<uint8_t>(a + buf_[k]); b = static_cast<uint8_t>(b + a); }
            if (a == buf_[i + 6 + largo] && b == buf_[i + 7 + largo]) {
                ubx(std::vector<uint8_t>(buf_.begin() + static_cast<long>(i), buf_.begin() + static_cast<long>(i + 8 + largo)), out);
                i += 8 + largo;
            } else {
                ++errores_;
                ++i;
            }
        } else if (buf_[i] == '$') {   // NMEA
            size_t fin = i;
            while (fin < buf_.size() && buf_[fin] != '\n') ++fin;
            if (fin >= buf_.size()) { if (buf_.size() - i > 200) { ++i; continue; } break; }
            std::string linea(buf_.begin() + static_cast<long>(i), buf_.begin() + static_cast<long>(fin));
            if (!linea.empty() && linea.back() == '\r') linea.pop_back();
            nmea(linea, out);
            i = fin + 1;
        } else {
            ++i;
        }
    }
    buf_.erase(buf_.begin(), buf_.begin() + static_cast<long>(i));
    return out;
}

void DecodificadorNeo::ubx(const std::vector<uint8_t>& m, std::vector<LecturaGps>& out) {
    if (m[2] != 0x01 || m[3] != 0x07 || u2(&m[4]) < 92) return;   // solo NAV-PVT
    const uint8_t* p = &m[6];
    LecturaGps l;
    l.iTOW = u4(p + 0);
    l.anio = u2(p + 4); l.mes = p[6]; l.dia = p[7]; l.hora = p[8]; l.minuto = p[9]; l.segundo = p[10];
    l.horaValida = (p[11] & 0x03) == 0x03;
    l.fix3d = p[20] == 3 && (p[21] & 0x01);
    l.satelites = p[23];
    l.lon = i4(p + 24) * 1e-7;
    l.lat = i4(p + 28) * 1e-7;
    l.precisionM = u4(p + 40) / 1000.0;
    l.velMs = i4(p + 60) / 1000.0;
    l.precisionVelMs = u4(p + 68) / 1000.0;
    out.push_back(l);
}

void DecodificadorNeo::nmea(const std::string& linea, std::vector<LecturaGps>& out) {
    const auto ast = linea.rfind('*');
    if (linea.size() < 7 || ast == std::string::npos || ast + 3 > linea.size()) return;
    const std::string cuerpo = linea.substr(1, ast - 1);
    if (sumaNmea(cuerpo) != std::strtol(linea.substr(ast + 1, 2).c_str(), nullptr, 16)) { ++errores_; return; }
    const auto f = partes(cuerpo);
    if (f[0].size() < 5) return;
    const std::string tipo = f[0].substr(2);
    if (tipo == "GGA" && f.size() > 8) {
        nmea_ = LecturaGps{};
        nmea_.deNmea = true;
        nmea_.lat = coordenada(f[2], f[3]);
        nmea_.lon = coordenada(f[4], f[5]);
        const int calidad = std::atoi(f[6].c_str());
        nmea_.satelites = std::atoi(f[7].c_str());
        nmea_.fix3d = calidad >= 1 && nmea_.satelites >= 4;
        nmea_.precisionM = f[8].empty() ? 999 : std::atof(f[8].c_str()) * 3;   // si no llega GST: estimado con el HDOP
        hayGga_ = true;
    } else if (tipo == "GST" && f.size() > 7 && hayGga_) {
        const double la = std::atof(f[6].c_str()), lo = std::atof(f[7].c_str());
        if (!f[6].empty() && !f[7].empty()) nmea_.precisionM = std::sqrt(la * la + lo * lo);
    } else if (tipo == "RMC" && f.size() > 9) {
        LecturaGps l = hayGga_ ? nmea_ : LecturaGps{};
        l.deNmea = true;
        const bool activo = f[2] == "A";
        l.lat = coordenada(f[3], f[4]);
        l.lon = coordenada(f[5], f[6]);
        l.velMs = std::atof(f[7].c_str()) * 0.514444;
        if (!hayGga_) { l.fix3d = activo; l.precisionM = activo ? 10 : 999; }
        l.fix3d = l.fix3d && activo;
        if (f[1].size() >= 6 && f[9].size() == 6) {
            l.hora = std::atoi(f[1].substr(0, 2).c_str()); l.minuto = std::atoi(f[1].substr(2, 2).c_str()); l.segundo = std::atoi(f[1].substr(4, 2).c_str());
            l.dia = std::atoi(f[9].substr(0, 2).c_str()); l.mes = std::atoi(f[9].substr(2, 2).c_str()); l.anio = 2000 + std::atoi(f[9].substr(4, 2).c_str());
            l.horaValida = activo;
        }
        hayGga_ = false;
        out.push_back(l);
    }
}

std::vector<uint8_t> armarNavPvt(const LecturaGps& l) {
    std::vector<uint8_t> c(92, 0);
    pon4(c, 0, l.iTOW);
    pon2(c, 4, static_cast<uint16_t>(l.anio));
    c[6] = static_cast<uint8_t>(l.mes); c[7] = static_cast<uint8_t>(l.dia); c[8] = static_cast<uint8_t>(l.hora);
    c[9] = static_cast<uint8_t>(l.minuto); c[10] = static_cast<uint8_t>(l.segundo);
    c[11] = l.horaValida ? 0x07 : 0x00;
    c[20] = l.fix3d ? 3 : 0;
    c[21] = l.fix3d ? 0x01 : 0x00;
    c[23] = static_cast<uint8_t>(l.satelites);
    pon4(c, 24, static_cast<uint32_t>(static_cast<int32_t>(std::lround(l.lon * 1e7))));
    pon4(c, 28, static_cast<uint32_t>(static_cast<int32_t>(std::lround(l.lat * 1e7))));
    pon4(c, 40, static_cast<uint32_t>(std::lround(l.precisionM * 1000)));
    pon4(c, 60, static_cast<uint32_t>(static_cast<int32_t>(std::lround(l.velMs * 1000))));
    pon4(c, 68, static_cast<uint32_t>(std::lround(l.precisionVelMs * 1000)));
    return marco(0x01, 0x07, c);
}

std::string armarNmea(const LecturaGps& l) {
    char hora[16], fecha[8], la[16], lo[16];
    std::snprintf(hora, sizeof hora, "%02d%02d%02d.00", l.hora, l.minuto, l.segundo);
    std::snprintf(fecha, sizeof fecha, "%02d%02d%02d", l.dia, l.mes, l.anio % 100);
    const double alat = std::fabs(l.lat), alon = std::fabs(l.lon);
    std::snprintf(la, sizeof la, "%02d%08.5f", static_cast<int>(alat), (alat - std::floor(alat)) * 60);
    std::snprintf(lo, sizeof lo, "%03d%08.5f", static_cast<int>(alon), (alon - std::floor(alon)) * 60);
    const char* ns = l.lat < 0 ? "S" : "N";
    const char* ew = l.lon < 0 ? "W" : "E";
    auto frase = [](const std::string& cuerpo) {
        char s[4];
        std::snprintf(s, sizeof s, "%02X", sumaNmea(cuerpo));
        return "$" + cuerpo + "*" + s + "\r\n";
    };
    std::ostringstream o;
    char b[200];
    std::snprintf(b, sizeof b, "GNGGA,%s,%s,%s,%s,%s,%d,%02d,%.1f,25.0,M,14.0,M,,", hora, la, ns, lo, ew, l.fix3d ? 1 : 0, l.satelites,
                  l.precisionM / 3);
    o << frase(b);
    const double e = l.precisionM / std::sqrt(2.0);
    std::snprintf(b, sizeof b, "GNGST,%s,%.1f,%.1f,%.1f,0.0,%.2f,%.2f,%.1f", hora, l.precisionM, l.precisionM, l.precisionM, e, e, l.precisionM * 1.5);
    o << frase(b);
    std::snprintf(b, sizeof b, "GNRMC,%s,%s,%s,%s,%s,%s,%.3f,0.0,%s,,,A", hora, l.fix3d ? "A" : "V", la, ns, lo, ew, l.velMs / 0.514444, fecha);
    o << frase(b);
    return o.str();
}

std::vector<std::vector<uint8_t>> configuracionNeo() {
    std::vector<std::vector<uint8_t>> r;
    // CFG-PRT: UART1 a 115200, 8N1, entra UBX+NMEA, sale solo UBX
    std::vector<uint8_t> prt(20, 0);
    prt[0] = 1;
    pon4(prt, 4, 0x000008D0);
    pon4(prt, 8, 115200);
    pon2(prt, 12, 0x0003);
    pon2(prt, 14, 0x0001);
    r.push_back(marco(0x06, 0x00, prt));
    // CFG-RATE: una lectura cada 100 ms (10 Hz), hora GPS
    std::vector<uint8_t> rate(6, 0);
    pon2(rate, 0, 100); pon2(rate, 2, 1); pon2(rate, 4, 1);
    r.push_back(marco(0x06, 0x08, rate));
    // CFG-NAV5: modelo dinámico «automotor» (4)
    std::vector<uint8_t> nav5(36, 0);
    pon2(nav5, 0, 0x0001);
    nav5[2] = 4;
    r.push_back(marco(0x06, 0x24, nav5));
    // CFG-MSG: NAV-PVT en cada lectura; las frases NMEA de fábrica, apagadas
    r.push_back(marco(0x06, 0x01, {0x01, 0x07, 0x01}));
    for (uint8_t id = 0x00; id <= 0x05; ++id) r.push_back(marco(0x06, 0x01, {0xF0, id, 0x00}));
    // CFG-CFG: guardar en el módulo (con la pila, queda para el próximo arranque)
    std::vector<uint8_t> cfg(13, 0);
    pon4(cfg, 4, 0x00001F1F);
    cfg[12] = 0x17;
    r.push_back(marco(0x06, 0x09, cfg));
    return r;
}

}  // namespace beto
