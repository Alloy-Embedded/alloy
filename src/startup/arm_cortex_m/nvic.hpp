// Alloy Framework - ARM Cortex-M NVIC (Nested Vectored Interrupt Controller)
//
// Provides interrupt control functions for all ARM Cortex-M cores.
// Compatible with M0/M0+/M3/M4/M7/M23/M33/M55
//
// Functions provided:
// - Enable/disable interrupts
// - Set/get interrupt priority
// - Set/clear pending interrupts
// - Check interrupt active status

#pragma once

#include "core_common.hpp"
#include <cstdint>

namespace alloy::arm::cortex_m::nvic {

/// Enable an interrupt
/// @param irqn: Interrupt number (0-239)
inline void enable_irq(uint8_t irqn) {
    if (irqn < 240) {
        NVIC->ISER[irqn >> 5] = (1UL << (irqn & 0x1F));
    }
}

/// Disable an interrupt
/// @param irqn: Interrupt number (0-239)
inline void disable_irq(uint8_t irqn) {
    if (irqn < 240) {
        NVIC->ICER[irqn >> 5] = (1UL << (irqn & 0x1F));
    }
}

/// Get pending status of an interrupt
/// @param irqn: Interrupt number (0-239)
/// @return true if interrupt is pending
inline bool get_pending_irq(uint8_t irqn) {
    if (irqn < 240) {
        return (NVIC->ISPR[irqn >> 5] & (1UL << (irqn & 0x1F))) != 0;
    }
    return false;
}

/// Set pending status of an interrupt
/// @param irqn: Interrupt number (0-239)
inline void set_pending_irq(uint8_t irqn) {
    if (irqn < 240) {
        NVIC->ISPR[irqn >> 5] = (1UL << (irqn & 0x1F));
    }
}

/// Clear pending status of an interrupt
/// @param irqn: Interrupt number (0-239)
inline void clear_pending_irq(uint8_t irqn) {
    if (irqn < 240) {
        NVIC->ICPR[irqn >> 5] = (1UL << (irqn & 0x1F));
    }
}

/// Get active status of an interrupt
/// @param irqn: Interrupt number (0-239)
/// @return true if interrupt is active (currently being serviced)
inline bool get_active(uint8_t irqn) {
    if (irqn < 240) {
        return (NVIC->IABR[irqn >> 5] & (1UL << (irqn & 0x1F))) != 0;
    }
    return false;
}

/// Set priority of an interrupt
/// @param irqn: Interrupt number (0-239)
/// @param priority: Priority value (0 = highest priority)
///                  Note: Only the upper bits are used depending on implementation
///                  (e.g., 4 bits = 16 priority levels: 0x00, 0x10, 0x20, ..., 0xF0)
inline void set_priority(uint8_t irqn, uint8_t priority) {
    if (irqn < 240) {
        NVIC->IP[irqn] = priority;
    }
}

/// Get priority of an interrupt
/// @param irqn: Interrupt number (0-239)
/// @return Priority value (0 = highest priority)
inline uint8_t get_priority(uint8_t irqn) {
    if (irqn < 240) {
        return NVIC->IP[irqn];
    }
    return 0;
}

/// Check if an interrupt is enabled
/// @param irqn: Interrupt number (0-239)
/// @return true if interrupt is enabled
inline bool is_enabled(uint8_t irqn) {
    if (irqn < 240) {
        return (NVIC->ISER[irqn >> 5] & (1UL << (irqn & 0x1F))) != 0;
    }
    return false;
}

/// Set priority grouping
/// @param priority_group: Priority grouping value (0-7)
///   0 = 0 bits for pre-emption priority, all bits for subpriority
///   1 = 1 bit for pre-emption priority, remaining for subpriority
///   ...
///   7 = all bits for pre-emption priority, 0 bits for subpriority
///
/// Example for 4-bit implementation:
///   priority_group = 3:
///     - 3 bits pre-emption (8 levels)
///     - 1 bit sub-priority (2 levels)
inline void set_priority_grouping(uint32_t priority_group) {
    uint32_t reg_value;
    uint32_t prigroup = (priority_group & 0x07UL);

    reg_value  = SCB->AIRCR;
    reg_value &= ~(aircr::VECTKEY | aircr::PRIGROUP_Msk);
    reg_value  = (reg_value | aircr::VECTKEY | (prigroup << aircr::PRIGROUP_Pos));

    SCB->AIRCR = reg_value;
}

/// Get priority grouping
/// @return Priority grouping value (0-7)
inline uint32_t get_priority_grouping() {
    return (SCB->AIRCR & aircr::PRIGROUP_Msk) >> aircr::PRIGROUP_Pos;
}

/// Encode priority value considering priority grouping
/// @param priority_group: Priority grouping value
/// @param preempt_priority: Pre-emption priority value
/// @param sub_priority: Sub-priority value
/// @return Encoded priority value for set_priority()
inline uint8_t encode_priority(uint32_t priority_group, uint32_t preempt_priority, uint32_t sub_priority) {
    uint32_t preempt_bits = 7 - priority_group;
    uint32_t sub_bits = priority_group;

    // Ensure values fit in their bit fields
    preempt_priority &= ((1UL << preempt_bits) - 1);
    sub_priority &= ((1UL << sub_bits) - 1);

    return static_cast<uint8_t>((preempt_priority << sub_bits) | sub_priority) << 4;
}

} // namespace alloy::arm::cortex_m::nvic
