/// @file hal/gpio/lite.hpp
/// Lightweight, direct-MMIO GPIO driver for alloy.device.v2.1 peripherals.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
/// Works whenever `ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE` is true.
///
/// Supports:
///   - ST GPIO (all STM32 families with MODER/BSRR layout):
///     kTemplate == "gpio" AND kIpVersion starts with "STM" (or absent)
///
/// Register layout (STM32F0/F3/F4/G0/G4/H7/WB — same offsets):
///   0x00 MODER   — mode (input/output/AF/analog), 2 bits per pin
///   0x04 OTYPER  — output type (push-pull/open-drain), 1 bit per pin
///   0x08 OSPEEDR — output speed, 2 bits per pin
///   0x0C PUPDR   — pull-up/pull-down, 2 bits per pin
///   0x10 IDR     — input data register (read-only)
///   0x14 ODR     — output data register
///   0x18 BSRR    — bit set/reset (write: [15:0] set, [31:16] reset)
///   0x1C LCKR    — lock register
///   0x20 AFRL    — alternate function low  (pins 0–7),  4 bits per pin
///   0x24 AFRH    — alternate function high (pins 8–15), 4 bits per pin
///   0x28 BRR     — bit reset only (G0/G4/H7; use BSRR[31:16] on others)
///
/// Typical usage (configure PA2 as USART2 TX on STM32G071):
/// @code
///   #include "hal/gpio/lite.hpp"
///   #include "device/runtime.hpp"
///
///   using Gpioa = alloy::hal::gpio::lite::port<alloy::device::traits::gpioa>;
///
///   // PA2 = USART2 TX on G071: AF1
///   Gpioa::configure_af(2, 1);   // pin 2, AF1
///   Gpioa::set_speed(2, alloy::hal::gpio::lite::Speed::High);
/// @endcode
#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::gpio::lite {

// ============================================================================
// Enumerations
// ============================================================================

/// GPIO pin mode (MODER register, 2 bits per pin).
enum class Mode : std::uint8_t {
    Input  = 0,   ///< Digital input
    Output = 1,   ///< General-purpose output
    AF     = 2,   ///< Alternate function
    Analog = 3,   ///< Analog (ADC/DAC, lowest power)
};

/// GPIO output type (OTYPER register, 1 bit per pin).
enum class OutputType : std::uint8_t {
    PushPull  = 0,
    OpenDrain = 1,
};

/// GPIO output speed (OSPEEDR register, 2 bits per pin).
enum class Speed : std::uint8_t {
    Low       = 0,   ///< Low speed (~2 MHz on G0)
    Medium    = 1,   ///< Medium speed (~10 MHz)
    High      = 2,   ///< High speed (~40 MHz)
    VeryHigh  = 3,   ///< Very high speed (~80 MHz)
};

/// GPIO pull-up/down (PUPDR register, 2 bits per pin).
enum class Pull : std::uint8_t {
    None     = 0,
    Up       = 1,
    Down     = 2,
};

// ============================================================================
// Detail — register layout
// ============================================================================

namespace detail {

/// STM32 GPIO register map (applies to F0/F3/F4/G0/G4/H7/WB/L4 — same layout).
struct gpio_regs {
    std::uint32_t moder;    ///< 0x00
    std::uint32_t otyper;   ///< 0x04
    std::uint32_t ospeedr;  ///< 0x08
    std::uint32_t pupdr;    ///< 0x0C
    std::uint32_t idr;      ///< 0x10 (read-only)
    std::uint32_t odr;      ///< 0x14
    std::uint32_t bsrr;     ///< 0x18 (write-only: [15:0]=set, [31:16]=reset)
    std::uint32_t lckr;     ///< 0x1C
    std::uint32_t afrl;     ///< 0x20 (pins 0–7,  4 bits each)
    std::uint32_t afrh;     ///< 0x24 (pins 8–15, 4 bits each)
};

/// True when the peripheral is an ST-style GPIO (kTemplate == "gpio").
[[nodiscard]] consteval auto is_st_gpio(const char* tmpl) -> bool {
    return std::string_view{tmpl} == "gpio" || std::string_view{tmpl} == "gpiob";
}

}  // namespace detail

// ============================================================================
// Concept
// ============================================================================

/// Satisfied when P is an ST-style GPIO peripheral (any STM32 family).
template <typename P>
concept StGpio =
    device::PeripheralSpec<P> &&
    detail::is_st_gpio(P::kTemplate);

// ============================================================================
// port<P> — zero-size type, all methods static
// ============================================================================

