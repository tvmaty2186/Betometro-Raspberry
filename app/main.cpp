// Betómetro para Raspberry Pi: arranque del programa.
//
// Opciones (para desarrollo y capturas):
//   --pantalla NOMBRE        pantalla con la que arranca (inicio, carreras, …)
//   --orientacion h|v        horizontal o vertical
//   --tema NOMBRE            avispon, rally, sport, cabina
//   --fisica                 simula la pantalla real (720×1280 de pie) y gira la interfaz
//   --captura ARCHIVO.png    saca una captura y sale
//   --datos CARPETA          dónde está la base (en la Raspberry: /datos)
//   --muestra                carga carreras de ejemplo (no toca la base guardada hasta que se cambie algo)
//   --carrera NOMBRE         abre la ficha de esa carrera
#include <QCommandLineParser>
#include <QDirIterator>
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
#include "qr.h"
#include "sistema.h"
#include "actualizador.h"
#include "camara.h"
#include "pulsador_gpio.h"
#include "reportes.h"
#include "servidor.h"
#include "wifi.h"

#include <QStandardPaths>

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
    QCommandLineOption oGps(QStringLiteral("gps"), QStringLiteral("Puerto del GPS (en la Raspberry /dev/ttyAMA0; con el simulador /tmp/betometro-gps)"), QStringLiteral("puerto"));
    QCommandLineOption oEspera(QStringLiteral("espera"), QStringLiteral("ms antes de la captura"), QStringLiteral("ms"), QStringLiteral("1200"));
    QCommandLineOption oCarrera(QStringLiteral("carrera"), QStringLiteral("Carrera a abrir"), QStringLiteral("nombre"));
    op.addOptions({oPantalla, oOrient, oTema, oFisica, oCaptura, oDatos, oMuestra, oCarrera, oPrueba, oSinGps, oGps, oEspera});
    op.process(app);

    cargarFuentes();

    Contexto contexto;
    if (op.isSet(oOrient)) contexto.setHorizontal(op.value(oOrient) != QStringLiteral("v"));
    if (op.isSet(oTema)) contexto.setTema(op.value(oTema));
    contexto.setFisica(op.isSet(oFisica));
    contexto.setPantallaInicial(op.value(oPantalla));
    contexto.setPrueba(op.value(oPrueba));

    QString carpeta = op.value(oDatos);
    if (carpeta.isEmpty()) carpeta = qEnvironmentVariable("BETO_DATOS");
    if (carpeta.isEmpty()) carpeta = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    Datos datos(carpeta);
    // Reporte de errores: desde acá se anota todo lo que falle (también las pantallas QML)
    Reportes reportes(carpeta);
    if (qEnvironmentVariableIsSet("BETO_RASPBERRY")) reportes.instalar();
    if (op.isSet(oMuestra)) datos.cargarMuestra();
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
        else gps = new GpsSimulado(&app);
    }
    Crono crono(&datos, gps);
    Camara camara(&datos, carpeta);
    crono.ponerCamara(&camara);
    Actualizador actualizador(carpeta);
    contexto.setVersionNueva(actualizador.versionNueva());
    QObject::connect(&actualizador, &Actualizador::cambio, &contexto, [&] { contexto.setVersionNueva(actualizador.versionNueva()); });

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
    // Pulsador por cable (GPIO17, pin 11 a GND pin 9), con la pull-up interna. Solo en la Raspberry.
    PulsadorGpio* pulsador = qEnvironmentVariableIsSet("BETO_RASPBERRY") ? new PulsadorGpio(17, &app) : nullptr;
    if (pulsador && !pulsador->activo()) qWarning("Pulsador: %s", qPrintable(pulsador->error()));
    // «Betómetro en la red»: en la Raspberry en el puerto 80 (http://IP); en la PC en el 8080
    Servidor servidor(&datos, qEnvironmentVariableIsSet("BETO_RASPBERRY") ? 80 : 8080);
    servidor.ponerReportes(&reportes);
    QObject::connect(&crono, &Crono::enCursoChanged, &servidor, [&] {
        servidor.prender(!crono.enCurso());     // apagado en carrera
        reportes.ponerOcupado(crono.enCurso());  // y los reportes no se mandan en carrera
    });
    Sistema sistema;
    if (!op.isSet(oOrient)) contexto.setHorizontal(datos.cfg(QStringLiteral("orientacion")).toString() != QStringLiteral("vertical"));
    contexto.setTema(datos.cfg(QStringLiteral("tema")).toString());
    if (op.isSet(oTema)) contexto.setTema(op.value(oTema));
    contexto.setSol(datos.cfg(QStringLiteral("modoSol")).toString() == QStringLiteral("si") || op.value(oPrueba) == QStringLiteral("sol"));
    QObject::connect(&datos, &Datos::cfgChanged, &contexto, [&] {
        contexto.setSol(datos.cfg(QStringLiteral("modoSol")).toString() == QStringLiteral("si"));
        contexto.setTema(datos.cfg(QStringLiteral("tema")).toString());
    });

    QQmlApplicationEngine motor;
    motor.addImageProvider(QStringLiteral("qr"), new ProveedorQr);
    motor.rootContext()->setContextProperty(QStringLiteral("Wifi"), &wifi);
    motor.rootContext()->setContextProperty(QStringLiteral("Servidor"), &servidor);
    motor.rootContext()->setContextProperty(QStringLiteral("Pulsador"), pulsador);   // null en la PC
    motor.rootContext()->setContextProperty(QStringLiteral("Audio"), &audio);
    motor.rootContext()->setContextProperty(QStringLiteral("Sistema"), &sistema);
    motor.rootContext()->setContextProperty(QStringLiteral("Contexto"), &contexto);
    motor.rootContext()->setContextProperty(QStringLiteral("Datos"), &datos);
    motor.rootContext()->setContextProperty(QStringLiteral("Crono"), &crono);
    motor.rootContext()->setContextProperty(QStringLiteral("Camara"), &camara);
    motor.rootContext()->setContextProperty(QStringLiteral("Actualizador"), &actualizador);
    QObject::connect(&motor, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    motor.loadFromModule("Betometro", "Main");
    if (motor.rootObjects().isEmpty()) return 1;

    if (op.isSet(oCaptura)) {
        auto* ventana = qobject_cast<QQuickWindow*>(motor.rootObjects().first());
        const QString archivo = op.value(oCaptura);
        QTimer::singleShot(op.value(oEspera).toInt(), &app, [ventana, archivo] {
            const bool ok = ventana && ventana->grabWindow().save(archivo);
            if (!ok) qWarning("No pude guardar la captura en %s", qPrintable(archivo));
            QCoreApplication::exit(ok ? 0 : 2);
        });
    }
    return app.exec();
}
