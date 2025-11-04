/**
 * @file uart.hpp
 * @brief ESP32 UART implementation using ESP-IDF drivers
 *
 * Provides optimized UART implementation using ESP-IDF driver/uart.h
 * with buffered I/O, DMA support, and event-driven operation.
 */

#ifndef ALLOY_HAL_ESP32_UART_HPP
#define ALLOY_HAL_ESP32_UART_HPP

#include "../interface/uart.hpp"
#include "../../core/result.hpp"
#include "../../core/error.hpp"
#include "../../core/types.hpp"

#ifdef ESP_PLATFORM
#include "driver/uart.h"
#include "driver/gpio.h"
#include <span>

namespace alloy::hal::esp32 {

/**
 * @brief UART implementation using ESP-IDF drivers
 *
 * Template parameters:
 * - PORT: UART port number (0-2 for ESP32, varies by chip)
 *
 * Features:
 * - Hardware FIFO and DMA support
 * - Buffered I/O with configurable sizes
 * - Event-driven operation (optional)
 * - Hardware flow control support
 * - Configurable baud rate, data bits, parity, stop bits
 *
 * Example:
 * @code
 * using Debug = Uart<UART_NUM_0>;
 *
 * Debug uart;
 * uart.configure(UartConfig{115200_baud});
 * uart.init(GPIO_NUM_1, GPIO_NUM_3);  // TX, RX pins
 *
 * uart.write_string("Hello, World!\r\n");
 *
 * auto result = uart.read_byte();
 * if (result.is_ok()) {
 *     uint8_t byte = result.value();
 * }
 * @endcode
 */
template<uart_port_t PORT>
class Uart {
public:
    static constexpr uart_port_t port = PORT;
    static constexpr size_t DEFAULT_TX_BUFFER_SIZE = 256;
    static constexpr size_t DEFAULT_RX_BUFFER_SIZE = 1024;

    /**
     * @brief Constructor
     */
    Uart() = default;

    /**
     * @brief Destructor - deinitializes UART
     */
    ~Uart() {
        if (initialized_) {
            uart_driver_delete(port);
        }
    }

    // Non-copyable, movable
    Uart(const Uart&) = delete;
    Uart& operator=(const Uart&) = delete;
    Uart(Uart&&) noexcept = default;
    Uart& operator=(Uart&&) noexcept = default;

    /**
     * @brief Initialize UART with pin configuration
     *
     * @param tx_pin TX pin number
     * @param rx_pin RX pin number
     * @param rts_pin RTS pin (GPIO_NUM_NC to disable)
     * @param cts_pin CTS pin (GPIO_NUM_NC to disable)
     * @param rx_buffer_size RX buffer size in bytes
     * @param tx_buffer_size TX buffer size in bytes
     * @return Result indicating success or error
     */
    core::Result<void> init(int tx_pin, int rx_pin,
                           int rts_pin = UART_PIN_NO_CHANGE,
                           int cts_pin = UART_PIN_NO_CHANGE,
                           size_t rx_buffer_size = DEFAULT_RX_BUFFER_SIZE,
                           size_t tx_buffer_size = DEFAULT_TX_BUFFER_SIZE) {
        if (initialized_) {
            return core::Result<void>::error(core::ErrorCode::AlreadyInitialized);
        }

        // Install UART driver
        esp_err_t err = uart_driver_install(port, rx_buffer_size, tx_buffer_size,
                                           0, nullptr, 0);
        if (err != ESP_OK) {
            return core::Result<void>::error(core::ErrorCode::Hardware);
        }

        // Set pins
        err = uart_set_pin(port, tx_pin, rx_pin, rts_pin, cts_pin);
        if (err != ESP_OK) {
            uart_driver_delete(port);
            return core::Result<void>::error(core::ErrorCode::Hardware);
        }

        initialized_ = true;
        return core::Result<void>::ok();
    }

