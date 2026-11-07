#include "almacen.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace beto {

namespace {

bool existe(const std::string& ruta) {
    struct stat st{};
    return ::stat(ruta.c_str(), &st) == 0;
}

bool leerTodo(const std::string& ruta, std::string& out) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

bool escribirYForzar(const std::string& ruta, const std::string& datos, std::string* error) {
    int fd = ::open(ruta.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) { if (error) *error = "abrir " + ruta + ": " + std::strerror(errno); return false; }
    size_t hecho = 0;
    while (hecho < datos.size()) {
        ssize_t n = ::write(fd, datos.data() + hecho, datos.size() - hecho);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (error) *error = "escribir " + ruta + ": " + std::strerror(errno);
            ::close(fd);
            return false;
        }
        hecho += static_cast<size_t>(n);
    }
    if (::fsync(fd) != 0) {
        if (error) *error = "fsync " + ruta + ": " + std::strerror(errno);
        ::close(fd);
        return false;
    }
    return ::close(fd) == 0;
}

void forzarCarpeta(const std::string& carpeta) {
    int fd = ::open(carpeta.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd >= 0) { ::fsync(fd); ::close(fd); }
}

std::string carpetaDe(const std::string& ruta) { return fs::path(ruta).parent_path().string(); }

bool crearCarpeta(const std::string& carpeta, std::string* error) {
    std::error_code ec;
    fs::create_directories(carpeta, ec);
    if (ec) { if (error) *error = "crear " + carpeta + ": " + ec.message(); return false; }
    return true;
}

// Escritura segura: «.nuevo» + fsync, el actual pasa a «.anterior», «.nuevo» pasa a ser el archivo
bool escribirSeguro(const std::string& ruta, const std::string& texto, std::string* error) {
    const std::string nuevo = ruta + ".nuevo", anterior = ruta + ".anterior";
    if (!escribirYForzar(nuevo, texto, error)) {
        std::remove(nuevo.c_str());
        return false;
    }
    if (existe(ruta) && std::rename(ruta.c_str(), anterior.c_str()) != 0) {
        if (error) *error = "guardar copia anterior de " + ruta + ": " + std::strerror(errno);
        return false;
    }
    if (std::rename(nuevo.c_str(), ruta.c_str()) != 0) {
        if (error) *error = "reemplazar " + ruta + ": " + std::strerror(errno);
        return false;   // al leer se usa «.nuevo», que está completo
    }
    forzarCarpeta(carpetaDe(ruta));
    return true;
}

// Lee un archivo probando, en orden, .nuevo, el archivo y .anterior. true si leyó alguno (un objeto JSON).
// Un archivo roto se aparta como «.danado» (danado = true); un «.nuevo» a medias es lo normal si se cortó
// la luz guardando: se borra sin avisar (05-3).
bool leerRobusto(const std::string& ruta, Json& out, bool& danado, std::string* origen = nullptr) {
    std::remove((ruta + ".nuevo.danado").c_str());   // de versiones anteriores
    auto leer = [&](const std::string& r) -> int {   // 1 bien, 0 no está, -1 roto
        std::string texto;
        if (!existe(r) || !leerTodo(r, texto)) return 0;
        try {
            Json j = Json::leer(texto);
            if (!j.esObjeto()) throw ErrorJson("no es un objeto");
            out = std::move(j);
            return 1;
        } catch (const ErrorJson&) {
            return -1;
        }
    };
    for (const std::string& r : {ruta + ".nuevo", ruta, ruta + ".anterior"}) {
        const int l = leer(r);
        if (l == 0) continue;
        if (l < 0) {
            if (r == ruta + ".nuevo") { std::remove(r.c_str()); continue; }
            danado = true;
            std::rename(r.c_str(), (r + ".danado").c_str());   // se guarda aparte, por las dudas
            continue;
        }
        if (origen) *origen = r;
        return true;
    }
    return false;
}

bool ordenCarreras(const Carrera& a, const Carrera& b) {
    return a.creada != b.creada ? a.creada < b.creada : a.id < b.id;
}

}  // namespace

Almacen::Almacen(std::string carpeta) : carpeta_(std::move(carpeta)) {}

std::string Almacen::nombreArchivo(const std::string& id) {
    std::string n;
    for (char c : id) {
        const bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_';
        n += ok ? c : '_';
    }
    if (n.empty()) n = "sin-id";
    return n + ".json";
}

