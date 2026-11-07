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
        gpsInfo_["ultimoDato"] = QDateTime::currentMSecsSinceEpoch();   // para saber si el módulo sigue hablando
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
    if (!camara_) return;
    connect(camara_, &Camara::primerCuadroListo, this, [this] { asociarVideo(); });
    // Se cortó sola en carrera (no por Terminar ni a mano): se vuelve a grabar en otro archivo. Con poco lugar, no.
    reintentoVideo_.setInterval(10000);
    connect(camara_, &Camara::grabandoChanged, this, [this] {
        if (enCurso() && !camara_->grabando() && !camara_->falla().isEmpty() && !camara_->falla().startsWith(QLatin1String("Poco lugar"))
            && !reintentoVideo_.isActive())
            reintentoVideo_.start();
    });
    connect(&reintentoVideo_, &QTimer::timeout, this, [this] {
        if (!enCurso() || camara_->grabando() || !datos_->config()["camara"].aBool(false)) { reintentoVideo_.stop(); return; }
        camara_->detectar();   // por si la volvieron a enchufar
        if (!camaraActiva()) return;   // todavía no está: se prueba en 10 s
        empezarVideo();
        if (camara_->grabando()) { videoPendiente_ = true; asociarVideo(); reintentoVideo_.stop(); }
    });
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
                                      {"girar", datos_->config()["camaraGirar"].aBool(false)},
                                      {"espejo", datos_->config()["camaraEspejo"].aBool(false)}});
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
    recorrido_ = id == QLatin1String("recorrido");
    if (recorrido_) {   // recorrido libre: se guarda recién al largar
        practicaSinGuardar_ = true;
        carreraId_.clear();
        c.practica = true;
        c.nombre = "Recorrido libre";
        c.tiempoIdealMs = kRecorridoIdealMs;
        c.distanciaM = 0;
        c.patron = "todas";
    } else if (id.isEmpty()) {   // práctica: se guarda recién al largar
        practicaSinGuardar_ = true;
        carreraId_.clear();
        c.practica = true;
        c.nombre = "Práctica";
        c.tiempoIdealMs = cfg["tiempoIdealMs"].aEntero(240000);
        c.distanciaM = cfg["distanciaM"].aEntero(3200);
        c.patron = cfg["patron"].aTexto("alt1");
    } else {
        const Carrera* g = datos_->buscar(id);
        if (!g) return;
        practicaSinGuardar_ = false;
        carreraId_ = id;
        c = *g;
        recorrido_ = esRecorrido(c);
    }
    datosLargada_ = datosLargada(c, cfg);
    largada_ = c.proximaLargada();
    if (auto* sim = qobject_cast<GpsSimulado*>(fuenteGps_)) {
        if (recorrido_) sim->ponerVuelta(3000, 240000);   // en la PC: andar a 45 km/h
        else sim->ponerVuelta(static_cast<double>(c.distanciaM), static_cast<double>(c.tiempoIdealMs));
    }

    const QLocale ar(QLocale::Spanish, QLocale::Argentina);
    auto tiempo = [](int64_t ms) {
        int64_t dec = ms / 100, s = dec / 10;
        return QString::number(s / 60) + ":" + QString::number(s % 60).rightJustified(2, '0') + ":" + QString::number(dec % 10);
    };
    info_ = {{"nombre", QString::fromStdString(c.nombre)}, {"practica", c.practica}, {"recorrido", recorrido_},
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
            carreraId_ = recorrido_ ? datos_->crearRecorrido() : datos_->crearPractica();
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
        marcas_ = Json::lista();
        paradoMs_ = 0;
        ultMuestraT_ = 0;
        ultTrazaT_ = ultMarcaT_ = -1e12;
        ultParcialT_ = t;
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
    if (recorrido_) {   // recorrido libre: la marca no corta nada, queda anotada con dónde y a cuánto iba
        if (t - ultMarcaT_ < 1000) { emit marcado(QStringLiteral("ignorada")); return QStringLiteral("ignorada"); }
        ultMarcaT_ = t;
        const Estado e = motor_->estado(t);
        Json m = Json::objeto();
        m["n"] = static_cast<int>(marcas_.elementos().size()) + 1;
        m["ms"] = static_cast<int64_t>(std::llround(e.transMs));
        m["m"] = redondearJs(std::max(0.0, -e.restanteM));
        m["kmh"] = e.vActKmh ? fijarDecimalesJs(*e.vActKmh, 1) : 0.0;
        m["fuente"] = fuente.toStdString();
        marcas_.agregar(std::move(m));
        if (!carreraId_.isEmpty()) datos_->ponerExtra(carreraId_, "marcas", marcas_);
        emit marcado(QStringLiteral("marca"));
        if (datos_->config()["clicMarca"].aBool(true)) emit sonido(QStringLiteral("clic"));
        refrescar();
        return QStringLiteral("marca");
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
            if (recorrido_) break;   // sin voz
            // sin «Última vuelta»: no hay cantidad fija (en la carrera pueden agregar vueltas)
            if (datos_->config()["avisoInicio"].aBool(true))
                if (auto f = anuncioVuelta(d.n, d.medida, 0)) emit frase(QString::fromStdString(*f));
            break;
        }
        case Evento::Cerrada:
        case Evento::Interrumpida: {
            const VueltaMotor& v = *d.vuelta;
            Vuelta g = Vuelta::desdeMotor(v, largada_, &traza_);
            if (recorrido_) { g.interrumpida = false; g.medida = false; }   // terminar es lo normal en un recorrido
            if (!carreraId_.isEmpty()) datos_->agregarVuelta(carreraId_, g);
            if (recorrido_ && !carreraId_.isEmpty()) datos_->quitarExtra(carreraId_, "enCurso");   // ya está la definitiva
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
    gpsInfo_["kmh"] = m.velMs * 3.6;                                  // velocidad de ahora (para el inicio)
    gpsInfo_["kmhCuando"] = QDateTime::currentMSecsSinceEpoch();
    emit gpsChanged();

    ultimaMuestra_ = m;
    if (!motor_) return;
    const bool valida = motor_->muestra(m);
    if (motor_->enCurso()) {
        if (valida) velMaxVuelta_ = std::max(velMaxVuelta_, m.velMs * 3.6);
        if (recorrido_) {
            if (ultMuestraT_ > 0 && m.velMs * 3.6 < 3) paradoMs_ += std::min(2000.0, m.t - ultMuestraT_);
            ultMuestraT_ = m.t;
            if (m.t - ultTrazaT_ < 1000) return;   // 1 por segundo: un recorrido largo entra entero
            ultTrazaT_ = m.t;
        }
        const Estado e = motor_->estado(m.t);   // como grabarTraza() de la app
        traza_.agregar(m, datosLargada_->distanciaM, e.restanteM);
        if (recorrido_ && m.t - ultParcialT_ >= 60000) guardarParcial(m.t);
    }
}

// Recorrido libre: cada minuto se guarda lo andado como extra["enCurso"]. Si se corta la luz antes de Terminar,
// al prender queda como la vuelta del recorrido, marcada «interrumpida» («se cortó» en el informe).
void Crono::guardarParcial(double t) {
    ultParcialT_ = t;
    if (carreraId_.isEmpty() || !motor_) return;
    const Estado e = motor_->estado(t);
    VueltaMotor v;
    v.n = e.n > 0 ? e.n : 1;
    v.medida = false;
    v.realMs = e.transMs;
    v.fuente = "corte";
    v.fin = QDateTime::currentMSecsSinceEpoch();
    v.distM = std::max(0.0, -e.restanteM);
    if (e.transMs > 1000) v.velPromKmh = *v.distM / (e.transMs / 1000) * 3.6;
    v.velMaxKmh = velMaxVuelta_;
    v.interrumpida = true;
    datos_->ponerExtra(carreraId_, "enCurso", Vuelta::desdeMotor(v, largada_, &traza_).aJson());
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
               {"recorrido", recorrido_},
               {"distM", recorrido_ ? std::max(0.0, -e.restanteM) : 0.0},
               {"promKmh", recorrido_ && e.transMs > 1000 ? std::max(0.0, -e.restanteM) / (e.transMs / 1000) * 3.6 : 0.0},
               {"paradoMs", paradoMs_},
               {"marcas", static_cast<int>(marcas_.elementos().size())},
               {"tiempoIdealMs", datosLargada_->tiempoIdealMs}, {"distanciaM", datosLargada_->distanciaM}};
    emit estadoChanged();
}

void Crono::voz() {
    if (!enCurso() || recorrido_) return;   // el recorrido libre va en silencio
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
