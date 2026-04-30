#pragma once

// st_gpio_schema.hpp — Task 1.2 (gpio-schema-open-traits)
//
// StGpioSchema — satisfies GpioSchemaImpl for ST GPIO IP (all STM32 families).
//
// Register layout (offsets from peripheral base):
//   0x00  MODER   — mode[1:0] per pin (00=input, 01=output, 10=AF, 11=analog)
//   0x04  OTYPER  — output type[0] per pin (0=push-pull, 1=open-drain)
//   0x08  OSPEEDR — speed[1:0] per pin
//   0x0C  PUPDR   — pull[1:0] per pin (00=none, 01=up, 10=down)
//   0x10  IDR     — input data[0] per pin (read-only)
//   0x14  ODR     — output data[0] per pin
//   0x18  BSRR    — bits[15:0]=set, bits[31:16]=reset (write-only)
//   0x20  AFR[0]  — AF[3:0] for pins 0–7
//   0x24  AFR[1]  — AF[3:0] for pins 8–15

#include <cstdint>

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/runtime_ops.hpp"  // synth_field_ref is in runtime_connector, pulled here via

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime;

struct StGpioSchema {
    static constexpr const char* kSchemaName = "st_gpio";

    [[nodiscard]] static constexpr auto mode_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x00u, static_cast<int>(pin * 2), 2);
    }

    [[nodiscard]] static constexpr auto input_data_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x10u, pin, 1);
    }

    // output_set_field: BSRR bits [pin] (set side — high 16 bits not used here, written 1 to set)
    [[nodiscard]] static constexpr auto output_set_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x18u, pin, 1);
    }

    // output_clear_field: BSRR bits [pin+16] (reset side)
    [[nodiscard]] static constexpr auto output_clear_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x18u, pin + 16, 1);
    }

    [[nodiscard]] static constexpr auto pull_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x0Cu, static_cast<int>(pin * 2), 2);
    }

    // Optional fields
    [[nodiscard]] static constexpr auto speed_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x08u, static_cast<int>(pin * 2), 2);
    }

    [[nodiscard]] static constexpr auto af_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        const auto offset = pin < 8 ? 0x20u : 0x24u;
        return _synth(base, offset, static_cast<int>((pin % 8) * 4), 4);
    }

    [[nodiscard]] static constexpr auto open_drain_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x04u, pin, 1);
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

static_assert(GpioSchemaImpl<StGpioSchema>, "StGpioSchema must satisfy GpioSchemaImpl");
static_assert(HasSpeedField<StGpioSchema>);
static_assert(HasAfField<StGpioSchema>);
static_assert(HasOpenDrainField<StGpioSchema>);

}  // namespace alloy::hal::gpio::detail
