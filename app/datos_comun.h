// Ayudas que comparten los archivos de Datos (datos*.cpp): la clase está repartida por tema.
#pragma once

#include <QString>
#include <string>

inline QString q(const std::string& s) { return QString::fromStdString(s); }
inline std::string s(const QString& t) { return t.toStdString(); }
