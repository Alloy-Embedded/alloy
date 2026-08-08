// The escape hatch: alloy is not all-or-nothing.
//
// A company adopting alloy has a drawer full of CubeMX-generated code and a
// register it needs to poke that no HAL exposes. This example does all three
// of the things that adoption depends on, in one image that CI builds:
//
//   1. calls a vendor-HAL-shaped C function with a CMSIS-shaped pointer,
//   2. writes a register directly through `alloy::dev::` + the generated IP
//      struct, which is typed and keeps its address from the chip database,
//   3. writes a register through a raw literal address — the last resort, and
//      the one thing this framework exists to make unnecessary,
//
// while a vendor CMSIS-shaped device header sits INCLUDED ABOVE every alloy
// header, so the include-order collision question is answered by the build
// rather than by a paragraph.
//
// See docs/guide/escape-hatch.md for what does collide (it is at link time,
// not include time) and how to avoid it.

// Deliberately FIRST: a vendor device header is a wall of object-like macros
// (GPIOA, RCC, __IO). If any of them collided with an alloy name, putting it
// here is what would break the build.
#include "vendor_device.h"
#include "vendor_hal.h"

#include <alloy/board.hpp>
#include <alloy/device.hpp>
#include <alloy/irq.hpp>
#include <alloy/time.hpp>

#include <cstdint>

// --- The bridge -------------------------------------------------------------
// alloy's facts come from the chip database; the vendor header carries its own
// copy of the same constants. Where both exist, say so at COMPILE TIME. If a
// vendor header and the database ever disagreed about where a peripheral
// lives, this is the line that would tell you — rather than a device that
// half-works on the bench.
static_assert(GPIOA_BASE == alloy::dev::gpioa.base,
              "vendor header and alloy-devices disagree about GPIOA");
static_assert(GPIOB_BASE == alloy::dev::gpiob.base,
              "vendor header and alloy-devices disagree about GPIOB");

namespace {

constexpr std::uint32_t kLedPin = 5;   // PA5 — the board's `led` role

// --- Route 2: a typed poke through the generated register description -------
// `alloy::dev::gpioa.base` is the address from the database; `gpio_v2::regs`
// is the generated layout, with its offsets static_asserted in the header.
// This is the escape hatch alloy recommends: no HAL involved, but no address
// or offset typed by hand either.
void poke_via_alloy_dev(bool on) {
    using gpio = alloy::dev::gpioa_t::ip;
    auto& regs = *reinterpret_cast<gpio::regs*>(alloy::dev::gpioa.base);
    regs.BSRR = on ? (std::uint32_t{1} << kLedPin)
                   : (std::uint32_t{1} << (kLedPin + 16u));
}

// --- Route 3: a raw literal address -----------------------------------------
// Legal, and sometimes the only way to reach an erratum workaround. It is also
// exactly what alloy's first contract guard forbids in framework code, because
// the address is now a fact with no provenance and no board portability. Keep
// these in one file and comment the source (datasheet, revision, page).
void poke_via_literal(bool on) {
    // STM32G071 RM0444 rev 5, §6.4.7: GPIOA_BSRR at 0x5000'0018.
    auto* bsrr = reinterpret_cast<volatile std::uint32_t*>(0x50000018u);
    *bsrr = on ? (std::uint32_t{1} << kLedPin)
               : (std::uint32_t{1} << (kLedPin + 16u));
}

// --- The link-time hazard, demonstrated on purpose --------------------------
// alloy's generated vector table defines every `<NAME>_IRQHandler` as a WEAK
// thunk into alloy_irq_dispatch(). A strong definition here silently wins —
// which is how a CubeMX file "stops alloy::irq from working" with no warning.
// TIM2 is not used by this example, so overriding it is safe AND visible: the
// image's TIM2_IRQHandler is this function, not the thunk.
volatile std::uint32_t vendor_isr_hits = 0;

}  // namespace

extern "C" void TIM2_IRQHandler(void) {   // NOLINT: deliberately strong
    vendor_isr_hits = vendor_isr_hits + 1u;   // volatile: no ++
}

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("escape hatch: three routes to one LED\r\n");

    // Route 1: hand alloy's address to the vendor's pointer type. The cast is
    // the whole interop story — the vendor function is unmodified C.
    auto* vendor_port = reinterpret_cast<GPIO_TypeDef*>(alloy::dev::gpioa.base);

    // Bring the pin up with alloy, so the vendor call below has something
    // configured to write to. Configuration through alloy, poking through
    // whatever you like, is the mix this page is about.
    board::led.off();

    for (unsigned step = 0;; ++step) {
        switch (step % 3u) {
        case 0:
            uart.write("alloy  ");
            board::led.toggle();
            break;
        case 1:
            uart.write("vendor ");
            VENDOR_GPIO_TogglePin(vendor_port, kLedPin);
            break;
        default:
            uart.write("raw    ");
            poke_via_alloy_dev(true);
            alloy::sleep_for(std::chrono::microseconds{50'000});
            poke_via_literal(false);
            break;
        }
        uart.write("ok\r\n");
        alloy::sleep_for(std::chrono::microseconds{250'000});
        if (vendor_isr_hits != 0u) {          // never true; keeps the ISR alive
            uart.write("tim2\r\n");
        }
    }
}
