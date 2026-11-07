// Clima (pedido de Matías, 05/11/2026). Pronóstico de Open-Meteo: gratis, sin cuenta ni clave.
// - Pronóstico por coordenadas: ahora, hora por hora y 16 días (api.open-meteo.com).
// - Buscar localidad mientras se escribe: buscador de Open-Meteo (GeoNames); si no encuentra nada,
//   el de OpenStreetMap (Nominatim), que tolera mejor los errores. Se elige de la lista: siempre es real.
// - Ubicación del GPS: la última posición buena; el nombre del lugar sale de OpenStreetMap.
// - Sin wifi: queda el último pronóstico bajado de cada lugar (<datos>/clima.json), con su hora.
// En la PC (sin BETO_RASPBERRY) no sale a internet: inventa un pronóstico para ver las pantallas.
#pragma once

#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariant>

class Clima : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList sugerencias READ sugerencias NOTIFY sugerenciasChanged)   // [{nombre, detalle, lat, lon}]
    Q_PROPERTY(bool buscandoLugar READ buscandoLugar NOTIFY sugerenciasChanged)
    Q_PROPERTY(bool hayGps READ hayGps NOTIFY gpsChanged)
    Q_PROPERTY(double gpsLat READ gpsLat NOTIFY gpsChanged)
    Q_PROPERTY(double gpsLon READ gpsLon NOTIFY gpsChanged)
    Q_PROPERTY(QString nombreGps READ nombreGps NOTIFY gpsChanged)
    Q_PROPERTY(int version READ version NOTIFY cambio)   // sube cada vez que llega un pronóstico (para refrescar)

public:
    Clima(const QString& carpetaDatos, QObject* padre = nullptr);
    QVariantList sugerencias() const { return sugerencias_; }
    bool buscandoLugar() const { return buscandoLugar_; }
    bool hayGps() const { return hayGps_; }
    double gpsLat() const { return gpsLat_; }
    double gpsLon() const { return gpsLon_; }
    QString nombreGps() const { return nombreGps_; }
    int version() const { return version_; }

    Q_INVOKABLE void buscarLugar(const QString& texto);
    Q_INVOKABLE void pedir(double lat, double lon, bool forzar = false);   // baja el pronóstico si es viejo
    // {ok, cargando, error, actualizado ("HH:mm"), viejo, actual{...}, horas[...], dias[...]}
    Q_INVOKABLE QVariantMap pronostico(double lat, double lon);
    // Un día ({fecha AAAA-MM-DD}): {disponible, desde (dd/MM), codigo, max, min, lluvia, mm, viento, rafagas}
    Q_INVOKABLE QVariantMap dia(double lat, double lon, const QString& fecha);
    // Las horas de un día (para verlo hora por hora)
    Q_INVOKABLE QVariantList horasDe(double lat, double lon, const QString& fecha);

    void ponerPosicion(double lat, double lon);   // desde el GPS
    void ponerCarrera(bool enCurso);              // en carrera no se pide el nombre del lugar (11A-1.1)

    static int kDias;   // días de pronóstico (16 en Open-Meteo)
    static constexpr int kLugares = 20;   // lugares guardados como mucho (11A-2.1)
    static QString clave(double lat, double lon);   // lat,lon redondeadas a 0,05° (≈5 km). Pública para probarla
    int lugaresEnCache() const { return cache_.size(); }

signals:
    void sugerenciasChanged();
    void gpsChanged();
    void cambio();

private:
    struct Entrada { QJsonObject datos; qint64 cuando = 0, intento = 0; bool pidiendo = false; QString error; };
    void podarCache(const QString& salvo = {});   // deja como mucho kLugares (los más nuevos; «salvo» queda siempre)
    QJsonObject inventado(double lat, double lon) const;   // para la PC
    void guardarCache() const;
    void cargarCache();
    void buscarEnOsm(const QString& texto, int pedido);
    void nombrarGps();
    void avisar();
    bool avisoPendiente_ = false;

    QNetworkAccessManager red_;
    bool real_ = false;
    QString archivo_;
    QHash<QString, Entrada> cache_;
    QVariantList sugerencias_;
    bool buscandoLugar_ = false;
    int pedidoLugar_ = 0;
    bool hayGps_ = false, enCarrera_ = false, nombrando_ = false;
    double gpsLat_ = 0, gpsLon_ = 0, nombradoLat_ = 999, nombradoLon_ = 999;
    qint64 intentoNombre_ = 0;   // cuándo se pidió el nombre por última vez (si falló, se espera 2 min)
    QString nombreGps_;
    int version_ = 0;
};
