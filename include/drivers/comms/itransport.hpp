#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace drivers::comms {

class ITransport {
public:
    ITransport() = default;
    virtual ~ITransport() = default;

    ITransport(const ITransport&) = delete;
    ITransport& operator=(const ITransport&) = delete;
    ITransport(ITransport&&) = delete;
    ITransport& operator=(ITransport&&) = delete;

    virtual bool init() = 0;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual void setTimeout(uint32_t milliseconds) = 0;

    virtual void send(const uint8_t* data, size_t length) = 0;
    virtual bool receive(uint8_t* buffer, size_t length) = 0;
    virtual bool write(uint8_t* byte) = 0;
    virtual bool read(uint8_t* byte) = 0;

protected:
    uint32_t timeout_{1000}; // Default timeout in milliseconds

    static constexpr size_t MAX_BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;
    static constexpr size_t DEFAULT_BUFFER_SIZE = 256;
    static constexpr size_t MAX_TIMEOUT = 5000;
    static constexpr size_t MIN_TIMEOUT = 100;
};
} // namespace drivers::comms