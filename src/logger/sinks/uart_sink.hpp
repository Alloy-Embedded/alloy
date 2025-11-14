/**
 * @file uart_sink.hpp
 * @brief UART Logger Sink
 *
 * Provides a logger sink that outputs to UART for embedded systems.
 * Supports both blocking and non-blocking operation.
 */

#pragma once

#include "logger/sink.hpp"
#include <cstring>

namespace alloy::logger {

/**
 * @brief UART sink for logger output
 *
 * Outputs log messages to a UART peripheral. Can operate in blocking
 * or non-blocking mode depending on the UART implementation.
 *
 * @tparam UartImpl UART implementation (must have write() method)
 *
 * Example:
 * @code
 * // Create UART instance
 * auto uart = Uart<>::quick_setup_tx_only<TxPin>(BaudRate{115200});
 *
 * // Create sink
 * UartSink<decltype(uart)> uart_sink(uart);
 *
 * // Add to logger
 * Logger::add_sink(&uart_sink);
 * @endcode
 */
template <typename UartImpl>
class UartSink : public Sink {
public:
    /**
     * @brief Construct UART sink
     *
     * @param uart Reference to UART instance (must remain valid)
     */
    explicit UartSink(UartImpl& uart) : uart_(uart), ready_(true) {}

    /**
     * @brief Check if sink is ready to accept data
     *
     * @return true if UART is ready
     */
    bool is_ready() const override {
        return ready_;
    }

    /**
     * @brief Write data to UART
     *
     * Outputs the formatted log message to UART.
     *
     * @param data Pointer to data buffer
     * @param size Number of bytes to write
     */
    void write(const char* data, size_t size) override {
        if (!ready_ || data == nullptr || size == 0) {
            return;
        }

        // Write to UART (blocking)
        // Note: For production, consider adding timeout or non-blocking option
        for (size_t i = 0; i < size; ++i) {
            uart_.write_byte(data[i]);
        }
    }

    /**
     * @brief Flush any buffered data
     *
     * For UART, this is typically a no-op as data is transmitted immediately.
     */
    void flush() override {
        // UART typically transmits immediately, so nothing to flush
        // If using buffered UART, implement flushing here
    }

    /**
     * @brief Enable or disable the sink
     *
     * @param enabled true to enable, false to disable
     */
    void set_enabled(bool enabled) {
        ready_ = enabled;
    }

private:
    UartImpl& uart_;
    bool ready_;
};

/**
 * @brief Helper function to create UART sink
 *
 * Deduces template parameters automatically.
 *
 * @tparam UartImpl UART implementation type
 * @param uart UART instance
 * @return UartSink<UartImpl>
 */
template <typename UartImpl>
UartSink<UartImpl> make_uart_sink(UartImpl& uart) {
    return UartSink<UartImpl>(uart);
}

}  // namespace alloy::logger
