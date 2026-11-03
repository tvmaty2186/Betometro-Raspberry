#include "pulsador_gpio.h"

#include <QDir>

#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <unistd.h>

double PulsadorGpio::ahoraMs() {
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

PulsadorGpio::PulsadorGpio(int linea, QObject* padre) : QObject(padre) {
    // El chip de los pines de la placa: en la Pi 3 es «pinctrl-bcm2835» (suele ser gpiochip0)
    const QStringList chips = QDir(QStringLiteral("/dev")).entryList({QStringLiteral("gpiochip*")}, QDir::System);
    int chip = -1;
    for (const QString& n : chips) {
        const int f = ::open(("/dev/" + n).toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
        if (f < 0) continue;
        gpiochip_info info{};
        if (ioctl(f, GPIO_GET_CHIPINFO_IOCTL, &info) == 0 && std::strstr(info.label, "bcm2835") && static_cast<int>(info.lines) > linea) { chip = f; break; }
        ::close(f);
    }
    if (chip < 0) { error_ = QStringLiteral("No encontré los pines de la placa"); return; }

    gpio_v2_line_request pedido{};
    pedido.offsets[0] = static_cast<__u32>(linea);
    pedido.num_lines = 1;
    std::strncpy(pedido.consumer, "betometro-pulsador", sizeof(pedido.consumer) - 1);
    pedido.config.flags = GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_EDGE_RISING | GPIO_V2_LINE_FLAG_EDGE_FALLING |
                          GPIO_V2_LINE_FLAG_BIAS_PULL_UP;   // pull-up interna: suelto = alto, apretado = bajo
    pedido.event_buffer_size = 64;
    if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &pedido) < 0) {
        error_ = QStringLiteral("No pude usar el GPIO%1: %2").arg(linea).arg(QString::fromLocal8Bit(std::strerror(errno)));
        ::close(chip);
        return;
    }
    ::close(chip);
    fd_ = pedido.fd;

    gpio_v2_line_values v{};
    v.mask = 1;
    if (ioctl(fd_, GPIO_V2_LINE_GET_VALUES_IOCTL, &v) == 0) apretado_ = !(v.bits & 1);

    aviso_ = std::make_unique<QSocketNotifier>(fd_, QSocketNotifier::Read);
    connect(aviso_.get(), &QSocketNotifier::activated, this, &PulsadorGpio::leer);
    reloj_.setSingleShot(true);
    connect(&reloj_, &QTimer::timeout, this, &PulsadorGpio::revisar);
}

PulsadorGpio::~PulsadorGpio() {
    if (fd_ >= 0) ::close(fd_);
}

void PulsadorGpio::leer() {
    gpio_v2_line_event ev[16];
    const ssize_t n = ::read(fd_, ev, sizeof(ev));
    if (n <= 0) return;
    for (ssize_t i = 0; i < n / static_cast<ssize_t>(sizeof(gpio_v2_line_event)); i++) {
        const bool bajo = ev[i].id == GPIO_V2_LINE_EVENT_FALLING_EDGE;
        filtro_.flanco(ev[i].timestamp_ns / 1e6, bajo);   // la hora la puso el kernel en la interrupción
        if (bajo != apretado_) { apretado_ = bajo; emit cambio(); }
    }
    revisar();
}

void PulsadorGpio::revisar() {
    const double ahora = ahoraMs();
    if (const auto t = filtro_.revisar(ahora)) emit marca(std::max(0.0, ahora - *t));
    if (const auto prox = filtro_.proximoVencimiento()) reloj_.start(std::max(1, static_cast<int>(*prox - ahora) + 1));
}
