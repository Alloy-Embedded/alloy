#pragma once

// gpio_schema_concept.hpp — Task 1.1 (gpio-schema-open-traits)
//
// GpioSchemaImpl concept: open, vendor-extensible GPIO backend.
//
// Schema types provide static methods that compute RuntimeFieldRef values from
// a peripheral base address + pin number.  New vendors add a schema type;
// no edits to core HAL files are needed.
//
// Required static methods (constexpr):
//   mode_field(base_address, pin)        → FieldRef  (MODER, PIO_OER, GDIR)
//   input_data_field(base_address, pin)  → FieldRef  (IDR, PDSR, GPIO_DR)
//   output_set_field(base_address, pin)  → FieldRef  (BSRR set, SODR, SIODR)
//   output_clear_field(base_address, pin)→ FieldRef  (BSRR clr, CODR)
//   pull_field(base_address, pin)        → FieldRef  (PUPDR, pull-ctrl)
//
// Optional static methods (detected via HasSpeedField / HasAfField / HasOpenDrainField):
//   speed_field(base_address, pin)       → FieldRef  (OSPEEDR, drive-strength)
//   af_field(base_address, pin)          → FieldRef  (AFR, PSEL, IOMUXC)
//   open_drain_field(base_address, pin)  → FieldRef  (OTYPER, ODE bit)

#include <concepts>
#include <cstdint>

#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime;

// ---------------------------------------------------------------------------
// GpioSchemaImpl concept (Task 1.1)
// ---------------------------------------------------------------------------

template <typename T>
concept GpioSchemaImpl = requires(std::uintptr_t base_addr, int pin) {
    { T::mode_field(base_addr, pin)         } -> std::same_as<rt::FieldRef>;
    { T::input_data_field(base_addr, pin)   } -> std::same_as<rt::FieldRef>;
    { T::output_set_field(base_addr, pin)   } -> std::same_as<rt::FieldRef>;
    { T::output_clear_field(base_addr, pin) } -> std::same_as<rt::FieldRef>;
    { T::pull_field(base_addr, pin)         } -> std::same_as<rt::FieldRef>;
    requires std::is_convertible_v<decltype(T::kSchemaName), const char*>;
};

template <typename T>
concept HasSpeedField = GpioSchemaImpl<T> && requires(std::uintptr_t b, int p) {
    { T::speed_field(b, p) } -> std::same_as<rt::FieldRef>;
};

template <typename T>
concept HasAfField = GpioSchemaImpl<T> && requires(std::uintptr_t b, int p) {
    { T::af_field(b, p) } -> std::same_as<rt::FieldRef>;
};

template <typename T>
concept HasOpenDrainField = GpioSchemaImpl<T> && requires(std::uintptr_t b, int p) {
    { T::open_drain_field(b, p) } -> std::same_as<rt::FieldRef>;
};

}  // namespace alloy::hal::gpio::detail
