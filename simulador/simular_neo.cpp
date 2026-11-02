// Simulador del GPS NEO-M8N: crea un puerto serie virtual y manda por ahí los mismos mensajes que el módulo
// (UBX NAV-PVT, o NMEA con --nmea), como si el auto diera vueltas a un circuito.
// Solo para desarrollo: el programa lee el puerto igual que el UART real de la Raspberry.
//
// Uso: simular_neo [--enlace /tmp/betometro-gps] [--hz 5] [--kmh 48] [--largo 3200] [--nmea]
//                  [--mala 0.05] [--corte 20] [--segundos N] [--modulo m8|7|6]
//   --modulo     se hace pasar por un NEO-7M (NAV-PVT de 84 bytes) o un NEO-6M (solo NMEA)
//   --mala P     probabilidad de una lectura con mala precisión (30 a 60 m)
//   --corte S    cada vuelta, S segundos sin señal (como entre árboles)
// Después: betometro --gps /tmp/betometro-gps
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>
#include <string>
#include <thread>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "neo.h"

int main(int argc, char** argv) {
    std::string enlace = "/tmp/betometro-gps";
    double hz = 5, kmh = 48, largo = 3200, mala = 0, corte = 0, segundos = 0;
    bool nmea = false;
    std::string modulo = "m8";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto sig = [&] { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (a == "--enlace") enlace = sig();
        else if (a == "--hz") hz = std::atof(sig().c_str());
        else if (a == "--kmh") kmh = std::atof(sig().c_str());
        else if (a == "--largo") largo = std::atof(sig().c_str());
        else if (a == "--mala") mala = std::atof(sig().c_str());
        else if (a == "--corte") corte = std::atof(sig().c_str());
        else if (a == "--segundos") segundos = std::atof(sig().c_str());
        else if (a == "--nmea") nmea = true;
        else if (a == "--modulo") modulo = sig();
        else { std::fprintf(stderr, "Opción desconocida: %s\n", a.c_str()); return 2; }
    }
    const int maestro = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (maestro < 0 || grantpt(maestro) || unlockpt(maestro)) { std::perror("posix_openpt"); return 1; }
    const char* esclavo = ptsname(maestro);
    termios t{};
    tcgetattr(maestro, &t);
    cfmakeraw(&t);
    tcsetattr(maestro, TCSANOW, &t);
    unlink(enlace.c_str());
    if (symlink(esclavo, enlace.c_str()) != 0) std::perror("symlink");
    std::printf("GPS simulado (%s) en %s (%s): %s a %.0f Hz, %.0f km/h, vuelta de %.0f m\n", modulo.c_str(), enlace.c_str(), esclavo,
                nmea ? "NMEA" : "UBX NAV-PVT", hz, kmh, largo);
    std::fflush(stdout);

    // Como un NEO-M8N original: contesta la versión (MON-VER) y acepta la configuración (ACK-ACK)
    beto::VersionNeo version{"EXT CORE 3.01 (107888)", "00080000",
                             {"ROM BASE 2.01 (75331)", "FWVER=SPG 3.01", "PROTVER=18.00", "MOD=NEO-M8N-0", "GPS;GLO;GAL;BDS", "SBAS;IMES;QZSS"}};
    if (modulo == "7") version = beto::VersionNeo{"1.00 (59842)", "00070000", {"PROTVER 14.00", "GPS;SBAS;GLO;QZSS"}};
    if (modulo == "6") { version = beto::VersionNeo{"7.03 (45969)", "00040007", {}}; nmea = true; }
    std::vector<uint8_t> entrada;
    auto atender = [&] {
        uint8_t b[512];
        ssize_t n;
        while ((n = read(maestro, b, sizeof b)) > 0) entrada.insert(entrada.end(), b, b + n);
        size_t i = 0;
        while (i + 8 <= entrada.size()) {
            if (entrada[i] != 0xB5 || entrada[i + 1] != 0x62) { ++i; continue; }
            const size_t largo = entrada[i + 4] | (entrada[i + 5] << 8);
            if (i + 8 + largo > entrada.size()) break;
            const uint8_t clase = entrada[i + 2], id = entrada[i + 3];
            std::vector<uint8_t> r;
            if (clase == 0x0A && id == 0x04) r = beto::armarMonVer(version);
            else if (clase == 0x06) {
                r = {0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, clase, id};
                uint8_t a = 0, c = 0;
                for (size_t k = 2; k < r.size(); ++k) { a = static_cast<uint8_t>(a + r[k]); c = static_cast<uint8_t>(c + a); }
                r.push_back(a); r.push_back(c);
            }
            if (!r.empty() && write(maestro, r.data(), r.size()) < 0) break;
            i += 8 + largo;
        }
        entrada.erase(entrada.begin(), entrada.begin() + static_cast<long>(i));
    };
    std::mt19937 azar(7);
    std::uniform_real_distribution<double> u(0, 1);
    const double paso = 1.0 / hz, base = kmh / 3.6, R = largo / (2 * M_PI);
    double recorrido = 0, s = 0;
    uint32_t iTOW = static_cast<uint32_t>((std::time(nullptr) % 604800) * 1000);
    iTOW -= iTOW % static_cast<uint32_t>(1000 / hz);
    auto proximo = std::chrono::steady_clock::now();
    while (segundos <= 0 || s < segundos) {
        const double v = std::max(0.0, base * (1 + 0.12 * std::sin(s / 9) + 0.06 * std::sin(s / 2.7)) + (u(azar) - 0.5) * 0.3);
        recorrido += v * paso;
        const double a = recorrido / R, enVuelta = std::fmod(recorrido, largo);
        const bool sinSenal = corte > 0 && enVuelta < base * corte;
        beto::LecturaGps l;
        l.iTOW = iTOW;
        const std::time_t ahora = std::time(nullptr);
        std::tm utc{};
        gmtime_r(&ahora, &utc);
        l.anio = utc.tm_year + 1900; l.mes = utc.tm_mon + 1; l.dia = utc.tm_mday;
        l.hora = utc.tm_hour; l.minuto = utc.tm_min; l.segundo = utc.tm_sec;
        l.horaValida = true;
        l.fix3d = true;
        l.satelites = 9 + static_cast<int>(u(azar) * 4);
        l.lat = -34.6 + R * 0.8 * std::sin(a) / 110540.0;
        l.lon = -58.4 + R * 1.4 * std::cos(a) / (111320.0 * std::cos(34.6 * M_PI / 180));
        l.velMs = v;
        l.precisionM = u(azar) < mala ? 30 + u(azar) * 30 : 1.2 + u(azar) * 1.5;
        l.precisionVelMs = 0.1 + u(azar) * 0.2;
        if (!sinSenal) {
            if (nmea) {
                const std::string txt = beto::armarNmea(l);
                if (write(maestro, txt.data(), txt.size()) < 0) break;
            } else {
                auto m = beto::armarNavPvt(l);
                if (modulo == "7") {   // u-blox 7: NAV-PVT de 84 bytes
                    m.erase(m.begin() + 6 + 84, m.end() - 2);
                    m[4] = 84;
                    uint8_t a = 0, b = 0;
                    for (size_t k = 2; k < m.size() - 2; ++k) { a = static_cast<uint8_t>(a + m[k]); b = static_cast<uint8_t>(b + a); }
                    m[m.size() - 2] = a; m[m.size() - 1] = b;
                }
                if (write(maestro, m.data(), m.size()) < 0) break;
            }
        }
        atender();
        iTOW += static_cast<uint32_t>(1000 / hz);
        s += paso;
        proximo += std::chrono::microseconds(static_cast<long>(1e6 / hz));
        std::this_thread::sleep_until(proximo);
    }
    unlink(enlace.c_str());
    return 0;
}
