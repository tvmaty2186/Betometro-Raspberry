// Betómetro para Raspberry Pi: arranque del programa.
//
// Opciones (para desarrollo y capturas):
//   --pantalla NOMBRE        pantalla con la que arranca (inicio, carreras, …)
//   --orientacion h|v        horizontal o vertical
//   --tema NOMBRE            avispon, rally, sport, cabina
//   --fisica                 simula la pantalla real (720×1280 de pie) y gira la interfaz
//   --captura ARCHIVO.png    saca una captura y sale
//   --datos CARPETA          dónde está la base (en la Raspberry: /datos)
//   --muestra                carga carreras de ejemplo (no toca la base guardada hasta que se cambie algo);
//                            con BETO_MUESTRA_LARGA=1 suma una de 60 vueltas y una práctica de 40
//                            (--prueba largar-60 en la pantalla carrera: va por la vuelta 60)
//   --carrera NOMBRE         abre la ficha de esa carrera
//   --captura-tele ARCHIVO   con --captura (o sola): también saca la ventana de la tele (modo tele)
//   --prueba tele-qr | tele-carreras | tele-remoto   modo tele con el código del celu, con la lista de carreras,
//                            o listo para probar el celu como control (las dos últimas: Paraná también con video)
//   --prueba cortado         Paraná con el video en dos partes (se desenchufó la cámara): el aviso del informe
//   --prueba prueba-completa con --pantalla prueba: arranca la Prueba completa (capturas; BETO_PRUEBA_RAPIDA=1 la acorta)
//   --prueba grafico | mapa  en el informe: el gráfico o el mapa a pantalla completa, con zoom y un punto tocado
//                            (1.0.30). Con BETO_MUESTRA_RECORRIDO=1 --muestra suma un recorrido libre de 40 km
//   BETO_FIJO=1              (variable) para las capturas comparadas: GPS simulado sin azar, hora «12:00», sin
//                            animación en el inicio. La fecha la fija faketime (herramientas/comparar-capturas.sh)
#include <QCommandLineParser>
#include <QDirIterator>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QSocketNotifier>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

// Al apagar (systemctl stop / poweroff) llega SIGTERM: se cierra prolijo, así el reporte de
// errores no lo toma como una caída. La señal se pasa por un socket (seguro dentro de una señal).
static int gAvisoSalir[2] = {-1, -1};
static void alTerminar(int) { char c = 1; if (::write(gAvisoSalir[0], &c, 1) < 0) {} }

#include "contexto.h"
#include "crono.h"
#include "datos.h"
#include "audio.h"
#include "gps.h"
#include "salud.h"
#include "vigilante.h"
#include "clima.h"
#include "qr.h"
#include "sistema.h"
#include "actualizador.h"
#include "tienda.h"
#include "camara.h"
#include "pulsador_gpio.h"
#include "reportes.h"
#include "simulacion.h"
#include "prueba_completa.h"
#include "servidor.h"
#include "control_remoto.h"
#include "wifi.h"
#include "hdmi.h"
#include "drive.h"
#include "imagen_video.h"
#include "video_gst.h"

#include <QStandardPaths>
#include <sys/timex.h>
#include <time.h>

