// Corre un caso (texto de órdenes, ver casos.js) con el motor en C++ y escribe lo que pasó.
// Es el mismo formato que correr-app.js, para comparar línea por línea.
// Uso: correr_motor < caso.txt > salida.txt
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "json.h"
#include "motor.h"

using namespace beto;

static std::string txt(double x) { return numeroComoJs(x); }
static std::string txt(const std::optional<double>& x) { return x ? numeroComoJs(*x) : "-"; }
static const char* tf(bool b) { return b ? "true" : "false"; }
static double num(const std::string& s) { return std::strtod(s.c_str(), nullptr); }

int main() {
    std::ios::sync_with_stdio(false);
    std::ostringstream out;
    std::optional<Motor> motor;
    bool terminarPendiente = false;
    int vueltasTotal = 0;

    auto oyente = [&](Evento tipo, const DatosEvento& d) {
        switch (tipo) {
            case Evento::Vuelta:
                out << "ev vuelta n=" << d.n << " medida=" << tf(d.medida) << " inicio=" << txt(d.inicio) << "\n";
                break;
            case Evento::Cerrada:
            case Evento::Interrumpida: {
                const VueltaMotor& v = *d.vuelta;
                out << "ev " << (tipo == Evento::Cerrada ? "cerrada" : "interrumpida") << " n=" << v.n
                    << " medida=" << tf(v.medida) << " idealMs=" << txt(v.idealMs) << " realMs=" << txt(v.realMs)
                    << " difMs=" << txt(v.difMs) << " fuente=" << v.fuente << " distM=" << txt(v.distM)
                    << " velPromKmh=" << txt(v.velPromKmh) << " velMaxKmh=" << txt(v.velMaxKmh)
                    << " factorVuelta=" << txt(v.factorVuelta) << "\n";
                if (tipo == Evento::Cerrada && vueltasTotal && v.n >= vueltasTotal) terminarPendiente = true;
                break;
            }
            case Evento::Ignorada: out << "ev ignorada\n"; break;
            case Evento::Descartada: out << "ev descartada t=" << txt(d.muestra->t) << "\n"; break;
            case Evento::Terminada: out << "ev terminada\n"; break;
        }
    };

    std::string linea;
    while (std::getline(std::cin, linea)) {
        if (linea.empty()) continue;
        std::istringstream ss(linea);
        std::vector<std::string> p;
        for (std::string x; ss >> x;) p.push_back(x);
        const std::string& op = p[0];
        if (op == "carrera") {
            DatosLargada c;
            c.tiempoIdealMs = num(p[1]);
            c.distanciaM = num(p[2]);
            c.patron = p[3];
            c.vueltasTotal = std::atoi(p[4].c_str());
            c.antirreboteMs = num(p[5]);
            c.umbralPrecisionM = num(p[6]);
            c.calibrar = p[7] == "1";
            c.congelarMs = num(p[8]);
            c.factorGps = num(p[9]);
            vueltasTotal = c.vueltasTotal;
            motor.emplace(c, oyente);
        } else if (op == "m") {
            Muestra m;
            m.t = num(p[1]);
            m.velMs = num(p[2]);
            m.precisionM = num(p[3]);
            bool ok = motor->muestra(m);   // primero la muestra: puede emitir 'descartada'
            out << "r muestra " << tf(ok) << "\n";
        } else if (op == "k") {
            ResultadoMarca r = motor->marcar(num(p[1]), p[2]);
            const char* nombre = r == ResultadoMarca::Largada ? "largada" : r == ResultadoMarca::Marca ? "marca" : "ignorada";
            out << "r marcar " << nombre << " factor=" << txt(motor->factor()) << "\n";
            if (terminarPendiente) {
                terminarPendiente = false;
                if (motor->enCurso()) motor->terminar(num(p[1]), false);
            }
        } else if (op == "x") {
            if (motor->enCurso()) motor->terminar(num(p[1]), true);
        } else if (op == "e") {
            Estado e = motor->estado(num(p[1]));
            out << "est t=" << p[1] << " enCurso=" << tf(e.enCurso) << " n=" << e.n << " medida=" << tf(e.medida)
                << " transMs=" << txt(e.transMs) << " restanteMs=" << txt(e.restanteMs)
                << " proxMedida=" << tf(e.proxMedida) << " sinGps=" << tf(e.sinGps) << " vActKmh=" << txt(e.vActKmh)
                << " vBaseKmh=" << txt(e.vBaseKmh) << " vNecKmh=" << txt(e.vNecKmh) << " congelada=" << tf(e.congelada)
                << " restanteM=" << txt(e.restanteM) << " deltaS=" << txt(e.deltaS) << "\n";
        } else if (op == "ca") {
            motor->ponerAntirrebote(num(p[1]));
        } else if (op == "cu") {
            motor->ponerUmbralPrecision(num(p[1]));
        } else if (op == "cc") {
            motor->ponerCalibrar(p[1] == "1");
        } else {
            std::cerr << "Orden desconocida: " << linea << "\n";
            return 2;
        }
    }
    std::cout << out.str();
    return 0;
}
