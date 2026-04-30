#pragma once

// rp2040_gpio_schema.hpp — Task 4.2 (gpio-schema-open-traits)
//
// Rp2040GpioSchema — satisfies GpioSchemaImpl for RP2040 / RP2350 SIO GPIO.
//
// RP2040 SIO GPIO register layout (SIO_BASE = 0xD0000000):
//   +0x020  GPIO_OUT        — Output value (bits 0–29)
//   +0x024  GPIO_OUT_SET    — Atomic set output high
//   +0x028  GPIO_OUT_CLR    — Atomic set output low
//   +0x02C  GPIO_OUT_XOR    — Atomic toggle
//   +0x030  GPIO_OE         — Output enable (1 = output)
//   +0x034  GPIO_OE_SET     — Atomic set OE
//   +0x038  GPIO_OE_CLR     — Atomic clear OE (= input)
//   +0x03C  GPIO_OE_XOR     — Atomic toggle OE
//   +0x004  GPIO_IN         — Input value (read-only)
//
// Pull-up/pull-down on RP2040 are controlled through the pads subsystem
// (PADS_BANK0_BASE = 0x4001C000), not via SIO registers.
// pull_field returns kInvalidFieldRef; caller must configure pads separately.
//
// mode_field: GPIO_OE_SET/GPIO_OE_CLR — use GPIO_OE_SET to enable output
//             (bit pin = 1 → output; write GPIO_OE_CLR to revert to input)
// To stay uniform with the concept (single mode_field), we return GPIO_OE_SET
// for output detection; the generic configure path already handles set vs. clear.

#include <cstdint>

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime;

struct Rp2040GpioSchema {
    static constexpr const char* kSchemaName = "rp2040_sio_gpio";

    // mode_field: GPIO_OE_SET register, bit pin (write 1 → enables output)
    [[nodiscard]] static constexpr auto mode_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x034u, pin, 1);  // GPIO_OE_SET
    }

    // input_data_field: GPIO_IN register, bit pin
    [[nodiscard]] static constexpr auto input_data_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x004u, pin, 1);  // GPIO_IN
    }

    // output_set_field: GPIO_OUT_SET register, bit pin
    [[nodiscard]] static constexpr auto output_set_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x024u, pin, 1);  // GPIO_OUT_SET
    }

    // output_clear_field: GPIO_OUT_CLR register, bit pin
    [[nodiscard]] static constexpr auto output_clear_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x028u, pin, 1);  // GPIO_OUT_CLR
    }

    // pull_field: RP2040 pulls are in PADS_BANK0 — not in SIO.
    // Returns kInvalidFieldRef; configure pulls through the connect layer.
    [[nodiscard]] static constexpr auto pull_field(std::uintptr_t /*base*/, int /*pin*/) -> rt::FieldRef {
        return rt::kInvalidFieldRef;
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

static_assert(GpioSchemaImpl<Rp2040GpioSchema>,
              "Rp2040GpioSchema must satisfy GpioSchemaImpl");

}  // namespace alloy::hal::gpio::detail
