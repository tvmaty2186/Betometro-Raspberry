// Puente entre la base (núcleo: modelo + almacén) y la pantalla (QML).
// Las reglas son las de la app: docs/app/lo-aprendido.md, secciones 5.2 a 5.5.
#pragma once

#include <QDate>
#include <QObject>
#include <QVariant>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "almacen.h"
#include "modelo.h"

class Datos : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList proximas READ proximas NOTIFY carrerasChanged)
    Q_PROPERTY(QVariantList corridas READ corridas NOTIFY carrerasChanged)
    Q_PROPERTY(bool horaValida READ horaValida NOTIFY horaChanged)
    Q_PROPERTY(QVariantMap ajustes READ ajustes NOTIFY cfgChanged)   // cfg completo, para enlazar en la pantalla

public:
    explicit Datos(const QString& carpeta, QObject* padre = nullptr);
    ~Datos() override;                     // espera a que termine de guardar
    void guardarYa();                      // espera a que lo pendiente quede en la tarjeta (para pruebas y al cerrar)

    QVariantList proximas() const;
    QVariantList corridas() const;
    bool horaValida() const { return horaValida_; }
    void setHoraValida(bool v) { if (v != horaValida_) { horaValida_ = v; emit horaChanged(); emit carrerasChanged(); } }

    // Una carrera lista para mostrar (o una nueva con los valores de la última práctica si id está vacío)
    Q_INVOKABLE QVariantMap carrera(const QString& id) const;
    // Guarda (crea o cambia). Devuelve {ok, error, id}
    Q_INVOKABLE QVariantMap guardarCarrera(const QVariantMap& c);
    Q_INVOKABLE bool borrarCarrera(const QString& id);

    // Para el crono
    QString crearPractica();                                   // práctica nueva con los valores de cfg; devuelve el id
    QString crearRecorrido();                                  // «Recorrido libre»: sin vueltas, sin tiempo ni distancia
    bool ponerExtra(const QString& id, const std::string& clave, const beto::Json& valor);   // p. ej. las marcas del recorrido
    bool agregarVuelta(const QString& id, const beto::Vuelta& v);
    // Video de una largada: {archivo, largada, desdeLargadaMs, girar}. Va en «videos» de la carrera.
    bool agregarVideo(const QString& id, const QVariantMap& video);
    Q_INVOKABLE QVariantList videos(const QString& id) const;
    // Donde la cámara deja los videos (como Camara: <datos>/videos). La usa «Betómetro en la red».
    QString carpetaVideos() const { return carpeta_ + QStringLiteral("/videos"); }
    // Los videos de la carpeta, del más nuevo al más viejo (Ajustes → Cámara → «Ver videos»):
    // {archivo, bytes, tam («230 MB»), cuando (ms), hora («05/11/26 10:32»), carreraId, carrera, fecha, prueba}
    Q_INVOKABLE QVariantList videosGuardados() const;
    // Borra un video: el archivo y lo que dice de él en las carreras. Nunca borra informes ni carreras.
    // No sabe si la cámara está grabando: desde la pantalla se usa Camara.borrarVideo.
    bool borrarVideo(const QString& archivo);
    // Saca un video de las carreras, sin tocar el archivo (cuando la cámara borra los viejos). true si había alguno.
    bool olvidarVideo(const QString& archivo);
    // El video pasó de .mkv a .mp4: cambia el nombre en las carreras (el archivo ya lo cambió la cámara)
    bool renombrarVideo(const QString& viejo, const QString& nuevo);
    // Un nombre de video que se puede tocar: sin carpetas ni «..», termina en .mkv o .mp4
    static bool nombreVideoValido(const QString& archivo);
    const beto::Carrera* buscar(const QString& id) const;
    const beto::Json& config() const { return cfg_; }

    // Informe (sección 9 de la app): datos para dibujar, HTML para compartir y su nombre
    Q_INVOKABLE QVariantMap informe(const QString& id) const;
    Q_INVOKABLE QString nombreInforme(const QString& id) const;
    Q_INVOKABLE QString guardarInformeHtml(const QString& id) const;   // devuelve la ruta del archivo
    QByteArray informeHtml(const QString& id) const;                   // el HTML del informe (vacío si no está)
    // Desde el informe: marca una vuelta como medida o libre (la libre no suma error). No las interrumpidas.
    Q_INVOKABLE bool cambiarMedida(const QString& id, int indice);
    // Desde el informe: junta una vuelta con la siguiente (marca de más)
    Q_INVOKABLE bool unirConSiguiente(const QString& id, int indice);
    Q_INVOKABLE bool restablecer(const QString& id);        // vuelve la carrera a como se corrió
    Q_INVOKABLE bool modificada(const QString& id) const;   // ¿se cambió algo desde el informe?
    // Lugar libre en la partición de datos, en texto («11,2 GB»)
    Q_INVOKABLE QString lugarLibre() const;

    // Configuración (cfg = fábrica + lo guardado)
    Q_INVOKABLE QVariant cfg(const QString& clave) const;
    QVariantMap ajustes() const;
    Q_INVOKABLE void ponerCfg(const QString& clave, const QVariant& valor);   // también listas y objetos (armados de widgets)

    // Carga carreras de ejemplo (para capturas y pruebas)
    void cargarMuestra();

    const beto::Base& base() const { return base_; }

signals:
    void carrerasChanged();
    void cfgChanged();
    void horaChanged();
    void error(const QString& texto);

private:
    QVariantMap resumen(const beto::Carrera& c) const;
    bool finalizada(const beto::Carrera& c) const;
    QDate hoy() const { return QDate::currentDate(); }
    void guardar();

    QString carpeta_;
    beto::Almacen almacen_;
    beto::Base base_;
    beto::Json cfg_;
    bool horaValida_ = true;

    // Guardado por detrás (04/11/2026): con el GPS a 5 Hz la base pesa varios MB y guardarla en la SD de la
    // Pi 3 tardaba medio segundo: al marcar, la pantalla se congelaba. Ahora guardar() hace una copia y un
    // hilo aparte la escribe (siempre la última; si llegan varias seguidas, se escribe solo la más nueva).
    void hiloGuardar();
    std::thread hilo_;
    std::mutex m_;
    std::condition_variable cv_, listo_;
    std::shared_ptr<beto::Base> pendiente_;
    bool escribiendo_ = false, terminar_ = false;
};
