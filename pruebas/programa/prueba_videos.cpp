// Pruebas de la lista de videos guardados (Ajustes → Cámara → «Ver videos»): listar, borrar,
// sacar de la carrera y no aceptar nombres raros («../x.mkv»).
#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QElapsedTimer>
#include <QProcess>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

#include "camara.h"
#include "datos.h"

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c) do { ++chequeos; if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

static void crear(const QString& ruta, int kb, QDateTime cuando) {
    QFile f(ruta);
    f.open(QIODevice::WriteOnly);
    f.write(QByteArray(kb * 1024, 'x'));
    f.close();
    f.open(QIODevice::ReadWrite);
    f.setFileTime(cuando, QFileDevice::FileModificationTime);
}

static QVariantMap buscar(const QVariantList& l, const QString& archivo) {
    for (const auto& v : l) if (v.toMap()["archivo"] == archivo) return v.toMap();
    return {};
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    QTemporaryDir afuera;   // un archivo fuera de la carpeta de videos, que no se tiene que poder borrar
    {
        Datos d(dir.path());
        Camara cam(&d, dir.path());   // crea <datos>/videos
        const QString vid = d.carpetaVideos();
        const QDateTime ahora = QDateTime::currentDateTime();
        crear(vid + "/carrera-1.mkv", 300, ahora.addDays(-2));
        crear(vid + "/carrera-2.mkv", 200, ahora.addDays(-1));
        crear(vid + "/prueba-1.mkv", 10, ahora.addSecs(-60));
        crear(vid + "/prueba-2.mp4", 10, ahora.addSecs(-30));
        crear(vid + "/suelto.mkv", 5, ahora.addDays(-3));
        crear(vid + "/notas.txt", 1, ahora);
        crear(dir.path() + "/x.mkv", 1, ahora);   // en <datos>, no en videos
        crear(afuera.path() + "/x.mkv", 1, ahora);

        const QString id = d.guardarCarrera({{"nombre", "Santa Clara"}, {"fecha", "2026-11-04"}, {"distanciaM", 2500},
                                             {"tiempoIdealMs", 180000}, {"patron", "todas"}})["id"].toString();
        CHEQUEAR(!id.isEmpty());
        CHEQUEAR(d.agregarVideo(id, {{"archivo", "carrera-1.mkv"}, {"largada", 1}, {"desdeLargadaMs", 5000}}));
        CHEQUEAR(d.agregarVideo(id, {{"archivo", "carrera-2.mkv"}, {"largada", 2}, {"desdeLargadaMs", 4000}}));

        // Lista: solo .mkv y .mp4, del más nuevo al más viejo
        QVariantList l = d.videosGuardados();
        CHEQUEAR(l.size() == 5);
        CHEQUEAR(l.size() == 5 && l[0].toMap()["archivo"] == "prueba-2.mp4" && l[4].toMap()["archivo"] == "suelto.mkv");
        const QVariantMap c1 = buscar(l, "carrera-1.mkv");
        CHEQUEAR(c1["carreraId"] == id && c1["carrera"] == "Santa Clara" && c1["fecha"] == "04/11/26");
        CHEQUEAR(c1["bytes"].toLongLong() == 300 * 1024 && c1["tam"] == "1 MB" && !c1["prueba"].toBool());
        CHEQUEAR(buscar(l, "prueba-1.mkv")["prueba"].toBool() && buscar(l, "prueba-1.mkv")["carreraId"].toString().isEmpty());
        CHEQUEAR(buscar(l, "suelto.mkv")["carrera"].toString().isEmpty());
        CHEQUEAR(cam.videos().size() == 5 && !cam.videos()[0].toMap()["grabando"].toBool());

        // Nombres que no se aceptan: nunca sale de la carpeta de videos ni borra otra cosa
        for (const QString& malo : {QStringLiteral("../x.mkv"), QStringLiteral("..\\x.mkv"), QStringLiteral("/x.mkv"),
                                    afuera.path() + "/x.mkv", QStringLiteral("notas.txt"), QStringLiteral(""),
                                    QStringLiteral(".."), QStringLiteral("a/../b.mkv"), QStringLiteral("base.json")}) {
            CHEQUEAR(!Datos::nombreVideoValido(malo));
            CHEQUEAR(!d.borrarVideo(malo));
            CHEQUEAR(!cam.borrarVideo(malo));
        }
        CHEQUEAR(QFile::exists(dir.path() + "/x.mkv") && QFile::exists(afuera.path() + "/x.mkv") && QFile::exists(vid + "/notas.txt"));
        CHEQUEAR(!d.borrarVideo("no-existe.mkv"));

        // Borrar uno de la carrera: se va el archivo y su entrada; la carrera queda
        CHEQUEAR(cam.borrarVideo("carrera-1.mkv"));
        CHEQUEAR(!QFile::exists(vid + "/carrera-1.mkv"));
        CHEQUEAR(d.videos(id).size() == 1 && d.videos(id)[0].toMap()["archivo"] == "carrera-2.mkv");
        CHEQUEAR(d.buscar(id) != nullptr);
        CHEQUEAR(cam.lugar()["videos"].toInt() == 4);

        // El último: la carrera queda sin «videos» (el informe no muestra «Video»)
        CHEQUEAR(d.borrarVideo("carrera-2.mkv"));
        CHEQUEAR(d.videos(id).isEmpty() && !d.buscar(id)->extra.tiene("videos"));
        CHEQUEAR(d.buscar(id) != nullptr && d.corridas().size() + d.proximas().size() == 1);

        // Una entrada sin archivo (ya se borró a mano) también se saca
        CHEQUEAR(d.agregarVideo(id, {{"archivo", "perdido.mkv"}, {"largada", 1}}));
        CHEQUEAR(d.borrarVideo("perdido.mkv") && d.videos(id).isEmpty());

        // Los de prueba, todos juntos
        CHEQUEAR(cam.borrarPruebas() == 2);
        l = d.videosGuardados();
        CHEQUEAR(l.size() == 1 && l[0].toMap()["archivo"] == "suelto.mkv");
        CHEQUEAR(cam.lugar()["videos"].toInt() == 1);

        // Sacar de las carreras sin tocar el archivo (lo usa la cámara cuando borra los viejos)
        CHEQUEAR(d.agregarVideo(id, {{"archivo", "suelto.mkv"}, {"largada", 1}}));
        CHEQUEAR(d.olvidarVideo("suelto.mkv") && QFile::exists(vid + "/suelto.mkv") && d.videos(id).isEmpty());
        CHEQUEAR(!d.olvidarVideo("suelto.mkv"));
        d.guardarYa();
    }
    {
        // Quedó guardado: al volver a abrir, la carrera sigue y sin videos
        Datos d(dir.path());
        CHEQUEAR(d.corridas().size() + d.proximas().size() == 1);
        const QString id = (d.corridas().isEmpty() ? d.proximas() : d.corridas())[0].toMap()["id"].toString();
        CHEQUEAR(d.videos(id).isEmpty());
    }
    // MP4: si está el ayudante (con GStreamer), un video de prueba se pasa solo de .mkv a .mp4 y la carrera
    // queda apuntando al .mp4. En carrera no se pasa.
    const QString ayudante = QCoreApplication::applicationDirPath() + "/betometro-camara";
    if (QFile::exists(ayudante)) {
        QTemporaryDir dir2;
        Datos d(dir2.path());
        Camara cam(&d, dir2.path());
        const QString mkv = d.carpetaVideos() + "/carrera-mp4.mkv", mp4 = d.carpetaVideos() + "/carrera-mp4.mp4";
        QProcess g;
        g.start(ayudante, {"grabar", "--prueba", "--audio", "prueba", "--salida", mkv});
        g.waitForStarted();
        QElapsedTimer t; t.start();
        while (t.elapsed() < 3000) QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        g.terminate();
        g.waitForFinished(5000);
        CHEQUEAR(QFileInfo(mkv).size() > 1000);
        const QString id = d.guardarCarrera({{"nombre", "Video"}, {"fecha", "2026-11-04"}, {"distanciaM", 2500},
                                             {"tiempoIdealMs", 180000}, {"patron", "todas"}})["id"].toString();
        CHEQUEAR(d.agregarVideo(id, {{"archivo", "carrera-mp4.mkv"}, {"largada", 1}, {"desdeLargadaMs", 5000}}));
        cam.ponerEnCarrera(true);
        t.restart();
        while (t.elapsed() < 7000) QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        CHEQUEAR(!cam.convirtiendo() && QFile::exists(mkv));   // en carrera: nada
        cam.ponerEnCarrera(false);
        t.restart();
        while (t.elapsed() < 30000 && !(QFile::exists(mp4) && !cam.convirtiendo())) QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        CHEQUEAR(QFileInfo(mp4).size() > 1000);
        CHEQUEAR(!QFile::exists(mkv) && !QFile::exists(mp4 + ".parte"));
        const QVariantList v = d.videos(id);
        CHEQUEAR(v.size() == 1 && v[0].toMap()["archivo"] == "carrera-mp4.mp4");
    }
    std::cout << chequeos << " chequeos, " << fallas << " fallas\n";
    return fallas ? 1 : 0;
}
