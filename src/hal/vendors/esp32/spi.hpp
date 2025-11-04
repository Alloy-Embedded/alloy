/**
 * @file spi.hpp
 * @brief ESP32 SPI implementation using ESP-IDF drivers
 *
 * Provides optimized SPI master implementation using ESP-IDF driver/spi_master.h
 * with DMA support and transaction queuing.
 */

#ifndef ALLOY_HAL_ESP32_SPI_HPP
#define ALLOY_HAL_ESP32_SPI_HPP

#include "../interface/spi.hpp"
#include "../../core/result.hpp"
#include "../../core/error.hpp"
#include "../../core/types.hpp"

#ifdef ESP_PLATFORM
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <span>

namespace alloy::hal::esp32 {

/**
 * @brief SPI Master implementation using ESP-IDF drivers
 *
 * Template parameters:
 * - HOST: SPI host (SPI2_HOST or SPI3_HOST for ESP32)
 *
 * Features:
 * - Hardware SPI with DMA support
 * - Up to 80 MHz clock speed
 * - All 4 SPI modes supported
 * - Transaction queuing for efficiency
 * - Flexible data sizes (8-bit, 16-bit, 32-bit)
 * - Half-duplex and full-duplex modes
 *
 * Example:
 * @code
 * using Spi2 = SpiMaster<SPI2_HOST>;
 *
 * Spi2 spi;
 * spi.init(GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_13);  // CLK, MISO, MOSI
 * spi.configure(SpiConfig{SpiMode::Mode0, 1000000});  // 1 MHz
 *
 * // Add device
 * auto device = spi.add_device(GPIO_NUM_15);  // CS pin
 *
 * // Transfer data
 * uint8_t tx_data[] = {0x01, 0x02, 0x03};
 * uint8_t rx_data[3];
 * device.transfer(tx_data, rx_data);
 * @endcode
 */
template<spi_host_device_t HOST>
class SpiMaster {
public:
    static constexpr spi_host_device_t host = HOST;

    /**
     * @brief SPI device handle
     */
    class Device {
    public:
        Device() = default;
        ~Device() {
            if (handle_) {
                spi_bus_remove_device(handle_);
            }
        }

        Device(Device&& other) noexcept : handle_(other.handle_) {
            other.handle_ = nullptr;
        }

        Device& operator=(Device&& other) noexcept {
            if (this != &other) {
                if (handle_) {
                    spi_bus_remove_device(handle_);
                }
                handle_ = other.handle_;
                other.handle_ = nullptr;
            }
            return *this;
        }

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        /**
         * @brief Transfer data (full duplex)
         *
         * @param tx_buffer Data to transmit
         * @param rx_buffer Buffer for received data
         * @return Result indicating success or error
         */
        core::Result<void> transfer(std::span<const core::u8> tx_buffer,
                                   std::span<core::u8> rx_buffer) {
            if (!handle_) {
                return core::Result<void>::error(core::ErrorCode::NotInitialized);
            }

            size_t length = std::min(tx_buffer.size(), rx_buffer.size());
            if (length == 0) {
                return core::Result<void>::ok();
            }

            spi_transaction_t trans = {};
            trans.length = length * 8;  // Length in bits
            trans.tx_buffer = tx_buffer.data();
            trans.rx_buffer = rx_buffer.data();

            esp_err_t err = spi_device_transmit(handle_, &trans);
            if (err != ESP_OK) {
                return core::Result<void>::error(core::ErrorCode::Hardware);
            }

            return core::Result<void>::ok();
        }

        /**
         * @brief Transmit data only
         *
         * @param buffer Data to transmit
         * @return Result indicating success or error
         */
        core::Result<void> transmit(std::span<const core::u8> buffer) {
            if (!handle_) {
                return core::Result<void>::error(core::ErrorCode::NotInitialized);
            }

            if (buffer.empty()) {
                return core::Result<void>::ok();
            }

            spi_transaction_t trans = {};
            trans.length = buffer.size() * 8;  // Length in bits
            trans.tx_buffer = buffer.data();
            trans.rx_buffer = nullptr;

            esp_err_t err = spi_device_transmit(handle_, &trans);
            if (err != ESP_OK) {
                return core::Result<void>::error(core::ErrorCode::Hardware);
            }

            return core::Result<void>::ok();
        }

