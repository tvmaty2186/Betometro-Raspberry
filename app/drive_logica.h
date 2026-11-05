// Google Drive: la parte que no sale a internet (para probarla sola en pruebas/programa/prueba_drive.cpp).
// Nombres de carpetas, la regla de la wifi, qué falta subir, el «Range» de una subida cortada,
// revisar el JSON del cliente de Google y pasar los errores a palabras simples.
#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace drive {

// De a cuánto se sube un archivo: 4 MB (Google pide múltiplos de 256 KB)
constexpr qint64 kPedazo = 4 * 1024 * 1024;
static_assert(kPedazo % (256 * 1024) == 0, "el pedazo tiene que ser múltiplo de 256 KB");

// «AAAA-MM-DD Nombre» (sin «/» ni caracteres raros). Sin fecha: solo el nombre.
QString nombreCarpeta(const QString& fechaIso, const QString& nombre);

// Respuesta 308 de Google: «Range: bytes=0-N» → lo próximo a mandar es N+1. Sin «Range»: 0 (no llegó nada).
// false si el texto no se entiende.
bool siguienteByte(const QByteArray& range, qint64& siguiente);

// ¿Se pueden subir videos con esta wifi? Solo con una de las elegidas (ninguna elegida: nunca).
// La wifi propia del Betómetro no tiene internet.
bool wifiParaVideos(const QString& conectada, bool propia, const QStringList& elegidas);

// Revisa el JSON que se baja de Google Cloud («installed» o «web», con client_id y client_secret).
// Devuelve "" si está bien (y llena `limpio` con {client_id, client_secret}); si no, el problema en palabras.
QString revisarCliente(const QByteArray& json, QJsonObject* limpio = nullptr);

// Lo que escribe «curl -i»: el último bloque de cabeceras (saltea «100 Continue»), el «Range» y el cuerpo.
// false si no hay una respuesta HTTP entera.
bool leerRespuesta(const QByteArray& crudo, int& codigo, QByteArray& range, QByteArray& cuerpo);

// «2,1 GB», «350 MB»
QString textoBytes(qint64 b);

// El error de Google (código HTTP y cuerpo) en palabras simples
QString errorEnPalabras(int http, const QByteArray& cuerpo);

// ---- Qué falta subir ----
struct VideoLocal { QString archivo; qint64 bytes = 0; };
struct CarreraLocal {
    QString id, nombre, carpeta;          // carpeta: «AAAA-MM-DD Nombre»
    QString versionInforme;               // huella del HTML (si cambia, se vuelve a subir)
    QList<VideoLocal> videos;             // solo los que están en la tarjeta
};
struct Tarea {
    enum Tipo { Informe, Video } tipo = Informe;
    QString carrera, nombre, carpeta;     // id y nombre de la carrera, y su carpeta en Drive
    QString archivo;                      // video: nombre del archivo
    qint64 bytes = 0;
    QString idDrive;                      // informe ya subido antes: se reemplaza (PATCH)
};
struct Opciones { bool informes = true, videos = true, videosPermitidos = false; };
struct Pendientes {
    QList<Tarea> tareas;                  // en orden: primero los informes (chicos), después los videos
    int videosEsperando = 0;              // videos que esperan la wifi de casa
    qint64 bytesEsperando = 0;
};
// `estado`: «carreras» de drive.json ({id: {informe:{id, version}, videos:{archivo:{id}}}})
Pendientes pendientes(const QJsonObject& estadoCarreras, const QList<CarreraLocal>& carreras, const Opciones& op);

}  // namespace drive
