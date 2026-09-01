#pragma once

#include <cstddef>
#include <cstdint>

namespace drivers::comms {

/// Common interface for byte-oriented hardware transports (UART, I2C, SPI).
/// Implementations must be internally thread-safe: any public method may be
/// called concurrently from multiple threads.
class ITransport {
public:
    ITransport() = default;
    virtual ~ITransport() = default;

    ITransport(const ITransport&) = delete;
    ITransport& operator=(const ITransport&) = delete;
    ITransport(ITransport&&) = delete;
    ITransport& operator=(ITransport&&) = delete;

    /// Opens and configures the underlying device. Idempotent: returns true
    /// immediately if already open.
    virtual bool init() = 0;

    /// Re-applies the current configuration to an already-`init()`'d device.
    virtual bool connect() = 0;

    /// Closes the underlying device, if open. Safe to call repeatedly.
    virtual void disconnect() = 0;

    /// Returns true if the underlying device is currently open.
    virtual bool isConnected() const = 0;

    /// Writes exactly `length` bytes from `data`. Logs and discards the
    /// payload on error or partial write; does not throw.
    virtual void send(const uint8_t* data, size_t length) = 0;

    /// Reads exactly `length` bytes into `buffer`. Returns false without
    /// partial results being usable if the transfer could not be completed.
    virtual bool receive(uint8_t* buffer, size_t length) = 0;

protected:
    static constexpr size_t MAX_BUFFER_SIZE = 1024;
};

} // namespace drivers::comms
