// Puente entre la base (núcleo: modelo + almacén) y la pantalla (QML).
// Las reglas son las de la app: docs/app/lo-aprendido.md, secciones 5.2 a 5.5.
// La clase está repartida por tema (1.0.30, auditoría 01-1.2):
//   datos.cpp           la base de carreras: cargar, crear, cambiar y borrar
//   datos_resumen.cpp   el resumen de cada carrera para la pantalla y las listas próximas/corridas
//   datos_informe.cpp   el informe (datos, HTML) y los cambios que se hacen desde él
//   datos_videos.cpp    los videos de las carreras y los archivos de videos/
//   datos_config.cpp    la configuración (cfg, ajustes, ponerCfg)
//   datos_guardado.cpp  el guardado: un archivo por carrera, en un hilo aparte
//   datos_muestra.cpp   las carreras de ejemplo (--muestra)
#pragma once

#include <QDate>
#include <QObject>
#include <QTimer>
#include <QVariant>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "almacen.h"
#include "modelo.h"

class Datos : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList proximas READ proximas NOTIFY carrerasChanged)
    Q_PROPERTY(QVariantList corridas READ corridas NOTIFY carrerasChanged)
    Q_PROPERTY(bool horaValida READ horaValida NOTIFY horaChanged)
    Q_PROPERTY(QVariantMap ajustes READ ajustes NOTIFY cfgChanged)   // cfg completo, para enlazar en la pantalla
    Q_PROPERTY(bool guardaBien READ guardaBien NOTIFY guardaBienChanged)   // false: el último guardado falló (tarjeta llena o dañada)

public:
    explicit Datos(const QString& carpeta, QObject* padre = nullptr);
    ~Datos() override;                     // espera a que termine de guardar
    void guardarYa();                      // espera a que lo pendiente quede en la tarjeta (para pruebas y al cerrar)

    QVariantList proximas() const;
    QVariantList corridas() const;
    bool horaValida() const { return horaValida_; }
    bool guardaBien() const { return guardaBien_; }
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
    bool quitarExtra(const QString& id, const std::string& clave);
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
    // Marca virtual por GPS de cada vuelta (solo referencia, para el informe): hay, dudosa, realMs, difMs, pulsadorMenosGpsMs
    Q_INVOKABLE QVariantList marcasGps(const QString& id) const;
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
    Q_INVOKABLE bool pocoLugar() const;   // menos de 1 GB: la cámara empieza a borrar videos y la base puede no entrar

    // Configuración (cfg = fábrica + lo guardado)
    Q_INVOKABLE QVariant cfg(const QString& clave) const;
    QVariantMap ajustes() const;
    Q_INVOKABLE void ponerCfg(const QString& clave, const QVariant& valor);   // también listas y objetos (armados de widgets)

    // Carga carreras de ejemplo (para capturas y pruebas)
    void cargarMuestra();

    const beto::Base& base() const { return base_; }
    // Toda la base en un JSON como el de la app ({schemaVersion, config, carreras}): «Betómetro en la red»
    // (GET /betometro.json) y la copia que sube Drive
    QByteArray exportar() const { return QByteArray::fromStdString(base_.aJson().escribir()); }

signals:
    void guardaBienChanged();
    void carrerasChanged();
    void cfgChanged();
    void horaChanged();
    void error(const QString& texto);

private:
    QVariantMap resumen(const beto::Carrera& c) const;
    bool finalizada(const beto::Carrera& c) const;
    QDate hoy() const { return QDate::currentDate(); }
    // Un archivo por carrera (1.0.29, auditoría 05-4): se guarda solo lo que cambió, la carrera «id» o la
    // configuración. Se copia solo esa carrera (no la base entera, 04-2.2) y un hilo aparte la escribe.
    void guardar(const QString& id);        // esa carrera, al instante
    void guardarConfig();
    void borrar(const QString& id);         // su archivo va a la papelera
    // Ajustes (brillo, volumen) y marcas: se juntan 0,3 s antes de guardar, así deslizar el brillo no
    // reescribe la carrera 20 veces (auditoría 04-2.3). Las vueltas se guardan al instante.
    void guardarLuego(const QString& id);
    void guardarConfigLuego();
    void guardarPendientes();               // lo que se estaba juntando
    QTimer guardarLuego_;
    std::set<QString> luego_;
    bool configLuego_ = false;

    QString carpeta_;
    beto::Almacen almacen_;
    beto::Base base_;
    beto::Json cfg_;
    bool horaValida_ = true;
    bool guardaBien_ = true;

    // Guardado por detrás (04/11/2026): escribir en la SD de la Pi 3 tardaba y al marcar la pantalla se
    // congelaba. guardar() encola una copia de la carrera (o de la config) y un hilo aparte la escribe.
    // Si la misma carrera llega varias veces seguidas se escribe solo la más nueva.
    struct Tarea {
        enum Tipo { Carrera, Config, Borrar } tipo = Carrera;
        std::string id;
        std::shared_ptr<beto::Carrera> carrera;
        std::shared_ptr<beto::Json> config;
    };
    void encolar(Tarea t);
    void hiloGuardar();
    std::thread hilo_;
    std::mutex m_;
    std::condition_variable cv_, listo_;
    std::vector<Tarea> pendientes_;
    bool escribiendo_ = false, terminar_ = false;
};
