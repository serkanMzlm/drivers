#include "drivers/comms/spi_transport.hpp"

#include "posix_device.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

using namespace drivers::comms;

SPI::SPI(uint8_t bus_number, uint8_t chip_select)
    : bus_number_(bus_number), chip_select_(chip_select), config_(SpiConfig::defaults()) {}

SPI::SPI(uint8_t bus_number, uint8_t chip_select, const SpiConfig& config)
    : bus_number_(bus_number), chip_select_(chip_select), config_(config) {}

SPI::~SPI() {
    disconnect();
}

std::string SPI::devicePath() const {
    return "/dev/spidev" + std::to_string(bus_number_) + "." + std::to_string(chip_select_);
}

bool SPI::init() {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ >= 0)
        return true;

    const std::string path = devicePath();

    fd_ = detail::openCharDevice(path, O_RDWR);
    if (fd_ < 0) {
        std::cerr << "SPI: failed to open device " << path << ": " << std::strerror(errno) << "\n";
        return false;
    }

    if (!applyConfigLocked()) {
        closeLocked();
        return false;
    }
    return true;
}

bool SPI::applyConfigLocked() {
    if (fd_ < 0)
        return false;

    uint8_t mode = config_.mode();
    if (ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0) {
        std::cerr << "SPI: failed to set mode: " << std::strerror(errno) << "\n";
        return false;
    }

    uint8_t lsb = config_.lsbFirst() ? 1 : 0;
    if (ioctl(fd_, SPI_IOC_WR_LSB_FIRST, &lsb) < 0) {
        std::cerr << "SPI: failed to set bit order: " << std::strerror(errno) << "\n";
        return false;
    }

    uint8_t bits = config_.bitsPerWord();
    if (ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        std::cerr << "SPI: failed to set bits per word: " << std::strerror(errno) << "\n";
        return false;
    }

    uint32_t speed = config_.speedHz();
    if (ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        std::cerr << "SPI: failed to set max speed: " << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool SPI::connect() {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0) {
        std::cerr << "SPI: init() must be called before connect()\n";
        return false;
    }

    return applyConfigLocked();
}

void SPI::closeLocked() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

void SPI::disconnect() {
    std::lock_guard<std::mutex> lk(mtx_);
    closeLocked();
}

bool SPI::isConnected() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return fd_ >= 0;
}

bool SPI::transfer(const uint8_t* tx, uint8_t* rx, size_t length) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (fd_ < 0 || length == 0 || length > MAX_BUFFER_SIZE)
        return false;

    std::vector<uint8_t> dummy_tx;
    const uint8_t* tx_ptr = tx;
    if (tx_ptr == nullptr) {
        dummy_tx.assign(length, 0x00);
        tx_ptr = dummy_tx.data();
    }

    spi_ioc_transfer tr{};
    tr.tx_buf = reinterpret_cast<uintptr_t>(tx_ptr);
    tr.rx_buf = reinterpret_cast<uintptr_t>(rx);
    tr.len = static_cast<uint32_t>(length);
    tr.speed_hz = config_.speedHz();
    tr.bits_per_word = config_.bitsPerWord();
    tr.cs_change = 0;

    if (ioctl(fd_, SPI_IOC_MESSAGE(1), &tr) < 0) {
        std::cerr << "SPI: transfer failed: " << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}

void SPI::send(const uint8_t* data, size_t length) {
    if (data == nullptr)
        return;

    transfer(data, nullptr, length);
}

bool SPI::receive(uint8_t* buffer, size_t length) {
    if (buffer == nullptr)
        return false;

    return transfer(nullptr, buffer, length);
}

bool SPI::writeRegister(uint8_t reg, uint8_t value) {
    const uint8_t tx[2] = {static_cast<uint8_t>(reg & 0x7F), value};
    return transfer(tx, nullptr, sizeof(tx));
}

std::optional<uint8_t> SPI::readRegister(uint8_t reg) {
    const uint8_t tx[2] = {static_cast<uint8_t>(reg | 0x80), 0x00};
    uint8_t rx[2] = {0, 0};

    if (!transfer(tx, rx, sizeof(tx)))
        return std::nullopt;

    return rx[1];
}

bool SPI::setMode(uint8_t mode) {
    std::lock_guard<std::mutex> lk(mtx_);

    auto next = config_.withMode(mode);
    if (!next) {
        std::cerr << "SPI: invalid mode " << static_cast<int>(mode) << " (must be 0-3)\n";
        return false;
    }

    const SpiConfig previous = config_;
    config_ = *next;

    if (fd_ >= 0 && !applyConfigLocked()) {
        config_ = previous;
        if (!applyConfigLocked())
            std::cerr << "SPI: rollback to previous config also failed\n";
        return false;
    }
    return true;
}

bool SPI::setSpeed(uint32_t speed_hz) {
    std::lock_guard<std::mutex> lk(mtx_);

    auto next = config_.withSpeed(speed_hz);
    if (!next) {
        std::cerr << "SPI: invalid speed " << speed_hz << " Hz\n";
        return false;
    }

    const SpiConfig previous = config_;
    config_ = *next;

    if (fd_ >= 0 && !applyConfigLocked()) {
        config_ = previous;
        if (!applyConfigLocked())
            std::cerr << "SPI: rollback to previous config also failed\n";
        return false;
    }
    return true;
}

bool SPI::setBitsPerWord(uint8_t bits) {
    std::lock_guard<std::mutex> lk(mtx_);

    auto next = config_.withBitsPerWord(bits);
    if (!next) {
        std::cerr << "SPI: invalid bits per word " << static_cast<int>(bits) << "\n";
        return false;
    }

    const SpiConfig previous = config_;
    config_ = *next;

    if (fd_ >= 0 && !applyConfigLocked()) {
        config_ = previous;
        if (!applyConfigLocked())
            std::cerr << "SPI: rollback to previous config also failed\n";
        return false;
    }
    return true;
}

bool SPI::setBitOrder(bool lsb_first) {
    std::lock_guard<std::mutex> lk(mtx_);

    auto next = config_.withBitOrder(lsb_first);
    if (!next)
        return false;

    const SpiConfig previous = config_;
    config_ = *next;

    if (fd_ >= 0 && !applyConfigLocked()) {
        config_ = previous;
        if (!applyConfigLocked())
            std::cerr << "SPI: rollback to previous config also failed\n";
        return false;
    }
    return true;
}
