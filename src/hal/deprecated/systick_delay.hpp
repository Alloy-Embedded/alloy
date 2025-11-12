/**
 * Universal SYSTICK_DELAY Header
 *
 * Automatically includes the correct SYSTICK_DELAY implementation based on the platform
 * defined in the build system (CMake).
 *
 * Usage in application code:
 *   #include "hal/systick_delay.hpp"
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
    #include "platform/arm/systick_delay.hpp"
#elif defined(ALLOY_PLATFORM_LINUX)
    #include "platform/linux/systick_delay.hpp"
#elif defined(ALLOY_PLATFORM_SAME70)
    #include "platform/same70/systick_delay.hpp"
#elif defined(ALLOY_PLATFORM_SAMV71)
    #include "platform/samv71/systick_delay.hpp"
#elif defined(ALLOY_PLATFORM_STM32F0)
    #include "platform/stm32f0/systick_delay.hpp"
#elif defined(ALLOY_PLATFORM_STM32F1)
    #include "platform/stm32f1/systick_delay.hpp"
#elif defined(ALLOY_PLATFORM_STM32F4)
    #include "platform/stm32f4/systick_delay.hpp"

#else
    #error "No platform defined! Please define ALLOY_PLATFORM_* in your build system."
    #error "Supported platforms: ALLOY_PLATFORM_ARM, ALLOY_PLATFORM_LINUX, ALLOY_PLATFORM_SAME70, ALLOY_PLATFORM_SAMV71, ALLOY_PLATFORM_STM32F0, ALLOY_PLATFORM_STM32F1, ALLOY_PLATFORM_STM32F4"
    #error "Example CMake: target_compile_definitions(my_target PRIVATE ALLOY_PLATFORM_STM32F4)"
#endif

/**
 * After including this header, you can use the SYSTICK_DELAY implementation
 * for the selected platform without any code changes.
 *
 * Example:
 *   // Works on any platform with SYSTICK_DELAY support
 *   #include "hal/systick_delay.hpp"
 *
 *   // Use SYSTICK_DELAY API (platform-specific implementation)
 *   // ... your code here ...
 */
