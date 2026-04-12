#pragma once

#include <concepts>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

enum class IrqNumber : u16 {
    SysTick = 0,
    UART0 = 10,
    UART1 = 11,
    UART2 = 12,
    UART3 = 13,
    SPI0 = 20,
    SPI1 = 21,
    SPI2 = 22,
    I2C0 = 30,
    I2C1 = 31,
    I2C2 = 32,
    DMA_Stream0 = 40,
    DMA_Stream1 = 41,
    DMA_Stream2 = 42,
    DMA_Stream3 = 43,
    DMA_Stream4 = 44,
    DMA_Stream5 = 45,
    DMA_Stream6 = 46,
    DMA_Stream7 = 47,
    ADC0 = 50,
    ADC1 = 51,
    TIMER0 = 60,
    TIMER1 = 61,
    TIMER2 = 62,
    TIMER3 = 63,
    EXTI0 = 70,
    EXTI1 = 71,
    EXTI2 = 72,
    EXTI3 = 73,
    EXTI4 = 74,
    EXTI5_9 = 75,
    EXTI10_15 = 76,
};

enum class IrqPriority : u8 {
    Highest = 0,
    VeryHigh = 32,
    High = 64,
    AboveNormal = 96,
    Normal = 128,
    BelowNormal = 160,
    Low = 192,
    VeryLow = 224,
    Lowest = 255
};

struct InterruptConfig {
    IrqNumber irq_number;
    IrqPriority priority = IrqPriority::Normal;
    bool enable = true;
    bool trigger_pending = false;

    constexpr auto is_valid() const -> bool { return true; }
};

class CriticalSection {
   public:
    CriticalSection() noexcept;
    ~CriticalSection() noexcept;

    CriticalSection(const CriticalSection&) = delete;
    auto operator=(const CriticalSection&) -> CriticalSection& = delete;
    CriticalSection(CriticalSection&&) = delete;
    auto operator=(CriticalSection&&) -> CriticalSection& = delete;

   private:
    u32 saved_state_{};
};

template <typename T>
concept InterruptController = requires(IrqNumber irq, IrqPriority priority) {
    { T::enable_global() } -> std::same_as<void>;
    { T::disable_global() } -> std::same_as<void>;
    { T::save_and_disable() } -> std::same_as<u32>;
    { T::restore(u32{}) } -> std::same_as<void>;
    { T::enable(irq) } -> std::same_as<Result<void, ErrorCode>>;
    { T::disable(irq) } -> std::same_as<Result<void, ErrorCode>>;
    { T::set_priority(irq, priority) } -> std::same_as<Result<void, ErrorCode>>;
    { T::get_priority(irq) } -> std::same_as<Result<IrqPriority, ErrorCode>>;
    { T::is_enabled(irq) } -> std::same_as<bool>;
    { T::is_pending(irq) } -> std::same_as<bool>;
    { T::set_pending(irq) } -> std::same_as<Result<void, ErrorCode>>;
    { T::clear_pending(irq) } -> std::same_as<Result<void, ErrorCode>>;
};

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
