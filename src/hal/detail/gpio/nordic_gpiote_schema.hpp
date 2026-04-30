#pragma once

// nordic_gpiote_schema.hpp — Task 4.1 (gpio-schema-open-traits)
//
// NordicGpioteSchema — satisfies GpioSchemaImpl for nRF52840 GPIO/GPIOTE IP.
//
// nRF52840 GPIO register layout (P0 base = 0x50000000, P1 = 0x50000300):
//   +0x504  OUT         — Write output value (bit per pin)
//   +0x508  OUTSET      — Set output high
//   +0x50C  OUTCLR      — Set output low
//   +0x510  IN          — Read input value
//   +0x514  DIR         — Direction (1=output, 0=input)
//   +0x518  DIRSET      — Set direction to output
//   +0x51C  DIRCLR      — Set direction to input
//   +0x700  PIN_CNF[n]  — Pin configuration word per pin:
//                          bits[1:0]  DIR (0=input, 1=output)
//                          bits[3:2]  INPUT (0=connect, 1=disconnect)
//                          bits[5:4]  PULL (0=none, 1=down, 3=up)
//                          bits[10:8] DRIVE
//                          bits[17:16] SENSE
//
// This schema uses PIN_CNF for mode/pull and OUT_SET/OUT_CLR for writes.
// The DIR bit is in PIN_CNF[n][0]; pull bits in PIN_CNF[n][5:4].

#include <cstdint>

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime;

struct NordicGpioteSchema {
    static constexpr const char* kSchemaName = "nordic_gpiote";

    // mode_field: PIN_CNF[n] bit 0 — DIR (0=input, 1=output)
    [[nodiscard]] static constexpr auto mode_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, static_cast<std::uint32_t>(0x700u + pin * 4), 0, 1);
    }

    // input_data_field: IN register, bit pin
    [[nodiscard]] static constexpr auto input_data_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x510u, pin, 1);
    }

    // output_set_field: OUTSET register, bit pin
    [[nodiscard]] static constexpr auto output_set_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x508u, pin, 1);
    }

    // output_clear_field: OUTCLR register, bit pin
    [[nodiscard]] static constexpr auto output_clear_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x50Cu, pin, 1);
    }

    // pull_field: PIN_CNF[n] bits [5:4] — PULL (0=none, 1=down, 3=up)
    [[nodiscard]] static constexpr auto pull_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, static_cast<std::uint32_t>(0x700u + pin * 4), 4, 2);
    }

private:
    [[nodiscard]] static constexpr auto _synth(std::uintptr_t base, std::uint32_t offset,
                                                int bit_off, int bit_w) -> rt::FieldRef {
        if (base == 0u || bit_off < 0 || bit_w <= 0) { return rt::kInvalidFieldRef; }
        return {
            .reg = { .base_address = base, .offset_bytes = offset, .valid = true },
            .bit_offset = static_cast<std::uint16_t>(bit_off),
            .bit_width  = static_cast<std::uint16_t>(bit_w),
            .valid      = true,
        };
    }
};

static_assert(GpioSchemaImpl<NordicGpioteSchema>,
              "NordicGpioteSchema must satisfy GpioSchemaImpl");

}  // namespace alloy::hal::gpio::detail
