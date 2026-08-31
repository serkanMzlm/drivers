#pragma once

#include <drivers/comms/itransport.hpp>
#include <mutex>
#include <optional>

namespace drivers::comms {

class SpiConfig {
public:
    static SpiConfig defaults() noexcept { return SpiConfig{0, 8, 500000, false}; }

    static std::optional<SpiConfig> make(uint8_t mode, uint8_t bits_per_word, uint32_t speed_hz,
                                         bool lsb_first) noexcept {
        if (mode > 3 || speed_hz == 0 || bits_per_word < 0 || bits_per_word > 32)
            return std::nullopt;

        return SpiConfig{mode, bits_per_word, speed_hz, lsb_first};
    }

    std::optional<SpiConfig> withMode(uint8_t mode) const noexcept {
        return make(mode, bits_per_word_, speed_hz_, lsb_first_);
    }
    std::optional<SpiConfig> withSpeed(uint32_t speed_hz) const noexcept {
        return make(mode_, bits_per_word_, speed_hz, lsb_first_);
    }
    std::optional<SpiConfig> withBitsPerWord(uint8_t bits) const noexcept {
        return make(mode_, bits, speed_hz_, lsb_first_);
    }
    std::optional<SpiConfig> withBitOrder(bool lsb_first) const noexcept {
        return make(mode_, bits_per_word_, speed_hz_, lsb_first);
    }

    uint8_t mode() const noexcept { return mode_; }
    uint8_t bitsPerWord() const noexcept { return bits_per_word_; }
    uint32_t speedHz() const noexcept { return speed_hz_; }
    bool lsbFirst() const noexcept { return lsb_first_; }

private:
    constexpr SpiConfig(uint8_t mode, uint8_t bits, uint32_t speed, bool lsb) noexcept
        : mode_(mode), bits_per_word_(bits), speed_hz_(speed), lsb_first_(lsb) {}

    uint8_t mode_;
    uint8_t bits_per_word_;
    uint32_t speed_hz_;
    bool lsb_first_;
};

class SPI : public ITransport {
public:
    SPI(uint8_t bus_number, uint8_t chip_select);
    SPI(uint8_t bus_number, uint8_t chip_select, const SpiConfig& config);
    ~SPI() override;

    bool init() override;
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    void setTimeout(uint32_t milliseconds) override;

    void send(const uint8_t* data, size_t length) override;
    bool receive(uint8_t* buffer, size_t length) override;
    bool write(uint8_t* byte) override;
    bool read(uint8_t* byte) override;

    bool transfer(const uint8_t* tx, uint8_t* rx, size_t length);

    bool writeRegister(uint8_t reg, uint8_t value);
    std::optional<uint8_t> readRegister(uint8_t reg);

    bool setMode(uint8_t mode);
    bool setSpeed(uint32_t speed_hz);
    bool setBitsPerWord(uint8_t bits);
    bool setBitOrder(bool lsb_first);

private:
    bool applyConfig();
    std::string devicePath() const;

    uint8_t bus_number_{0};
    uint8_t chip_select_{0};
    SpiConfig config_;
    int fd_{-1};

    mutable std::mutex mtx_;
};
} // namespace drivers::comms