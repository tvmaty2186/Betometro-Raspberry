// Buzón de comandos: de dónde saca el comando del texto del pedido
#include "buzon.h"

#include <cstdio>

static int fallas = 0;
static void ver(const QString& cuerpo, const QString& esperado) {
    const QString r = Buzon::comandoDe(cuerpo);
    if (r != esperado) { ++fallas; std::printf("MAL: «%s» dio «%s» (esperaba «%s»)\n", qPrintable(cuerpo), qPrintable(r), qPrintable(esperado)); }
}

int main() {
    ver(QStringLiteral("free -m"), QStringLiteral("free -m"));
    ver(QStringLiteral("Mirá la memoria:\n```\nfree -m\nuptime\n```\ngracias"), QStringLiteral("free -m\nuptime"));
    ver(QStringLiteral("```sh\nsystemctl status betometro\n```"), QStringLiteral("systemctl status betometro"));
    ver(QStringLiteral("```bash\r\ndf -h\r\n```"), QStringLiteral("df -h"));
    ver(QStringLiteral("  \n "), QString());
    ver(QStringLiteral("```\nuno\n```\n```\ndos\n```"), QStringLiteral("uno"));   // el primero
    std::printf(fallas ? "Buzón: %d fallas\n" : "Buzón: bien\n", fallas);
    return fallas ? 1 : 0;
}