        /**
         * @brief Receive data only
         *
         * @param buffer Buffer for received data
         * @return Result indicating success or error
         */
        core::Result<void> receive(std::span<core::u8> buffer) {
            if (!handle_) {
                return core::Result<void>::error(core::ErrorCode::NotInitialized);
            }

            if (buffer.empty()) {
                return core::Result<void>::ok();
            }

            spi_transaction_t trans = {};
            trans.rxlength = buffer.size() * 8;  // Length in bits
            trans.tx_buffer = nullptr;
            trans.rx_buffer = buffer.data();

            esp_err_t err = spi_device_transmit(handle_, &trans);
            if (err != ESP_OK) {
                return core::Result<void>::error(core::ErrorCode::Hardware);
            }

            return core::Result<void>::ok();
        }

    private:
        friend class SpiMaster;
        spi_device_handle_t handle_ = nullptr;

        explicit Device(spi_device_handle_t handle) : handle_(handle) {}
    };

    /**
     * @brief Constructor
     */
    SpiMaster() = default;

    /**
     * @brief Destructor - deinitializes SPI bus
     */
    ~SpiMaster() {
        if (initialized_) {
            spi_bus_free(host);
        }
    }

    // Non-copyable, movable
    SpiMaster(const SpiMaster&) = delete;
    SpiMaster& operator=(const SpiMaster&) = delete;
    SpiMaster(SpiMaster&&) noexcept = default;
    SpiMaster& operator=(SpiMaster&&) noexcept = default;

    /**
     * @brief Initialize SPI bus with pin configuration
     *
     * @param clk_pin Clock pin
     * @param miso_pin MISO pin (GPIO_NUM_NC to disable)
     * @param mosi_pin MOSI pin (GPIO_NUM_NC to disable)
     * @param max_transfer_size Maximum transfer size in bytes (0 = default 4092)
     * @param use_dma Enable DMA for transfers
     * @return Result indicating success or error
     */
    core::Result<void> init(int clk_pin, int miso_pin, int mosi_pin,
                           int max_transfer_size = 0,
                           bool use_dma = true) {
        if (initialized_) {
            return core::Result<void>::error(core::ErrorCode::AlreadyInitialized);
        }

        spi_bus_config_t bus_config = {};
        bus_config.mosi_io_num = mosi_pin;
        bus_config.miso_io_num = miso_pin;
        bus_config.sclk_io_num = clk_pin;
        bus_config.quadwp_io_num = -1;
        bus_config.quadhd_io_num = -1;
        bus_config.max_transfer_sz = max_transfer_size;

        esp_err_t err = spi_bus_initialize(host, &bus_config,
                                          use_dma ? SPI_DMA_CH_AUTO : SPI_DMA_DISABLED);
        if (err != ESP_OK) {
            return core::Result<void>::error(core::ErrorCode::Hardware);
        }

        initialized_ = true;
        return core::Result<void>::ok();
    }

    /**
     * @brief Add SPI device to bus
     *
     * @param cs_pin Chip select pin
     * @param config SPI configuration
     * @return Result with Device handle or error
     */
    core::Result<Device> add_device(int cs_pin, const SpiConfig& config = {}) {
        if (!initialized_) {
            return core::Result<Device>::error(core::ErrorCode::NotInitialized);
        }

        spi_device_interface_config_t dev_config = {};
        dev_config.clock_speed_hz = config.clock_speed;
        dev_config.mode = static_cast<uint8_t>(config.mode);
        dev_config.spics_io_num = cs_pin;
        dev_config.queue_size = 7;

        // Set bit order
        if (config.bit_order == SpiBitOrder::LsbFirst) {
            dev_config.flags |= SPI_DEVICE_BIT_LSBFIRST;
        }

        spi_device_handle_t handle;
        esp_err_t err = spi_bus_add_device(host, &dev_config, &handle);
        if (err != ESP_OK) {
            return core::Result<Device>::error(core::ErrorCode::Hardware);
        }

        return core::Result<Device>::ok(Device(handle));
    }

private:
    bool initialized_ = false;
};

// Type aliases for convenience
using Spi2 = SpiMaster<SPI2_HOST>;
using Spi3 = SpiMaster<SPI3_HOST>;

} // namespace alloy::hal::esp32

#endif // ESP_PLATFORM

#endif // ALLOY_HAL_ESP32_SPI_HPP
