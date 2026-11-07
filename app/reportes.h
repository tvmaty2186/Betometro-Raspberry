// Reporte de errores automático (pedido de Matías, 03/11/2026).
// - Junta lo que falla: avisos y errores del programa (incluidos los de las pantallas QML) y los
//   cierres inesperados (si al prender no se había cerrado bien: caída o corte de contacto).
// - Se guarda en <datos>/reportes/errores.log, con versión y hora, chico (se recorta solo).
// - Se ve y se descarga desde «Betómetro en la red» (/reportes).
// - Si hay llave (<datos>/reportes.token: una llave de GitHub que SOLO pueda crear issues en el
//   repo de reportes), cuando hay internet y no hay carrera los manda como issue. Sin datos personales.
//   De a 60 KB (un issue por parte, nada se pierde). Si GitHub rechaza la llave (venció: 401/403), se
//   anota una vez, se deja de mandar hasta el próximo arranque y se ve en /reportes (11A-1.3).
#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>
#include <QTimer>

class Reportes : public QObject {
    Q_OBJECT
public:
    Reportes(const QString& carpetaDatos, QObject* padre = nullptr);
    ~Reportes() override;
    void instalar();                         // empieza a escuchar los mensajes del programa
    void anotar(const QString& tipo, const QString& texto);
    QByteArray texto() const;                // todo el registro (para la página)
    void ponerOcupado(bool siCarrera) { carrera_ = siCarrera; }
    bool hayLlave() const;                   // está reportes.token (se puede mandar por internet)
    QString problema() const { return problema_; }   // «la llave venció…» (para la página /reportes)
    void enviarAhora() { enviar(); }         // botón «Mandar el reporte» del simulador
    static bool esRuido(const QString& m);   // avisos que no son errores (público para probarlo)

private:
    void enviar();
    static QByteArray leerArranque();        // número del arranque del sistema (/proc/sys/kernel/random/boot_id)
    void juntarSistema(bool arranqueAnterior);
    QString carpeta_, archivo_, enviadoHasta_, marcaVivo_;
    QNetworkAccessManager red_;
    QTimer cada_;
    bool carrera_ = false, llaveRechazada_ = false, mandando_ = false;
    QString problema_;
    qint64 recortado_ = 0;   // bytes sacados del registro al recortarlo (para correr «enviado hasta»)
};