static void cargarFuentes() {
    QDirIterator it(QStringLiteral(":/recursos/fuentes"), {QStringLiteral("*.ttf"), QStringLiteral("*.woff2"), QStringLiteral("*.woff")});
    while (it.hasNext()) {
        const QString archivo = it.next();
        if (QFontDatabase::addApplicationFont(archivo) < 0) qWarning("No pude cargar la fuente %s", qPrintable(archivo));
    }
}

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QSocketNotifier* avisoSalir = nullptr;
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, gAvisoSalir) == 0) {
        avisoSalir = new QSocketNotifier(gAvisoSalir[1], QSocketNotifier::Read, &app);
        QObject::connect(avisoSalir, &QSocketNotifier::activated, &app, [] { char c; if (::read(gAvisoSalir[1], &c, 1) < 0) {} QCoreApplication::quit(); });
        std::signal(SIGTERM, alTerminar);
        std::signal(SIGINT, alTerminar);
    }
    app.setApplicationName(QStringLiteral("Betómetro"));
    app.setApplicationVersion(QStringLiteral(BETO_VERSION));

    QCommandLineParser op;
    op.addHelpOption();
    QCommandLineOption oPantalla(QStringLiteral("pantalla"), QStringLiteral("Pantalla inicial"), QStringLiteral("nombre"), QStringLiteral("inicio"));
    QCommandLineOption oOrient(QStringLiteral("orientacion"), QStringLiteral("h o v"), QStringLiteral("o"));
    QCommandLineOption oTema(QStringLiteral("tema"), QStringLiteral("Tema"), QStringLiteral("tema"));
    QCommandLineOption oFisica(QStringLiteral("fisica"), QStringLiteral("Pantalla real de pie, interfaz girada"));
    QCommandLineOption oCaptura(QStringLiteral("captura"), QStringLiteral("Guardar captura y salir"), QStringLiteral("archivo"));
    QCommandLineOption oDatos(QStringLiteral("datos"), QStringLiteral("Carpeta de la base"), QStringLiteral("carpeta"));
    QCommandLineOption oMuestra(QStringLiteral("muestra"), QStringLiteral("Carreras de ejemplo"));
    QCommandLineOption oPrueba(QStringLiteral("prueba"), QStringLiteral("Abrir algo al empezar (capturas)"), QStringLiteral("que"));
    QCommandLineOption oSinGps(QStringLiteral("sin-gps"), QStringLiteral("Sin GPS (solo cronómetro)"));
    QCommandLineOption oGps(QStringLiteral("gps"), QStringLiteral("Puerto del GPS («auto»: USB y si no los pines; o una ruta como /dev/ttyAMA0 o /tmp/betometro-gps)"), QStringLiteral("puerto"));
    QCommandLineOption oEspera(QStringLiteral("espera"), QStringLiteral("ms antes de la captura"), QStringLiteral("ms"), QStringLiteral("1200"));
    QCommandLineOption oCarrera(QStringLiteral("carrera"), QStringLiteral("Carrera a abrir"), QStringLiteral("nombre"));
    QCommandLineOption oCapturaTele(QStringLiteral("captura-tele"), QStringLiteral("Guardar captura de la ventana de la tele y salir"), QStringLiteral("archivo"));
    op.addOptions({oPantalla, oOrient, oTema, oFisica, oCaptura, oCapturaTele, oDatos, oMuestra, oCarrera, oPrueba, oSinGps, oGps, oEspera});
    op.process(app);

    cargarFuentes();

    Contexto contexto;
    if (op.isSet(oOrient)) contexto.setHorizontal(op.value(oOrient) != QStringLiteral("v"));
    if (op.isSet(oTema)) contexto.setTema(op.value(oTema));
    contexto.setFisica(op.isSet(oFisica));
    contexto.setPantallaInicial(op.value(oPantalla));
    contexto.setPrueba(op.value(oPrueba));
    const bool fijo = qEnvironmentVariableIsSet("BETO_FIJO");
    contexto.setFijo(fijo);

    QString carpeta = op.value(oDatos);
    if (carpeta.isEmpty()) carpeta = qEnvironmentVariable("BETO_DATOS");
    if (carpeta.isEmpty()) carpeta = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // La lista de complementos de GStreamer (video): se guarda con los datos. En la Pi el servicio no puede escribir
    // en /root (ProtectHome) y GStreamer la volvía a armar cada vez: la pantalla de Cámara quedaba trabada unos 8 s
    // la primera vez (vigilante, 08/11/2026). Los ayudantes (betometro-camara) la heredan.
    if (!qEnvironmentVariableIsSet("GST_REGISTRY_1_0") && !carpeta.isEmpty())
        qputenv("GST_REGISTRY_1_0", QFile::encodeName(carpeta + QStringLiteral("/gstreamer-registro.bin")));
    Datos datos(carpeta);
    // Reporte de errores: desde acá se anota todo lo que falle (también las pantallas QML)
    Reportes reportes(carpeta);
    if (qEnvironmentVariableIsSet("BETO_RASPBERRY")) reportes.instalar();
    if (op.isSet(oMuestra)) datos.cargarMuestra();
    // --prueba tele-carreras / tele-remoto: una segunda carrera con video (Paraná, con el mismo
    // videos/muestra.mkv), para probar «Carreras». Ojo: queda guardado en la carpeta de --datos.
    if (op.isSet(oMuestra) && (op.value(oPrueba) == QStringLiteral("tele-carreras") || op.value(oPrueba) == QStringLiteral("tele-remoto"))
        && QFile::exists(datos.carpetaVideos() + QStringLiteral("/muestra.mkv"))) {
        for (const auto& c : datos.base().carreras)
            if (c.nombre == "Paraná" && datos.videos(QString::fromStdString(c.id)).isEmpty()) {
                datos.agregarVideo(QString::fromStdString(c.id), {{"archivo", "muestra.mkv"}, {"largada", 1}, {"desdeLargadaMs", 3000}});
                break;
            }
    }
    // --prueba cortado: un segundo video en la misma largada (la cámara se cortó): el informe avisa (07-3.3)
    if (op.isSet(oMuestra) && op.value(oPrueba) == QStringLiteral("cortado"))
        for (const auto& c : datos.base().carreras)
            if (c.nombre == "Paraná") {
                datos.agregarVideo(QString::fromStdString(c.id), {{"archivo", "muestra.mkv"}, {"largada", 1}, {"desdeLargadaMs", 3000}});
                datos.agregarVideo(QString::fromStdString(c.id), {{"archivo", "muestra-2.mkv"}, {"largada", 1}, {"desdeLargadaMs", -240000}});
                break;
            }
    // --prueba prueba-completa con la cámara de prueba: la cámara prendida, para que la prueba la grabe (capturas)
    if (op.value(oPrueba) == QStringLiteral("prueba-completa") && qEnvironmentVariableIsSet("BETO_CAMARA_PRUEBA")) datos.ponerCfg(QStringLiteral("camara"), true);
    if (op.isSet(oCarrera)) {
        for (const auto& c : datos.base().carreras)
            if (QString::fromStdString(c.nombre) == op.value(oCarrera)) contexto.setIdInicial(QString::fromStdString(c.id));
    }

    // GPS: en la PC, el simulado (el real por UART llega en E4)
    // GPS: el puerto que se diga (UART real o el simulador del NEO-M8N); si no, en la PC uno interno sencillo
    FuenteGps sinGps;
    QString puertoGps = op.value(oGps);
    if (puertoGps.isEmpty()) puertoGps = qEnvironmentVariable("BETO_GPS");
    FuenteGps* gps = &sinGps;
    if (!op.isSet(oSinGps)) {
        if (!puertoGps.isEmpty()) gps = new GpsNeo(puertoGps, datos.cfg(QStringLiteral("gpsSoloGps")).toBool(), datos.cfg(QStringLiteral("gpsModulo")).toString(), &app);
        else gps = new GpsSimulado(&app, fijo);
    }
    Crono crono(&datos, gps);
    // Hora del equipo desde el GPS (11B-1.2, Matías 07/11): en el autódromo casi nunca hay internet, y la Pi
    // arranca con la hora en que se apagó. Si internet (NTP) no la puso y la del GPS difiere más de 2 s, se
    // corrige el reloj. El cronómetro no se mueve: usa el reloj monotónico. Sin GPS ni internet no se bloquea nada.
    if (qEnvironmentVariableIsSet("BETO_RASPBERRY")) {
        // De dónde salió la hora, para el Chequeo previo: internet (NTP), GPS, o ninguna (la del último apagado)
        static bool horaDelGps = false;
        auto revisarHora = [&contexto] {
            timex tx{};
            const bool ntp = ::adjtimex(&tx) >= 0 && !(tx.status & STA_UNSYNC);
            contexto.setHoraFuente(ntp ? QStringLiteral("internet") : horaDelGps ? QStringLiteral("gps") : QString());
        };
        revisarHora();
        auto* relojHora = new QTimer(&app);
        QObject::connect(relojHora, &QTimer::timeout, &app, revisarHora);
        relojHora->start(10000);
        QObject::connect(gps, &FuenteGps::datos, &app, [&datos, &crono, revisarHora](int, const QDateTime& utc) {
            if (!utc.isValid() || utc.date().year() < 2025) return;
            // Con la carrera en curso no se toca la hora: las vueltas ya cerradas quedarían con otra fecha que las
            // nuevas. Se corrige con la primera lectura después de Terminar (auditoría 12, 08/11/2026)
            if (crono.enCurso()) return;
            timex tx{};
            if (::adjtimex(&tx) >= 0 && !(tx.status & STA_UNSYNC)) return;   // internet ya la puso: manda NTP
            const qint64 dif = utc.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch();
            // Una vez puesta, solo se vuelve a corregir si se corrió más de 5 s (la lectura del GPS trae segundos enteros
            // y llega ~0,1 s tarde: con 2 s a veces se corregía dos veces seguidas; visto en los reportes de la 1.0.29)
            if (std::llabs(dif) < (horaDelGps ? 5000 : 2000)) { if (!horaDelGps) { horaDelGps = true; revisarHora(); } return; }
            const qint64 ms = utc.toMSecsSinceEpoch() + 100;   // la lectura llega ~0,1 s después del segundo
            timespec ts{static_cast<time_t>(ms / 1000), static_cast<long>((ms % 1000) * 1000000)};
            if (::clock_settime(CLOCK_REALTIME, &ts) == 0) {
                qWarning("Hora puesta desde el GPS: %s (el equipo estaba %+.1f s corrido)",
                         qPrintable(utc.toLocalTime().toString(QStringLiteral("dd/MM/yyyy HH:mm:ss"))), dif / 1000.0);
                emit datos.carrerasChanged();   // «hoy» cambió: las listas de carreras se vuelven a armar
                horaDelGps = true;
                revisarHora();
            }
        });
    }
    // Clima: pronóstico de Open-Meteo por la ubicación del GPS o por una localidad
    Clima clima(carpeta);
    QObject::connect(gps, &FuenteGps::muestra, &clima, [&clima](const beto::Muestra& m) {
        if (m.lat && m.lon && m.precisionM < 50) clima.ponerPosicion(*m.lat, *m.lon);
    });
    Camara camara(&datos, carpeta);
    crono.ponerCamara(&camara);
    Actualizador actualizador(carpeta);
    Tienda tienda(carpeta);   // el catálogo de herramientas (Betometro-tienda)
    contexto.setVersionNueva(actualizador.versionNueva());
    QObject::connect(&actualizador, &Actualizador::cambio, &contexto, [&] { contexto.setVersionNueva(actualizador.versionNueva()); });
    if (op.value(oPrueba).contains(QStringLiteral("nueva"))) contexto.setVersionNueva(QStringLiteral("1.0.21"));   // muestra «Actualizar»

    Audio audio;
    audio.ponerVolumen(datos.cfg(QStringLiteral("volumen")).toDouble());
    audio.ponerVoz(datos.cfg(QStringLiteral("voz")).toString());
    QObject::connect(&datos, &Datos::cfgChanged, &audio, [&] {
        audio.ponerVolumen(datos.cfg(QStringLiteral("volumen")).toDouble());
        audio.ponerVoz(datos.cfg(QStringLiteral("voz")).toString());
    });
    QObject::connect(&crono, &Crono::sonido, &audio, &Audio::reproducir);
    QObject::connect(&crono, &Crono::frase, &audio, [&](const QString& f) { audio.reproducir(Audio::archivoDeFrase(f)); });
    Wifi wifi;
    auto ponerPropia = [&] {
        auto siONada = [&](const char* k) { const QVariant v = datos.cfg(QLatin1String(k)); return !v.isValid() || v.toBool(); };
        wifi.ponerPropia(siONada("wifiPropia"), datos.cfg(QStringLiteral("numeroAuto")).toString());
        wifi.ponerRedes(siONada("wifiRedes"));   // apagado: no se conecta a ninguna red, solo la propia
    };
    ponerPropia();
    QObject::connect(&datos, &Datos::cfgChanged, &wifi, ponerPropia);
    // Apenas conecta el wifi, se busca si hay versión nueva (para el «· Actualizar» del inicio)
    QObject::connect(&wifi, &Wifi::conectado, &actualizador, [&] { QTimer::singleShot(5000, &actualizador, &Actualizador::buscar); });
    // Pulsador por cable (GPIO17, pin 11 a GND pin 9), con la pull-up interna. Solo en la Raspberry.
    PulsadorGpio* pulsador = qEnvironmentVariableIsSet("BETO_RASPBERRY") ? new PulsadorGpio(17, &app) : nullptr;
    if (pulsador && !pulsador->activo()) qWarning("Pulsador: %s", qPrintable(pulsador->error()));
    // «Betómetro en la red»: en la Raspberry en el puerto 80 (http://IP); en la PC en el 8080
    Servidor servidor(&datos, qEnvironmentVariableIsSet("BETO_RASPBERRY") ? 80 : 8080);
    servidor.ponerReportes(&reportes);
    // El celu como control remoto del modo tele (/control, docs/video.md)
    ControlRemoto controlRemoto;
    servidor.ponerControl(&controlRemoto);
    // Google Drive: sube informes y videos solo, con wifi, sin carrera y sin la cámara grabando (docs/google-drive.md)
    Drive drive(&datos, carpeta, op.value(oPrueba));
    servidor.ponerDrive(&drive);
    drive.ponerBorrador([&camara](const QString& archivo) { return camara.borrarVideo(archivo); });
    auto condicionesDrive = [&] { drive.ponerCondiciones(wifi.conectada(), wifi.propia(), crono.enCurso() || camara.grabando() || camara.convirtiendo()); };
    condicionesDrive();
    QObject::connect(&wifi, &Wifi::cambio, &drive, condicionesDrive);
    QObject::connect(&crono, &Crono::enCursoChanged, &drive, condicionesDrive);
    QObject::connect(&camara, &Camara::grabandoChanged, &drive, condicionesDrive);
    QObject::connect(&camara, &Camara::convirtiendoChanged, &drive, condicionesDrive);
    // Pasar los videos a MP4: nunca en carrera
    camara.ponerEnCarrera(crono.enCurso());
    QObject::connect(&crono, &Crono::enCursoChanged, &camara, [&] { camara.ponerEnCarrera(crono.enCurso()); });
    // Prendido si la opción «Mostrar informes en la red» está prendida y no hay carrera
    auto prenderServidor = [&] {
        const QVariant v = datos.cfg(QStringLiteral("informesRed"));
        servidor.prender(!crono.enCurso() && (!v.isValid() || v.isNull() || v.toBool()));
        reportes.ponerOcupado(crono.enCurso());  // los reportes no se mandan en carrera
        clima.ponerCarrera(crono.enCurso());     // ni el clima pide el nombre del lugar (11A-1.1)
    };
    prenderServidor();
    QObject::connect(&crono, &Crono::enCursoChanged, &servidor, prenderServidor);
    QObject::connect(&datos, &Datos::cfgChanged, &servidor, prenderServidor);
    Sistema sistema;
    // Modo tele: si hay una tele en el HDMI (docs/video.md). --prueba tele-reiniciar: como si estuviera
    // enchufada pero Qt no la viera
    Hdmi hdmi(op.value(oPrueba) == QStringLiteral("tele-reiniciar"));
    // Salud del equipo: tensión baja, calor, vigilante y procesador al máximo en carrera
    Salud salud;
    QObject::connect(&salud, &Salud::aviso, &app, [&](const QString& texto) {
        const int n = crono.estado().value(QStringLiteral("n")).toInt();
        qWarning("%s", qPrintable(texto + (crono.enCurso() && n > 0 ? QStringLiteral(" · en la vuelta %1").arg(n) : QString())));
    });
    salud.modoCarrera(crono.enCurso());
    QObject::connect(&crono, &Crono::enCursoChanged, &salud, [&] { salud.modoCarrera(crono.enCurso()); });
    QObject::connect(&crono, &Crono::enCursoChanged, &wifi, [&] { wifi.modoCarrera(crono.enCurso()); });
    // Vigilante de memoria y trabas: si algo se pone pesado, manda un reporte con quién lo está usando
    Vigilante vigilante(carpeta, &reportes);
    vigilante.enCarrera = [&] { return crono.enCurso(); };
    vigilante.tension = [&] {
        if (!salud.sabe()) return QStringLiteral("no se sabe");
        QString t = salud.tensionBaja() ? QStringLiteral("BAJA ahora") : salud.huboTensionBaja() ? QStringLiteral("bajó desde que prendió") : QStringLiteral("bien");
        if (salud.calor()) t += QStringLiteral(" · se frena por calor");
        return t;
    };
    vigilante.andando = [&] {
        QStringList a;
        if (crono.enCurso()) a << QStringLiteral("carrera en curso");
        if (camara.grabando()) a << QStringLiteral("cámara grabando");
        if (camara.convirtiendo()) a << QStringLiteral("pasando un video a MP4");
        if (drive.subiendo()) a << QStringLiteral("Drive: ") + drive.texto();
        return a;
    };
    // Después de «Despejar memoria», vuelve al Chequeo previo
    if (vigilante.despejeAntes() >= 0 && !op.isSet(oPantalla)) contexto.setPantallaInicial(QStringLiteral("chequeo"));
    // Brillo: al prender y cada vez que cambia en Ajustes
    auto ponerBrillo = [&] {
        const QVariant b = datos.cfg(QStringLiteral("brillo"));
        sistema.brillo(b.isValid() && !b.isNull() ? b.toInt() : 100);
    };
    ponerBrillo();
    QObject::connect(&datos, &Datos::cfgChanged, &sistema, ponerBrillo);
    if (!op.isSet(oOrient)) contexto.setHorizontal(datos.cfg(QStringLiteral("orientacion")).toString() != QStringLiteral("vertical"));
    contexto.ponerCarpeta(carpeta);   // la foto propia del inicio (inicio-h.jpg / inicio-v.jpg)
    contexto.setTema(datos.cfg(QStringLiteral("tema")).toString());
    if (op.isSet(oTema)) contexto.setTema(op.value(oTema));
    contexto.setSol(datos.cfg(QStringLiteral("modoSol")).toString() == QStringLiteral("si") || op.value(oPrueba) == QStringLiteral("sol"));
    QObject::connect(&datos, &Datos::cfgChanged, &contexto, [&] {
        contexto.setSol(datos.cfg(QStringLiteral("modoSol")).toString() == QStringLiteral("si"));
        contexto.setTema(datos.cfg(QStringLiteral("tema")).toString());
    });

    // El video se dibuja como imagen: el VideoOutput de Qt queda negro en la Pi 3 (app/imagen_video.h)
    qmlRegisterType<ImagenVideo>("Betometro.Video", 1, 0, "ImagenVideo");
    // Y se reproduce con GStreamer, que usa el descompresor de video de la Raspberry (app/video_gst.h)
    qmlRegisterType<VideoGst>("Betometro.Video", 1, 0, "VideoGst");
    Simulacion simulacion(carpeta, &audio, &reportes);   // antes que el motor de QML: se destruye después
    // Prueba completa (Ajustes → Prueba completa, o desde el celu en /prueba): la Pi se revisa sola y manda el resultado
    PruebaCompleta::Piezas piezas;
    piezas.carpetaDatos = carpeta; piezas.contexto = &contexto; piezas.datos = &datos; piezas.crono = &crono; piezas.audio = &audio;
    piezas.camara = &camara; piezas.reportes = &reportes; piezas.wifi = &wifi; piezas.drive = &drive; piezas.simulacion = &simulacion;
    piezas.pulsador = pulsador;
    PruebaCompleta pruebaCompleta(piezas);
    servidor.ponerPrueba(&pruebaCompleta);
    servidor.ponerContexto(&contexto);   // Personalizar: tema y foto del inicio
    QQmlApplicationEngine motor;
    pruebaCompleta.ponerMotor(&motor);
    motor.addImageProvider(QStringLiteral("qr"), new ProveedorQr);
    motor.rootContext()->setContextProperty(QStringLiteral("Wifi"), &wifi);
    motor.rootContext()->setContextProperty(QStringLiteral("Servidor"), &servidor);
    motor.rootContext()->setContextProperty(QStringLiteral("ControlRemoto"), &controlRemoto);
    motor.rootContext()->setContextProperty(QStringLiteral("Pulsador"), pulsador);   // null en la PC
    motor.rootContext()->setContextProperty(QStringLiteral("Audio"), &audio);
    motor.rootContext()->setContextProperty(QStringLiteral("Sistema"), &sistema);
    motor.rootContext()->setContextProperty(QStringLiteral("Contexto"), &contexto);
    motor.rootContext()->setContextProperty(QStringLiteral("Datos"), &datos);
    motor.rootContext()->setContextProperty(QStringLiteral("Crono"), &crono);
    motor.rootContext()->setContextProperty(QStringLiteral("Camara"), &camara);
    motor.rootContext()->setContextProperty(QStringLiteral("Actualizador"), &actualizador);
    motor.rootContext()->setContextProperty(QStringLiteral("Tienda"), &tienda);
    motor.rootContext()->setContextProperty(QStringLiteral("Simulador"), &simulacion);
    motor.rootContext()->setContextProperty(QStringLiteral("PruebaCompleta"), &pruebaCompleta);
    motor.rootContext()->setContextProperty(QStringLiteral("Salud"), &salud);
    motor.rootContext()->setContextProperty(QStringLiteral("Vigilante"), &vigilante);
    motor.rootContext()->setContextProperty(QStringLiteral("Clima"), &clima);
    motor.rootContext()->setContextProperty(QStringLiteral("Hdmi"), &hdmi);
    motor.rootContext()->setContextProperty(QStringLiteral("Drive"), &drive);
    QObject::connect(&motor, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    motor.loadFromModule("Betometro", "Main");
    if (motor.rootObjects().isEmpty()) return 1;
    salud.listo();

    if (op.isSet(oCaptura) || op.isSet(oCapturaTele)) {
        auto* ventana = qobject_cast<QQuickWindow*>(motor.rootObjects().first());
        const QString archivo = op.value(oCaptura), archivoTele = op.value(oCapturaTele);
        QTimer::singleShot(op.value(oEspera).toInt(), &app, [ventana, archivo, archivoTele] {
            bool ok = true;
            if (!archivo.isEmpty()) {
                ok = ventana && ventana->grabWindow().save(archivo);
                if (!ok) qWarning("No pude guardar la captura en %s", qPrintable(archivo));
            }
            if (!archivoTele.isEmpty()) {
                QQuickWindow* tele = nullptr;
                for (QWindow* w : QGuiApplication::topLevelWindows())
                    if (w->objectName() == QLatin1String("tele")) tele = qobject_cast<QQuickWindow*>(w);
                const bool okTele = tele && tele->isVisible() && tele->grabWindow().save(archivoTele);
                if (!okTele) qWarning("No pude guardar la captura de la tele en %s (¿hay una segunda pantalla?)", qPrintable(archivoTele));
                ok = ok && okTele;
            }
            QCoreApplication::exit(ok ? 0 : 2);
        });
    }
    return app.exec();
}
