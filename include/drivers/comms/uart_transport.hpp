#pragma once

#include "drivers/comms/itransport.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <termios.h>
#include <utility>

namespace drivers::comms {

class UartConfig {
public:
    enum class Parity : uint8_t { None, Even, Odd };
    enum class StopBits : uint8_t { One, Two };

    static UartConfig defaults() noexcept {
        return UartConfig{115200, 8, Parity::None, StopBits::One};
    }

    static std::optional<UartConfig> make(uint32_t baud, uint8_t data_bits, Parity parity,
                                          StopBits stop_bits) noexcept {
        if (!isSupportedBaud(baud))
            return std::nullopt;
        if (data_bits < 5 || data_bits > 8)
            return std::nullopt;
        return UartConfig{baud, data_bits, parity, stop_bits};
    }

    std::optional<UartConfig> withBaud(uint32_t baud) const noexcept {
        return make(baud, data_bits_, parity_, stop_bits_);
    }
    std::optional<UartConfig> withDataBits(uint8_t data_bits) const noexcept {
        return make(baud_, data_bits, parity_, stop_bits_);
    }
    std::optional<UartConfig> withParity(Parity parity) const noexcept {
        return make(baud_, data_bits_, parity, stop_bits_);
    }
    std::optional<UartConfig> withStopBits(StopBits stop_bits) const noexcept {
        return make(baud_, data_bits_, parity_, stop_bits);
    }

    uint32_t baud() const noexcept { return baud_; }
    speed_t toTermiosBaud() const noexcept {
        for (const auto& [b, s] : kBaudMap) {
            if (baud_ == b)
                return s;
        }

        return B0;
    }
    uint8_t dataBits() const noexcept { return data_bits_; }
    Parity parity() const noexcept { return parity_; }
    StopBits stopBits() const noexcept { return stop_bits_; }

    static bool isSupportedBaud(uint32_t baud) noexcept {
        for (const auto& [b, s] : kBaudMap) {
            if (baud == b)
                return true;
        }
        return false;
    }

private:
    static constexpr std::pair<uint32_t, speed_t> kBaudMap[] = {
        {9600, B9600},     {19200, B19200},   {38400, B38400},   {57600, B57600},
        {115200, B115200}, {230400, B230400}, {460800, B460800}, {921600, B921600},
    };

    constexpr UartConfig(uint32_t baud, uint8_t data_bits, Parity parity,
                         StopBits stop_bits) noexcept
        : baud_(baud), data_bits_(data_bits), parity_(parity), stop_bits_(stop_bits) {}

    uint32_t baud_;
    uint8_t data_bits_;
    Parity parity_;
    StopBits stop_bits_;
};

class UART : public ITransport {
public:
    explicit UART(std::string device_path)
        : device_path_(std::move(device_path)), config_(UartConfig::defaults()) {}

    UART(std::string device_path, const UartConfig& config)
        : device_path_(std::move(device_path)), config_(config) {}
        
    ~UART() override { disconnect(); }

    bool init() override;
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    void setTimeout(uint32_t milliseconds) override;

    void send(const uint8_t* data, size_t length) override;
    bool receive(uint8_t* buffer, size_t length) override;
    bool write(uint8_t* byte) override;
    bool read(uint8_t* byte) override;

    bool setBaud(uint32_t baud);
    bool setParity(UartConfig::Parity parity);
    bool setStopBits(UartConfig::StopBits stop_bits);
    bool setDataBits(uint8_t data_bits);

private:
    bool applyConfig();

    std::string device_path_;
    UartConfig config_;
    int fd_{-1};

    mutable std::mutex mtx_;
};

} // namespace drivers::comms