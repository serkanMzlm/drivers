#include "drivers/comms/uart_transport.hpp"

#include "posix_device.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>

using namespace drivers::comms;

bool UART::init() {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ >= 0)
        return true;

    fd_ = detail::openCharDevice(device_path_, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ < 0) {
        std::cerr << "UART: failed to open device " << device_path_ << ": " << std::strerror(errno)
                  << "\n";
        return false;
    }

    if (::fcntl(fd_, F_SETFL, 0) < 0) {
        std::cerr << "UART: fcntl() failed: " << std::strerror(errno) << "\n";
        closeLocked();
        return false;
    }

    if (!applyConfigLocked()) {
        closeLocked();
        return false;
    }
    return true;
}

bool UART::applyConfigLocked() {
    if (fd_ < 0)
        return false;

    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) {
        std::cerr << "UART: tcgetattr failed: " << std::strerror(errno) << "\n";
        return false;
    }

    const speed_t spd = config_.toTermiosBaud();
    cfsetispeed(&tty, spd);
    cfsetospeed(&tty, spd);

    tty.c_cflag &= ~static_cast<tcflag_t>(PARENB | CSTOPB | CSIZE | CRTSCTS);
    tty.c_cflag |= static_cast<tcflag_t>(CS8 | CREAD | CLOCAL);
    tty.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO | ECHOE | ECHONL | ISIG);
    tty.c_iflag &= ~static_cast<tcflag_t>(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP |
                                          INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~static_cast<tcflag_t>(OPOST | ONLCR);

    switch (config_.dataBits()) {
    case 5:
        tty.c_cflag |= CS5;
        break;
    case 6:
        tty.c_cflag |= CS6;
        break;
    case 7:
        tty.c_cflag |= CS7;
        break;
    default:
        tty.c_cflag |= CS8;
        break;
    }

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    switch (config_.parity()) {
    case UartConfig::Parity::None:
        tty.c_cflag &= ~PARENB;
        break;
    case UartConfig::Parity::Even:
        tty.c_cflag |= PARENB;
        tty.c_cflag &= ~PARODD;
        break;
    case UartConfig::Parity::Odd:
        tty.c_cflag |= PARENB;
        tty.c_cflag |= PARODD;
        break;
    }

    if (config_.stopBits() == UartConfig::StopBits::Two)
        tty.c_cflag |= CSTOPB;
    else
        tty.c_cflag &= ~CSTOPB;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        std::cerr << "UART: tcsetattr failed: " << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool UART::connect() {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0) {
        std::cerr << "UART: init() must be called before connect()\n";
        return false;
    }
    return applyConfigLocked();
}

void UART::closeLocked() {
    if (fd_ >= 0) {
        ::tcdrain(fd_);
        ::close(fd_);
        fd_ = -1;
    }
}

void UART::disconnect() {
    std::lock_guard<std::mutex> lk(mtx_);
    closeLocked();
}

bool UART::isConnected() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return fd_ >= 0;
}

void UART::send(const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || data == nullptr || length == 0 || length > MAX_BUFFER_SIZE)
        return;

    const ssize_t written = ::write(fd_, data, length);
    if (written < 0 || static_cast<size_t>(written) != length)
        std::cerr << "UART: send failed: " << std::strerror(errno) << "\n";
}

bool UART::receive(uint8_t* buffer, size_t length) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || buffer == nullptr || length == 0 || length > MAX_BUFFER_SIZE)
        return false;

    size_t total = 0;
    while (total < length) {
        const ssize_t got = ::read(fd_, buffer + total, length - total);
        if (got < 0) {
            std::cerr << "UART: receive failed: " << std::strerror(errno) << "\n";
            return false;
        }
        if (got == 0)
            return false;
        total += static_cast<size_t>(got);
    }
    return true;
}

bool UART::setBaud(uint32_t baud) {
    std::lock_guard<std::mutex> lk(mtx_);

    auto next = config_.withBaud(baud);
    if (!next) {
        std::cerr << "UART: unsupported baud " << baud << "\n";
        return false;
    }

    const UartConfig previous = config_;
    config_ = *next;

    if (fd_ >= 0 && !applyConfigLocked()) {
        config_ = previous;
        if (!applyConfigLocked())
            std::cerr << "UART: rollback to previous config also failed\n";
        return false;
    }
    return true;
}

bool UART::setParity(UartConfig::Parity parity) {
    std::lock_guard<std::mutex> lk(mtx_);

    auto next = config_.withParity(parity);
    if (!next)
        return false;

    const UartConfig previous = config_;
    config_ = *next;

    if (fd_ >= 0 && !applyConfigLocked()) {
        config_ = previous;
        if (!applyConfigLocked())
            std::cerr << "UART: rollback to previous config also failed\n";
        return false;
    }
    return true;
}

bool UART::setStopBits(UartConfig::StopBits stop_bits) {
    std::lock_guard<std::mutex> lk(mtx_);

    auto next = config_.withStopBits(stop_bits);
    if (!next)
        return false;

    const UartConfig previous = config_;
    config_ = *next;

    if (fd_ >= 0 && !applyConfigLocked()) {
        config_ = previous;
        if (!applyConfigLocked())
            std::cerr << "UART: rollback to previous config also failed\n";
        return false;
    }
    return true;
}

bool UART::setDataBits(uint8_t data_bits) {
    std::lock_guard<std::mutex> lk(mtx_);

    auto next = config_.withDataBits(data_bits);
    if (!next) {
        std::cerr << "UART: invalid data bits " << static_cast<int>(data_bits) << "\n";
        return false;
    }

    const UartConfig previous = config_;
    config_ = *next;

    if (fd_ >= 0 && !applyConfigLocked()) {
        config_ = previous;
        if (!applyConfigLocked())
            std::cerr << "UART: rollback to previous config also failed\n";
        return false;
    }
    return true;
}
