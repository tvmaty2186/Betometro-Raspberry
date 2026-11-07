// Pruebas de Google Drive sin internet (app/drive_logica.*): nombre de las carpetas, el «Range» de una
// subida cortada, la regla de la wifi para los videos, qué falta subir y el JSON del cliente de Google.
#include <QJsonObject>
#include <iostream>

#include "drive_logica.h"

static int fallas = 0, chequeos = 0;
#define CHEQUEAR(c) do { ++chequeos; if (!(c)) { ++fallas; std::cerr << __FILE__ << ":" << __LINE__ << ": falló " #c "\n"; } } while (0)

using namespace drive;

int main() {
    // ---- Carpetas: «AAAA-MM-DD Nombre»
    CHEQUEAR(nombreCarpeta("2026-11-08", "Santa Clara") == "2026-11-08 Santa Clara");
    CHEQUEAR(nombreCarpeta("2026-11-08", "  Gran   Premio ") == "2026-11-08 Gran Premio");
    CHEQUEAR(nombreCarpeta("2026-11-05", "Práctica 05/11/26 10:32") == "2026-11-05 Práctica 05-11-26 10:32");
    CHEQUEAR(nombreCarpeta("", "Sin fecha") == "Sin fecha");
    CHEQUEAR(nombreCarpeta("2026-11-08", "") == "2026-11-08 Carrera");

    // ---- «Range: bytes=0-N» → se sigue en N+1
    qint64 sig = -1;
    CHEQUEAR(siguienteByte("bytes=0-4194303", sig) && sig == 4194304);
    CHEQUEAR(siguienteByte("", sig) && sig == 0);                 // sin Range: no llegó nada
    CHEQUEAR(siguienteByte(" bytes=0-0 ", sig) && sig == 1);
    CHEQUEAR(!siguienteByte("bytes=5-10", sig));                  // siempre empieza en 0
    CHEQUEAR(!siguienteByte("bytes=abc", sig));
    CHEQUEAR(!siguienteByte("items=0-10", sig));
    CHEQUEAR(kPedazo % (256 * 1024) == 0 && kPedazo == 4 * 1024 * 1024);

    // ---- La respuesta de curl -i
    int http = 0;
    QByteArray range, cuerpo;
    CHEQUEAR(leerRespuesta("HTTP/2 308\r\nrange: bytes=0-8388607\r\ncontent-length: 0\r\n\r\n", http, range, cuerpo));
    CHEQUEAR(http == 308 && range == "bytes=0-8388607" && cuerpo.isEmpty());
    CHEQUEAR(leerRespuesta("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"id\":\"abc\"}", http, range, cuerpo));
    CHEQUEAR(http == 200 && range.isEmpty() && cuerpo == "{\"id\":\"abc\"}");
    CHEQUEAR(!leerRespuesta("", http, range, cuerpo));
    CHEQUEAR(!leerRespuesta("HTTP/1.1 200 OK\r\nX: y", http, range, cuerpo));   // cortada

    // ---- Wifi: los videos solo con las elegidas; ninguna elegida = nunca; la propia no tiene internet
    CHEQUEAR(wifiParaVideos("Casa Russo", false, {"Casa Russo"}));
    CHEQUEAR(!wifiParaVideos("Celu de Matías", false, {"Casa Russo"}));
    CHEQUEAR(!wifiParaVideos("Casa Russo", false, {}));
    CHEQUEAR(!wifiParaVideos("", false, {"Casa Russo"}));
    CHEQUEAR(!wifiParaVideos("Casa Russo", true, {"Casa Russo"}));

    // ---- Qué falta subir
    QList<CarreraLocal> cs;
    cs << CarreraLocal{"c1", "Santa Clara", "2026-11-08 Santa Clara", "h1", {{"a.mkv", 1000}, {"b.mkv", 2000}}};
    cs << CarreraLocal{"c2", "Tandil", "2026-10-01 Tandil", "h2", {{"c.mp4", 500}}};
    Opciones op;
    op.videosPermitidos = true;
    Pendientes p = pendientes({}, cs, op);
    CHEQUEAR(p.tareas.size() == 5);
    CHEQUEAR(p.tareas[0].tipo == Tarea::Informe && p.tareas[1].tipo == Tarea::Informe);   // primero los informes
    CHEQUEAR(p.tareas[2].tipo == Tarea::Video && p.tareas[2].archivo == "a.mkv" && p.tareas[2].carpeta == "2026-11-08 Santa Clara");
    // Ya subido: informe con la misma versión y un video
    const QJsonObject estado{{"c1", QJsonObject{{"informe", QJsonObject{{"id", "I1"}, {"version", "h1"}}},
                                                {"videos", QJsonObject{{"a.mkv", QJsonObject{{"id", "V1"}}}}}}},
                             {"c2", QJsonObject{{"informe", QJsonObject{{"id", "I2"}, {"version", "viejo"}}}}}};
    p = pendientes(estado, cs, op);
    CHEQUEAR(p.tareas.size() == 3);
    CHEQUEAR(p.tareas[0].tipo == Tarea::Informe && p.tareas[0].carrera == "c2" && p.tareas[0].idDrive == "I2");   // cambió: se reemplaza
    CHEQUEAR(p.tareas[1].archivo == "b.mkv" && p.tareas[2].archivo == "c.mp4");
    // Sin la wifi de casa: los videos esperan
    op.videosPermitidos = false;
    p = pendientes(estado, cs, op);
    CHEQUEAR(p.tareas.size() == 1 && p.videosEsperando == 2 && p.bytesEsperando == 2500);
    // Opciones apagadas
    op.informes = false; op.videos = false;
    p = pendientes({}, cs, op);
    CHEQUEAR(p.tareas.isEmpty() && p.videosEsperando == 0);
    // Todo subido
    op = Opciones{};
    op.videosPermitidos = true;
    const QJsonObject todo{{"c1", QJsonObject{{"informe", QJsonObject{{"id", "I1"}, {"version", "h1"}}},
                                              {"videos", QJsonObject{{"a.mkv", QJsonObject{{"id", "V1"}}}, {"b.mkv", QJsonObject{{"id", "V2"}}}}}}},
                           {"c2", QJsonObject{{"informe", QJsonObject{{"id", "I2"}, {"version", "h2"}}},
                                              {"videos", QJsonObject{{"c.mp4", QJsonObject{{"id", "V3"}}}}}}}};
    CHEQUEAR(pendientes(todo, cs, op).tareas.isEmpty());
    // La copia de la base: con los informes, después de ellos y antes que los videos; solo cuando cambió
    op.baseVersion = "b1";
    p = pendientes(estado, cs, op);
    CHEQUEAR(p.tareas.size() == 4 && p.tareas[1].tipo == Tarea::Base && p.tareas[1].archivo == "betometro.json" && p.tareas[1].version == "b1");
    CHEQUEAR(p.tareas[1].idDrive.isEmpty() && p.tareas[2].tipo == Tarea::Video);
    p = pendientes(estado, cs, op, QJsonObject{{"id", "B1"}, {"version", "viejo"}});
    CHEQUEAR(p.tareas.size() == 4 && p.tareas[1].tipo == Tarea::Base && p.tareas[1].idDrive == "B1");   // cambió: se reemplaza
    CHEQUEAR(pendientes(todo, cs, op, QJsonObject{{"id", "B1"}, {"version", "b1"}}).tareas.isEmpty());   // ya está
    op.informes = false;
    CHEQUEAR(pendientes({}, cs, op).tareas.size() == 3);   // sin informes tampoco va la base (solo los 3 videos)
    op.informes = true;
    op.baseVersion.clear();

    // ---- El JSON del cliente de Google (valores inventados)
    QJsonObject limpio;
    CHEQUEAR(revisarCliente(R"({"installed":{"client_id":"123-abc.apps.googleusercontent.com","client_secret":"GOCSPX-prueba","project_id":"x"}})", &limpio).isEmpty());
    CHEQUEAR(limpio.value("client_id").toString() == "123-abc.apps.googleusercontent.com" && limpio.value("client_secret").toString() == "GOCSPX-prueba");
    CHEQUEAR(limpio.size() == 2);   // solo lo que hace falta
    CHEQUEAR(revisarCliente(R"({"web":{"client_id":"9.apps.googleusercontent.com","client_secret":"s"}})").isEmpty());
    CHEQUEAR(revisarCliente(R"({"client_id":"9.apps.googleusercontent.com","client_secret":"s"})").isEmpty());
    CHEQUEAR(!revisarCliente(R"({"installed":{"client_id":"9.apps.googleusercontent.com"}})").isEmpty());   // sin secreto
    CHEQUEAR(!revisarCliente(R"({"installed":{"client_id":"otro","client_secret":"s"}})").isEmpty());
    CHEQUEAR(!revisarCliente("esto no es json").isEmpty());
    CHEQUEAR(!revisarCliente("[1,2]").isEmpty());

    // ---- Textos
    CHEQUEAR(textoBytes(2254857830LL) == "2,1 GB");
    CHEQUEAR(textoBytes(350LL * 1048576) == "350 MB");
    CHEQUEAR(errorEnPalabras(403, R"({"error":{"errors":[{"reason":"storageQuotaExceeded"}]}})") == "Drive lleno");
    CHEQUEAR(errorEnPalabras(400, R"({"error":"invalid_grant"})") == "Se venció el permiso: tocá Conectar");
    CHEQUEAR(errorEnPalabras(0, "") == "Sin internet");

    std::cout << chequeos - fallas << "/" << chequeos << " bien\n";
    return fallas ? 1 : 0;
}
