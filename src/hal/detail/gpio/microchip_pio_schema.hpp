#pragma once

// microchip_pio_schema.hpp — Task 1.3 (gpio-schema-open-traits)
//
// MicrochipPioSchema — satisfies GpioSchemaImpl for Microchip PIO IP
// (SAME70, SAM4, SAMD, SAMV71, etc.).
//
// Register layout (offsets from PIO base, e.g. PIOA = 0x400E0E00):
//   0x00  PER   — PIO Enable (write 1 to enable pin under PIO control)
//   0x04  PDR   — PIO Disable (write 1 to disable pin / give to peripheral)
//   0x08  PSR   — PIO Status (read: 1 = PIO controls pin)
//   0x10  OER   — Output Enable Register (write 1 → output)
//   0x14  ODR   — Output Disable Register (write 1 → input)
//   0x18  OSR   — Output Status Register
//   0x30  SODR  — Set Output Data Register (write 1 → pin high)
//   0x34  CODR  — Clear Output Data Register (write 1 → pin low)
//   0x3C  PDSR  — Pin Data Status Register (read: current input level)
//   0x60  PUER  — Pull-Up Enable Register
//   0x64  PUDR  — Pull-Up Disable Register
//   0x90  PPDER — Pull-Down Enable Register   (SAME70+)
//   0x94  PPDDR — Pull-Down Disable Register  (SAME70+)
//   0x70  ABCDSR1 — Peripheral select 1
//   0x74  ABCDSR2 — Peripheral select 2
//   0xE4  DRIVER — Drive (open-drain control)

#include <cstdint>

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime;

struct MicrochipPioSchema {
    static constexpr const char* kSchemaName = "microchip_pio_v";

    // mode_field: OER/ODR — we use OER (output enable) as the "mode" for output direction.
    // Value 1 = output, 0 = input (via ODR write).
    // Note: PIO control requires PER to be set first (handled in configure step).
    [[nodiscard]] static constexpr auto mode_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x10u, pin, 1);  // OER
    }

    [[nodiscard]] static constexpr auto input_data_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x3Cu, pin, 1);  // PDSR
    }

    [[nodiscard]] static constexpr auto output_set_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x30u, pin, 1);  // SODR
    }

    [[nodiscard]] static constexpr auto output_clear_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x34u, pin, 1);  // CODR
    }

    // pull_field: PUER (write 1 to enable pull-up) — used for pull-up state.
    // Pull-down (PPDER) is handled separately in configure step.
    [[nodiscard]] static constexpr auto pull_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x60u, pin, 1);  // PUER
    }

    // Optional: PIO peripheral disable field (used to hand pin to peripheral IP)
    [[nodiscard]] static constexpr auto pio_enable_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x00u, pin, 1);  // PER
    }

    [[nodiscard]] static constexpr auto pio_disable_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x04u, pin, 1);  // PDR
    }

    [[nodiscard]] static constexpr auto output_disable_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x14u, pin, 1);  // ODR
    }

    [[nodiscard]] static constexpr auto pull_up_disable_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x64u, pin, 1);  // PUDR
    }

    [[nodiscard]] static constexpr auto pull_down_enable_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x90u, pin, 1);  // PPDER
    }

    [[nodiscard]] static constexpr auto pull_down_disable_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0x94u, pin, 1);  // PPDDR
    }

    [[nodiscard]] static constexpr auto open_drain_field(std::uintptr_t base, int pin) -> rt::FieldRef {
        return _synth(base, 0xE4u, pin, 1);  // DRIVER (multi-drive enable)
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

static_assert(GpioSchemaImpl<MicrochipPioSchema>, "MicrochipPioSchema must satisfy GpioSchemaImpl");
static_assert(HasOpenDrainField<MicrochipPioSchema>);

}  // namespace alloy::hal::gpio::detail
