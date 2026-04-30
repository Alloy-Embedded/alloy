#pragma once

// unknown_gpio_schema.hpp — Task 1.5 (gpio-schema-open-traits)
//
// UnknownGpioSchema — satisfies GpioSchemaImpl, returns kInvalidFieldRef for
// all fields.  Used as graceful fallback when a device's GPIO IP is not yet
// supported.  All configure() calls will return NotSupported.

#include <cstdint>

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime;

struct UnknownGpioSchema {
    static constexpr const char* kSchemaName = "unknown";

    [[nodiscard]] static constexpr auto mode_field(std::uintptr_t, int) -> rt::FieldRef {
        return rt::kInvalidFieldRef;
    }
    [[nodiscard]] static constexpr auto input_data_field(std::uintptr_t, int) -> rt::FieldRef {
        return rt::kInvalidFieldRef;
    }
    [[nodiscard]] static constexpr auto output_set_field(std::uintptr_t, int) -> rt::FieldRef {
        return rt::kInvalidFieldRef;
    }
    [[nodiscard]] static constexpr auto output_clear_field(std::uintptr_t, int) -> rt::FieldRef {
        return rt::kInvalidFieldRef;
    }
    [[nodiscard]] static constexpr auto pull_field(std::uintptr_t, int) -> rt::FieldRef {
        return rt::kInvalidFieldRef;
    }
};

static_assert(GpioSchemaImpl<UnknownGpioSchema>, "UnknownGpioSchema must satisfy GpioSchemaImpl");

}  // namespace alloy::hal::gpio::detail
