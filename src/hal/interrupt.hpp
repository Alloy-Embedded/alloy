#pragma once

#include "hal/interface/interrupt.hpp"

namespace alloy::hal {

class Interrupt {
   public:
    static void enable_all() noexcept;
    static void disable_all() noexcept;

    [[nodiscard]] static auto critical_section() noexcept -> CriticalSection { return {}; }

    [[nodiscard]] static Result<void, ErrorCode> enable(IrqNumber irq) noexcept;
    [[nodiscard]] static Result<void, ErrorCode> disable(IrqNumber irq) noexcept;
    [[nodiscard]] static auto is_enabled(IrqNumber irq) noexcept -> bool;

    [[nodiscard]] static Result<void, ErrorCode> set_priority(IrqNumber irq,
                                                              IrqPriority priority) noexcept;
    [[nodiscard]] static Result<IrqPriority, ErrorCode> get_priority(IrqNumber irq) noexcept;

    [[nodiscard]] static auto is_pending(IrqNumber irq) noexcept -> bool;
    [[nodiscard]] static Result<void, ErrorCode> set_pending(IrqNumber irq) noexcept;
    [[nodiscard]] static Result<void, ErrorCode> clear_pending(IrqNumber irq) noexcept;

    [[nodiscard]] static Result<void, ErrorCode> configure(const InterruptConfig& config) noexcept {
        if (!config.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        if (auto result = set_priority(config.irq_number, config.priority); !result.is_ok()) {
            return result;
        }

        if (config.trigger_pending) {
            if (auto result = set_pending(config.irq_number); !result.is_ok()) {
                return result;
            }
        }

        return config.enable ? enable(config.irq_number) : disable(config.irq_number);
    }
};

class ScopedInterruptDisable {
   public:
    ScopedInterruptDisable() noexcept : cs_() {}
    ~ScopedInterruptDisable() noexcept = default;

    ScopedInterruptDisable(const ScopedInterruptDisable&) = delete;
    auto operator=(const ScopedInterruptDisable&) -> ScopedInterruptDisable& = delete;
    ScopedInterruptDisable(ScopedInterruptDisable&&) = delete;
    auto operator=(ScopedInterruptDisable&&) -> ScopedInterruptDisable& = delete;

   private:
    CriticalSection cs_;
};

}  // namespace alloy::hal
