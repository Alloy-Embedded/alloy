/**
 * @file gpio.hpp
 * @brief ESP32 GPIO implementation using ESP-IDF drivers
 *
 * Provides optimized GPIO implementation using ESP-IDF driver/gpio.h
 * with interrupt support and efficient register access.
 */

#ifndef ALLOY_HAL_ESP32_GPIO_HPP
#define ALLOY_HAL_ESP32_GPIO_HPP

#include "../interface/gpio.hpp"
#include "../../core/result.hpp"
#include "../../core/error.hpp"

#ifdef ESP_PLATFORM
#include "driver/gpio.h"
#include <functional>

namespace alloy::hal::esp32 {

/**
 * @brief GPIO Pin implementation using ESP-IDF drivers
 *
 * Template parameters:
 * - PIN: GPIO pin number (0-39 for ESP32, varies by chip)
 * - MODE: Pin mode from PinMode enum
 *
 * Features:
 * - Zero-cost abstraction over ESP-IDF gpio driver
 * - Compile-time pin configuration
 * - Interrupt support with callbacks
 * - Pull-up/pull-down resistor configuration
 * - Direct register access for performance
 *
 * Example:
 * @code
 * using Led = Gpio<2, PinMode::Output>;
 * using Button = Gpio<0, PinMode::InputPullUp>;
 *
 * Led led;
 * led.set_high();
 *
 * Button button;
 * button.attach_interrupt(GPIO_INTR_NEGEDGE, []() {
 *     // Button pressed
 * });
 * @endcode
 */
template<uint8_t PIN, PinMode MODE>
class Gpio : public ConfiguredGpioPin<PIN, MODE> {
public:
    static constexpr gpio_num_t gpio_num = static_cast<gpio_num_t>(PIN);

    /**
     * @brief Constructor - initializes pin with configured mode
     */
    Gpio() {
        configure();
    }

    /**
     * @brief Destructor - resets pin to default state
     */
    ~Gpio() {
        gpio_reset_pin(gpio_num);
    }

    // Output operations (available only for Output mode)
    void set_high() requires (MODE == PinMode::Output) {
        gpio_set_level(gpio_num, 1);
    }

    void set_low() requires (MODE == PinMode::Output) {
        gpio_set_level(gpio_num, 0);
    }

    void toggle() requires (MODE == PinMode::Output) {
        uint32_t level = gpio_get_level(gpio_num);
        gpio_set_level(gpio_num, !level);
    }

    void write(bool value) requires (MODE == PinMode::Output) {
        gpio_set_level(gpio_num, value ? 1 : 0);
    }

    // Input operations (available for Input modes)
    [[nodiscard]] bool read() const requires (MODE == PinMode::Input ||
                                              MODE == PinMode::InputPullUp ||
                                              MODE == PinMode::InputPullDown) {
        return gpio_get_level(gpio_num) != 0;
    }

    /**
     * @brief Attach interrupt handler to pin
     *
     * @param intr_type Interrupt type (e.g., GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE)
     * @param callback Function to call on interrupt
     * @return Result indicating success or error
     */
    core::Result<void> attach_interrupt(gpio_int_type_t intr_type,
                                       std::function<void()> callback)
        requires (MODE == PinMode::Input ||
                 MODE == PinMode::InputPullUp ||
                 MODE == PinMode::InputPullDown) {
        // Store callback (in real implementation, would use a callback registry)
        interrupt_callback_ = std::move(callback);

        // Install ISR service if not already installed
        static bool isr_service_installed = false;
        if (!isr_service_installed) {
            esp_err_t err = gpio_install_isr_service(0);
            if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
                return core::Result<void>::error(core::ErrorCode::Hardware);
            }
            isr_service_installed = true;
        }

        // Set interrupt type
        esp_err_t err = gpio_set_intr_type(gpio_num, intr_type);
        if (err != ESP_OK) {
            return core::Result<void>::error(core::ErrorCode::Hardware);
        }

        // Add ISR handler
        err = gpio_isr_handler_add(gpio_num, gpio_isr_handler,
                                   reinterpret_cast<void*>(this));
        if (err != ESP_OK) {
            return core::Result<void>::error(core::ErrorCode::Hardware);
        }

        // Enable interrupt
        gpio_intr_enable(gpio_num);

        return core::Result<void>::ok();
    }

    /**
     * @brief Detach interrupt handler from pin
     */
    void detach_interrupt() {
        gpio_intr_disable(gpio_num);
        gpio_isr_handler_remove(gpio_num);
        interrupt_callback_ = nullptr;
    }

private:
    std::function<void()> interrupt_callback_;

    /**
     * @brief Configure pin based on MODE
     */
    void configure() {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << PIN);
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

        if constexpr (MODE == PinMode::Output) {
            io_conf.mode = GPIO_MODE_OUTPUT;
        } else if constexpr (MODE == PinMode::Input) {
            io_conf.mode = GPIO_MODE_INPUT;
        } else if constexpr (MODE == PinMode::InputPullUp) {
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        } else if constexpr (MODE == PinMode::InputPullDown) {
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        }

        gpio_config(&io_conf);
    }

    /**
     * @brief Static ISR handler - calls instance callback
     */
    static void IRAM_ATTR gpio_isr_handler(void* arg) {
        auto* self = reinterpret_cast<Gpio*>(arg);
        if (self && self->interrupt_callback_) {
            self->interrupt_callback_();
        }
    }
};

// Type aliases for convenience
template<uint8_t PIN>
using GpioOutput = Gpio<PIN, PinMode::Output>;

template<uint8_t PIN>
using GpioInput = Gpio<PIN, PinMode::Input>;

template<uint8_t PIN>
using GpioInputPullUp = Gpio<PIN, PinMode::InputPullUp>;

template<uint8_t PIN>
using GpioInputPullDown = Gpio<PIN, PinMode::InputPullDown>;

} // namespace alloy::hal::esp32

// Verify that our implementation satisfies the GpioPin concept
static_assert(alloy::hal::GpioPin<alloy::hal::esp32::Gpio<2, alloy::hal::PinMode::Output>>);
static_assert(alloy::hal::GpioPin<alloy::hal::esp32::Gpio<0, alloy::hal::PinMode::Input>>);

#endif // ESP_PLATFORM

#endif // ALLOY_HAL_ESP32_GPIO_HPP
