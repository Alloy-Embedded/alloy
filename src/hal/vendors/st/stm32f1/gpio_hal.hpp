#pragma once

#include <cstdint>

/**
 * STM32F1 GPIO HAL
 *
 * STM32F1 uses CRL/CRH configuration registers (different from F4/F7)
 * - CRL: Configures pins 0-7 (4 bits per pin)
 * - CRH: Configures pins 8-15 (4 bits per pin)
 *
 * TODO: Implement full STM32F1 GPIO HAL with CRL/CRH support
 */

namespace alloy::hal::stm32f1 {

/**
 * GPIO Pin Template for STM32F1
 * Hardware: MCU-specific hardware definitions (register addresses)
 * Pin: Pin number (0-127, calculated as port*16 + pin_number)
 */
template<typename Hardware, uint8_t Pin>
class GPIOPin {
public:
    static constexpr uint8_t PIN_NUM = Pin % 16;
    static constexpr uint8_t PORT_NUM = Pin / 16;

    // TODO: Implement STM32F1-specific GPIO functions
    // - configureOutput() using CRL/CRH
    // - configureInput() using CRL/CRH
    // - configureAlternateFunction() using remapping
    // - set(), clear(), toggle(), read()

    static void configureOutput() {
        // Placeholder implementation
    }

    static void set() {
        // Placeholder implementation
    }

    static void clear() {
        // Placeholder implementation
    }
};

}  // namespace alloy::hal::stm32f1
