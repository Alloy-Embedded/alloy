// default_handler.cpp — Task 3.2 (add-irq-vector-hal)
//
// Weak default ISR that dispatches to the RAM vector table entry.
// The linker resolves <VectorName>_IRQHandler references to these weak symbols.
// When the user installs a handler via irq::set_handler<P>(fn), the RAM table
// entry is filled and dispatch() calls it instead of spinning.
//
// Usage:
//   Each Cortex-M peripheral vector listed in the startup linker script is
//   aliased to `alloy_default_irq_handler` below.  Any translation unit that
//   calls ALLOY_IRQ_HANDLER(PeripheralId) overrides the specific vector.
//
// For host builds (no linker vector table) this file compiles but the weak
// symbols are never referenced.

#include "hal/irq/irq_table.hpp"

namespace alloy::hal::irq {

/// Central dispatch point called by all weak ISR stubs below.
/// Reads the IRQ number from the hardware (NVIC ICSR[8:0]) on Cortex-M, or
/// falls back to a linear scan of the table on other architectures.
/// On host builds this is a no-op.
extern "C"
[[gnu::weak]]
void alloy_default_irq_dispatch(std::uint16_t irqn) noexcept {
    dispatch_irq(irqn);
}

}  // namespace alloy::hal::irq

// ---------------------------------------------------------------------------
// Weak ISR stubs — one per vector slot (Cortex-M style).
// The startup file should alias every peripheral vector to one of these, or
// directly to alloy_irq_stub_<n>.  Override with ALLOY_IRQ_HANDLER(P).
// ---------------------------------------------------------------------------
//
// Each stub reads ICSR.VECTACTIVE (bits [8:0]) to recover the active IRQ
// number, then calls alloy_default_irq_dispatch().
//
// A simpler alternative: declare a single __attribute__((weak, alias(...)))
// for all 240 Cortex-M slots.  We use an explicit per-slot approach so that
// the linker map is readable.

#if defined(__ARM_ARCH) || defined(__CORTEX_M)

// Generic Cortex-M ICSR read: bits [8:0] = VECTACTIVE (1-based; subtract 16 for IRQn)
static inline std::uint16_t _alloy_read_irqn() noexcept {
    constexpr std::uintptr_t kICSR = 0xE000ED04u;
    const auto vectactive = (*reinterpret_cast<const volatile std::uint32_t*>(kICSR)) & 0x1FFu;
    return static_cast<std::uint16_t>(vectactive > 16u ? vectactive - 16u : 0u);
}

// Macro: emit a weak default ISR for IRQ slot N
#define ALLOY_DEFAULT_ISR(n) \
    extern "C" [[gnu::weak]] void ALLOY_IRQ_SLOT_##n##_Handler() noexcept { \
        alloy::hal::irq::alloy_default_irq_dispatch(_alloy_read_irqn()); \
    }

