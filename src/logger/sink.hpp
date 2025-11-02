#pragma once

#include <cstddef>

namespace alloy {
namespace logger {

/**
 * Base class for logger output sinks
 *
 * A sink receives formatted log messages and writes them to an output
 * destination (UART, file, network, memory buffer, etc.)
 *
 * Example implementation:
 *
 * class MySink : public Sink {
 * public:
 *     void write(const char* data, size_t length) override {
 *         // Write to your output device
 *         my_device.send(data, length);
 *     }
 *
 *     void flush() override {
 *         // Optional: flush buffered data
 *         my_device.flush();
 *     }
 * };
 */
class Sink {
public:
    virtual ~Sink() = default;

    /**
     * Write formatted log message to the sink
     *
     * @param data Pointer to formatted message (null-terminated)
     * @param length Length of message in bytes (excluding null terminator)
     *
     * Note: This method will be called from the Logger with mutex held (on RTOS)
     *       Keep implementation fast and non-blocking if possible
     */
    virtual void write(const char* data, size_t length) = 0;

    /**
     * Flush any buffered data (optional)
     *
     * Default implementation does nothing.
     * Override if your sink buffers data.
     */
    virtual void flush() {}

    /**
     * Check if sink is ready to receive data (optional)
     *
     * Default implementation returns true.
     * Override if your sink needs initialization or can be in not-ready state.
     *
     * @return true if sink is ready, false otherwise
     */
    virtual bool is_ready() const { return true; }
};

} // namespace logger
} // namespace alloy