bool Almacen::leerCarreras(const std::string& carpeta, std::vector<Carrera>& out, std::vector<std::string>& avisos) const {
    bool danado = false;
    std::error_code ec;
    if (!fs::is_directory(carpeta, ec)) return false;
    // Los archivos de la carpeta, agrupados por carrera (X.json, X.json.nuevo y X.json.anterior son la misma)
    std::set<std::string> bases;
    for (const auto& e : fs::directory_iterator(carpeta, ec)) {
        if (!e.is_regular_file(ec)) continue;
        std::string n = e.path().filename().string();
        for (const char* sufijo : {".json", ".json.nuevo", ".json.anterior"}) {
            const size_t ls = std::strlen(sufijo);
            if (n.size() > ls && n.compare(n.size() - ls, ls, sufijo) == 0) {
                bases.insert(n.substr(0, n.size() - ls + 5));   // queda «X.json»
                break;
            }
        }
    }
    for (const std::string& b : bases) {
        Json j;
        bool roto = false;
        if (!leerRobusto(carpeta + "/" + b, j, roto)) {
            if (roto) avisos.push_back("carrera ilegible en " + carpeta + "/" + b + " (quedó como .danado)");
        } else {
            if (roto) avisos.push_back("carrera dañada en " + carpeta + "/" + b + ": se usó la copia anterior");
            out.push_back(Carrera::desdeJson(j));
        }
        danado = danado || roto;
    }
    std::stable_sort(out.begin(), out.end(), ordenCarreras);
    return danado;
}

Almacen::Lectura Almacen::cargar() const {
    Lectura r;
    if (!migrar(r)) return r;   // la base vieja no se pudo migrar: se sigue con lo leído de ella (y se avisa)
    Json cfg;
    bool danado = false;
    std::string origen;
    if (leerRobusto(archivoConfig(), cfg, danado, &origen)) {
        r.base.config = std::move(cfg);
        r.origen = carpeta_;
        if (danado) r.avisos.push_back("config.json dañado: se usó " + origen);
    } else if (danado) {
        r.avisos.push_back("config.json ilegible (quedó como .danado): configuración de fábrica");
    }
    r.habiaDanado = danado;
    std::error_code ec;
    if (fs::is_directory(carpetaCarreras(), ec)) {
        r.origen = carpetaCarreras();
        if (leerCarreras(carpetaCarreras(), r.base.carreras, r.avisos)) r.habiaDanado = true;
    }
    return r;
}

