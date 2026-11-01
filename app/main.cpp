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
#include "gps.h"

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
    QCommandLineOption oCarrera(QStringLiteral("carrera"), QStringLiteral("Carrera a abrir"), QStringLiteral("nombre"));
    op.addOptions({oPantalla, oOrient, oTema, oFisica, oCaptura, oDatos, oMuestra, oCarrera, oPrueba, oSinGps});
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
    FuenteGps sinGps;
    GpsSimulado* simulado = op.isSet(oSinGps) ? nullptr : new GpsSimulado(&app);
    Crono crono(&datos, simulado ? static_cast<FuenteGps*>(simulado) : &sinGps);

    QQmlApplicationEngine motor;
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
