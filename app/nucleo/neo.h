// Mensajes del GPS u-blox NEO-M8N: lectura (para el programa) y armado (para el simulador y las pruebas).
// - UBX NAV-PVT (clase 0x01, id 0x07, 92 bytes): todo lo que usa Betómetro en un solo mensaje.
// - NMEA RMC + GGA + GST: por si el módulo no está configurado en UBX (de fábrica manda NMEA a 9600).
// C++ puro, sin Qt.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace beto {

struct LecturaGps {
    uint32_t iTOW = 0;            // ms de la semana GPS (hora del GPS de esta lectura); 0 en NMEA
    bool fix3d = false;           // posición 3D buena (fixType 3 y gnssFixOK) — mejora G6
    int satelites = 0;
    double lat = 0, lon = 0;      // grados
    double precisionM = 999;      // error horizontal estimado (hAcc) — el que se compara con el umbral
    double velMs = 0;             // velocidad sobre el suelo (Doppler)
    double precisionVelMs = 99;   // error de la velocidad (sAcc) — mejora G4
    bool horaValida = false;
    int anio = 0, mes = 0, dia = 0, hora = 0, minuto = 0, segundo = 0;   // UTC
    bool deNmea = false;
};

// Versión que informa el módulo (UBX-MON-VER). Sirve para saber si es un NEO-M8N original:
// los originales corren el firmware desde su memoria flash («EXT CORE 3.01»), hardware 00080000 y «MOD=NEO-M8N-0».
struct VersionNeo {
    std::string software;            // «EXT CORE 3.01 (107888)» o «ROM CORE 3.01 (107888)»
    std::string hardware;            // «00080000» = u-blox M8; «00040007» = u-blox 6 (NEO-6M)
    std::vector<std::string> extensiones;   // «FWVER=SPG 3.01», «PROTVER=18.00», «MOD=NEO-M8N-0»…
    std::string modelo() const;      // «NEO-M8N-0» o vacío
    std::string firmware() const;    // «SPG 3.01» o lo que haya
    bool conFlash() const { return software.rfind("EXT CORE", 0) == 0; }
    bool esM8() const { return hardware == "00080000"; }
    bool pareceOriginal() const { return esM8() && conFlash() && modelo().rfind("NEO-M8N", 0) == 0; }
};

class DecodificadorNeo {
public:
    // Se le pasan los bytes como llegan del puerto; devuelve las lecturas completas
    std::vector<LecturaGps> agregar(const uint8_t* datos, size_t n);
    int errores() const { return errores_; }   // mensajes con suma de control mala
    int acks() const { return acks_; }         // configuraciones aceptadas (UBX-ACK-ACK)
    int naks() const { return naks_; }         // configuraciones rechazadas (UBX-ACK-NAK)
    int navPvt() const { return navPvt_; }     // NAV-PVT recibidos
    int validos() const { return validos_; }   // mensajes sanos de cualquier tipo (para detectar la velocidad del puerto)
    const std::optional<VersionNeo>& version() const { return version_; }

private:
    void ubx(const std::vector<uint8_t>& m, std::vector<LecturaGps>& out);
    void nmea(const std::string& linea, std::vector<LecturaGps>& out);
    std::vector<uint8_t> buf_;
    // NMEA llega en varias frases por lectura: se juntan hasta el RMC
    LecturaGps nmea_;
    bool hayGga_ = false;
    int errores_ = 0, acks_ = 0, naks_ = 0, navPvt_ = 0, validos_ = 0;
    std::optional<VersionNeo> version_;
};

// Armado de mensajes (simulador y pruebas)
std::vector<uint8_t> armarNavPvt(const LecturaGps& l);
std::string armarNmea(const LecturaGps& l);   // GGA + GST + RMC
std::vector<uint8_t> armarMonVer(const VersionNeo& v);   // respuesta del simulador

// Configuración del módulo (hoja de datos NEO-M8 UBX-15031086 R14 y protocolo UBX-13003221):
// - El NEO-M8N llega a 5 Hz con GPS + GLONASS (dos sistemas a la vez) y a 10 Hz con un solo sistema.
//   De fábrica en Betómetro: GPS + GLONASS (+ QZSS y SBAS) a 5 Hz. Con `soloGps`: GPS (+ QZSS y SBAS) a 10 Hz.
// - Modelo «automotor», solo NAV-PVT en cada lectura, NMEA apagado y se guarda en el módulo.
// El cambio de velocidad del puerto (9600 → 115200) va aparte: `cambioA115200()`.
std::vector<uint8_t> cambioA115200();
std::vector<std::vector<uint8_t>> configuracionNeo(bool soloGps = false);
int hzDe(bool soloGps);
std::vector<uint8_t> pedirVersion();                       // UBX-MON-VER (pregunta)
std::vector<std::vector<uint8_t>> volverANmea();           // si el módulo no manda NAV-PVT: GGA, GST y RMC

}  // namespace beto
