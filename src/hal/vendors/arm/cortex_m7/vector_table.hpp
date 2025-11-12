#pragma once

/**
 * @file vector_table.hpp
 * @brief Modern C++23 Constexpr Vector Table Builder
 *
 * Provides compile-time vector table construction with type safety.
 * All configuration happens at compile time with zero runtime overhead.
 */

#include <cstdint>
#include <array>
#include <concepts>

namespace alloy::hal::arm {

/**
 * @brief Concept for interrupt handlers
 *
 * Ensures handlers are callable with no arguments and return void.
 */
template<typename T>
concept InterruptHandler = requires(T t) {
    { t() } -> std::same_as<void>;
};

/**
 * @brief ARM Cortex-M Vector Table Entry
 *
 * In ARM Cortex-M, the vector table has a special structure:
 * - Entry 0: Initial stack pointer (raw address value)
 * - Entries 1+: Exception/interrupt handlers (function pointers)
 *
 * This union allows representing both types. When placed in the vector table,
 * the hardware interprets entry 0 as a stack pointer address and all other
 * entries as function pointers to jump to.
 */
union VectorEntry {
    uintptr_t address;      // For stack pointer (entry 0)
    void (*handler)();      // For exception/interrupt handlers (entries 1+)

    // Default constructor for handler
    constexpr VectorEntry() : handler(nullptr) {}

    // Constructor for raw address (stack pointer)
    constexpr explicit VectorEntry(uintptr_t addr) : address(addr) {}

    // Constructor for function pointer (handlers)
    constexpr VectorEntry(void (*h)()) : handler(h) {}
};

/**
 * @brief Compile-time vector table builder
 *
 * Builds ARM Cortex-M vector table at compile time using constexpr.
 * Provides fluent API for readable, type-safe configuration.
 *
 * @tparam VectorCount Total number of vectors (exceptions + IRQs)
 *
 * @example
 * @code
 * constexpr auto vt = make_vector_table<96>()
 *     .set_stack_pointer(0x20400000)
 *     .set_handler(1, &Reset_Handler)
 *     .set_handler(15, &SysTick_Handler)
 *     .get();
 * @endcode
 */
template<size_t VectorCount>
class VectorTableBuilder {
public:
    using HandlerType = void(*)();

    // Standard Cortex-M exception indices (0-15)
    static constexpr size_t STACK_POINTER_IDX = 0;
    static constexpr size_t RESET_HANDLER_IDX = 1;
    static constexpr size_t NMI_HANDLER_IDX = 2;
    static constexpr size_t HARD_FAULT_HANDLER_IDX = 3;
    static constexpr size_t MEM_MANAGE_HANDLER_IDX = 4;
    static constexpr size_t BUS_FAULT_HANDLER_IDX = 5;
    static constexpr size_t USAGE_FAULT_HANDLER_IDX = 6;
    static constexpr size_t SVCALL_HANDLER_IDX = 11;
    static constexpr size_t DEBUG_MON_HANDLER_IDX = 12;
    static constexpr size_t PENDSV_HANDLER_IDX = 14;
    static constexpr size_t SYSTICK_HANDLER_IDX = 15;

    // Reserved vectors (7-10, 13) are nullptr

    /**
     * @brief Default constructor
     *
     * Initializes all vectors to default handler.
     */
    consteval VectorTableBuilder() {
        // Entry 0 will be set by set_stack_pointer()
        // Initialize all handler entries (1+) to default handler
        for (size_t i = 1; i < VectorCount; ++i) {
            vectors_[i] = VectorEntry(&default_handler);
        }
    }

    /**
     * @brief Set handler for specific vector
     *
     * @param index Vector index (0 = stack pointer, 1 = reset, etc.)
     * @param handler Function pointer to handler
     * @return Reference to builder for chaining
     */
    constexpr VectorTableBuilder& set_handler(size_t index, HandlerType handler) {
        if (index < VectorCount) {
            vectors_[index] = VectorEntry(handler);
        }
        return *this;
    }

    /**
     * @brief Set initial stack pointer (vector 0)
     *
     * @param sp Stack pointer address (top of stack)
     * @return Reference to builder for chaining
     *
     * This sets the initial stack pointer value in entry 0 of the vector table.
     * The hardware loads this value into the stack pointer on reset.
     */
    constexpr VectorTableBuilder& set_stack_pointer(uintptr_t sp) {
        vectors_[0] = VectorEntry(sp);
        return *this;
    }

