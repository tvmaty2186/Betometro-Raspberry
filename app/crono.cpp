#include "crono.h"

#include <QDateTime>
#include <QFileInfo>

#include "camara.h"

#include <QLocale>
#include <cmath>

using namespace beto;

static QVariant opt(const std::optional<double>& v) { return v ? QVariant(*v) : QVariant(); }

Crono::Crono(Datos* datos, FuenteGps* gps, QObject* padre) : QObject(padre), datos_(datos), fuenteGps_(gps) {
    connect(fuenteGps_, &FuenteGps::muestra, this, &Crono::alMuestra);
    connect(fuenteGps_, &FuenteGps::modulo, this, [this](const QString& modelo, const QString& fw, bool original, int hz) {
        gpsInfo_["modulo"] = modelo;
        gpsInfo_["firmware"] = fw;
        gpsInfo_["original"] = original;
        gpsInfo_["hzConfig"] = hz;
        emit gpsChanged();
    });
    connect(fuenteGps_, &FuenteGps::datos, this, [this](int satelites, const QDateTime& utc) {
        gpsInfo_["satelites"] = satelites;
        gpsInfo_["hora"] = utc.isValid() ? utc.toLocalTime().toString(QStringLiteral("HH:mm:ss")) : QString();
        gpsInfo_["ok"] = gpsInfo_.contains("hz");
        emit gpsChanged();
    });
    reloj_.setTimerType(Qt::PreciseTimer);
    reloj_.setInterval(50);   // pantalla: 20 veces por segundo (la voz va aparte, cada 15 ms)
    connect(&reloj_, &QTimer::timeout, this, &Crono::refrescar);
    reloj_.start();
    relojVoz_.setTimerType(Qt::PreciseTimer);
    relojVoz_.setInterval(15);   // como la app
    connect(&relojVoz_, &QTimer::timeout, this, &Crono::voz);
    relojVoz_.start();
}

// ---------- Video de la carrera ----------
void Crono::ponerCamara(Camara* c) {
    camara_ = c;
    if (camara_) connect(camara_, &Camara::primerCuadroListo, this, [this] { asociarVideo(); });
}

bool Crono::camaraActiva() const { return camara_ && camara_->hay() && datos_->config()["camara"].aBool(false); }