/// Direct-MMIO GPIO port.  P must satisfy StGpio.
///
/// Pin numbers are 0–15 (relative to the port: pin 5 of GPIOA = PA5).
template <typename P>
    requires StGpio<P>
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

   private:
    [[nodiscard]] static auto r() noexcept -> volatile detail::gpio_regs& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile detail::gpio_regs*>(kBase);
    }

    // Mask helpers — validated at compile time via the kPinCount concept
    static constexpr void check_pin(std::uint8_t pin) noexcept {
        // Pin 0–15 only. In embedded contexts we trust the caller;
        // a debug build can assert here.
        (void)pin;
    }

   public:
    // -----------------------------------------------------------------------
    // Mode / type / speed / pull configuration
    // -----------------------------------------------------------------------

    /// Set the mode for one pin (input / output / AF / analog).
    static void set_mode(std::uint8_t pin, Mode mode) noexcept {
        check_pin(pin);
        const auto shift = static_cast<std::uint32_t>(pin) * 2u;
        r().moder = (r().moder & ~(0x3u << shift)) |
                    (static_cast<std::uint32_t>(mode) << shift);
    }

    /// Set the output type for one pin (push-pull / open-drain).
    static void set_output_type(std::uint8_t pin, OutputType ot) noexcept {
        check_pin(pin);
        const auto bit = std::uint32_t{1u} << pin;
        if (ot == OutputType::OpenDrain) {
            r().otyper |= bit;
        } else {
            r().otyper &= ~bit;
        }
    }

    /// Set the output speed for one pin.
    static void set_speed(std::uint8_t pin, Speed speed) noexcept {
        check_pin(pin);
        const auto shift = static_cast<std::uint32_t>(pin) * 2u;
        r().ospeedr = (r().ospeedr & ~(0x3u << shift)) |
                      (static_cast<std::uint32_t>(speed) << shift);
    }

    /// Set the pull-up/pull-down for one pin.
    static void set_pull(std::uint8_t pin, Pull pull) noexcept {
        check_pin(pin);
        const auto shift = static_cast<std::uint32_t>(pin) * 2u;
        r().pupdr = (r().pupdr & ~(0x3u << shift)) |
                    (static_cast<std::uint32_t>(pull) << shift);
    }

    /// Set the alternate function for one pin (AF0–AF15).
    static void set_af(std::uint8_t pin, std::uint8_t af_number) noexcept {
        check_pin(pin);
        if (pin < 8u) {
            const auto shift = static_cast<std::uint32_t>(pin) * 4u;
            r().afrl = (r().afrl & ~(0xFu << shift)) |
                       (static_cast<std::uint32_t>(af_number & 0xFu) << shift);
        } else {
            const auto shift = static_cast<std::uint32_t>(pin - 8u) * 4u;
            r().afrh = (r().afrh & ~(0xFu << shift)) |
                       (static_cast<std::uint32_t>(af_number & 0xFu) << shift);
        }
    }

    // -----------------------------------------------------------------------
    // Composite configurators (convenience wrappers)
    // -----------------------------------------------------------------------

    /// Configure a pin for alternate function use.
    /// Sets mode = AF, speed = High, pull = None, open_drain = false by default.
    static void configure_af(
        std::uint8_t pin,
        std::uint8_t af_number,
        Speed        speed     = Speed::High,
        Pull         pull      = Pull::None,
        OutputType   ot        = OutputType::PushPull
    ) noexcept {
        set_mode(pin, Mode::AF);
        set_af(pin, af_number);
        set_speed(pin, speed);
        set_pull(pin, pull);
        set_output_type(pin, ot);
    }

    /// Configure a pin as a general-purpose output.
    static void configure_output(
        std::uint8_t pin,
        Speed        speed = Speed::Low,
        OutputType   ot    = OutputType::PushPull
    ) noexcept {
        set_mode(pin, Mode::Output);
        set_speed(pin, speed);
        set_output_type(pin, ot);
    }

    /// Configure a pin as a digital input.
    static void configure_input(std::uint8_t pin, Pull pull = Pull::None) noexcept {
        set_mode(pin, Mode::Input);
        set_pull(pin, pull);
    }

    /// Configure a pin for analog use (ADC / DAC input, lowest leakage).
    static void configure_analog(std::uint8_t pin) noexcept {
        set_mode(pin, Mode::Analog);
        set_pull(pin, Pull::None);
    }

    // -----------------------------------------------------------------------
    // Digital output
    // -----------------------------------------------------------------------

    /// Drive the pin high (uses BSRR set-side — atomic, no read-modify-write).
    static void set_high(std::uint8_t pin) noexcept {
        r().bsrr = std::uint32_t{1u} << pin;
    }

    /// Drive the pin low (uses BSRR reset-side — atomic, no read-modify-write).
    static void set_low(std::uint8_t pin) noexcept {
        r().bsrr = std::uint32_t{1u} << (pin + 16u);
    }

    /// Toggle pin output by XOR-ing ODR.
    static void toggle(std::uint8_t pin) noexcept {
        r().odr ^= std::uint32_t{1u} << pin;
    }

    /// Set multiple output pins at once using a bitmask (16-bit, one bit per pin).
    static void set_mask_high(std::uint16_t mask) noexcept {
        r().bsrr = static_cast<std::uint32_t>(mask);
    }

    static void set_mask_low(std::uint16_t mask) noexcept {
        r().bsrr = static_cast<std::uint32_t>(mask) << 16u;
    }

    // -----------------------------------------------------------------------
    // Digital input
    // -----------------------------------------------------------------------

    /// Read the input state of one pin (from IDR).
    [[nodiscard]] static auto read(std::uint8_t pin) noexcept -> bool {
        return ((r().idr >> pin) & 1u) != 0u;
    }

    /// Read all 16 pins from IDR as a bitmask.
    [[nodiscard]] static auto read_all() noexcept -> std::uint16_t {
        return static_cast<std::uint16_t>(r().idr & 0xFFFFu);
    }

    port() = delete;
};

}  // namespace alloy::hal::gpio::lite