    /**
     * @brief Set reset handler (vector 1)
     *
     * @param handler Reset handler function
     * @return Reference to builder for chaining
     */
    constexpr VectorTableBuilder& set_reset_handler(HandlerType handler) {
        return set_handler(RESET_HANDLER_IDX, handler);
    }

    /**
     * @brief Set NMI handler (vector 2)
     *
     * @param handler NMI handler function
     * @return Reference to builder for chaining
     */
    constexpr VectorTableBuilder& set_nmi_handler(HandlerType handler) {
        return set_handler(NMI_HANDLER_IDX, handler);
    }

    /**
     * @brief Set hard fault handler (vector 3)
     *
     * @param handler Hard fault handler function
     * @return Reference to builder for chaining
     */
    constexpr VectorTableBuilder& set_hard_fault_handler(HandlerType handler) {
        return set_handler(HARD_FAULT_HANDLER_IDX, handler);
    }

    /**
     * @brief Set SysTick handler (vector 15)
     *
     * @param handler SysTick handler function
     * @return Reference to builder for chaining
     */
    constexpr VectorTableBuilder& set_systick_handler(HandlerType handler) {
        return set_handler(SYSTICK_HANDLER_IDX, handler);
    }

    /**
     * @brief Set reserved vectors to nullptr
     *
     * Vectors 7-10 and 13 are reserved in Cortex-M.
     *
     * @return Reference to builder for chaining
     */
    constexpr VectorTableBuilder& set_reserved_null() {
        vectors_[7] = VectorEntry(nullptr);
        vectors_[8] = VectorEntry(nullptr);
        vectors_[9] = VectorEntry(nullptr);
        vectors_[10] = VectorEntry(nullptr);
        vectors_[13] = VectorEntry(nullptr);
        return *this;
    }

    /**
     * @brief Get the constructed vector table
     *
     * @return Const reference to vector array
     */
    constexpr const auto& get() const {
        return vectors_;
    }

    /**
     * @brief Get vector count
     *
     * @return Total number of vectors
     */
    static constexpr size_t size() {
        return VectorCount;
    }

private:
    std::array<VectorEntry, VectorCount> vectors_{};

    /**
     * @brief Default handler (infinite loop)
     *
     * Used for uninitialized vectors.
     */
    static void default_handler() {
        while (true) {
            __asm volatile("nop");
        }
    }
};

/**
 * @brief Helper to create vector table with fluent API
 *
 * @tparam VectorCount Total number of vectors
 * @return VectorTableBuilder instance
 *
 * @example
 * @code
 * constexpr auto vt = make_vector_table<96>()
 *     .set_stack_pointer(0x20460000)
 *     .set_reset_handler(&Reset_Handler)
 *     .set_systick_handler(&SysTick_Handler)
 *     .set_reserved_null()
 *     .get();
 * @endcode
 */
template<size_t VectorCount>
consteval auto make_vector_table() {
    return VectorTableBuilder<VectorCount>{};
}

/**
 * @brief Common vector table sizes for different Cortex-M cores
 */
namespace VectorTableSizes {
    // Standard exceptions
    static constexpr size_t CORTEX_M_EXCEPTIONS = 16;

    // Common configurations
    static constexpr size_t CORTEX_M0_TYPICAL = 16 + 32;   // 16 exceptions + 32 IRQs
    static constexpr size_t CORTEX_M3_TYPICAL = 16 + 68;   // 16 exceptions + 68 IRQs
    static constexpr size_t CORTEX_M4_TYPICAL = 16 + 82;   // 16 exceptions + 82 IRQs
    static constexpr size_t CORTEX_M7_TYPICAL = 16 + 80;   // 16 exceptions + 80 IRQs (SAME70)

    // SAME70 specific
    static constexpr size_t SAME70 = 16 + 80;  // 96 total vectors

    // STM32 common
    static constexpr size_t STM32F1 = 16 + 60;  // 76 total vectors
    static constexpr size_t STM32F4 = 16 + 82;  // 98 total vectors
}

} // namespace alloy::hal::arm
