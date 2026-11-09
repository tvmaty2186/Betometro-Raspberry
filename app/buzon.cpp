#include "buzon.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QUrl>

namespace {
const char* kIssues = "https://api.github.com/repos/tvmaty2186/Betometro-reportes/issues";
const char* kDueno = "tvmaty2186";
constexpr int kCadaMs = 2 * 60 * 1000;
constexpr int kMaxMs = 5 * 60 * 1000;     // un comando, como mucho
constexpr int kMaxSalida = 60000;          // lo que entra en un comentario (se deja el final)
}  // namespace

Buzon::Buzon(const QString& carpetaDatos, QObject* padre) : QObject(padre), carpeta_(carpetaDatos) {
    QFile h(carpeta_ + QStringLiteral("/buzon.hechos"));
    if (h.open(QIODevice::ReadOnly))
        for (const QByteArray& l : h.readAll().split('\n')) if (l.trimmed().toInt() > 0) hechos_.insert(l.trimmed().toInt());
    cada_.setInterval(kCadaMs);
    connect(&cada_, &QTimer::timeout, this, &Buzon::revisar);
}

QString Buzon::comandoDe(const QString& cuerpo) {
    static const QRegularExpression bloque(QStringLiteral("```[a-zA-Z]*\\r?\\n(.*?)```"), QRegularExpression::DotMatchesEverythingOption);
    const auto m = bloque.match(cuerpo);
    return (m.hasMatch() ? m.captured(1) : cuerpo).trimmed();
}

QByteArray Buzon::llave() const {
    QFile t(carpeta_ + QStringLiteral("/reportes.token"));
    return t.open(QIODevice::ReadOnly) ? t.readAll().trimmed() : QByteArray();
}

QNetworkRequest Buzon::pedido(const QString& url) const {
    QNetworkRequest q{QUrl(url)};
    q.setRawHeader("Authorization", "Bearer " + llave());
    q.setRawHeader("Accept", "application/vnd.github+json");
    q.setRawHeader("User-Agent", "Betometro/" BETO_VERSION);
    q.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    q.setTransferTimeout(30000);
    return q;
}

void Buzon::empezar() {
    // Si se cortó en el medio de un comando (se reinició Betómetro o la Pi), se avisa y se cierra
    QFile e(carpeta_ + QStringLiteral("/buzon.encurso"));
    if (e.open(QIODevice::ReadOnly)) {
        const int n = e.readAll().trimmed().toInt();
        e.close();
        e.remove();
        if (n > 0 && !hechos_.contains(n))
            QTimer::singleShot(60000, this, [this, n] {
                contestar(n, QStringLiteral("Se cortó en el medio: Betómetro se reinició o la Pi se apagó mientras corría (v" BETO_VERSION ")."));
            });
    }
    cada_.start();
    QTimer::singleShot(90000, this, &Buzon::revisar);
}

void Buzon::revisar() {
    if (ocupado_ || llaveMala_ || (puede && !puede())) return;
    if (llave().isEmpty()) return;
    ocupado_ = true;
    QNetworkRequest q = pedido(QString::fromLatin1(kIssues) + QStringLiteral("?labels=comando&state=open&creator=%1&sort=created&direction=asc&per_page=10").arg(QLatin1String(kDueno)));
    if (!etag_.isEmpty()) q.setRawHeader("If-None-Match", etag_);
    QNetworkReply* r = red_.get(q);
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        const int http = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (http == 401 || http == 403) {
            llaveMala_ = true;
            qWarning("Buzón: GitHub no deja leer los comandos con la llave de reportes (HTTP %d); se deja hasta el próximo arranque", http);
        }
        if (http != 200) { ocupado_ = false; return; }   // 304: nada nuevo
        etag_ = r->rawHeader("ETag");
        for (const QJsonValue& v : QJsonDocument::fromJson(r->readAll()).array()) {
            const QJsonObject o = v.toObject();
            const int n = o.value("number").toInt();
            // Solo del dueño (además de que el repo es privado); los pull requests no
            if (hechos_.contains(n) || o.contains("pull_request")) continue;
            if (o.value("user").toObject().value("login").toString() != QLatin1String(kDueno)) continue;
            if (o.value("author_association").toString() != QLatin1String("OWNER")) continue;
            const QString cmd = comandoDe(o.value("body").toString());
            if (cmd.isEmpty()) { contestar(n, QStringLiteral("No encontré el comando (va en un bloque ```).")); continue; }
            correr(n, cmd);
            return;   // de a uno; el siguiente en la próxima vuelta
        }
        ocupado_ = false;
    });
}

