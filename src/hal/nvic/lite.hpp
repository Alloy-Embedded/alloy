/// @file hal/nvic/lite.hpp
/// Cortex-M NVIC (Nested Vectored Interrupt Controller) — lite driver.
///
/// Direct register access to the NVIC — no descriptor-runtime required.
/// Works on all Cortex-M3/M4/M7/M33 cores (STM32F / G / H / L / U / WB).
///
/// Pairs with the peripheral-level IRQ enables added to the lite HAL drivers
/// (uart, spi, i2c, timer, adc, lptim, …).  Typical flow:
///
/// @code
///   #include "hal/nvic/lite.hpp"
///   #include "hal/uart.hpp"
///
///   using Nvic  = alloy::hal::nvic::lite::controller;
///   using Uart1 = alloy::hal::uart::lite::port<dev::usart1>;
///
///   // --- setup ---
///   Uart1::configure({.baudrate = 115200u, .clock_hz = 64'000'000u});
///   Uart1::enable_rx_irq();                  // peripheral side
///   Nvic::set_priority(37u, 128u);           // USART1 IRQ = 37 on G4, prio half
///   Nvic::enable_irq(37u);                   // NVIC side
///   Nvic::enable_all();                      // global PRIMASK
///
///   // --- ISR (defined elsewhere) ---
///   extern "C" void USART1_IRQHandler() {
///       if (auto b = Uart1::try_read_byte()) { process(*b); }
///   }
/// @endcode
///
/// NVIC register map (Cortex-M, fixed addresses):
///   0xE000E100  ISER[n]  — interrupt set-enable   (1 bit per IRQ)
///   0xE000E180  ICER[n]  — interrupt clear-enable
///   0xE000E200  ISPR[n]  — interrupt set-pending
///   0xE000E280  ICPR[n]  — interrupt clear-pending
///   0xE000E300  IABR[n]  — interrupt active-bit    (read-only)
///   0xE000E400  IPR[irq] — priority byte (8-bit; upper N bits implemented)
///
/// Priority bits: Cortex-M4/M7 typically implement 4 bits → values 0–240 in
/// steps of 16.  Lower value = higher priority.  0 = highest, 240 = lowest.
/// The number of implemented bits varies; use `priority_shift()` to align.
#pragma once

#include <cstdint>

namespace alloy::hal::nvic::lite {

namespace detail {

inline constexpr std::uintptr_t kIserBase = 0xE000E100u;
inline constexpr std::uintptr_t kIcerBase = 0xE000E180u;
inline constexpr std::uintptr_t kIsprBase = 0xE000E200u;
inline constexpr std::uintptr_t kIcprBase = 0xE000E280u;
inline constexpr std::uintptr_t kIabrBase = 0xE000E300u;
inline constexpr std::uintptr_t kIprBase  = 0xE000E400u;

// SCB System Control Block — ICSR and AIRCR for system-level control
inline constexpr std::uintptr_t kIcsrAddr  = 0xE000ED04u;  ///< Interrupt Control & State
inline constexpr std::uintptr_t kAircrAddr = 0xE000ED0Cu;  ///< App Interrupt & Reset Control
inline constexpr std::uintptr_t kShcsrAddr = 0xE000ED24u;  ///< System Handler Control & State

[[nodiscard]] inline auto reg32(std::uintptr_t addr) noexcept
    -> volatile std::uint32_t& {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<volatile std::uint32_t*>(addr);
}

[[nodiscard]] inline auto iser(std::uint32_t word) noexcept
    -> volatile std::uint32_t& {
    return reg32(kIserBase + word * 4u);
}
[[nodiscard]] inline auto icer(std::uint32_t word) noexcept
    -> volatile std::uint32_t& {
    return reg32(kIcerBase + word * 4u);
}
[[nodiscard]] inline auto ispr(std::uint32_t word) noexcept
    -> volatile std::uint32_t& {
    return reg32(kIsprBase + word * 4u);
}
[[nodiscard]] inline auto icpr(std::uint32_t word) noexcept
    -> volatile std::uint32_t& {
    return reg32(kIcprBase + word * 4u);
}
[[nodiscard]] inline auto iabr(std::uint32_t word) noexcept
    -> volatile std::uint32_t& {
    return reg32(kIabrBase + word * 4u);
}
[[nodiscard]] inline auto ipr(std::uint32_t irq) noexcept
    -> volatile std::uint8_t& {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<volatile std::uint8_t*>(kIprBase + irq);
}

}  // namespace detail

/// Cortex-M NVIC controller — all methods are static.
///
/// IRQ numbers match the device vector table (device-specific; check the
/// startup file or RM for your MCU).  IRQ0 = first device IRQ; negative
/// numbers (HardFault, SysTick …) are handled via the SCB, not NVIC.
class controller {
   public:
    // -----------------------------------------------------------------------
    // Enable / disable individual IRQs
    // -----------------------------------------------------------------------

