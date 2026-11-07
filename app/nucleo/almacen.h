// Almacén: la base en la partición de datos, un archivo por carrera (auditoría 05, sección 4, opción B).
//
//   <datos>/config.json               lo guardado de la configuración (sin mezclar con fábrica)
//   <datos>/carreras/<id>.json        cada carrera, exactamente como una «carrera» del formato de la app
//   <datos>/carreras/papelera/        las carreras borradas (se vacían a los 30 días)
//   <datos>/betometro.json.migrado    la base vieja de un solo archivo, después de migrarla (nunca se borra)
//
// Escritura segura de cada archivo (para que un corte de corriente no rompa nada):
//   1. Se escribe todo en «X.nuevo» y se fuerza a disco (fsync).
//   2. El archivo actual pasa a ser «X.anterior» (la copia anterior).
//   3. «.nuevo» pasa a ser «X» y se fuerza la carpeta a disco.
// Al leer cada archivo se prueba, en orden: .nuevo, X, .anterior. Se usa el primero que esté sano; uno roto
// se aparta como «.danado». Un archivo roto no afecta a los demás.
//
// Migración (al arrancar): si todavía está «betometro.json» (versiones hasta la 1.0.28), se escribe config y
// cada carrera en «carreras.nuevo/», se vuelve a leer todo y se compara con lo original; recién entonces se
// renombra a «carreras/» y «betometro.json» pasa a «.migrado». Si se corta a mitad, la próxima vez repite.
#pragma once

#include <string>
#include <vector>

#include "modelo.h"

namespace beto {

class Almacen {
public:
    explicit Almacen(std::string carpeta);

    struct Lectura {
        Base base;
        std::string origen;                // «carreras/» si había algo; vacío si no había nada
        bool habiaDanado = false;          // algún archivo estaba roto (se guarda aparte como «.danado»)
        bool migrado = false;              // se pasó la base vieja de un archivo a un archivo por carrera
        std::vector<std::string> avisos;   // para el reporte de errores (qWarning en Datos)
    };
    // Lee todo (y antes migra la base vieja si hace falta). Las carreras quedan por fecha de creación.
    Lectura cargar() const;

    // Cada uno devuelve false si no pudo escribir (y deja lo anterior como estaba)
    bool guardarConfig(const Json& config, std::string* error = nullptr) const;
    bool guardarCarrera(const Carrera& c, std::string* error = nullptr) const;
    // Mueve el archivo de la carrera a la papelera (no se pierde hasta 30 días después)
    bool borrarCarrera(const std::string& id, std::string* error = nullptr) const;
    void vaciarPapelera(int dias = 30) const;

    const std::string& carpeta() const { return carpeta_; }
    std::string archivoConfig() const { return carpeta_ + "/config.json"; }
    std::string carpetaCarreras() const { return carpeta_ + "/carreras"; }
    std::string carpetaPapelera() const { return carpetaCarreras() + "/papelera"; }
    std::string archivoCarrera(const std::string& id) const { return carpetaCarreras() + "/" + nombreArchivo(id); }
    std::string archivoViejo() const { return carpeta_ + "/betometro.json"; }   // la base de un solo archivo
    static std::string nombreArchivo(const std::string& id);                     // «<id>.json», sin caracteres raros

private:
    // Lee «carreras» de una carpeta (cada archivo con su .nuevo y .anterior). Devuelve si había alguno roto.
    bool leerCarreras(const std::string& carpeta, std::vector<Carrera>& out, std::vector<std::string>& avisos) const;
    bool migrar(Lectura& l) const;

    std::string carpeta_;
};

}  // namespace beto
