#pragma once

// espressif_gpio_schema.hpp — Task 4.3 (gpio-schema-open-traits)
//
// EspressifGpioSchema — satisfies GpioSchemaImpl for ESP32 GPIO matrix IP.
//
// ESP32 GPIO register layout (GPIO_BASE = 0x3FF44000):
//   +0x004  GPIO_OUT_REG     — Output value (bits 0–31 = GPIO0–31)
//   +0x008  GPIO_OUT_W1TS    — Write-1-to-set output
//   +0x00C  GPIO_OUT_W1TC    — Write-1-to-clear output
//   +0x020  GPIO_OUT1_REG    — Output value (bits 0–7 = GPIO32–39)
//   +0x024  GPIO_OUT1_W1TS
//   +0x028  GPIO_OUT1_W1TC
//   +0x034  GPIO_ENABLE_REG  — Output enable (1=output, 0=input) GPIO0–31
//   +0x038  GPIO_ENABLE_W1TS — Enable set
//   +0x03C  GPIO_ENABLE_W1TC — Enable clear
//   +0x04C  GPIO_ENABLE1_REG — GPIO32–39
//   +0x050  GPIO_ENABLE1_W1TS
//   +0x054  GPIO_ENABLE1_W1TC
//   +0x03C  GPIO_IN_REG      — Input value GPIO0–31
//   +0x044  GPIO_IN1_REG     — Input value GPIO32–39
//
// Pull-up/pull-down are configured in the IO_MUX block (0x3FF49000),
// not in GPIO matrix registers.  pull_field returns kInvalidFieldRef.
//
// For GPIO32–39 the OUT/ENABLE are at different offsets (GPIO1 bank).
// This schema handles GPIO0–31.  Use a separate schema for GPIO32+ if needed.

#include <cstdint>

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime;

struct EspressifGpioSchema {
    static constexpr const char* kSchemaName = "espressif_gpio_matrix";

    // mode_field: GPIO_ENABLE_W1TS register, bit pin (write 1 → output)
    [[nodiscard]] static constexpr auto mode_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        if (pin < 0 || pin >= 32) { return rt::kInvalidFieldRef; }
        return _synth(base, 0x038u, pin, 1);  // GPIO_ENABLE_W1TS
    }

    // input_data_field: GPIO_IN_REG, bit pin
    [[nodiscard]] static constexpr auto input_data_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        if (pin < 0 || pin >= 32) { return rt::kInvalidFieldRef; }
        return _synth(base, 0x03Cu, pin, 1);  // GPIO_IN_REG
    }

    // output_set_field: GPIO_OUT_W1TS, bit pin
    [[nodiscard]] static constexpr auto output_set_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        if (pin < 0 || pin >= 32) { return rt::kInvalidFieldRef; }
        return _synth(base, 0x008u, pin, 1);  // GPIO_OUT_W1TS
    }

    // output_clear_field: GPIO_OUT_W1TC, bit pin
    [[nodiscard]] static constexpr auto output_clear_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        if (pin < 0 || pin >= 32) { return rt::kInvalidFieldRef; }
        return _synth(base, 0x00Cu, pin, 1);  // GPIO_OUT_W1TC
    }

    // pull_field: ESP32 pulls are in IO_MUX, not in GPIO matrix.
    // Returns kInvalidFieldRef; caller configures pulls via connect layer.
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

static_assert(GpioSchemaImpl<EspressifGpioSchema>,
              "EspressifGpioSchema must satisfy GpioSchemaImpl");

}  // namespace alloy::hal::gpio::detail
