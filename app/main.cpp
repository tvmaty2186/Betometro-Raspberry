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

#include "contexto.h"
#include "crono.h"
#include "datos.h"
#include "audio.h"
#include "gps.h"
#include "qr.h"
#include "sistema.h"
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
    QCommandLineOption oCarrera(QStringLiteral("carrera"), QStringLiteral("Carrera a abrir"), QStringLiteral("nombre"));
    op.addOptions({oPantalla, oOrient, oTema, oFisica, oCaptura, oDatos, oMuestra, oCarrera, oPrueba, oSinGps, oGps});
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
        if (!puertoGps.isEmpty()) gps = new GpsNeo(puertoGps, &app);
        else gps = new GpsSimulado(&app);
    }
    Crono crono(&datos, gps);

    Audio audio;
    audio.ponerVolumen(datos.cfg(QStringLiteral("volumen")).toDouble());
    QObject::connect(&datos, &Datos::cfgChanged, &audio, [&] { audio.ponerVolumen(datos.cfg(QStringLiteral("volumen")).toDouble()); });
    QObject::connect(&crono, &Crono::sonido, &audio, &Audio::reproducir);
    QObject::connect(&crono, &Crono::frase, &audio, [&](const QString& f) { audio.reproducir(Audio::archivoDeFrase(f)); });
    Wifi wifi;
    Sistema sistema;
    if (!op.isSet(oOrient)) contexto.setHorizontal(datos.cfg(QStringLiteral("orientacion")).toString() != QStringLiteral("vertical"));
    contexto.setTema(datos.cfg(QStringLiteral("tema")).toString());
    if (op.isSet(oTema)) contexto.setTema(op.value(oTema));

    QQmlApplicationEngine motor;
    motor.addImageProvider(QStringLiteral("qr"), new ProveedorQr);
    motor.rootContext()->setContextProperty(QStringLiteral("Wifi"), &wifi);
    motor.rootContext()->setContextProperty(QStringLiteral("Audio"), &audio);
    motor.rootContext()->setContextProperty(QStringLiteral("Sistema"), &sistema);
    motor.rootContext()->setContextProperty(QStringLiteral("Contexto"), &contexto);
    motor.rootContext()->setContextProperty(QStringLiteral("Datos"), &datos);
    motor.rootContext()->setContextProperty(QStringLiteral("Crono"), &crono);
    QObject::connect(&motor, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    motor.loadFromModule("Betometro", "Main");
    if (motor.rootObjects().isEmpty()) return 1;

    if (op.isSet(oCaptura)) {
        auto* ventana = qobject_cast<QQuickWindow*>(motor.rootObjects().first());
        const QString archivo = op.value(oCaptura);
        QTimer::singleShot(1200, &app, [ventana, archivo] {
            const bool ok = ventana && ventana->grabWindow().save(archivo);
            if (!ok) qWarning("No pude guardar la captura en %s", qPrintable(archivo));
            QCoreApplication::exit(ok ? 0 : 2);
        });
    }
    return app.exec();
}
