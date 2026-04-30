#pragma once

// nxp_imxrt_gpio_schema.hpp — Task 1.4 (gpio-schema-open-traits)
//
// NxpImxrtGpioSchema — satisfies GpioSchemaImpl for NXP i.MX RT GPIO IP.
//
// Register layout (offsets from GPIO base, e.g. GPIO1 = 0x401B8000):
//   0x00  DR      — Data Register (output data)
//   0x04  GDIR    — GPIO Direction (1=output, 0=input) per pin
//   0x08  PSR     — Pad Sample Register (input data, read-only)
//   0x0C  ICR1    — Interrupt Config 1 (pins 0–15)
//   0x10  ICR2    — Interrupt Config 2 (pins 16–31)
//   0x14  IMR     — Interrupt Mask Register
//   0x18  ISR     — Interrupt Status Register
//   0x1C  EDGE_SEL — Edge Select Register
//   0x88  DR_SET  — Data Set (write 1 to set pin high, no read-modify-write)
//   0x8C  DR_CLEAR— Data Clear (write 1 to set pin low)
//   0x90  DR_TOGGLE—Data Toggle
//
// Note: DR_SET/DR_CLEAR require i.MX RT1010+. For legacy i.MX RT (1020, 1060)
// without these registers, output_set_field and output_clear_field fall back to DR.

#include <cstdint>

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime;

struct NxpImxrtGpioSchema {
    static constexpr const char* kSchemaName = "nxp_imxrt_gpio_v1";

    [[nodiscard]] static constexpr auto mode_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x04u, pin, 1);  // GDIR
    }

    [[nodiscard]] static constexpr auto input_data_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x08u, pin, 1);  // PSR (Pad Sample Register)
    }

    // Use DR_SET register for atomic set (i.MX RT1010+)
    [[nodiscard]] static constexpr auto output_set_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x88u, pin, 1);  // DR_SET
    }

    // Use DR_CLEAR register for atomic clear
    [[nodiscard]] static constexpr auto output_clear_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x8Cu, pin, 1);  // DR_CLEAR
    }

    // NXP i.MX RT has no built-in pull resistors in GPIO — they're in IOMUXC pad config.
    // Return invalid ref here; HAL must handle this via connect/pinmux.
    [[nodiscard]] static constexpr auto pull_field(std::uintptr_t /*base*/, int /*pin*/) -> rt::FieldRef {
        return rt::kInvalidFieldRef;  // pulls configured via IOMUXC, not GPIO registers
    }

    [[nodiscard]] static constexpr auto output_value_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x00u, pin, 1);  // DR
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

static_assert(GpioSchemaImpl<NxpImxrtGpioSchema>, "NxpImxrtGpioSchema must satisfy GpioSchemaImpl");

}  // namespace alloy::hal::gpio::detail
