#pragma once

#include "../sink.hpp"

namespace alloy::logger {

/**
 * UART output sink for logger
 *
 * Writes log messages to a UART peripheral.
 * Template-based to work with any UART implementation.
 *
 * Usage:
 *   UartSink<Uart0> uart_sink(my_uart);
 *   Logger::add_sink(&uart_sink);
 *
 * @tparam UartImpl UART implementation (must have write() method)
 */
template <typename UartImpl>
class UartSink : public Sink {
   public:
    /**
     * Construct UART sink
     *
     * @param uart Reference to UART peripheral (must remain valid)
     */
    explicit UartSink(UartImpl& uart) : uart_(uart) {}

    /**
     * Write log message to UART
     *
     * @param data Log message
     * @param length Message length
     */
    void write(const char* data, size_t length) override {
        // Write message to UART
        uart_.write(reinterpret_cast<const uint8_t*>(data), length);
    }

    /**
     * Flush UART buffer (if supported)
     */
    void flush() override {
        // Most UART implementations don't need explicit flush
        // Override if your UART has a buffer that needs flushing
    }

    /**
     * Check if UART is ready
     *
     * @return true if UART is initialized
     */
    bool is_ready() const override { return uart_.is_initialized(); }

   private:
    UartImpl& uart_;
};

}  // namespace alloy::logger
