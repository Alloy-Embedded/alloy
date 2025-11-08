/**
 * Universal I2C Header
 *
 * Automatically includes the correct I2C implementation based on the platform
 * defined in the build system (CMake).
 *
 * Usage in application code:
 *   #include "hal/i2c.hpp"
 *
 * The build system defines ALLOY_PLATFORM which determines which implementation to include.
 *
 * Supported platforms: arm, linux, same70, samv71, stm32f0, stm32f1, stm32f4
 *
 * Example CMake:
 *   set(ALLOY_PLATFORM "stm32f4")
 */

#pragma once

#if defined(ALLOY_PLATFORM_ARM)
    #include "platform/arm/i2c.hpp"
#elif defined(ALLOY_PLATFORM_LINUX)
    #include "platform/linux/i2c.hpp"
#elif defined(ALLOY_PLATFORM_SAME70)
    #include "platform/same70/i2c.hpp"
#elif defined(ALLOY_PLATFORM_SAMV71)
    #include "platform/samv71/i2c.hpp"
#elif defined(ALLOY_PLATFORM_STM32F0)
    #include "platform/stm32f0/i2c.hpp"
#elif defined(ALLOY_PLATFORM_STM32F1)
    #include "platform/stm32f1/i2c.hpp"
#elif defined(ALLOY_PLATFORM_STM32F4)
    #include "platform/stm32f4/i2c.hpp"

#else
    #error "No platform defined! Please define ALLOY_PLATFORM_* in your build system."
    #error "Supported platforms: ALLOY_PLATFORM_ARM, ALLOY_PLATFORM_LINUX, ALLOY_PLATFORM_SAME70, ALLOY_PLATFORM_SAMV71, ALLOY_PLATFORM_STM32F0, ALLOY_PLATFORM_STM32F1, ALLOY_PLATFORM_STM32F4"
    #error "Example CMake: target_compile_definitions(my_target PRIVATE ALLOY_PLATFORM_STM32F4)"
#endif

/**
 * After including this header, you can use the I2C implementation
 * for the selected platform without any code changes.
 *
 * Example:
 *   // Works on any platform with I2C support
 *   #include "hal/i2c.hpp"
 *
 *   // Use I2C API (platform-specific implementation)
 *   // ... your code here ...
 */