    /// Enable the IRQ with number `irq_n` in the NVIC (set ISER bit).
    ///
    /// The peripheral-side interrupt source must also be enabled
    /// (e.g. `Uart1::enable_rx_irq()`) before any interrupt fires.
    static void enable_irq(std::uint32_t irq_n) noexcept {
        detail::iser(irq_n >> 5u) = 1u << (irq_n & 31u);
    }

    /// Disable the IRQ with number `irq_n` (set ICER bit).
    static void disable_irq(std::uint32_t irq_n) noexcept {
        detail::icer(irq_n >> 5u) = 1u << (irq_n & 31u);
    }

    /// True when the IRQ is currently enabled in the NVIC.
    [[nodiscard]] static auto is_enabled(std::uint32_t irq_n) noexcept -> bool {
        return ((detail::iser(irq_n >> 5u) >> (irq_n & 31u)) & 1u) != 0u;
    }

    // -----------------------------------------------------------------------
    // Priority
    // -----------------------------------------------------------------------

    /// Set the preemption priority for `irq_n`.
    ///
    /// The hardware stores priority in the upper N bits of each IPR byte
    /// (typically 4 bits on Cortex-M4, giving values 0–240 in steps of 16).
    /// Lower numerical value = higher hardware priority.
    ///
    /// @param irq_n    Device IRQ number (≥ 0).
    /// @param priority Raw 8-bit priority value.  For 4-bit devices use
    ///                 multiples of 16 (0, 16, 32 … 240).
    static void set_priority(std::uint32_t irq_n, std::uint8_t priority) noexcept {
        detail::ipr(irq_n) = priority;
    }

    /// Read the current priority for `irq_n`.
    [[nodiscard]] static auto get_priority(std::uint32_t irq_n) noexcept
        -> std::uint8_t {
        return detail::ipr(irq_n);
    }

    // -----------------------------------------------------------------------
    // Pending
    // -----------------------------------------------------------------------

    /// Software-trigger an interrupt (set ISPR bit — useful for testing ISRs).
    static void set_pending(std::uint32_t irq_n) noexcept {
        detail::ispr(irq_n >> 5u) = 1u << (irq_n & 31u);
    }

    /// Clear a pending interrupt (set ICPR bit).
    static void clear_pending(std::uint32_t irq_n) noexcept {
        detail::icpr(irq_n >> 5u) = 1u << (irq_n & 31u);
    }

    /// True when the IRQ is pending in the NVIC.
    [[nodiscard]] static auto is_pending(std::uint32_t irq_n) noexcept -> bool {
        return ((detail::ispr(irq_n >> 5u) >> (irq_n & 31u)) & 1u) != 0u;
    }

    // -----------------------------------------------------------------------
    // Active status
    // -----------------------------------------------------------------------

    /// True when the IRQ is currently being serviced (IABR bit).
    ///
    /// Useful inside an ISR to confirm which IRQ is executing,
    /// or to detect preemption scenarios.
    [[nodiscard]] static auto is_active(std::uint32_t irq_n) noexcept -> bool {
        return ((detail::iabr(irq_n >> 5u) >> (irq_n & 31u)) & 1u) != 0u;
    }

    // -----------------------------------------------------------------------
    // Global interrupt control (PRIMASK)
    // -----------------------------------------------------------------------

    /// Enable all interrupts globally (clears PRIMASK — `cpsie i`).
    ///
    /// Should be called after all peripherals and IRQ priorities are
    /// configured to avoid spurious interrupts during setup.
    static void enable_all() noexcept {
        __asm volatile("cpsie i" ::: "memory");
    }

    /// Disable all interrupts globally (sets PRIMASK — `cpsid i`).
    ///
    /// Use for short critical sections.  Prefer `critical_section` RAII
    /// for structured use.  Does NOT affect NMI or HardFault.
    static void disable_all() noexcept {
        __asm volatile("cpsid i" ::: "memory");
    }

    /// True when global interrupts are currently enabled (PRIMASK = 0).
    [[nodiscard]] static auto interrupts_enabled() noexcept -> bool {
        std::uint32_t primask = 0u;
        __asm volatile("mrs %0, primask" : "=r"(primask) :: "memory");
        return (primask & 1u) == 0u;
    }

    // -----------------------------------------------------------------------
    // RAII critical section
    // -----------------------------------------------------------------------

    /// RAII guard that disables global interrupts on construction and
    /// restores the previous PRIMASK state on destruction.
    ///
    /// @code
    ///   {
    ///       auto guard = alloy::hal::nvic::lite::controller::critical_section{};
    ///       // interrupts disabled here
    ///   }  // restored on scope exit
    /// @endcode
    struct critical_section {
        critical_section() noexcept {
            __asm volatile("mrs %0, primask" : "=r"(saved_) :: "memory");
            controller::disable_all();
        }
        ~critical_section() noexcept {
            // Restore original PRIMASK (re-enables only if it was enabled before)
            __asm volatile("msr primask, %0" :: "r"(saved_) : "memory");
        }
        critical_section(const critical_section&) = delete;
        critical_section& operator=(const critical_section&) = delete;
    private:
        std::uint32_t saved_{};
    };

    controller() = delete;
};

}  // namespace alloy::hal::nvic::lite
