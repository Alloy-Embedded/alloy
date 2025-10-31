/**
 * @file alloy/hal/uart.hpp
 * @brief Generic UART interface - automatically maps to vendor-specific implementation
 *
 * This is a portable header that applications should include.
 * CMake automatically redirects to the correct vendor/family implementation
 * based on the ALLOY_MCU configuration.
 *
 * Usage in your application:
 * @code
 * #include "alloy/hal/uart.hpp"
 *
 * using namespace alloy::hal;
 *
 * UartDevice<UsartId::USART1> serial;
 * serial.configure({core::baud_rates::Baud115200});
 * serial.write_byte('H');
 * @endcode
 *
 * The same code works on any MCU - just change ALLOY_MCU in CMake!
 */

#ifndef ALLOY_HAL_UART_HPP
#define ALLOY_HAL_UART_HPP

// This header is automatically configured by CMake
// It includes the vendor-specific implementation based on ALLOY_MCU

#ifndef ALLOY_HAL_IMPL_UART
#error "This header should not be included directly. Use CMake to configure the project."
#endif

// Include the vendor-specific implementation
#include ALLOY_HAL_IMPL_UART

// Re-export into alloy::hal namespace for portability
namespace alloy::hal {
    #ifdef ALLOY_HAL_VENDOR_NAMESPACE
        using namespace ALLOY_HAL_VENDOR_NAMESPACE;
    #endif
}

#endif // ALLOY_HAL_UART_HPP
