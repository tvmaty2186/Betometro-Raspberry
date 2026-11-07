#include "almacen.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

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

}  // namespace

Almacen::Almacen(std::string carpeta, std::string nombre) : carpeta_(std::move(carpeta)), nombre_(std::move(nombre)) {}

Almacen::Lectura Almacen::cargar() const {
    Lectura r;
    const std::string base = archivo();
    // «.nuevo» primero: si está sano es lo último que se guardó (se cortó la luz antes de reemplazar).
    // Si se cortó mientras se escribía, está incompleto, no se puede leer y se pasa al siguiente.
    std::remove((base + ".nuevo.danado").c_str());   // de versiones anteriores: un guardado cortado no es un daño
    auto leer = [&](const std::string& ruta, Base& b) -> int {   // 1 bien, 0 no está, -1 roto
        std::string texto;
        if (!existe(ruta) || !leerTodo(ruta, texto)) return 0;
        try {
            Json j = Json::leer(texto);
            if (!j.esObjeto()) throw ErrorJson("no es un objeto");
            b = Base::desdeJson(j);
            return 1;
        } catch (const ErrorJson&) {
            return -1;
        }
    };
    for (const std::string& ruta : {base + ".nuevo", base, base + ".anterior"}) {
        const int l = leer(ruta, r.base);
        if (l == 0) continue;
        if (l < 0) {
            // «.nuevo» a medias es lo normal si se cortó la luz guardando: se borra sin avisar (05-3)
            if (ruta == base + ".nuevo") { std::remove(ruta.c_str()); continue; }
            r.habiaDanado = true;
            std::rename(ruta.c_str(), (ruta + ".danado").c_str());   // se guarda aparte, por las dudas
            continue;
        }
        r.origen = ruta;
        // Base sana pero sin carreras y la copia anterior con carreras: algo raro pasó, se usa la anterior (05-3)
        if (r.base.carreras.empty() && ruta != base + ".anterior") {
            Base ant;
            if (leer(base + ".anterior", ant) == 1 && !ant.carreras.empty()) {
                r.base = std::move(ant);
                r.origen = base + ".anterior";
                r.habiaDanado = true;
            }
        }
        return r;
    }
    return r;   // base vacía
}

bool Almacen::guardar(const Base& b, std::string* error) const {
    const std::string ruta = archivo(), nuevo = ruta + ".nuevo", anterior = ruta + ".anterior";
    if (!escribirYForzar(nuevo, b.aJson().escribir(), error)) {
        std::remove(nuevo.c_str());
        return false;
    }
    if (existe(ruta) && std::rename(ruta.c_str(), anterior.c_str()) != 0) {
        if (error) *error = "guardar copia anterior: " + std::string(std::strerror(errno));
        return false;
    }
    if (std::rename(nuevo.c_str(), ruta.c_str()) != 0) {
        if (error) *error = "reemplazar: " + std::string(std::strerror(errno));
        return false;   // al leer se usa «.nuevo», que está completo
    }
    forzarCarpeta(carpeta_);
    return true;
}

}  // namespace beto