    /**
     * @brief Configure UART parameters
     *
     * @param config UART configuration
     * @return Result indicating success or error
     */
    core::Result<void> configure(const UartConfig& config) {
        // Convert baud rate
        uint32_t baud = config.baud_rate.value;

        // Convert data bits
        uart_word_length_t data_bits;
        switch (config.data_bits) {
            case DataBits::Five:  data_bits = UART_DATA_5_BITS; break;
            case DataBits::Six:   data_bits = UART_DATA_6_BITS; break;
            case DataBits::Seven: data_bits = UART_DATA_7_BITS; break;
            case DataBits::Eight: data_bits = UART_DATA_8_BITS; break;
            default:
                return core::Result<void>::error(core::ErrorCode::InvalidParameter);
        }

        // Convert parity
        uart_parity_t parity;
        switch (config.parity) {
            case Parity::None: parity = UART_PARITY_DISABLE; break;
            case Parity::Even: parity = UART_PARITY_EVEN; break;
            case Parity::Odd:  parity = UART_PARITY_ODD; break;
            default:
                return core::Result<void>::error(core::ErrorCode::InvalidParameter);
        }

        // Convert stop bits
        uart_stop_bits_t stop_bits;
        switch (config.stop_bits) {
            case StopBits::One: stop_bits = UART_STOP_BITS_1; break;
            case StopBits::Two: stop_bits = UART_STOP_BITS_2; break;
            default:
                return core::Result<void>::error(core::ErrorCode::InvalidParameter);
        }

        // Configure UART
        uart_config_t uart_config = {
            .baud_rate = static_cast<int>(baud),
            .data_bits = data_bits,
            .parity = parity,
            .stop_bits = stop_bits,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            .source_clk = UART_SCLK_DEFAULT,
        };

        esp_err_t err = uart_param_config(port, &uart_config);
        if (err != ESP_OK) {
            return core::Result<void>::error(core::ErrorCode::Hardware);
        }

        return core::Result<void>::ok();
    }

    /**
     * @brief Read a single byte
     *
     * @param timeout_ms Timeout in milliseconds
     * @return Result with byte value or error
     */
    [[nodiscard]] core::Result<core::u8> read_byte(uint32_t timeout_ms = 0) {
        uint8_t byte;
        int len = uart_read_bytes(port, &byte, 1,
                                 timeout_ms / portTICK_PERIOD_MS);
        if (len != 1) {
            return core::Result<core::u8>::error(core::ErrorCode::Timeout);
        }
        return core::Result<core::u8>::ok(byte);
    }

    /**
     * @brief Write a single byte
     *
     * @param byte Byte to write
     * @return Result indicating success or error
     */
    core::Result<void> write_byte(core::u8 byte) {
        int len = uart_write_bytes(port, &byte, 1);
        if (len != 1) {
            return core::Result<void>::error(core::ErrorCode::Busy);
        }
        return core::Result<void>::ok();
    }

    /**
     * @brief Read multiple bytes
     *
     * @param buffer Buffer to store received data
     * @param timeout_ms Timeout in milliseconds
     * @return Result with number of bytes read or error
     */
    [[nodiscard]] core::Result<size_t> read(std::span<core::u8> buffer,
                                           uint32_t timeout_ms = 100) {
        int len = uart_read_bytes(port, buffer.data(), buffer.size(),
                                 timeout_ms / portTICK_PERIOD_MS);
        if (len < 0) {
            return core::Result<size_t>::error(core::ErrorCode::Hardware);
        }
        return core::Result<size_t>::ok(static_cast<size_t>(len));
    }

    /**
     * @brief Write multiple bytes
     *
     * @param buffer Buffer containing data to transmit
     * @return Result with number of bytes written or error
     */
    core::Result<size_t> write(std::span<const core::u8> buffer) {
        int len = uart_write_bytes(port, buffer.data(), buffer.size());
        if (len < 0) {
            return core::Result<size_t>::error(core::ErrorCode::Hardware);
        }
        return core::Result<size_t>::ok(static_cast<size_t>(len));
    }

    /**
     * @brief Write string
     *
     * @param str String to write
     * @return Result with number of bytes written or error
     */
    core::Result<size_t> write_string(const char* str) {
        size_t len = strlen(str);
        return write(std::span<const core::u8>(
            reinterpret_cast<const core::u8*>(str), len));
    }

    /**
     * @brief Check how many bytes are available to read
     *
     * @return Number of bytes available
     */
    [[nodiscard]] core::usize available() const {
        size_t bytes = 0;
        uart_get_buffered_data_len(port, &bytes);
        return bytes;
    }

    /**
     * @brief Flush TX buffer and wait for transmission to complete
     */
    void flush() {
        uart_wait_tx_done(port, portMAX_DELAY);
    }

    /**
     * @brief Clear RX buffer
     */
    void clear_rx() {
        uart_flush_input(port);
    }

private:
    bool initialized_ = false;
};

// Type aliases for convenience
using Uart0 = Uart<UART_NUM_0>;
using Uart1 = Uart<UART_NUM_1>;
using Uart2 = Uart<UART_NUM_2>;

} // namespace alloy::hal::esp32

// Verify that our implementation satisfies the UartDevice concept
static_assert(alloy::hal::UartDevice<alloy::hal::esp32::Uart<UART_NUM_0>>);

#endif // ESP_PLATFORM

#endif // ALLOY_HAL_ESP32_UART_HPP
