/**
 * @file alloy/hal/gpio.hpp
 * @brief Generic GPIO interface - automatically maps to vendor-specific implementation
 *
 * This is a portable header that applications should include.
 * CMake automatically redirects to the correct vendor/family implementation
 * based on the ALLOY_MCU configuration.
 *
 * Usage in your application:
 * @code
 * #include "alloy/hal/gpio.hpp"
 *
 * using namespace alloy::hal;
 *
 * GpioPin<13> led;
 * led.configure(PinMode::Output);
 * led.set_high();
 * @endcode
 *
 * The same code works on any MCU - just change ALLOY_MCU in CMake!
 */

#ifndef ALLOY_HAL_GPIO_HPP
#define ALLOY_HAL_GPIO_HPP

// This header is automatically configured by CMake
// It includes the vendor-specific implementation based on ALLOY_MCU

#ifndef ALLOY_HAL_IMPL_GPIO
#error "This header should not be included directly. Use CMake to configure the project."
#endif

// Include the vendor-specific implementation
// ALLOY_HAL_IMPL_GPIO is defined by CMake as the path to the real implementation
#include ALLOY_HAL_IMPL_GPIO

// Re-export into alloy::hal namespace for portability
namespace alloy::hal {
    // The vendor-specific implementation is already in alloy::hal::vendor::family
    // We use 'using' to bring it into the generic alloy::hal namespace

    // Import from the vendor-specific namespace
    #ifdef ALLOY_HAL_VENDOR_NAMESPACE
        using namespace ALLOY_HAL_VENDOR_NAMESPACE;
    #endif
}

#endif // ALLOY_HAL_GPIO_HPP
