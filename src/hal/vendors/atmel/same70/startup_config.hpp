#pragma once

/**
 * @file startup_config.hpp
 * @brief SAME70-specific Startup Configuration
 *
 * Provides memory layout information from linker script for startup code.
 */

#include <cstdint>

namespace alloy::hal::same70 {

/**
 * @brief SAME70-specific startup configuration
 *
 * Provides memory layout information from linker script.
 * All methods are constexpr for compile-time access.
 *
 * Linker script must define these symbols:
 * - _sidata: Source of .data in flash
 * - _sdata:  Start of .data in RAM
 * - _edata:  End of .data in RAM
 * - _sbss:   Start of .bss
 * - _ebss:   End of .bss
 * - _estack: Top of stack
 * - __init_array_start: Start of constructor array
 * - __init_array_end:   End of constructor array
 */
struct StartupConfig {
    // Linker script symbols (extern declarations)
    // These are defined by the linker, not by us
    // We declare them here to access their addresses

    /**
     * @brief Get source address of .data section in flash
     *
     * @return Pointer to .data source (in flash)
     */
    static uint32_t* data_src_start() {
        extern uint32_t _sidata;
        return &_sidata;
    }

    /**
     * @brief Get destination start of .data section in RAM
     *
     * @return Pointer to .data start (in RAM)
     */
    static uint32_t* data_dst_start() {
        extern uint32_t _sdata;
        return &_sdata;
    }

    /**
     * @brief Get destination end of .data section in RAM
     *
     * @return Pointer to .data end (in RAM)
     */
    static uint32_t* data_dst_end() {
        extern uint32_t _edata;
        return &_edata;
    }

    /**
     * @brief Get start of .bss section
     *
     * @return Pointer to .bss start
     */
    static uint32_t* bss_start() {
        extern uint32_t _sbss;
        return &_sbss;
    }

    /**
     * @brief Get end of .bss section
     *
     * @return Pointer to .bss end
     */
    static uint32_t* bss_end() {
        extern uint32_t _ebss;
        return &_ebss;
    }

    /**
     * @brief Get top of stack
     *
     * @return Stack pointer value (top of stack)
     */
    static uintptr_t stack_top() {
        extern uint32_t _estack;
        return reinterpret_cast<uintptr_t>(&_estack);
    }

    /**
     * @brief Get start of .init_array (C++ constructors)
     *
     * @return Pointer to constructor array start
     */
    static void (**init_array_start)() {
        extern void (*__init_array_start)();
        return &__init_array_start;
    }

    /**
     * @brief Get end of .init_array (C++ constructors)
     *
     * @return Pointer to constructor array end
     */
    static void (**init_array_end)() {
        extern void (*__init_array_end)();
        return &__init_array_end;
    }

    // Memory configuration constants (from SAME70 datasheet)

    /**
     * @brief Flash base address
     */
    static constexpr uintptr_t FLASH_BASE = 0x00400000;

    /**
     * @brief Flash size (2 MB for SAME70Q21B)
     */
    static constexpr size_t FLASH_SIZE = 0x00200000;  // 2 MB

    /**
     * @brief SRAM base address
     */
    static constexpr uintptr_t SRAM_BASE = 0x20400000;

    /**
     * @brief SRAM size (384 KB for SAME70Q21B)
     */
    static constexpr size_t SRAM_SIZE = 0x00060000;  // 384 KB

    /**
     * @brief Stack size (default 4 KB)
     */
    static constexpr size_t STACK_SIZE = 0x00001000;  // 4 KB

    /**
     * @brief Heap size (remaining SRAM)
     */
    static constexpr size_t HEAP_SIZE = SRAM_SIZE - STACK_SIZE;

    // Vector table configuration

    /**
     * @brief Number of standard Cortex-M exceptions
     */
    static constexpr size_t EXCEPTION_COUNT = 16;

    /**
     * @brief Number of SAME70-specific IRQs
     */
    static constexpr size_t IRQ_COUNT = 80;

    /**
     * @brief Total vector count
     */
    static constexpr size_t VECTOR_COUNT = EXCEPTION_COUNT + IRQ_COUNT;

    // Clock configuration (defaults, can be changed by pre_main_init)

    /**
     * @brief Default CPU frequency (300 MHz max)
     */
    static constexpr uint32_t DEFAULT_CPU_FREQ_HZ = 300'000'000;

    /**
     * @brief Main crystal frequency (12 MHz on SAME70 Xplained)
     */
    static constexpr uint32_t MAIN_XTAL_FREQ_HZ = 12'000'000;

    /**
     * @brief Slow clock frequency (32.768 kHz)
     */
    static constexpr uint32_t SLOW_CLOCK_FREQ_HZ = 32'768;
};

} // namespace alloy::hal::same70
