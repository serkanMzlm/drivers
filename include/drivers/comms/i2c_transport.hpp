#pragma once

#include "drivers/comms/itransport.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

namespace drivers::comms {

/// Immutable, validated I2C slave address (7-bit or 10-bit).
class I2CConfig {
public:
    /// Validates and builds a config; returns std::nullopt on invalid input.
    static std::optional<I2CConfig> make(uint16_t address) noexcept {
        constexpr uint16_t k7BitMax = 0x77;
        constexpr uint16_t k10BitMax = 0x3FF;

        const bool ten_bit = address > k7BitMax;
        const uint16_t limit = ten_bit ? k10BitMax : k7BitMax;
        if (address > limit)
            return std::nullopt;

        return I2CConfig(address, ten_bit);
    }

    uint16_t address() const noexcept { return address_; }
    bool tenBit() const noexcept { return ten_bit_; }

private:
    constexpr I2CConfig(uint16_t address, bool ten_bit) noexcept
        : address_(address), ten_bit_(ten_bit) {}

    uint16_t address_;
    bool ten_bit_;
};

/// Linux i2c-dev backed I2C transport. Thread-safe: every public method
/// (bar the constructors) takes `mtx_` for its full duration.
class I2C : public ITransport {
public:
    explicit I2C(uint8_t bus_number);
    I2C(uint8_t bus_number, const I2CConfig& config);
    ~I2C() override;

    bool init() override;
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    void send(const uint8_t* data, size_t length) override;
    bool receive(uint8_t* buffer, size_t length) override;

    /// Validates, applies, and (on hardware rejection) rolls back the slave
    /// address. No-op on the live device until `init()` has been called.
    bool setAddress(uint16_t address);

    bool writeRegister(uint8_t reg, uint8_t value);
    std::optional<uint8_t> readRegister(uint8_t reg);

private:
    /// Selects `target_` on the open fd via ioctl. Caller must hold `mtx_`.
    bool applyAddressLocked();
    /// Closes `fd_` if open and resets it to -1. Caller must hold `mtx_`.
    void closeLocked();
    std::string devicePath() const;

    uint8_t bus_number_{0};
    std::optional<I2CConfig> target_;
    int fd_{-1};

    bool smbus_supported_{false};

    mutable std::mutex mtx_;
};

} // namespace drivers::comms
