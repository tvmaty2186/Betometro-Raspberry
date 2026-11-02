#include "audio.h"

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QUrl>

QString Audio::carpeta() const {
    const QStringList opciones = {qEnvironmentVariable("BETO_VOZ"), QCoreApplication::applicationDirPath() + "/voz",
                                  QStringLiteral("/usr/share/betometro/voz")};
    for (const auto& c : opciones)
        if (!c.isEmpty() && QDir(c).exists("n05.wav")) return c;
    return {};
}

Audio::Audio(QObject* padre) : QObject(padre) {
    const QString dir = carpeta();
    if (dir.isEmpty()) {
        qWarning("No encontré los audios de la voz (se arman con voz/grabar.py)");
        return;
    }
    for (const auto& f : QDir(dir).entryInfoList({QStringLiteral("*.wav")}, QDir::Files)) {
        auto* e = new QSoundEffect(this);
        e->setSource(QUrl::fromLocalFile(f.absoluteFilePath()));
        efectos_.insert(f.baseName(), e);
    }
    emit cambio();
}

void Audio::ponerVolumen(double porcentaje) {
    volumen_ = std::max(0.0, std::min(100.0, porcentaje)) / 100.0;
    for (auto* e : efectos_) e->setVolume(static_cast<float>(volumen_));
}

void Audio::reproducir(const QString& nombre) {
    auto* e = efectos_.value(nombre);
    if (!e) return;
    e->setVolume(static_cast<float>(volumen_));
    e->play();
}

QString Audio::archivoDeFrase(const QString& frase) {
    const bool medida = frase.endsWith(QStringLiteral("medida"));
    if (frase.startsWith(QStringLiteral("Última vuelta"))) return medida ? "ultima_m" : "ultima_p";
    if (frase == QStringLiteral("Carrera completa")) return "completa";
    static const QRegularExpression re(QStringLiteral("^Vuelta (\\d+), "));
    const auto m = re.match(frase);
    if (m.hasMatch()) {
        const int n = m.captured(1).toInt();
        if (n >= 1 && n <= 60) return QStringLiteral("vuelta_%1_%2").arg(n, 2, 10, QLatin1Char('0')).arg(medida ? "m" : "p");
        return medida ? "medida" : "practica";
    }
    return {};
}
