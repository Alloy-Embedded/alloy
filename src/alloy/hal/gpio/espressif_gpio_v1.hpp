// GPIO driver for the classic ESP32 (LX6) GPIO block with output matrix.
//
// BEHAVIOR only: base/offsets come from the generated IP header. Two banks
// (pins 0-31 / 32-39). Output goes through the GPIO matrix: FUNCn_OUT_SEL
// is set to the "simple GPIO" signal — like RP2040's FUNCSEL=SIO, the
// bypass value is an IP-semantic constant of this matrix (TRM §4.2.3).
// Pads for GPIO0/2/4-class pins default to the GPIO IO_MUX function; v1
// does not touch IO_MUX (per-pin non-sequential offsets arrive with full
// matrix routing support).

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/ip/espressif/gpio_v1.hpp"

namespace alloy::hal {

template <class Pin>
    requires std::same_as<typename Pin::port_t::ip, alloy::ip::espressif::gpio_v1>
struct pin_impl<Pin> {
    using Port = typename Pin::port_t;
    using IP = typename Port::ip;
    static constexpr unsigned index = Pin::index;
    static_assert(index < 40, "ESP32 has GPIO0-39");
    static constexpr bool bank1 = index >= 32;
    static constexpr std::uint32_t bit = 1u << (bank1 ? index - 32 : index);

    static constexpr std::uint32_t kSimpleGpioOut = 0x100;  // matrix bypass signal

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Port::base);
    }

    static void make_output() {
        auto& sel = alloy::reg_at(Port::base, IP::FUNC_OUT_SEL_CFG_offset,
                                  IP::FUNC_OUT_SEL_CFG_stride, index);
        IP::out_sel.write(sel, kSimpleGpioOut);
        if constexpr (bank1) {
            r().ENABLE1_W1TS = bit;
        } else {
            r().ENABLE_W1TS = bit;
        }
    }

    static void make_input() {
        if constexpr (bank1) {
            r().ENABLE1_W1TC = bit;
        } else {
            r().ENABLE_W1TC = bit;
        }
    }

    static void set_high() {
        if constexpr (bank1) {
            r().OUT1_W1TS = bit;
        } else {
            r().OUT_W1TS = bit;
        }
    }

    static void set_low() {
        if constexpr (bank1) {
            r().OUT1_W1TC = bit;
        } else {
            r().OUT_W1TC = bit;
        }
    }

    static void toggle() {
        const std::uint32_t out = bank1 ? r().OUT1 : r().OUT;
        if (out & bit) {
            set_low();
        } else {
            set_high();
        }
    }

    [[nodiscard]] static bool read() {
        return ((bank1 ? r().IN1 : r().IN) & bit) != 0u;
    }
};

}  // namespace alloy::hal
