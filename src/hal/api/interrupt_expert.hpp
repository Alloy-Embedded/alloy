#pragma once

#include "hal/interrupt.hpp"

namespace alloy::hal {

using InterruptExpertConfig = InterruptConfig;

namespace expert {

[[nodiscard]] inline auto configure(const InterruptConfig& config) -> Result<void, ErrorCode> {
    return Interrupt::configure(config);
}

[[nodiscard]] inline auto is_pending(IrqNumber irq) -> bool { return Interrupt::is_pending(irq); }

[[nodiscard]] inline auto set_pending(IrqNumber irq) -> Result<void, ErrorCode> {
    return Interrupt::set_pending(irq);
}

[[nodiscard]] inline auto clear_pending(IrqNumber irq) -> Result<void, ErrorCode> {
    return Interrupt::clear_pending(irq);
}

[[nodiscard]] inline auto is_active(IrqNumber irq) -> bool {
    (void)irq;
    return false;
}

[[nodiscard]] inline auto get_priority(IrqNumber irq) -> Result<u8, ErrorCode> {
    if (auto result = Interrupt::get_priority(irq); result.is_ok()) {
        return Ok(static_cast<u8>(result.value()));
    } else {
        return Err(result.error());
    }
}

[[nodiscard]] inline auto set_priority_grouping(u8 grouping) -> Result<void, ErrorCode> {
    if (grouping > 7u) {
        return Err(ErrorCode::InvalidParameter);
    }
    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