// Emit stubs for IRQ slots 0–127 (covers most Cortex-M devices)
ALLOY_DEFAULT_ISR(0)   ALLOY_DEFAULT_ISR(1)   ALLOY_DEFAULT_ISR(2)   ALLOY_DEFAULT_ISR(3)
ALLOY_DEFAULT_ISR(4)   ALLOY_DEFAULT_ISR(5)   ALLOY_DEFAULT_ISR(6)   ALLOY_DEFAULT_ISR(7)
ALLOY_DEFAULT_ISR(8)   ALLOY_DEFAULT_ISR(9)   ALLOY_DEFAULT_ISR(10)  ALLOY_DEFAULT_ISR(11)
ALLOY_DEFAULT_ISR(12)  ALLOY_DEFAULT_ISR(13)  ALLOY_DEFAULT_ISR(14)  ALLOY_DEFAULT_ISR(15)
ALLOY_DEFAULT_ISR(16)  ALLOY_DEFAULT_ISR(17)  ALLOY_DEFAULT_ISR(18)  ALLOY_DEFAULT_ISR(19)
ALLOY_DEFAULT_ISR(20)  ALLOY_DEFAULT_ISR(21)  ALLOY_DEFAULT_ISR(22)  ALLOY_DEFAULT_ISR(23)
ALLOY_DEFAULT_ISR(24)  ALLOY_DEFAULT_ISR(25)  ALLOY_DEFAULT_ISR(26)  ALLOY_DEFAULT_ISR(27)
ALLOY_DEFAULT_ISR(28)  ALLOY_DEFAULT_ISR(29)  ALLOY_DEFAULT_ISR(30)  ALLOY_DEFAULT_ISR(31)
ALLOY_DEFAULT_ISR(32)  ALLOY_DEFAULT_ISR(33)  ALLOY_DEFAULT_ISR(34)  ALLOY_DEFAULT_ISR(35)
ALLOY_DEFAULT_ISR(36)  ALLOY_DEFAULT_ISR(37)  ALLOY_DEFAULT_ISR(38)  ALLOY_DEFAULT_ISR(39)
ALLOY_DEFAULT_ISR(40)  ALLOY_DEFAULT_ISR(41)  ALLOY_DEFAULT_ISR(42)  ALLOY_DEFAULT_ISR(43)
ALLOY_DEFAULT_ISR(44)  ALLOY_DEFAULT_ISR(45)  ALLOY_DEFAULT_ISR(46)  ALLOY_DEFAULT_ISR(47)
ALLOY_DEFAULT_ISR(48)  ALLOY_DEFAULT_ISR(49)  ALLOY_DEFAULT_ISR(50)  ALLOY_DEFAULT_ISR(51)
ALLOY_DEFAULT_ISR(52)  ALLOY_DEFAULT_ISR(53)  ALLOY_DEFAULT_ISR(54)  ALLOY_DEFAULT_ISR(55)
ALLOY_DEFAULT_ISR(56)  ALLOY_DEFAULT_ISR(57)  ALLOY_DEFAULT_ISR(58)  ALLOY_DEFAULT_ISR(59)
ALLOY_DEFAULT_ISR(60)  ALLOY_DEFAULT_ISR(61)  ALLOY_DEFAULT_ISR(62)  ALLOY_DEFAULT_ISR(63)
ALLOY_DEFAULT_ISR(64)  ALLOY_DEFAULT_ISR(65)  ALLOY_DEFAULT_ISR(66)  ALLOY_DEFAULT_ISR(67)
ALLOY_DEFAULT_ISR(68)  ALLOY_DEFAULT_ISR(69)  ALLOY_DEFAULT_ISR(70)  ALLOY_DEFAULT_ISR(71)
ALLOY_DEFAULT_ISR(72)  ALLOY_DEFAULT_ISR(73)  ALLOY_DEFAULT_ISR(74)  ALLOY_DEFAULT_ISR(75)
ALLOY_DEFAULT_ISR(76)  ALLOY_DEFAULT_ISR(77)  ALLOY_DEFAULT_ISR(78)  ALLOY_DEFAULT_ISR(79)
ALLOY_DEFAULT_ISR(80)  ALLOY_DEFAULT_ISR(81)  ALLOY_DEFAULT_ISR(82)  ALLOY_DEFAULT_ISR(83)
ALLOY_DEFAULT_ISR(84)  ALLOY_DEFAULT_ISR(85)  ALLOY_DEFAULT_ISR(86)  ALLOY_DEFAULT_ISR(87)
ALLOY_DEFAULT_ISR(88)  ALLOY_DEFAULT_ISR(89)  ALLOY_DEFAULT_ISR(90)  ALLOY_DEFAULT_ISR(91)
ALLOY_DEFAULT_ISR(92)  ALLOY_DEFAULT_ISR(93)  ALLOY_DEFAULT_ISR(94)  ALLOY_DEFAULT_ISR(95)
ALLOY_DEFAULT_ISR(96)  ALLOY_DEFAULT_ISR(97)  ALLOY_DEFAULT_ISR(98)  ALLOY_DEFAULT_ISR(99)
ALLOY_DEFAULT_ISR(100) ALLOY_DEFAULT_ISR(101) ALLOY_DEFAULT_ISR(102) ALLOY_DEFAULT_ISR(103)
ALLOY_DEFAULT_ISR(104) ALLOY_DEFAULT_ISR(105) ALLOY_DEFAULT_ISR(106) ALLOY_DEFAULT_ISR(107)
ALLOY_DEFAULT_ISR(108) ALLOY_DEFAULT_ISR(109) ALLOY_DEFAULT_ISR(110) ALLOY_DEFAULT_ISR(111)
ALLOY_DEFAULT_ISR(112) ALLOY_DEFAULT_ISR(113) ALLOY_DEFAULT_ISR(114) ALLOY_DEFAULT_ISR(115)
ALLOY_DEFAULT_ISR(116) ALLOY_DEFAULT_ISR(117) ALLOY_DEFAULT_ISR(118) ALLOY_DEFAULT_ISR(119)
ALLOY_DEFAULT_ISR(120) ALLOY_DEFAULT_ISR(121) ALLOY_DEFAULT_ISR(122) ALLOY_DEFAULT_ISR(123)
ALLOY_DEFAULT_ISR(124) ALLOY_DEFAULT_ISR(125) ALLOY_DEFAULT_ISR(126) ALLOY_DEFAULT_ISR(127)

#undef ALLOY_DEFAULT_ISR

#endif  // __ARM_ARCH / __CORTEX_M