void Crono::empezarVideo() {
    if (!camaraActiva() || camara_->grabando()) return;
    camara_->empezar(QStringLiteral("carrera-") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
}

// Anota en la carrera en qué momento del video fue la largada. Con eso el informe sabe
// dónde empieza cada vuelta (las vueltas de una largada van una atrás de la otra).
void Crono::asociarVideo() {
    if (!videoPendiente_ || !camara_ || !camara_->grabando() || camara_->primerCuadro() <= 0 || carreraId_.isEmpty()) return;
    videoPendiente_ = false;
    datos_->agregarVideo(carreraId_, {{"archivo", QFileInfo(camara_->archivo()).fileName()}, {"largada", largada_},
                                      {"desdeLargadaMs", qRound64(largadaT_ - camara_->primerCuadro())},
                                      {"girar", datos_->config()["camaraGirar"].aBool(false)}});
}

void Crono::grabarAMano() {
    if (!camaraActiva()) return;
    if (camara_->grabando()) { camara_->detener(0); return; }
    empezarVideo();
    if (enCurso()) { videoPendiente_ = true; asociarVideo(); }
}

void Crono::preparar(const QString& id) {
    if (enCurso()) return;
    motor_.reset();
    ultima_.clear();
    emit ultimaChanged();
    const Json& cfg = datos_->config();
    Carrera c;
    if (id.isEmpty()) {   // práctica: se guarda recién al largar
        practicaSinGuardar_ = true;
        carreraId_.clear();
        c.practica = true;
        c.nombre = "Práctica libre";
        c.tiempoIdealMs = cfg["tiempoIdealMs"].aEntero(240000);
        c.distanciaM = cfg["distanciaM"].aEntero(3200);
        c.patron = cfg["patron"].aTexto("alt1");
    } else {
        const Carrera* g = datos_->buscar(id);
        if (!g) return;
        practicaSinGuardar_ = false;
        carreraId_ = id;
        c = *g;
    }
    datosLargada_ = datosLargada(c, cfg);
    largada_ = c.proximaLargada();
    if (auto* sim = qobject_cast<GpsSimulado*>(fuenteGps_)) sim->ponerVuelta(static_cast<double>(c.distanciaM), static_cast<double>(c.tiempoIdealMs));

    const QLocale ar(QLocale::Spanish, QLocale::Argentina);
    auto tiempo = [](int64_t ms) {
        int64_t dec = ms / 100, s = dec / 10;
        return QString::number(s / 60) + ":" + QString::number(s % 60).rightJustified(2, '0') + "," + QString::number(dec % 10);
    };
    info_ = {{"nombre", QString::fromStdString(c.nombre)}, {"practica", c.practica},
             {"tiempoIdealMs", static_cast<qlonglong>(c.tiempoIdealMs)}, {"distanciaM", static_cast<qlonglong>(c.distanciaM)},
             {"ideal", tiempo(c.tiempoIdealMs)},
             {"distancia", ar.toString(static_cast<qlonglong>(c.distanciaM))}, {"patron", QString::fromStdString(c.patron)}};
    emit carreraChanged();
    refrescar();
    if (datos_->config()["camaraInicio"].aTexto("abrir") == "abrir") empezarVideo();
}

QString Crono::marcar(const QString& fuente, double edadMs) {
    const double t = reloj_ms_() - edadMs;   // la hora de la marca, lo primero de todo
    if (!datosLargada_) return QString();
    if (!motor_ || !motor_->enCurso()) {
        // LARGADA: motor nuevo para esta largada
        if (practicaSinGuardar_) {
            carreraId_ = datos_->crearPractica();
            practicaSinGuardar_ = false;
            if (const Carrera* c = datos_->buscar(carreraId_)) info_["nombre"] = QString::fromStdString(c->nombre);
            largada_ = 1;
            emit carreraChanged();
        } else if (const Carrera* c = datos_->buscar(carreraId_)) {
            largada_ = c->proximaLargada();
        }
        DatosLargada d = *datosLargada_;
        const Json& cfg = datos_->config();
        d.antirreboteMs = cfg["antirreboteMs"].aNumero(3000);
        d.umbralPrecisionM = cfg["umbralPrecisionM"].aNumero(20);
        d.calibrar = cfg["calibrar"].aBool(true);
        motor_ = std::make_unique<Motor>(d, [this](Evento e, const DatosEvento& x) { alEvento(e, x); });
        vueltasGuardadas_ = 0;
        ultima_.clear();
        emit ultimaChanged();
        if (datos_->config()["clicMarca"].aBool(true)) emit sonido(QStringLiteral("clic"));
        motor_->marcar(t, fuente.toStdString());
        // la distancia arranca justo en la largada: la última lectura con la hora de la marca
        if (ultimaMuestra_) {
            Muestra m = *ultimaMuestra_;
            m.t = t;
            motor_->muestra(m);
        }
        emit enCursoChanged();
        emit marcado(QStringLiteral("largada"));
        // Video: con «a mano» no arranca solo; si ya estaba grabando, se anota la largada
        largadaT_ = t;
        if (datos_->config()["camaraInicio"].aTexto("abrir") != "mano") empezarVideo();
        videoPendiente_ = camara_ && camara_->grabando();
        asociarVideo();
        refrescar();
        return QStringLiteral("largada");
    }
    const ResultadoMarca r = motor_->marcar(t, fuente.toStdString());
    if (r == ResultadoMarca::Ignorada) {
        emit aviso(QStringLiteral("Marca ignorada (antirrebote)"));
        emit marcado(QStringLiteral("ignorada"));
        return QStringLiteral("ignorada");
    }
    emit marcado(QStringLiteral("marca"));
    if (datos_->config()["clicMarca"].aBool(true)) emit sonido(QStringLiteral("clic"));
    refrescar();
    return QStringLiteral("marca");
}

void Crono::terminar() { if (enCurso()) terminarCon(QStringLiteral("Carrera terminada"), true); }

void Crono::salir() {
    if (enCurso()) return;
    if (camara_ && camara_->grabando()) camara_->detener(0);
    motor_.reset();
    datosLargada_.reset();
    emit carreraChanged();
}

void Crono::terminarCon(const QString& motivo, bool aMano) {
    motor_->terminar(reloj_ms_(), aMano);
    // El video sigue unos segundos más (para ver la llegada) y para solo
    if (camara_ && camara_->grabando()) camara_->detener(static_cast<int>(datos_->config()["camaraFinS"].aNumero(10)));
    emit enCursoChanged();
    emit terminada(motivo, vueltasGuardadas_, carreraId_);
    refrescar();
}

void Crono::alEvento(Evento e, const DatosEvento& d) {
    switch (e) {
        case Evento::Vuelta: {
            velMaxVuelta_ = 0;
            traza_.empezar(d.inicio);
            programador_.reiniciarVuelta();
            // sin «Última vuelta»: no hay cantidad fija (en la carrera pueden agregar vueltas)
            if (datos_->config()["avisoInicio"].aBool(true))
                if (auto f = anuncioVuelta(d.n, d.medida, 0)) emit frase(QString::fromStdString(*f));
            break;
        }
        case Evento::Cerrada:
        case Evento::Interrumpida: {
            const VueltaMotor& v = *d.vuelta;
            Vuelta g = Vuelta::desdeMotor(v, largada_, &traza_);
            if (!carreraId_.isEmpty()) datos_->agregarVuelta(carreraId_, g);
            if (e == Evento::Cerrada) {
                ++vueltasGuardadas_;
                const double dif = v.difMs ? *v.difMs : 0;
                ultima_ = {{"n", v.n}, {"realMs", v.realMs}, {"difMs", dif}, {"medida", v.medida}};
                emit ultimaChanged();
            }
            break;
        }
        default:
            break;
    }
}

void Crono::alMuestra(const Muestra& m) {
    // lecturas por segundo (último segundo)
    tiemposGps_.push_back(m.t);
    while (!tiemposGps_.empty() && m.t - tiemposGps_.front() >= 1000) tiemposGps_.erase(tiemposGps_.begin());
    gpsInfo_["hz"] = static_cast<int>(tiemposGps_.size());
    gpsInfo_["precision"] = m.precisionM;
    gpsInfo_["ok"] = true;
    emit gpsChanged();

    ultimaMuestra_ = m;
    if (!motor_) return;
    const bool valida = motor_->muestra(m);
    if (motor_->enCurso()) {
        if (valida) velMaxVuelta_ = std::max(velMaxVuelta_, m.velMs * 3.6);
        const Estado e = motor_->estado(m.t);   // como grabarTraza() de la app
        traza_.agregar(m, datosLargada_->distanciaM, e.restanteM);
    }
}

void Crono::refrescar() {
    if (!datosLargada_) return;
    const double t = reloj_ms_();
    Estado e;
    if (motor_) {
        e = motor_->estado(t);
    } else {   // antes de la primera largada: vista en reposo
        Motor reposo(*datosLargada_);
        if (ultimaMuestra_) reposo.muestra(*ultimaMuestra_);
        e = reposo.estado(t);
    }
    estado_ = {{"enCurso", e.enCurso}, {"n", e.n}, {"medida", e.medida}, {"transMs", e.transMs},
               {"restanteMs", e.restanteMs}, {"proxMedida", e.proxMedida}, {"sinGps", e.sinGps},
               {"vActKmh", opt(e.vActKmh)}, {"vBaseKmh", opt(e.vBaseKmh)}, {"vNecKmh", opt(e.vNecKmh)},
               {"congelada", e.congelada}, {"restanteM", e.restanteM}, {"deltaS", opt(e.deltaS)},
               {"velMaxVuelta", velMaxVuelta_},
               {"tiempoIdealMs", datosLargada_->tiempoIdealMs}, {"distanciaM", datosLargada_->distanciaM}};
    emit estadoChanged();
}

void Crono::voz() {
    if (!enCurso()) return;
    const Json& cfg = datos_->config();
    AjustesVoz a;
    a.cuentaN = static_cast<int>(cfg["cuentaN"].aNumero(20));
    a.enVoz = cfg["modoCuenta"].aTexto("voz") == "voz";
    a.anticipoMs = cfg["anticipoMs"].aNumero(40);
    a.pitidoCero = cfg["pitidoCero"].aBool(false);
    a.pitidoCadaS = cfg["pitidoCadaS"].aNumero(0);
    for (const Sonido& s : programador_.tic(motor_->estado(reloj_ms_()), a)) {
        switch (s.tipo) {
            case Sonido::Tipo::Numero: emit sonido(QStringLiteral("n%1").arg(s.numero, 2, 10, QLatin1Char('0'))); break;
            case Sonido::Tipo::Cero: emit sonido(QStringLiteral("n00")); break;
            case Sonido::Tipo::PitidoCorto: emit sonido(QStringLiteral("corto")); break;
            case Sonido::Tipo::PitidoLargo: emit sonido(QStringLiteral("largo")); break;
            case Sonido::Tipo::Tic: emit sonido(QStringLiteral("tic")); break;
            case Sonido::Tipo::Frase: emit frase(QString::fromStdString(s.frase)); break;
        }
    }
}
