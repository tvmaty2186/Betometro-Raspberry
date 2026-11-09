// Buzón de comandos por GitHub (Matías, 09/11/2026): para revisar y arreglar la Pi a distancia, sin ssh.
// - Los pedidos son issues del repo privado de reportes con la etiqueta «comando», abiertos por el dueño
//   (tvmaty2186, OWNER). El comando es el bloque ``` del texto (o todo el texto).
// - La Pi mira cada 2 minutos, solo con internet, sin carrera y con «Ayuda a distancia» prendida (Conectividad).
//   La pregunta es chica (con ETag: si no hay nada nuevo GitHub contesta «304» y no cuenta).
// - Corre el comando fuera del encierro del servicio (systemd-run), hasta 5 minutos, contesta con lo que salió
//   como comentario y cierra el issue. Usa la misma llave que los reportes (<datos>/reportes.token).
// - Si Betómetro se reinicia en el medio (por ejemplo, el comando lo reinició), al volver lo avisa y lo cierra.
#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QTimer>

#include <functional>

class QProcess;

class Buzon : public QObject {
    Q_OBJECT
public:
    Buzon(const QString& carpetaDatos, QObject* padre = nullptr);
    std::function<bool()> puede;            // internet, sin carrera y prendido
    void empezar();                         // cada 2 min (y al rato de arrancar)
    void revisar();                         // ahora (por ejemplo, al conectarse a internet)
    // Saca el comando del texto del issue: el primer bloque ``` o, si no hay, todo el texto (público para probarlo)
    static QString comandoDe(const QString& cuerpo);

private:
    QByteArray llave() const;
    QNetworkRequest pedido(const QString& url) const;
    void correr(int numero, const QString& comando);
    void contestar(int numero, const QString& texto);
    void hecho(int numero);
    QString carpeta_;
    QNetworkAccessManager red_;
    QTimer cada_;
    QByteArray etag_;
    QSet<int> hechos_;
    bool ocupado_ = false, llaveMala_ = false;
};
