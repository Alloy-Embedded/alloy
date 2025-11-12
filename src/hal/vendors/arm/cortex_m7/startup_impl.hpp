#pragma once

/**
 * @file startup_impl.hpp
 * @brief Modern C++23 Startup Implementation
 *
 * Provides modern, type-safe startup sequence for ARM Cortex-M.
 * All operations use standard library algorithms for clarity and safety.
 */

#include <cstdint>
#include <cstring>
#include <algorithm>
#include "init_hooks.hpp"

namespace alloy::hal::arm {

/**
 * @brief Modern C++23 startup implementation
 *
 * Replaces legacy C-style startup with modern C++ idioms:
 * - Uses std::copy for .data initialization
 * - Uses std::fill for .bss initialization
 * - Uses std::for_each for constructor calls
 * - Type-safe, readable, maintainable
 */
class StartupImpl {
public:
    /**
     * @brief Initialize .data section (copy from flash to RAM)
     *
     * Uses modern std::copy instead of manual loop.
     *
     * @param src_start Pointer to .data in flash (_sidata)
     * @param dst_start Pointer to .data start in RAM (_sdata)
     * @param dst_end Pointer to .data end in RAM (_edata)
     *
     * @example
     * @code
     * extern uint32_t _sidata, _sdata, _edata;
     * StartupImpl::init_data_section(&_sidata, &_sdata, &_edata);
     * @endcode
     */
    static void init_data_section(
        uint32_t* src_start,
        uint32_t* dst_start,
        uint32_t* dst_end
    ) {
        // Modern C++: use std::copy
        std::copy(
            src_start,
            src_start + (dst_end - dst_start),
            dst_start
        );
    }

    /**
     * @brief Initialize .bss section (zero memory)
     *
     * Uses modern std::fill instead of manual loop.
     *
     * @param start Pointer to .bss start (_sbss)
     * @param end Pointer to .bss end (_ebss)
     *
     * @example
     * @code
     * extern uint32_t _sbss, _ebss;
     * StartupImpl::init_bss_section(&_sbss, &_ebss);
     * @endcode
     */
    static void init_bss_section(
        uint32_t* start,
        uint32_t* end
    ) {
        // Modern C++: use std::fill
        std::fill(start, end, 0);
    }

    /**
     * @brief Call C++ static constructors
     *
     * Iterates through .init_array section and calls each constructor.
     *
     * @param start Pointer to __init_array_start
     * @param end Pointer to __init_array_end
     *
     * @example
     * @code
     * extern void (*__init_array_start)();
     * extern void (*__init_array_end)();
     * StartupImpl::call_init_array(&__init_array_start, &__init_array_end);
     * @endcode
     */
    static void call_init_array(
        void (**start)(),
        void (**end)()
    ) {
        // Call each constructor in order
        std::for_each(start, end, [](auto fn) {
            if (fn) fn();
        });
    }

    /**
     * @brief Full startup sequence
     *
     * Executes complete startup with initialization hooks.
     *
     * @tparam Config Configuration providing memory layout information
     *
     * Sequence:
     * 1. early_init() hook
     * 2. Initialize .data section
     * 3. Initialize .bss section
     * 4. Call static constructors
     * 5. pre_main_init() hook
     * 6. Call main()
     * 7. Infinite loop (if main returns)
     *
     * @example
     * @code
     * extern "C" [[noreturn]] void Reset_Handler() {
     *     StartupImpl::startup_sequence<Same70StartupConfig>();
     * }
     * @endcode
     */
    template<typename Config>
    [[noreturn]]
    static void startup_sequence() {
        // 1. Early initialization (before .data/.bss)
        early_init();

        // 2. Initialize .data section
        init_data_section(
            Config::data_src_start(),
            Config::data_dst_start(),
            Config::data_dst_end()
        );

        // 3. Initialize .bss section
        init_bss_section(
            Config::bss_start(),
            Config::bss_end()
        );

        // 4. Call static constructors
        call_init_array(
            Config::init_array_start(),
            Config::init_array_end()
        );

        // 5. Pre-main initialization
        pre_main_init();

        // 6. Call main
        extern int main();
        main();

        // 7. If main returns, infinite loop
        while (true) {
            __asm volatile("wfi");  // Wait for interrupt (save power)
        }
    }

    /**
     * @brief Minimal startup sequence (no hooks)
     *
     * For cases where hooks are not needed.
     *
     * @tparam Config Configuration providing memory layout information
     */
    template<typename Config>
    [[noreturn]]
    static void minimal_startup_sequence() {
        // Initialize .data and .bss
        init_data_section(
            Config::data_src_start(),
            Config::data_dst_start(),
            Config::data_dst_end()
        );

        init_bss_section(
            Config::bss_start(),
            Config::bss_end()
        );

        // Call static constructors
        call_init_array(
            Config::init_array_start(),
            Config::init_array_end()
        );

        // Call main
        extern int main();
        main();

        // Infinite loop
        while (true) {
            __asm volatile("wfi");
        }
    }

    /**
     * @brief Custom startup sequence
     *
     * Allows applications to customize each step.
     *
     * @tparam Config Configuration providing memory layout
     * @tparam PreDataFn Function to call before .data init
     * @tparam PostDataFn Function to call after .data/.bss init
     * @tparam PreMainFn Function to call before main
     */
    template<typename Config, typename PreDataFn, typename PostDataFn, typename PreMainFn>
    [[noreturn]]
    static void custom_startup_sequence(PreDataFn pre_data, PostDataFn post_data, PreMainFn pre_main) {
        // Custom pre-data hook
        pre_data();

        // Initialize .data and .bss
        init_data_section(
            Config::data_src_start(),
            Config::data_dst_start(),
            Config::data_dst_end()
        );

        init_bss_section(
            Config::bss_start(),
            Config::bss_end()
        );

        // Custom post-data hook
        post_data();

        // Call static constructors
        call_init_array(
            Config::init_array_start(),
            Config::init_array_end()
        );

        // Custom pre-main hook
        pre_main();

        // Call main
        extern int main();
        main();

        // Infinite loop
        while (true) {
            __asm volatile("wfi");
        }
    }
};

/**
 * @brief Startup utilities
 */
namespace startup_utils {

/**
 * @brief Check if .data section needs initialization
 *
 * @param src Source address in flash
 * @param dst Destination address in RAM
 * @return true if addresses differ (copy needed)
 */
constexpr bool needs_data_init(const void* src, const void* dst) {
    return src != dst;
}

/**
 * @brief Get .data section size
 *
 * @param start Start of .data in RAM
 * @param end End of .data in RAM
 * @return Size in bytes
 */
constexpr size_t data_section_size(const void* start, const void* end) {
    return static_cast<const uint8_t*>(end) - static_cast<const uint8_t*>(start);
}

/**
 * @brief Get .bss section size
 *
 * @param start Start of .bss
 * @param end End of .bss
 * @return Size in bytes
 */
constexpr size_t bss_section_size(const void* start, const void* end) {
    return static_cast<const uint8_t*>(end) - static_cast<const uint8_t*>(start);
}

} // namespace startup_utils

} // namespace alloy::hal::arm
