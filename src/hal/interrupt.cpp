/**
 * @file interrupt.cpp
 * @brief Platform-agnostic interrupt management implementation
 */

#include "hal/interrupt.hpp"

namespace alloy::hal {

// ============================================================================
// CriticalSection Implementation
// ============================================================================

CriticalSection::CriticalSection() noexcept {
    // Platform-specific: Save and disable interrupts
    // ARM Cortex-M: __get_PRIMASK(), __disable_irq()
    // RISC-V: csrr/csrc on mstatus
    // x86: pushf/cli
    saved_state_ = 0;  // Stub implementation
}

CriticalSection::~CriticalSection() noexcept {
    // Platform-specific: Restore interrupts
    // ARM Cortex-M: __set_PRIMASK(saved_state_)
    // RISC-V: csrw mstatus
    // x86: popf
}

// ============================================================================
// Interrupt Simple API Implementation
// ============================================================================

void Interrupt::enable_all() noexcept {
    // Platform-specific global interrupt enable
    // ARM Cortex-M: __enable_irq()
    // RISC-V: csrs mstatus, 0x8
    // x86: sti
}

void Interrupt::disable_all() noexcept {
    // Platform-specific global interrupt disable
    // ARM Cortex-M: __disable_irq()
    // RISC-V: csrc mstatus, 0x8
    // x86: cli
}

Result<void, ErrorCode> Interrupt::enable(IrqNumber irq) noexcept {
    // Platform-specific interrupt enable
    // ARM Cortex-M: NVIC_EnableIRQ()
    // RISC-V: PLIC enable
    return Ok();
}

Result<void, ErrorCode> Interrupt::disable(IrqNumber irq) noexcept {
    // Platform-specific interrupt disable
    // ARM Cortex-M: NVIC_DisableIRQ()
    // RISC-V: PLIC disable
    return Ok();
}

bool Interrupt::is_enabled(IrqNumber irq) noexcept {
    // Platform-specific check
    // ARM Cortex-M: NVIC_GetEnableIRQ()
    return false;
}

Result<void, ErrorCode> Interrupt::set_priority(
    IrqNumber irq,
    IrqPriority priority) noexcept {
    // Platform-specific priority setting
    // ARM Cortex-M: NVIC_SetPriority()
    return Ok();
}

Result<IrqPriority, ErrorCode> Interrupt::get_priority(IrqNumber irq) noexcept {
    // Platform-specific priority readback
    (void)irq;
    return Ok(IrqPriority::Normal);
}

bool Interrupt::is_pending(IrqNumber irq) noexcept {
    // Platform-specific pending flag readback
    (void)irq;
    return false;
}

Result<void, ErrorCode> Interrupt::set_pending(IrqNumber irq) noexcept {
    // Platform-specific pending flag set
    (void)irq;
    return Ok();
}

Result<void, ErrorCode> Interrupt::clear_pending(IrqNumber irq) noexcept {
    // Platform-specific pending flag clear
    (void)irq;
    return Ok();
}

}  // namespace alloy::hal
