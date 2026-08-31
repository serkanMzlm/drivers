#include "drivers/comms/uart_transport.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <utility>

using namespace drivers::comms;

bool UART::init() {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ >= 0)
        return true;

    fd_ = ::open(device_path_.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC | O_NDELAY);
    if (fd_ < 0) {
        std::cerr << "UART: failed to open device " << device_path_ << ": " << std::strerror(errno)
                  << "\n";
        return false;
    }

    if (::fcntl(fd_, F_SETFL, 0) < 0) {
        std::cerr << "UART: fcntl() failed: " << std::strerror(errno) << "\n";
        disconnect();
        return false;
    }

    if (!applyConfig()) {
        disconnect();
        return false;
    }
    return true;
}

bool UART::applyConfig() {
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
    tty.c_iflag &= ~static_cast<tcflag_t>(IXON | IXOFF | IXANY | IGNBRK | BRKINT |
                                          PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
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

    // Blocking read: en az 1 byte gelene kadar bekle, timeout yok.
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    // Parity.
    switch (config_.parity()) {
    case UartConfig::Parity::None:
        tty.c_cflag &= ~PARENB; // Parity üretme/kontrol etme.
        break;
    case UartConfig::Parity::Even:
        tty.c_cflag |= PARENB;  // Parity etkin.
        tty.c_cflag &= ~PARODD; // Çift (even).
        break;
    case UartConfig::Parity::Odd:
        tty.c_cflag |= PARENB;
        tty.c_cflag |= PARODD; // Tek (odd).
        break;
    }

    if (config_.stopBits() == UartConfig::StopBits::Two)
        tty.c_cflag |= CSTOPB; 
    else
        tty.c_cflag &= ~CSTOPB;

    tty.c_cflag &= ~CRTSCTS;

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
    return applyConfig();
}

void UART::disconnect() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (fd_ >= 0) {
        ::tcdrain(fd_);
        ::close(fd_);
        fd_ = -1;
    }
}

bool UART::isConnected() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return fd_ >= 0;
}

void UART::setTimeout(uint32_t milliseconds) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (milliseconds < MIN_TIMEOUT)
        milliseconds = MIN_TIMEOUT;
    if (milliseconds > MAX_TIMEOUT)
        milliseconds = MAX_TIMEOUT;
    timeout_ = milliseconds;
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

bool UART::write(uint8_t* byte) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || byte == nullptr)
        return false;

    return ::write(fd_, byte, 1) == 1;
}

bool UART::read(uint8_t* byte) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || byte == nullptr)
        return false;

    return ::read(fd_, byte, 1) == 1; 
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

    if (fd_ >= 0 && !applyConfig()) {
        config_ = previous; // Donanım reddetti → geri sar.
        applyConfig();      // Bilinen-iyi config'i yeniden yükle.
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

    if (fd_ >= 0 && !applyConfig()) {
        config_ = previous;
        applyConfig();
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

    if (fd_ >= 0 && !applyConfig()) {
        config_ = previous;
        applyConfig();
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

    if (fd_ >= 0 && !applyConfig()) {
        config_ = previous;
        applyConfig();
        return false;
    }
    return true;
}