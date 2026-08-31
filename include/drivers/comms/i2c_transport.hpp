#pragma once

#include "drivers/comms/itransport.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

namespace drivers::comms {

class I2CAddress {
public:
    static std::optional<I2CAddress> make(uint16_t address) noexcept {
        constexpr uint16_t k7BitMax = 0x77;
        constexpr uint16_t k10BitMax = 0x3FF;

        const bool ten_bit = address > k7BitMax;
        const uint16_t limit = ten_bit ? k10BitMax : k7BitMax;
        if (address > limit)
            return std::nullopt;

        return I2CAddress(address, ten_bit);
    }

    int16_t address() const noexcept { return address_; }
    bool tenBit() const noexcept { return ten_bit_; }

private:
    constexpr I2CAddress(uint16_t address, bool ten_bit) noexcept
        : address_(address), ten_bit_(ten_bit) {}

    uint16_t address_;
    bool ten_bit_;
};

class I2C : public ITransport {
public:
    explicit I2C(uint8_t bus_number);
    I2C(uint8_t bus_number, const uint8_t address);
    ~I2C() override;

    bool init() override;
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    void setTimeout(uint32_t milliseconds) override;

    void send(const uint8_t* data, size_t length) override;
    bool receive(uint8_t* buffer, size_t length) override;
    bool write(uint8_t* byte) override;
    bool read(uint8_t* byte) override;

    bool setAddress(uint16_t address);
    bool writeRegister(uint8_t reg, uint8_t value);
    std::optional<uint8_t> readRegister(uint8_t reg);

private:
    bool applyAddress();
    std::string devicePath() const;

private:
    uint8_t bus_number_{0};
    std::optional<I2CAddress> target_;
    int fd_{-1};

    bool smbus_supported_{false};

    mutable std::mutex mtx_;
};

} // namespace drivers::comms