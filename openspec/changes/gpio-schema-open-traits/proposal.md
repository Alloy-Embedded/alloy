# Proposal: Open GPIO Schema Traits (Replace Closed Enum)

## Status
`open` — architectural priority.

## Problem

The GPIO HAL uses a closed `GpioSchema` enum to dispatch between vendor IP
implementations:

```cpp
static constexpr auto schema = hal::detail::runtime::to_gpio_schema(
    peripheral_traits::kSchemaId);
// ...
if constexpr (schema == GpioSchema::st_gpio)       { /* STM32 MODER/BSRR */ }
else if constexpr (schema == GpioSchema::microchip_pio_v) { /* PIO_PER/CODR */ }
else if constexpr (schema == GpioSchema::nxp_imxrt_gpio_v1) { /* GDIR/DR */ }
```

Adding a new GPIO IP (Nordic GPIOTE, TI CC26x2 GPIO, Renesas RA GPIO, etc.)
requires modifying the HAL core, not just the codegen. This creates a hard
coupling between the vendor IP landscape and the alloy codebase.

The same pattern exists — with the same problem — for UART, SPI, and I2C
schemas.

## Proposed Solution

### GPIO Schema as an open Concept

Replace the closed enum with an open concept that any vendor schema can satisfy:

```cpp
// src/hal/detail/gpio_schema_concept.hpp
namespace alloy::hal::detail::gpio {

template <typename T>
concept GpioSchemaImpl = requires(
    T,
    device::PinId pin,
    std::uint32_t mode,   // e.g. 0=input 1=output 2=AF 3=analog
    std::uint32_t pull,   // 0=none 1=up 2=down
    std::uint32_t speed,  // vendor-defined
    std::uint32_t af,     // alternate function number
    bool high
) {
    // Field references for each GPIO attribute
    { T::mode_field(pin, mode)   } -> std::same_as<RuntimeFieldRef>;
    { T::output_set_field(pin)   } -> std::same_as<RuntimeFieldRef>;
    { T::output_clear_field(pin) } -> std::same_as<RuntimeFieldRef>;
    { T::input_data_field(pin)   } -> std::same_as<RuntimeFieldRef>;
    { T::pull_field(pin, pull)   } -> std::same_as<RuntimeFieldRef>;
    // Optional (default to kInvalidFieldRef):
    // { T::speed_field(pin, speed) } -> std::same_as<RuntimeFieldRef>;
    // { T::af_field(pin, af)       } -> std::same_as<RuntimeFieldRef>;
    // { T::open_drain_field(pin)   } -> std::same_as<RuntimeFieldRef>;
};

}  // namespace alloy::hal::detail::gpio
```

### Vendor schema implementations

Each vendor GPIO IP is a separate header in `src/hal/detail/gpio/`:

```cpp
// src/hal/detail/gpio/st_gpio_schema.hpp
struct StGpioSchema {
    // STM32 MODER register: 2 bits per pin
    static constexpr auto mode_field(device::PinId pin, std::uint32_t /*mode*/)
        -> RuntimeFieldRef
    {
        // Calculate MODER field ref from pin index
        const auto idx = static_cast<std::uint32_t>(pin) & 0xFu;
        return { kModerBase + (pin_port(pin) * 0x400u), idx * 2u, 2u, true };
    }

    // BSRR: BS bits (set) + BR bits (reset)
    static constexpr auto output_set_field(device::PinId pin)
        -> RuntimeFieldRef { /* ... */ }
    static constexpr auto output_clear_field(device::PinId pin)
        -> RuntimeFieldRef { /* ... */ }

    // IDR: 1 bit per pin
    static constexpr auto input_data_field(device::PinId pin)
        -> RuntimeFieldRef { /* ... */ }

    // PUPDR: 2 bits per pin
    static constexpr auto pull_field(device::PinId pin, std::uint32_t /*pull*/)
        -> RuntimeFieldRef { /* ... */ }

    // OSPEEDR: 2 bits per pin
    static constexpr auto speed_field(device::PinId pin, std::uint32_t /*speed*/)
        -> RuntimeFieldRef { /* ... */ }

    // AFRL/AFRH: 4 bits per pin
    static constexpr auto af_field(device::PinId pin, std::uint32_t /*af*/)
        -> RuntimeFieldRef { /* ... */ }
};

static_assert(alloy::hal::detail::gpio::GpioSchemaImpl<StGpioSchema>);
```

```cpp
// src/hal/detail/gpio/microchip_pio_schema.hpp
struct MicrochipPioSchema {
    // PIO_PER/PDR for enable, OER/ODR for direction, CODR/SODR for output,
    // PDSR for input, PUER/PUDR for pull-up
    // ...
};
```

```cpp
// src/hal/detail/gpio/nordic_gpiote_schema.hpp (new, no HAL changes needed)
struct NordicGpioteSchema {
    // OUT register: 1 bit per pin; DIR register; IN register
    // ...
};
```

### GpioSemanticTraits gains schema_type

The generated `GpioSemanticTraits<PinId>` adds a `schema_type` alias:

```cpp
// Generated for STM32 devices:
template <>
struct GpioSemanticTraits<PinId::PA0> {
    using schema_type = alloy::hal::detail::gpio::StGpioSchema;
    static constexpr bool kPresent = true;
    // ... existing fields ...
};
```

### pin_handle uses the schema_type directly

```cpp
template <typename Pin>
class pin_handle {
    using schema = typename device::GpioSemanticTraits<Pin::id>::schema_type;

    [[nodiscard]] auto set_output(bool high) const -> core::Result<void, core::ErrorCode> {
        if constexpr (high) {
            return detail::runtime::modify_field(schema::output_set_field(Pin::id), 1u);
        } else {
            return detail::runtime::modify_field(schema::output_clear_field(Pin::id), 1u);
        }
    }
    // ...
};
```

No `if constexpr (schema == GpioSchema::xxx)` chains remain. The correct
implementation is selected entirely by the `schema_type` alias in the traits.

### Backward compatibility

The old `GpioSchema` enum and `to_gpio_schema()` function are deprecated but
not removed in the initial migration. Existing generated devices emit both the
old `kSchemaId` field and the new `schema_type` alias. The HAL uses `schema_type`
when present and falls back to the enum when not.

After all supported devices emit `schema_type`, the enum path is removed.

## Impact on other HAL modules

The same refactor applies to UART, SPI, and I2C which have similar `if constexpr`
schema dispatch chains. Each gets its own concept + schema impl headers.
This spec covers GPIO first as a proof of concept. Follow-up specs:
`uart-schema-open-traits`, `spi-schema-open-traits`, `i2c-schema-open-traits`.
