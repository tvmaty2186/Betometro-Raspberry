// Simulador del GPS NEO-M8N: crea un puerto serie virtual y manda por ahí los mismos mensajes que el módulo
// (UBX NAV-PVT, o NMEA con --nmea), como si el auto diera vueltas a un circuito.
// Solo para desarrollo: el programa lee el puerto igual que el UART real de la Raspberry.
//
// Uso: simular_neo [--enlace /tmp/betometro-gps] [--hz 10] [--kmh 48] [--largo 3200] [--nmea]
//                  [--mala 0.05] [--corte 20] [--segundos N]
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
    double hz = 10, kmh = 48, largo = 3200, mala = 0, corte = 0, segundos = 0;
    bool nmea = false;
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
        else { std::fprintf(stderr, "Opción desconocida: %s\n", a.c_str()); return 2; }
    }
    const int maestro = posix_openpt(O_RDWR | O_NOCTTY);
    if (maestro < 0 || grantpt(maestro) || unlockpt(maestro)) { std::perror("posix_openpt"); return 1; }
    const char* esclavo = ptsname(maestro);
    termios t{};
    tcgetattr(maestro, &t);
    cfmakeraw(&t);
    tcsetattr(maestro, TCSANOW, &t);
    unlink(enlace.c_str());
    if (symlink(esclavo, enlace.c_str()) != 0) std::perror("symlink");
    std::printf("GPS simulado en %s (%s): %s a %.0f Hz, %.0f km/h, vuelta de %.0f m\n", enlace.c_str(), esclavo,
                nmea ? "NMEA" : "UBX NAV-PVT", hz, kmh, largo);
    std::fflush(stdout);

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
                const auto m = beto::armarNavPvt(l);
                if (write(maestro, m.data(), m.size()) < 0) break;
            }
        }
        iTOW += static_cast<uint32_t>(1000 / hz);
        s += paso;
        proximo += std::chrono::microseconds(static_cast<long>(1e6 / hz));
        std::this_thread::sleep_until(proximo);
    }
    unlink(enlace.c_str());
    return 0;
}
