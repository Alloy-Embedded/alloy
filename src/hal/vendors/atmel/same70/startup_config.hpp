#pragma once

/**
 * @file startup_config.hpp
 * @brief SAME70-specific Startup Configuration
 *
 * Provides memory layout information from linker script for startup code.
 */

#include <cstdint>

// Linker script symbols (must be in global scope)
extern "C" {
    extern uint32_t _sidata;  // Source of .data in flash
    extern uint32_t _sdata;   // Start of .data in RAM
    extern uint32_t _edata;   // End of .data in RAM
    extern uint32_t _sbss;    // Start of .bss
    extern uint32_t _ebss;    // End of .bss
    extern void (*__init_array_start)();  // Start of constructor array
    extern void (*__init_array_end)();    // End of constructor array
}

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
    /**
     * @brief Get source address of .data section in flash
     *
     * @return Pointer to .data source (in flash)
     */
    static uint32_t* data_src_start() {
        return &::_sidata;
    }

    /**
     * @brief Get destination start of .data section in RAM
     *
     * @return Pointer to .data start (in RAM)
     */
    static uint32_t* data_dst_start() {
        return &::_sdata;
    }

    /**
     * @brief Get destination end of .data section in RAM
     *
     * @return Pointer to .data end (in RAM)
     */
    static uint32_t* data_dst_end() {
        return &::_edata;
    }

    /**
     * @brief Get start of .bss section
     *
     * @return Pointer to .bss start
     */
    static uint32_t* bss_start() {
        return &::_sbss;
    }

    /**
     * @brief Get end of .bss section
     *
     * @return Pointer to .bss end
     */
    static uint32_t* bss_end() {
        return &::_ebss;
    }

    /**
     * @brief Get top of stack
     *
     * @return Stack pointer value (top of stack)
     */
    static constexpr uintptr_t stack_top() {
        // For constexpr, we return the expected stack top address
        // The linker script places _estack at SRAM_BASE + SRAM_SIZE
        return SRAM_BASE + SRAM_SIZE;
    }

    /**
     * @brief Get start of .init_array (C++ constructors)
     *
     * @return Pointer to constructor array start
     */
    static auto init_array_start() -> void (**)() {
        return &::__init_array_start;
    }

    /**
     * @brief Get end of .init_array (C++ constructors)
     *
     * @return Pointer to constructor array end
     */
    static auto init_array_end() -> void (**)() {
        return &::__init_array_end;
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
     *
     * SAME70 has multiple memory regions:
     * - DTCM: 0x20000000 (64KB) - Data Tightly Coupled Memory
     * - SRAM: 0x20400000 (384KB) - Normal SRAM for stack/heap
     *
     * We use SRAM, not DTCM, for regular stack/heap operations
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

    // Exception handler indices (Cortex-M7 standard)
    static constexpr size_t MEM_MANAGE_HANDLER_IDX = 4;   // Memory Management
    static constexpr size_t BUS_FAULT_HANDLER_IDX = 5;    // Bus Fault
    static constexpr size_t USAGE_FAULT_HANDLER_IDX = 6;  // Usage Fault
    static constexpr size_t SVCALL_HANDLER_IDX = 11;      // SVCall
    static constexpr size_t DEBUG_MON_HANDLER_IDX = 12;   // Debug Monitor
    static constexpr size_t PENDSV_HANDLER_IDX = 14;      // PendSV

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
