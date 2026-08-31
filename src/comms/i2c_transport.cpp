#include "drivers/comms/i2c_transport.hpp"

extern "C" {
#include <fcntl.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstring>
#include <iostream>

using namespace drivers::comms;

I2C::I2C(uint8_t bus_number) : bus_number_(bus_number) {}

I2C::I2C(uint8_t bus_number, const uint8_t address) : bus_number_(bus_number) {
    setAddress(address);
}

I2C::~I2C() {
    disconnect();
}

std::string I2C::devicePath() const {
    return "/dev/i2c-" + std::to_string(bus_number_);
}

bool I2C::setAddress(uint16_t address) {
    auto addr = I2CAddress::make(address);
    if (!addr) {
        std::cerr << "I2C: invalid address 0x" << std::hex << address << std::dec << "\n";
        return false;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    target_ = std::move(addr);

    if (fd_ > 0)
        return applyAddress();

    return true;
}

bool I2C::applyAddress() {
    if (fd_ < 0 || !target_)
        return false;

    if (ioctl(fd_, I2C_TENBIT, target_->tenBit() ? 1 : 0) < 0) {
        std::cerr << "I2C: failed to set 10-bit mode: " << strerror(errno) << "\n";
        return false;
    }

    if (ioctl(fd_, I2C_SLAVE, static_cast<unsigned long>(target_->address())) < 0) {
        std::cerr << "I2C: failed to set slave address 0x" << std::hex << target_->address()
                  << std::dec << ": " << std::strerror(errno) << "\n";
        return false;
    }

    return true;
}

bool I2C::init() {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ > 0)
        return true;

    const std::string path = devicePath();

    fd_ = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
    if (fd_ < 0) {
        std::cerr << "I2C: failed to open bus " << path << ": " << std::strerror(errno) << "\n";
        return false;
    }

    unsigned long funcs = 0;
    if (ioctl(fd_, I2C_FUNCS, &funcs) == 0) {
        smbus_supported_ = (funcs & I2C_FUNC_SMBUS_BYTE_DATA) != 0;
    } else {
        smbus_supported_ = false;
        std::cerr << "I2C: I2C_FUNCS query failed, assuming no SMBus: " << std::strerror(errno)
                  << "\n";
    }

    return true;
}

bool I2C::connect() {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0) {
        std::cerr << "I2C: init() must be called before connect()\n";
        return false;
    }
    if (!target_) {
        std::cerr << "I2C: connect() called without an address set\n";
        return false;
    }

    return applyAddress();
}

void I2C::disconnect() {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool I2C::isConnected() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return fd_ >= 0 && target_.has_value();
}

void I2C::setTimeout(uint32_t milliseconds) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (milliseconds < MIN_TIMEOUT)
        milliseconds = MIN_TIMEOUT;
    if (milliseconds > MAX_TIMEOUT)
        milliseconds = MAX_TIMEOUT;
    timeout_ = milliseconds;

    if (fd_ >= 0) {
        unsigned long units = timeout_ / 10;
        if (units == 0)
            units = 1;
        if (ioctl(fd_, I2C_TIMEOUT, units) < 0)
            std::cerr << "I2C: failed to set timeout: " << std::strerror(errno) << "\n";
    }
}

void I2C::send(const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || data == nullptr || length == 0 || length > MAX_BUFFER_SIZE)
        return;

    const ssize_t written = ::write(fd_, data, length);
    if (written < 0 || static_cast<size_t>(written) != length)
        std::cerr << "I2C: send failed: " << std::strerror(errno) << "\n";
}

bool I2C::receive(uint8_t* buffer, size_t length) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || buffer == nullptr || length == 0 || length > MAX_BUFFER_SIZE)
        return false;

    const ssize_t got = ::read(fd_, buffer, length);
    if (got < 0 || static_cast<size_t>(got) != length) {
        std::cerr << "I2C: receive failed: " << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool I2C::write(uint8_t* byte) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || byte == nullptr)
        return false;

    return ::write(fd_, byte, 1) == 1;
}

bool I2C::read(uint8_t* byte) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || byte == nullptr)
        return false;

    return ::read(fd_, byte, 1) == 1;
}

bool I2C::writeRegister(uint8_t reg, uint8_t value) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (fd_ < 0 || !target_)
        return false;

    if (smbus_supported_) {
        if (i2c_smbus_write_byte_data(fd_, reg, value) < 0) {
            std::cerr << "I2C: SMBus writeRegister failed (reg 0x" << std::hex
                      << static_cast<int>(reg) << std::dec << "): " << std::strerror(errno) << "\n";
            return false;
        }
        return true;
    }

    const uint8_t buf[2] = {reg, value};
    const ssize_t written = ::write(fd_, buf, sizeof(buf));

    if (written != static_cast<ssize_t>(sizeof(buf))) {
        std::cerr << "I2C: raw writeRegister failed (reg 0x" << std::hex << static_cast<int>(reg)
                  << std::dec << "): " << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}

std::optional<uint8_t> I2C::readRegister(uint8_t reg) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (fd_ < 0 || !target_)
        return std::nullopt;

    if (smbus_supported_) {
        const int result = i2c_smbus_read_byte_data(fd_, reg);
        if (result < 0) {
            std::cerr << "I2C: SMBus readRegister failed (reg 0x" << std::hex
                      << static_cast<int>(reg) << std::dec << "): " << std::strerror(errno) << "\n";
            return std::nullopt;
        }
        return static_cast<uint8_t>(result);
    }

    if (::write(fd_, &reg, 1) != 1) {
        std::cerr << "I2C: raw readRegister select failed (reg 0x" << std::hex
                  << static_cast<int>(reg) << std::dec << "): " << std::strerror(errno) << "\n";
        return std::nullopt;
    }

    uint8_t value = 0;
    if (::read(fd_, &value, 1) != 1) {
        std::cerr << "I2C: raw readRegister read failed (reg 0x" << std::hex
                  << static_cast<int>(reg) << std::dec << "): " << std::strerror(errno) << "\n";
        return std::nullopt;
    }
    return value;
}
