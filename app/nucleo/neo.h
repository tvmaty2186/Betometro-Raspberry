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

class DecodificadorNeo {
public:
    // Se le pasan los bytes como llegan del puerto; devuelve las lecturas completas
    std::vector<LecturaGps> agregar(const uint8_t* datos, size_t n);
    int errores() const { return errores_; }   // mensajes con suma de control mala

private:
    void ubx(const std::vector<uint8_t>& m, std::vector<LecturaGps>& out);
    void nmea(const std::string& linea, std::vector<LecturaGps>& out);
    std::vector<uint8_t> buf_;
    // NMEA llega en varias frases por lectura: se juntan hasta el RMC
    LecturaGps nmea_;
    bool hayGga_ = false;
    int errores_ = 0;
};

// Armado de mensajes (simulador y pruebas)
std::vector<uint8_t> armarNavPvt(const LecturaGps& l);
std::string armarNmea(const LecturaGps& l);   // GGA + GST + RMC
// Configuración que se le manda al módulo al arrancar (E4): 115200 baudios, solo GPS + GLONASS,
// 10 Hz, modelo automotor y solo NAV-PVT. Lista de mensajes UBX-CFG listos para escribir.
std::vector<std::vector<uint8_t>> configuracionNeo();

}  // namespace beto
