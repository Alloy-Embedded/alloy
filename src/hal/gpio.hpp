/**
 * Universal GPIO Header
 *
 * Automatically includes the correct GPIO implementation based on the MCU
 * defined in the build system (CMake).
 *
 * Usage in application code:
 *   #include "hal/gpio.hpp"
 *
 *   using namespace gpio;
 *   using LED = GPIOPin<pins::PC13>;
 *   LED::configureOutput();
 *   LED::set();
 *
 * The build system must define one of the following macros:
 *   - MCU_STM32F103C8, MCU_STM32F103CB, etc.
 *   - MCU_STM32F407VG, MCU_STM32F407ZG, etc.
 *   - Or any other supported MCU variant
 *
 * Example CMake:
 *   target_compile_definitions(my_app PRIVATE MCU_STM32F407VG)
 */

#pragma once

// ============================================================================
// STM32F0 Family
// ============================================================================
#if defined(MCU_STM32F030C6)
    #include "vendors/st/stm32f0/stm32f030c6/gpio.hpp"
#elif defined(MCU_STM32F030C8)
    #include "vendors/st/stm32f0/stm32f030c8/gpio.hpp"

// ============================================================================
// STM32F1 Family
// ============================================================================
#elif defined(MCU_STM32F103C4)
    #include "vendors/st/stm32f1/stm32f103c4/gpio.hpp"
#elif defined(MCU_STM32F103C6)
    #include "vendors/st/stm32f1/stm32f103c6/gpio.hpp"
#elif defined(MCU_STM32F103C8)
    #include "vendors/st/stm32f1/stm32f103c8/gpio.hpp"
#elif defined(MCU_STM32F103CB)
    #include "vendors/st/stm32f1/stm32f103cb/gpio.hpp"
#elif defined(MCU_STM32F103RB)
    #include "vendors/st/stm32f1/stm32f103rb/gpio.hpp"
#elif defined(MCU_STM32F103RC)
    #include "vendors/st/stm32f1/stm32f103rc/gpio.hpp"
#elif defined(MCU_STM32F103RE)
    #include "vendors/st/stm32f1/stm32f103re/gpio.hpp"

// ============================================================================
// STM32F4 Family
// ============================================================================
#elif defined(MCU_STM32F401CC)
    #include "vendors/st/stm32f4/stm32f401cc/gpio.hpp"
#elif defined(MCU_STM32F401CE)
    #include "vendors/st/stm32f4/stm32f401ce/gpio.hpp"

#elif defined(MCU_STM32F405RG)
    #include "vendors/st/stm32f4/stm32f405rg/gpio.hpp"

#elif defined(MCU_STM32F407VE)
    #include "vendors/st/stm32f4/stm32f407ve/gpio.hpp"
#elif defined(MCU_STM32F407VG)
    #include "vendors/st/stm32f4/stm32f407vg/gpio.hpp"
#elif defined(MCU_STM32F407ZG)
    #include "vendors/st/stm32f4/stm32f407zg/gpio.hpp"

#elif defined(MCU_STM32F411CE)
    #include "vendors/st/stm32f4/stm32f411ce/gpio.hpp"
#elif defined(MCU_STM32F411RE)
    #include "vendors/st/stm32f4/stm32f411re/gpio.hpp"

#elif defined(MCU_STM32F429ZI)
    #include "vendors/st/stm32f4/stm32f429zi/gpio.hpp"

// ============================================================================
// STM32F7 Family
// ============================================================================
#elif defined(MCU_STM32F722RE)
    #include "vendors/st/stm32f7/stm32f722re/gpio.hpp"
#elif defined(MCU_STM32F722ZE)
    #include "vendors/st/stm32f7/stm32f722ze/gpio.hpp"

#elif defined(MCU_STM32F745VG)
    #include "vendors/st/stm32f7/stm32f745vg/gpio.hpp"
#elif defined(MCU_STM32F745ZG)
    #include "vendors/st/stm32f7/stm32f745zg/gpio.hpp"
#elif defined(MCU_STM32F746VG)
    #include "vendors/st/stm32f7/stm32f746vg/gpio.hpp"
#elif defined(MCU_STM32F746ZG)
    #include "vendors/st/stm32f7/stm32f746zg/gpio.hpp"

#elif defined(MCU_STM32F765VI)
    #include "vendors/st/stm32f7/stm32f765vi/gpio.hpp"
#elif defined(MCU_STM32F767ZI)
    #include "vendors/st/stm32f7/stm32f767zi/gpio.hpp"

// ============================================================================
// Error: No MCU defined
// ============================================================================
#else
    #error "No MCU defined! Please define one of the supported MCU macros (e.g., MCU_STM32F407VG) in your build system."
    #error "Supported MCUs: STM32F030, STM32F103, STM32F401, STM32F405, STM32F407, STM32F411, STM32F429, STM32F7xx"
    #error "Example CMake: target_compile_definitions(my_target PRIVATE MCU_STM32F407VG)"
#endif

/**
 * After including this header, you can use:
 *
 * - namespace gpio - Convenience namespace for the selected MCU
 * - GPIOPin<pin> - GPIO pin template
 * - pins::* - Pin definitions (PA0, PA1, PC13, etc)
 * - pin_functions::* - Peripheral pin functions (USART1_TX, I2C1_SCL, etc)
 *
 * Example:
 *   using LED = GPIOPin<pins::PC13>;
 *   LED::configureOutput();
 *   LED::set();
 *
 *   using UART_TX = GPIOPin<pin_functions::usart::USART1_TX>;
 *   UART_TX::configureAlternateFunction(7);  // AF7 for USART1 (STM32F4)
 */