void Buzon::correr(int numero, const QString& comando) {
    QFile e(carpeta_ + QStringLiteral("/buzon.encurso"));
    if (e.open(QIODevice::WriteOnly | QIODevice::Truncate)) { e.write(QByteArray::number(numero)); e.close(); }
    qWarning("Buzón: corre el comando del pedido #%d", numero);
    auto* p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    p->setStandardInputFile(QProcess::nullDevice());
    if (qEnvironmentVariableIsSet("BETO_RASPBERRY"))
        // Fuera del encierro del servicio (ProtectSystem/ProtectHome): como en una consola de la Pi
        p->start(QStringLiteral("systemd-run"), {QStringLiteral("--quiet"), QStringLiteral("--wait"), QStringLiteral("--pipe"),
                 QStringLiteral("--collect"), QStringLiteral("-p"), QStringLiteral("RuntimeMaxSec=%1").arg(kMaxMs / 1000),
                 QStringLiteral("--setenv=HOME=/root"), QStringLiteral("/bin/bash"), QStringLiteral("-lc"), comando});
    else
        p->start(QStringLiteral("/bin/bash"), {QStringLiteral("-c"), comando});
    auto* reloj = new QElapsedTimer;
    reloj->start();
    auto* corte = new QTimer(p);
    corte->setSingleShot(true);
    connect(corte, &QTimer::timeout, p, [p] { p->kill(); });
    corte->start(kMaxMs + 15000);
    connect(p, &QProcess::finished, this, [this, p, numero, comando, reloj](int codigo, QProcess::ExitStatus estado) {
        QString salida = QString::fromUtf8(p->readAll());
        if (salida.size() > kMaxSalida) salida = QStringLiteral("… (se cortó el principio)\n") + salida.right(kMaxSalida);
        salida.replace(QStringLiteral("```"), QStringLiteral("'''"));
        const QString fin = estado == QProcess::CrashExit ? QStringLiteral("cortado (tardó demasiado)") : QStringLiteral("salió con %1").arg(codigo);
        contestar(numero, QStringLiteral("```\n$ %1\n%2\n```\n%3 · %4 s · v" BETO_VERSION " · %5")
                              .arg(comando, salida.trimmed(), fin).arg(reloj->elapsed() / 1000.0, 0, 'f', 1)
                              .arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy HH:mm:ss"))));
        delete reloj;
        p->deleteLater();
        QFile::remove(carpeta_ + QStringLiteral("/buzon.encurso"));
        ocupado_ = false;
    });
    connect(p, &QProcess::errorOccurred, this, [this, p, numero, reloj](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart) return;
        contestar(numero, QStringLiteral("No se pudo correr el comando: ") + p->errorString());
        delete reloj;
        p->deleteLater();
        QFile::remove(carpeta_ + QStringLiteral("/buzon.encurso"));
        ocupado_ = false;
    });
}

void Buzon::contestar(int numero, const QString& texto) {
    hecho(numero);   // aunque falle la respuesta, no se vuelve a correr
    const QString url = QString::fromLatin1(kIssues) + QStringLiteral("/%1").arg(numero);
    QNetworkReply* r = red_.post(pedido(url + QStringLiteral("/comments")), QJsonDocument(QJsonObject{{"body", texto}}).toJson(QJsonDocument::Compact));
    connect(r, &QNetworkReply::finished, this, [this, r, url, numero] {
        r->deleteLater();
        const int http = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (http != 201) qWarning("Buzón: no se pudo contestar el pedido #%d (HTTP %d)", numero, http);
        QNetworkReply* c = red_.sendCustomRequest(pedido(url), "PATCH",
                                                 QJsonDocument(QJsonObject{{"state", "closed"}, {"state_reason", "completed"}}).toJson(QJsonDocument::Compact));
        connect(c, &QNetworkReply::finished, c, &QObject::deleteLater);
    });
}

void Buzon::hecho(int numero) {
    hechos_.insert(numero);
    QFile h(carpeta_ + QStringLiteral("/buzon.hechos"));
    if (h.open(QIODevice::Append)) h.write(QByteArray::number(numero) + '\n');
}