// Migración de «betometro.json» (un solo archivo, hasta la 1.0.28). true si no hay nada que migrar o salió
// bien; false si falló (r.base queda con lo leído del archivo viejo y r.avisos dice por qué).
bool Almacen::migrar(Lectura& r) const {
    const std::string viejo = archivoViejo();
    if (!existe(viejo) && !existe(viejo + ".nuevo") && !existe(viejo + ".anterior")) return true;
    Json j;
    bool danado = false;
    std::string origen;
    if (!leerRobusto(viejo, j, danado, &origen)) {
        if (danado) r.avisos.push_back("betometro.json viejo ilegible: no se migró (quedó como .danado)");
        return true;   // no hay nada que migrar
    }
    Base vieja = Base::desdeJson(j);
    if (danado) r.avisos.push_back("betometro.json viejo dañado: se migró " + origen);
    // Base sana pero sin carreras y la copia anterior con carreras: algo raro pasó, se usa la anterior (05-3)
    if (vieja.carreras.empty() && origen != viejo + ".anterior") {
        std::string texto;
        try {
            if (leerTodo(viejo + ".anterior", texto)) {
                Base ant = Base::desdeJson(Json::leer(texto));
                if (!ant.carreras.empty()) { vieja = std::move(ant); r.avisos.push_back("betometro.json viejo sin carreras: se migró .anterior"); }
            }
        } catch (const ErrorJson&) {}
    }

    auto fallo = [&](const std::string& motivo) {
        r.avisos.push_back("No se pudo migrar betometro.json: " + motivo + ". Se sigue con el archivo viejo");
        r.base = vieja;
        r.origen = origen;
        r.habiaDanado = danado;
        return false;
    };
    // Todo se escribe en «carreras.nuevo/»; a «carreras/» recién cuando se comprobó. Si ya existe «carreras/»
    // (una migración anterior falló y después se guardó algo), se suman las que falten sin pisar las que están.
    const std::string nuevo = carpeta_ + "/carreras.nuevo";
    std::error_code ec;
    const bool fusionar = fs::is_directory(carpetaCarreras(), ec);
    fs::remove_all(nuevo, ec);
    std::string err;
    if (!crearCarpeta(nuevo, &err)) return fallo(err);
    Json cfgActual;
    bool rotoCfg = false;
    const bool escribirCfg = !fusionar || !leerRobusto(archivoConfig(), cfgActual, rotoCfg);
    if (escribirCfg && !guardarConfig(vieja.config, &err)) return fallo(err);
    for (const Carrera& c : vieja.carreras)
        if (!escribirSeguro(nuevo + "/" + nombreArchivo(c.id), c.aJson().escribir(), &err)) return fallo(err);

    // Se vuelve a leer todo y se compara con lo original: tiene que ser igual, carrera por carrera
    std::vector<Carrera> leidas;
    std::vector<std::string> avisos;
    if (leerCarreras(nuevo, leidas, avisos)) return fallo("algún archivo nuevo no se pudo leer");
    std::map<std::string, const Carrera*> porId;
    for (const Carrera& c : leidas) porId[c.id] = &c;
    std::set<std::string> ids;
    for (const Carrera& c : vieja.carreras) {
        ids.insert(c.id);
        auto it = porId.find(c.id);
        if (it == porId.end()) return fallo("falta la carrera " + c.id);
        if (it->second->aJson() != c.aJson()) return fallo("la carrera " + c.id + " no se leyó igual");
    }
    if (ids.size() != leidas.size()) return fallo("no coincide la cantidad de carreras");
    if (escribirCfg) {
        Json cfg;
        bool roto = false;
        if (!leerRobusto(archivoConfig(), cfg, roto) || cfg != vieja.config) return fallo("la configuración no se leyó igual");
    }

    // Recién ahora pasa a «carreras/» y el archivo viejo queda como «.migrado» (nunca se borra)
    if (!fusionar) {
        if (std::rename(nuevo.c_str(), carpetaCarreras().c_str()) != 0) return fallo(std::string("renombrar carreras.nuevo: ") + std::strerror(errno));
    } else {
        for (const auto& e : fs::directory_iterator(nuevo, ec)) {
            const std::string destino = carpetaCarreras() + "/" + e.path().filename().string();
            if (!existe(destino) && std::rename(e.path().c_str(), destino.c_str()) != 0) return fallo("mover " + destino + ": " + std::strerror(errno));
        }
        fs::remove_all(nuevo, ec);
    }
    forzarCarpeta(carpetaCarreras());
    forzarCarpeta(carpeta_);
    for (const std::string& v : {viejo, viejo + ".nuevo", viejo + ".anterior"})
        if (existe(v)) std::rename(v.c_str(), (v + ".migrado").c_str());
    forzarCarpeta(carpeta_);
    r.migrado = true;
    return true;
}

bool Almacen::guardarConfig(const Json& config, std::string* error) const {
    if (!crearCarpeta(carpeta_, error)) return false;
    return escribirSeguro(archivoConfig(), config.escribir(), error);
}

bool Almacen::guardarCarrera(const Carrera& c, std::string* error) const {
    if (!crearCarpeta(carpetaCarreras(), error)) return false;
    return escribirSeguro(archivoCarrera(c.id), c.aJson().escribir(), error);
}

bool Almacen::borrarCarrera(const std::string& id, std::string* error) const {
    const std::string ruta = archivoCarrera(id);
    std::remove((ruta + ".nuevo").c_str());
    std::remove((ruta + ".anterior").c_str());
    if (!existe(ruta)) return true;   // nunca se guardó (p. ej. una de muestra)
    if (!crearCarpeta(carpetaPapelera(), error)) return false;
    const std::string destino = carpetaPapelera() + "/" + nombreArchivo(id);
    if (std::rename(ruta.c_str(), destino.c_str()) != 0) {
        if (error) *error = "mover a la papelera " + ruta + ": " + std::strerror(errno);
        return false;
    }
    // La fecha del archivo pasa a ser la de hoy: así la papelera guarda 30 días desde que se borró
    std::error_code ec;
    fs::last_write_time(destino, fs::file_time_type::clock::now(), ec);
    forzarCarpeta(carpetaCarreras());
    forzarCarpeta(carpetaPapelera());
    return true;
}

void Almacen::vaciarPapelera(int dias) const {
    std::error_code ec;
    if (!fs::is_directory(carpetaPapelera(), ec)) return;
    const auto ahora = fs::file_time_type::clock::now();
    for (const auto& e : fs::directory_iterator(carpetaPapelera(), ec)) {
        if (!e.is_regular_file(ec)) continue;
        const auto t = fs::last_write_time(e.path(), ec);
        if (ec) continue;
        if (ahora - t > std::chrono::hours(24) * dias) fs::remove(e.path(), ec);
    }
}

}  // namespace beto
