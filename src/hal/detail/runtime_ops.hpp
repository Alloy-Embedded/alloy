#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/traits.hpp"
#include "device/runtime.hpp"
#include "hal/detail/resolved_route.hpp"

namespace alloy::hal::detail::runtime {

#if defined(ALLOY_ENABLE_HOST_MMIO_RUNTIME_HOOKS)
namespace test_support {

struct MmioHooks {
    auto (*read32)(std::uintptr_t address) -> std::uint32_t = nullptr;
    void (*write32)(std::uintptr_t address, std::uint32_t value) = nullptr;
};

[[nodiscard]] inline auto host_mmio_hooks() -> MmioHooks& {
    static auto hooks = MmioHooks{};
    return hooks;
}

}  // namespace test_support
#endif

using RegisterRef = device::runtime::RuntimeRegisterRef;
using FieldRef = device::runtime::RuntimeFieldRef;
using IndexedFieldRef = device::runtime::RuntimeIndexedFieldRef;

inline constexpr auto kInvalidRegisterRef = device::runtime::invalid_register_ref;
inline constexpr auto kInvalidFieldRef = device::runtime::invalid_field_ref;
inline constexpr auto kInvalidIndexedFieldRef = device::runtime::invalid_indexed_field_ref;

[[nodiscard]] constexpr auto ascii_lower(char ch) -> char {
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}

[[nodiscard]] constexpr auto ascii_iequals(std::string_view lhs, std::string_view rhs) -> bool {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (ascii_lower(lhs[index]) != ascii_lower(rhs[index])) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] constexpr auto trim_case_insensitive_prefix(std::string_view text,
                                                          std::string_view prefix)
    -> std::string_view {
    if (text.size() < prefix.size()) {
        return text;
    }

    if (!ascii_iequals(text.substr(0u, prefix.size()), prefix)) {
        return text;
    }

    text.remove_prefix(prefix.size());
    if (!text.empty() && text.front() == '_') {
        text.remove_prefix(1u);
    }
    return text;
}

template <auto Value>
[[nodiscard]] consteval auto enum_name() -> std::string_view {
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view function = __PRETTY_FUNCTION__;
    constexpr std::string_view needle = "Value = ";
#elif defined(_MSC_VER)
    constexpr std::string_view function = __FUNCSIG__;
    constexpr std::string_view needle = "enum_name<";
#else
#error Unsupported compiler for compile-time enum name extraction.
#endif

    const auto start = function.find(needle);
    static_assert(start != std::string_view::npos,
                  "Unable to parse compiler function signature for enum name extraction.");

    auto value_start = start + needle.size();
#if defined(_MSC_VER)
    const auto value_end = function.find(">(void)", value_start);
#else
    auto value_end = function.find(';', value_start);
    const auto bracket_end = function.find(']', value_start);
    if (value_end == std::string_view::npos ||
        (bracket_end != std::string_view::npos && bracket_end < value_end)) {
        value_end = bracket_end;
    }
#endif

    auto value = function.substr(value_start, value_end - value_start);
    if (const auto scope = value.rfind("::"); scope != std::string_view::npos) {
        value.remove_prefix(scope + 2u);
    }
    return value;
}

[[nodiscard]] constexpr auto selected_device_name() -> std::string_view {
    return device::SelectedDeviceTraits::name;
}

template <auto Value>
[[nodiscard]] consteval auto selected_scoped_enum_name() -> std::string_view {
    auto name = enum_name<Value>();
    name = trim_case_insensitive_prefix(name, selected_device_name());
    return name;
}

[[nodiscard]] inline auto mmio32(std::uintptr_t address) -> volatile std::uint32_t& {
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

[[nodiscard]] inline auto read_mmio32(std::uintptr_t address) -> std::uint32_t {
#if defined(ALLOY_ENABLE_HOST_MMIO_RUNTIME_HOOKS)
    const auto& hooks = test_support::host_mmio_hooks();
    if (hooks.read32 != nullptr) {
        return hooks.read32(address);
    }
#endif
    return static_cast<std::uint32_t>(mmio32(address));
}

inline void write_mmio32(std::uintptr_t address, std::uint32_t value) {
#if defined(ALLOY_ENABLE_HOST_MMIO_RUNTIME_HOOKS)
    const auto& hooks = test_support::host_mmio_hooks();
    if (hooks.write32 != nullptr) {
        hooks.write32(address, value);
        return;
    }
#endif
    mmio32(address) = value;
}

template <device::runtime::RegisterId Id>
[[nodiscard]] consteval auto register_ref() -> RegisterRef {
    using traits = device::runtime::RegisterTraits<Id>;
    if constexpr (!traits::kPresent) {
        return kInvalidRegisterRef;
    } else {
        return RegisterRef{Id, traits::kBaseAddress, traits::kOffsetBytes, true};
    }
}

template <device::runtime::FieldId Id>
[[nodiscard]] consteval auto field_ref() -> FieldRef {
    using traits = device::runtime::RegisterFieldTraits<Id>;
    if constexpr (!traits::kPresent) {
        return kInvalidFieldRef;
    } else {
        return FieldRef{Id, register_ref<traits::kRegisterId>(), traits::kBitOffset,
                        traits::kBitWidth, true};
    }
}

enum class GpioSchema : std::uint8_t {
    unknown,
    st_gpio,
    microchip_pio_v,
    nxp_imxrt_gpio_v1,
    nxp_gpio,
};

enum class UartSchema : std::uint8_t {
    unknown,
    st_sci3_v2_1_cube,
    st_sci2_v1_2_cube,
    microchip_uart_r,
    microchip_usart_zw,
    nxp_lpuart_v1,
};

enum class I2cSchema : std::uint8_t {
    unknown,
    st_i2c2_v1_1_cube,
    st_i2c1_v1_5_cube,
    microchip_twihs_z,
};

enum class SpiSchema : std::uint8_t {
    unknown,
    st_spi2s1_v3_3_cube,
    st_spi2s1_v2_2_cube,
    microchip_spi_zm,
};

template <typename SchemaId>
[[nodiscard]] constexpr auto to_gpio_schema(SchemaId schema_id) -> GpioSchema {
    if constexpr (requires { SchemaId::schema_alloy_gpio_st_gpio; }) {
        if (schema_id == SchemaId::schema_alloy_gpio_st_gpio) {
            return GpioSchema::st_gpio;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_gpio_microchip_pio_v; }) {
        if (schema_id == SchemaId::schema_alloy_gpio_microchip_pio_v) {
            return GpioSchema::microchip_pio_v;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_gpio_nxp_imxrt_gpio_v1; }) {
        if (schema_id == SchemaId::schema_alloy_gpio_nxp_imxrt_gpio_v1) {
            return GpioSchema::nxp_imxrt_gpio_v1;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_gpio_nxp_gpio; }) {
        if (schema_id == SchemaId::schema_alloy_gpio_nxp_gpio) {
            return GpioSchema::nxp_gpio;
        }
    }
    return GpioSchema::unknown;
}

template <typename SchemaId>
[[nodiscard]] constexpr auto to_uart_schema(SchemaId schema_id) -> UartSchema {
    if constexpr (requires { SchemaId::schema_alloy_uart_st_sci3_v2_1_cube; }) {
        if (schema_id == SchemaId::schema_alloy_uart_st_sci3_v2_1_cube) {
            return UartSchema::st_sci3_v2_1_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_uart_st_sci3_v1_4_cube; }) {
        if (schema_id == SchemaId::schema_alloy_uart_st_sci3_v1_4_cube) {
            return UartSchema::st_sci3_v2_1_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_uart_st_sci3_v1_2_cube; }) {
        if (schema_id == SchemaId::schema_alloy_uart_st_sci3_v1_2_cube) {
            return UartSchema::st_sci3_v2_1_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_uart_st_sci2_v1_2_cube; }) {
        if (schema_id == SchemaId::schema_alloy_uart_st_sci2_v1_2_cube) {
            return UartSchema::st_sci2_v1_2_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_uart_st_sci2_v1_2_f4_cube; }) {
        if (schema_id == SchemaId::schema_alloy_uart_st_sci2_v1_2_f4_cube) {
            return UartSchema::st_sci2_v1_2_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_uart_st_uart; }) {
        if (schema_id == SchemaId::schema_alloy_uart_st_uart) {
            return UartSchema::st_sci2_v1_2_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_uart_microchip_uart_r; }) {
        if (schema_id == SchemaId::schema_alloy_uart_microchip_uart_r) {
            return UartSchema::microchip_uart_r;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_uart_microchip_usart_zw; }) {
        if (schema_id == SchemaId::schema_alloy_uart_microchip_usart_zw) {
            return UartSchema::microchip_usart_zw;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_uart_nxp_lpuart_v1; }) {
        if (schema_id == SchemaId::schema_alloy_uart_nxp_lpuart_v1) {
            return UartSchema::nxp_lpuart_v1;
        }
    }
    return UartSchema::unknown;
}

template <typename SchemaId>
[[nodiscard]] constexpr auto to_i2c_schema(SchemaId schema_id) -> I2cSchema {
    if constexpr (requires { SchemaId::schema_alloy_i2c1_st_i2c2_v1_1_cube; }) {
        if (schema_id == SchemaId::schema_alloy_i2c1_st_i2c2_v1_1_cube) {
            return I2cSchema::st_i2c2_v1_1_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_i2c2_st_i2c2_v1_1_cube; }) {
        if (schema_id == SchemaId::schema_alloy_i2c2_st_i2c2_v1_1_cube) {
            return I2cSchema::st_i2c2_v1_1_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_i2c3_st_i2c2_v1_1_cube; }) {
        if (schema_id == SchemaId::schema_alloy_i2c3_st_i2c2_v1_1_cube) {
            return I2cSchema::st_i2c2_v1_1_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_i2c1_st_i2c1_v1_5_cube; }) {
        if (schema_id == SchemaId::schema_alloy_i2c1_st_i2c1_v1_5_cube) {
            return I2cSchema::st_i2c1_v1_5_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_i2c2_st_i2c1_v1_5_cube; }) {
        if (schema_id == SchemaId::schema_alloy_i2c2_st_i2c1_v1_5_cube) {
            return I2cSchema::st_i2c1_v1_5_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_i2c3_st_i2c1_v1_5_cube; }) {
        if (schema_id == SchemaId::schema_alloy_i2c3_st_i2c1_v1_5_cube) {
            return I2cSchema::st_i2c1_v1_5_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_i2c_microchip_twihs_z; }) {
        if (schema_id == SchemaId::schema_alloy_i2c_microchip_twihs_z) {
            return I2cSchema::microchip_twihs_z;
        }
    }
    return I2cSchema::unknown;
}

template <typename SchemaId>
[[nodiscard]] constexpr auto to_spi_schema(SchemaId schema_id) -> SpiSchema {
    if constexpr (requires { SchemaId::schema_alloy_spi_st_spi2s1_v3_3_cube; }) {
        if (schema_id == SchemaId::schema_alloy_spi_st_spi2s1_v3_3_cube) {
            return SpiSchema::st_spi2s1_v3_3_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_spi_st_spi2s1_v3_5_cube; }) {
        if (schema_id == SchemaId::schema_alloy_spi_st_spi2s1_v3_5_cube) {
            return SpiSchema::st_spi2s1_v3_3_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_spi_st_spi2s1_v2_2_cube; }) {
        if (schema_id == SchemaId::schema_alloy_spi_st_spi2s1_v2_2_cube) {
            return SpiSchema::st_spi2s1_v2_2_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_spi_st_spi; }) {
        if (schema_id == SchemaId::schema_alloy_spi_st_spi) {
            return SpiSchema::st_spi2s1_v2_2_cube;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_spi_microchip_spi_zm; }) {
        if (schema_id == SchemaId::schema_alloy_spi_microchip_spi_zm) {
            return SpiSchema::microchip_spi_zm;
        }
    }
    return SpiSchema::unknown;
}

[[nodiscard]] constexpr auto trim_prefix(std::string_view text, std::string_view prefix)
    -> std::string_view {
    if (text.starts_with(prefix)) {
        text.remove_prefix(prefix.size());
    }
    return text;
}

[[nodiscard]] constexpr auto ends_with_case_insensitive(std::string_view text,
                                                        std::string_view suffix) -> bool {
    if (suffix.size() > text.size()) {
        return false;
    }
    return ascii_iequals(text.substr(text.size() - suffix.size()), suffix);
}

template <typename OperationKind>
[[nodiscard]] constexpr auto is_operation_kind_set_bit(OperationKind kind) -> bool {
    if constexpr (requires { OperationKind::operation_kind_set_bit; }) {
        return kind == OperationKind::operation_kind_set_bit;
    } else {
        return false;
    }
}

struct RuntimePinLocation {
    device::runtime::PortId port_id = device::runtime::PortId::none;
    int pin_number = -1;
    bool valid = false;
};

template <std::size_t... Index>
[[nodiscard]] constexpr auto find_runtime_pin_location_impl(
    device::runtime::PinId pin_id, std::index_sequence<Index...>) -> RuntimePinLocation {
    auto resolved = RuntimePinLocation{};

    auto match = [&]<std::size_t I>() constexpr {
        constexpr auto candidate = device::runtime::runtime_pin_ids[I];
        using traits = device::runtime::PinTraits<candidate>;
        if (resolved.valid || candidate != pin_id || !traits::kPresent) {
            return;
        }

        resolved = {
            .port_id = traits::kPortId,
            .pin_number = traits::kPinNumber,
            .valid = true,
        };
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

[[nodiscard]] constexpr auto find_runtime_pin_location(device::runtime::PinId pin_id)
    -> RuntimePinLocation {
    return find_runtime_pin_location_impl(
        pin_id, std::make_index_sequence<std::size(device::runtime::runtime_pin_ids)>{});
}

template <std::size_t... Index>
[[nodiscard]] constexpr auto find_runtime_peripheral_base_address_impl(
    device::runtime::PeripheralId peripheral_id, std::index_sequence<Index...>) -> std::uintptr_t {
    auto resolved = std::uintptr_t{0};

    auto match = [&]<std::size_t I>() constexpr {
        constexpr auto candidate = device::runtime::runtime_peripheral_ids[I];
        using traits = device::runtime::PeripheralInstanceTraits<candidate>;
        if (resolved != 0u || candidate != peripheral_id || !traits::kPresent) {
            return;
        }
        resolved = traits::kBaseAddress;
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

[[nodiscard]] constexpr auto runtime_peripheral_base_address(
    device::runtime::PeripheralId peripheral_id) -> std::uintptr_t {
    return find_runtime_peripheral_base_address_impl(
        peripheral_id,
        std::make_index_sequence<std::size(device::runtime::runtime_peripheral_ids)>{});
}

[[nodiscard]] constexpr auto port_instance(device::runtime::PortId port_id) -> int {
    if (port_id == device::runtime::PortId::none) {
        return -1;
    }
    return static_cast<int>(port_id) - 1;
}

template <std::size_t... Index>
[[nodiscard]] constexpr auto find_gpio_peripheral_id_impl(
    device::runtime::PortId port_id,
    std::index_sequence<Index...>) -> device::runtime::PeripheralId {
    const auto target_instance = port_instance(port_id);
    auto resolved = device::runtime::PeripheralId::none;

    auto match = [&]<std::size_t I>() constexpr {
        constexpr auto peripheral_id = device::runtime::runtime_peripheral_ids[I];
        using traits = device::runtime::PeripheralInstanceTraits<peripheral_id>;
        if constexpr (traits::kPresent &&
                      traits::kPeripheralClassId == device::runtime::PeripheralClassId::class_gpio) {
            if (resolved == device::runtime::PeripheralId::none &&
                traits::kInstance == target_instance) {
                resolved = peripheral_id;
            }
        }
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

[[nodiscard]] constexpr auto find_gpio_peripheral_id(device::runtime::PortId port_id)
    -> device::runtime::PeripheralId {
    return find_gpio_peripheral_id_impl(
        port_id, std::make_index_sequence<std::size(device::runtime::runtime_peripheral_ids)>{});
}

template <std::size_t Begin, std::size_t End>
[[nodiscard]] constexpr auto find_runtime_register_ref_impl(
    device::runtime::RegisterId register_id) -> RegisterRef {
    if constexpr (Begin >= End) {
        return kInvalidRegisterRef;
    } else if constexpr ((End - Begin) == 1u) {
        constexpr auto candidate = device::runtime::runtime_register_ids[Begin];
        using traits = device::runtime::RegisterTraits<candidate>;
        if constexpr (!traits::kPresent) {
            return kInvalidRegisterRef;
        } else if (candidate == register_id) {
            return {
                .register_id = candidate,
                .base_address = traits::kBaseAddress,
                .offset_bytes = traits::kOffsetBytes,
                .valid = true,
            };
        } else {
            return kInvalidRegisterRef;
        }
    } else {
        constexpr auto mid = Begin + ((End - Begin) / 2u);
        const auto left = find_runtime_register_ref_impl<Begin, mid>(register_id);
        if (left.valid) {
            return left;
        }
        return find_runtime_register_ref_impl<mid, End>(register_id);
    }
}

[[nodiscard]] constexpr auto find_runtime_register_ref(device::runtime::RegisterId register_id)
    -> RegisterRef {
    return find_runtime_register_ref_impl<0u, std::size(device::runtime::runtime_register_ids)>(
        register_id);
}

template <std::size_t Begin, std::size_t End>
[[nodiscard]] constexpr auto find_runtime_register_ref_by_suffix_impl(
    device::runtime::PeripheralId peripheral_id, std::string_view suffix) -> RegisterRef {
    const auto base_address = runtime_peripheral_base_address(peripheral_id);
    if constexpr (Begin >= End) {
        return kInvalidRegisterRef;
    } else if constexpr ((End - Begin) == 1u) {
        constexpr auto candidate = device::runtime::runtime_register_ids[Begin];
        using traits = device::runtime::RegisterTraits<candidate>;
        if constexpr (!traits::kPresent) {
            return kInvalidRegisterRef;
        }
        auto name = selected_scoped_enum_name<candidate>();
        name = trim_prefix(name, "register_");
        if (traits::kBaseAddress == base_address && ends_with_case_insensitive(name, suffix)) {
            return {
                .register_id = candidate,
                .base_address = traits::kBaseAddress,
                .offset_bytes = traits::kOffsetBytes,
                .valid = true,
            };
        }
        return kInvalidRegisterRef;
    } else {
        constexpr auto mid = Begin + ((End - Begin) / 2u);
        const auto left = find_runtime_register_ref_by_suffix_impl<Begin, mid>(peripheral_id, suffix);
        if (left.valid) {
            return left;
        }
        return find_runtime_register_ref_by_suffix_impl<mid, End>(peripheral_id, suffix);
    }
}

[[nodiscard]] constexpr auto find_runtime_register_ref_by_suffix(
    device::runtime::PeripheralId peripheral_id, std::string_view suffix) -> RegisterRef {
    return find_runtime_register_ref_by_suffix_impl<0u,
                                                    std::size(device::runtime::runtime_register_ids)>(
        peripheral_id, suffix);
}

template <std::size_t Begin, std::size_t End>
[[nodiscard]] constexpr auto find_runtime_field_ref_by_register_and_offset_impl(
    device::runtime::RegisterId register_id, std::uint16_t bit_offset) -> FieldRef {
    if constexpr (Begin >= End) {
        return kInvalidFieldRef;
    } else if constexpr ((End - Begin) == 1u) {
        constexpr auto candidate = device::runtime::runtime_field_ids[Begin];
        using traits = device::runtime::RegisterFieldTraits<candidate>;
        if constexpr (!traits::kPresent) {
            return kInvalidFieldRef;
        }
        const auto reg = find_runtime_register_ref(traits::kRegisterId);
        if (reg.valid && traits::kRegisterId == register_id && traits::kBitOffset == bit_offset) {
            return {
                .field_id = candidate,
                .reg = reg,
                .bit_offset = traits::kBitOffset,
                .bit_width = traits::kBitWidth,
                .valid = true,
            };
        }
        return kInvalidFieldRef;
    } else {
        constexpr auto mid = Begin + ((End - Begin) / 2u);
        const auto left =
            find_runtime_field_ref_by_register_and_offset_impl<Begin, mid>(register_id, bit_offset);
        if (left.valid) {
            return left;
        }
        return find_runtime_field_ref_by_register_and_offset_impl<mid, End>(register_id, bit_offset);
    }
}

[[nodiscard]] constexpr auto find_runtime_field_ref_by_register_and_offset(
    device::runtime::RegisterId register_id, std::uint16_t bit_offset) -> FieldRef {
    return find_runtime_field_ref_by_register_and_offset_impl<
        0u, std::size(device::runtime::runtime_field_ids)>(register_id, bit_offset);
}

template <std::size_t Begin, std::size_t End>
[[nodiscard]] constexpr auto find_runtime_field_ref_impl(device::runtime::FieldId field_id)
    -> FieldRef {
    if constexpr (Begin >= End) {
        return kInvalidFieldRef;
    } else if constexpr ((End - Begin) == 1u) {
        constexpr auto candidate = device::runtime::runtime_field_ids[Begin];
        using traits = device::runtime::RegisterFieldTraits<candidate>;
        if constexpr (!traits::kPresent) {
            return kInvalidFieldRef;
        }
        const auto reg = find_runtime_register_ref(traits::kRegisterId);
        if (reg.valid && candidate == field_id) {
            return {
                .field_id = candidate,
                .reg = reg,
                .bit_offset = traits::kBitOffset,
                .bit_width = traits::kBitWidth,
                .valid = true,
            };
        }
        return kInvalidFieldRef;
    } else {
        constexpr auto mid = Begin + ((End - Begin) / 2u);
        const auto left = find_runtime_field_ref_impl<Begin, mid>(field_id);
        if (left.valid) {
            return left;
        }
        return find_runtime_field_ref_impl<mid, End>(field_id);
    }
}

[[nodiscard]] constexpr auto find_runtime_field_ref(device::runtime::FieldId field_id)
    -> FieldRef {
    return find_runtime_field_ref_impl<0u, std::size(device::runtime::runtime_field_ids)>(field_id);
}

template <typename OperationKind>
[[nodiscard]] constexpr auto is_operation_kind_clear_bit(OperationKind kind) -> bool {
    if constexpr (requires { OperationKind::operation_kind_clear_bit; }) {
        return kind == OperationKind::operation_kind_clear_bit;
    } else {
        return false;
    }
}

[[nodiscard]] constexpr auto field_mask(const FieldRef& field) -> std::uint32_t {
    if (!field.valid || field.bit_width == 0u) {
        return 0u;
    }
    if (field.bit_width >= 32u) {
        return 0xFFFF'FFFFu;
    }
    return ((1u << field.bit_width) - 1u) << field.bit_offset;
}

[[nodiscard]] constexpr auto indexed_field_mask(const IndexedFieldRef& field) -> std::uint32_t {
    if (!field.valid || field.bit_width == 0u) {
        return 0u;
    }
    if (field.bit_width >= 32u) {
        return 0xFFFF'FFFFu;
    }
    return ((1u << field.bit_width) - 1u) << field.bit_offset;
}

[[nodiscard]] constexpr auto indexed_field_address(const IndexedFieldRef& field,
                                                   std::size_t index) -> std::uintptr_t {
    return field.base_address + field.base_offset_bytes +
           (field.stride_bytes * static_cast<std::uintptr_t>(index));
}

inline auto write_register(const RegisterRef& reg, std::uint32_t value)
    -> core::Result<void, core::ErrorCode> {
    if (!reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    write_mmio32(reg.base_address + reg.offset_bytes, value);
    return core::Ok();
}

[[nodiscard]] inline auto read_register(const RegisterRef& reg)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (!reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    const auto value = read_mmio32(reg.base_address + reg.offset_bytes);
    return core::Ok<std::uint32_t>(std::uint32_t{value});
}

[[nodiscard]] inline auto field_bits(const FieldRef& field, std::uint32_t value)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return core::Ok((value << field.bit_offset) & field_mask(field));
}

[[nodiscard]] inline auto indexed_field_bits(const IndexedFieldRef& field, std::uint32_t value)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return core::Ok((value << field.bit_offset) & indexed_field_mask(field));
}

inline auto modify_field(const FieldRef& field, std::uint32_t value)
    -> core::Result<void, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto current = read_mmio32(field.reg.base_address + field.reg.offset_bytes);
    current &= ~field_mask(field);
    current |= (value << field.bit_offset) & field_mask(field);
    write_mmio32(field.reg.base_address + field.reg.offset_bytes, current);
    return core::Ok();
}

inline auto modify_indexed_field(const IndexedFieldRef& field, std::size_t index,
                                 std::uint32_t value)
    -> core::Result<void, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto current = read_mmio32(indexed_field_address(field, index));
    current &= ~indexed_field_mask(field);
    current |= (value << field.bit_offset) & indexed_field_mask(field);
    write_mmio32(indexed_field_address(field, index), current);
    return core::Ok();
}

[[nodiscard]] inline auto read_field(const FieldRef& field)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto value = read_mmio32(field.reg.base_address + field.reg.offset_bytes);
    return core::Ok((value & field_mask(field)) >> field.bit_offset);
}

inline auto modify_register_bit(const RegisterRef& reg, std::uint32_t bit_index, bool value)
    -> core::Result<void, core::ErrorCode> {
    if (!reg.valid || bit_index >= 32u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto current = read_mmio32(reg.base_address + reg.offset_bytes);
    const auto mask = static_cast<std::uint32_t>(1u << bit_index);
    current = value ? (current | mask) : (current & ~mask);
    write_mmio32(reg.base_address + reg.offset_bytes, current);
    return core::Ok();
}

template <typename Handle>
[[nodiscard]] consteval auto gpio_schema_for() -> GpioSchema {
    if constexpr (requires { Handle::schema; }) {
        return Handle::schema;
    } else if constexpr (requires { Handle::peripheral_traits::kSchemaId; }) {
        return to_gpio_schema(Handle::peripheral_traits::kSchemaId);
    } else {
        return GpioSchema::unknown;
    }
}

template <typename Handle>
[[nodiscard]] consteval auto uart_schema_for() -> UartSchema {
    if constexpr (requires { Handle::schema; }) {
        return Handle::schema;
    } else if constexpr (requires { Handle::peripheral_traits::kSchemaId; }) {
        return to_uart_schema(Handle::peripheral_traits::kSchemaId);
    } else {
        return UartSchema::unknown;
    }
}

template <typename Handle>
[[nodiscard]] consteval auto i2c_schema_for() -> I2cSchema {
    if constexpr (requires { Handle::schema; }) {
        return Handle::schema;
    } else if constexpr (requires { Handle::peripheral_traits::kSchemaId; }) {
        return to_i2c_schema(Handle::peripheral_traits::kSchemaId);
    } else {
        return I2cSchema::unknown;
    }
}

template <typename Handle>
[[nodiscard]] consteval auto spi_schema_for() -> SpiSchema {
    if constexpr (requires { Handle::schema; }) {
        return Handle::schema;
    } else if constexpr (requires { Handle::peripheral_traits::kSchemaId; }) {
        return to_spi_schema(Handle::peripheral_traits::kSchemaId);
    } else {
        return SpiSchema::unknown;
    }
}

template <device::runtime::PeripheralId PeripheralId, device::runtime::ClockGateId ClockGateId>
inline auto apply_clock_gate_typed() -> core::Result<void, core::ErrorCode> {
    using gate_traits = device::runtime::ClockGateTraits<ClockGateId>;
    using peripheral_traits = device::runtime::PeripheralInstanceTraits<PeripheralId>;

    if constexpr (!gate_traits::kPresent) {
        return core::Err(core::ErrorCode::NotSupported);
    } else if constexpr (gate_traits::kFieldId != device::runtime::FieldId::none) {
        return modify_field(field_ref<gate_traits::kFieldId>(), 1u);
    } else if constexpr (gate_traits::kRegisterId != device::runtime::RegisterId::none &&
                         peripheral_traits::kPeripheralClassId ==
                             device::runtime::PeripheralClassId::class_gpio &&
                         peripheral_traits::kInstance >= 0) {
        return modify_register_bit(register_ref<gate_traits::kRegisterId>(),
                                   static_cast<std::uint32_t>(peripheral_traits::kInstance), true);
    } else {
        return core::Err(core::ErrorCode::NotSupported);
    }
}

template <typename ActiveLevelId>
[[nodiscard]] consteval auto is_active_level_high(ActiveLevelId active_level_id) -> bool {
    if constexpr (requires { ActiveLevelId::active_level_high; }) {
        return active_level_id == ActiveLevelId::active_level_high;
    } else {
        return false;
    }
}

template <device::runtime::PeripheralId PeripheralId, device::runtime::ResetId ResetId>
inline auto release_reset_typed() -> core::Result<void, core::ErrorCode> {
    using reset_traits = device::runtime::ResetTraits<ResetId>;
    using peripheral_traits = device::runtime::PeripheralInstanceTraits<PeripheralId>;
    constexpr auto active_high = is_active_level_high(reset_traits::kActiveLevelId);

    if constexpr (!reset_traits::kPresent) {
        return core::Ok();
    } else if constexpr (reset_traits::kFieldId != device::runtime::FieldId::none) {
        constexpr auto inactive_value = active_high ? 0u : 1u;
        return modify_field(field_ref<reset_traits::kFieldId>(), inactive_value);
    } else if constexpr (reset_traits::kRegisterId != device::runtime::RegisterId::none &&
                         peripheral_traits::kPeripheralClassId ==
                             device::runtime::PeripheralClassId::class_gpio &&
                         peripheral_traits::kInstance >= 0) {
        return modify_register_bit(register_ref<reset_traits::kRegisterId>(),
                                   static_cast<std::uint32_t>(peripheral_traits::kInstance),
                                   !active_high);
    } else {
        return core::Ok();
    }
}

template <device::runtime::PeripheralId PeripheralId>
inline auto enable_peripheral_runtime_typed() -> core::Result<void, core::ErrorCode> {
    using peripheral_traits = device::runtime::PeripheralInstanceTraits<PeripheralId>;
    using binding_traits = device::runtime::PeripheralClockBindingTraits<PeripheralId>;

    if constexpr (!peripheral_traits::kPresent || !binding_traits::kPresent) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        if constexpr (binding_traits::kClockGateId != device::runtime::ClockGateId::none) {
            const auto gate_result =
                apply_clock_gate_typed<PeripheralId, binding_traits::kClockGateId>();
            if (gate_result.is_err()) {
                return gate_result;
            }
        }

        if constexpr (binding_traits::kResetId != device::runtime::ResetId::none) {
            const auto reset_result =
                release_reset_typed<PeripheralId, binding_traits::kResetId>();
            if (reset_result.is_err()) {
                return reset_result;
            }
        }

        return core::Ok();
    }
}

template <device::runtime::PeripheralId PeripheralId>
inline auto enable_gpio_port_runtime() -> core::Result<void, core::ErrorCode> {
    using peripheral_traits = device::runtime::PeripheralInstanceTraits<PeripheralId>;

    if constexpr (!peripheral_traits::kPresent ||
                  peripheral_traits::kPeripheralClassId !=
                      device::runtime::PeripheralClassId::class_gpio) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        return enable_peripheral_runtime_typed<PeripheralId>();
    }
}

template <std::size_t... Index>
inline auto enable_gpio_port_runtime_impl(
    device::runtime::PeripheralId peripheral_id,
    std::index_sequence<Index...>) -> core::Result<void, core::ErrorCode> {
    auto resolved = core::Result<void, core::ErrorCode>{
        core::Err(core::ErrorCode::NotSupported)};

    auto match = [&]<device::runtime::PeripheralId Candidate>() {
        using traits = device::runtime::PeripheralInstanceTraits<Candidate>;
        if constexpr (traits::kPresent &&
                      traits::kPeripheralClassId == device::runtime::PeripheralClassId::class_gpio) {
            if (resolved.is_err() && peripheral_id == Candidate) {
                resolved = enable_gpio_port_runtime<Candidate>();
            }
        }
    };

    (match.template operator()<device::runtime::runtime_peripheral_ids[Index]>(), ...);
    return resolved;
}

inline auto enable_gpio_port_runtime(device::runtime::PeripheralId peripheral_id)
    -> core::Result<void, core::ErrorCode> {
    if (peripheral_id == device::runtime::PeripheralId::none) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    return enable_gpio_port_runtime_impl(
        peripheral_id,
        std::make_index_sequence<std::size(device::runtime::runtime_peripheral_ids)>{});
}

inline auto apply_route_operation(const ::alloy::hal::detail::route::Operation& operation)
    -> core::Result<void, core::ErrorCode> {
    using kind = ::alloy::hal::detail::route::OperationKind;

    switch (operation.kind) {
        case kind::set_field:
            if (!operation.primary.valid) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            return modify_field(operation.primary, 1u);
        case kind::clear_field:
            if (!operation.primary.valid) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            return modify_field(operation.primary, 0u);
        case kind::configure_stm32_af: {
            const auto enable_result = enable_gpio_port_runtime(operation.peripheral_id);
            if (enable_result.is_err()) {
                return enable_result;
            }
            if (!operation.primary.valid || !operation.secondary.valid) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            const auto mode_result = modify_field(operation.primary, 0x2u);
            if (mode_result.is_err()) {
                return mode_result;
            }
            return modify_field(operation.secondary, operation.value);
        }
        case kind::configure_same70_pio: {
            const auto enable_result = enable_gpio_port_runtime(operation.peripheral_id);
            if (enable_result.is_err()) {
                return enable_result;
            }
            if (!operation.primary.valid || !operation.secondary.valid) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }

            const auto pdr_bits = field_bits(operation.primary, 1u);
            if (pdr_bits.is_err()) {
                return core::Err(core::ErrorCode{pdr_bits.unwrap_err()});
            }
            const auto pdr_result = write_register(operation.primary.reg, pdr_bits.unwrap());
            if (pdr_result.is_err()) {
                return pdr_result;
            }

            const auto mask = field_mask(operation.secondary);
            const auto abcdsr0_address =
                operation.secondary.reg.base_address + operation.secondary.reg.offset_bytes;
            const auto abcdsr1_address = abcdsr0_address + sizeof(std::uint32_t);
            auto abcdsr0 = read_mmio32(abcdsr0_address);
            auto abcdsr1 = read_mmio32(abcdsr1_address);
            abcdsr0 = (operation.value & 0x1u) != 0u ? (abcdsr0 | mask) : (abcdsr0 & ~mask);
            abcdsr1 = (operation.value & 0x2u) != 0u ? (abcdsr1 | mask) : (abcdsr1 & ~mask);
            write_mmio32(abcdsr0_address, abcdsr0);
            write_mmio32(abcdsr1_address, abcdsr1);
            return core::Ok();
        }
        case kind::apply_clock_gate_by_index:
        case kind::release_reset_by_index:
        case kind::invalid:
        default:
            return core::Err(core::ErrorCode::InvalidParameter);
    }
}

inline auto apply_route_operation(const device::runtime::RouteOperation& descriptor)
    -> core::Result<void, core::ErrorCode> {
    if (is_operation_kind_set_bit(descriptor.kind_id) ||
        is_operation_kind_clear_bit(descriptor.kind_id)) {
        if (descriptor.field_id == device::runtime::FieldId::none) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        const auto field = find_runtime_field_ref(descriptor.field_id);
        if (!field.valid) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        return modify_field(field, is_operation_kind_set_bit(descriptor.kind_id) ? 1u : 0u);
    }

    if (descriptor.kind_id == device::runtime::OperationKindId::operation_kind_write_selector) {
        if (descriptor.pin_id == device::runtime::PinId::none || descriptor.value_int < 0) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        const auto pin = find_runtime_pin_location(descriptor.pin_id);
        if (!pin.valid) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        const auto gpio_peripheral_id = find_gpio_peripheral_id(pin.port_id);
        if (gpio_peripheral_id == device::runtime::PeripheralId::none) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        const auto enable_result = enable_gpio_port_runtime(gpio_peripheral_id);
        if (enable_result.is_err()) {
            return enable_result;
        }

        const auto line_index = pin.pin_number;
        if (line_index < 0) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        if (const auto mode_register = find_runtime_register_ref_by_suffix(gpio_peripheral_id, "moder");
            mode_register.valid) {
            const auto afr_register = find_runtime_register_ref_by_suffix(
                gpio_peripheral_id, line_index < 8 ? "afrl" : "afrh");
            if (!afr_register.valid) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }

            const auto mode_field = find_runtime_field_ref_by_register_and_offset(
                mode_register.register_id, static_cast<std::uint16_t>(line_index * 2));
            const auto afr_field = find_runtime_field_ref_by_register_and_offset(
                afr_register.register_id, static_cast<std::uint16_t>((line_index % 8) * 4));
            if (!mode_field.valid || !afr_field.valid) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }

            const auto mode_result = modify_field(mode_field, 0x2u);
            if (mode_result.is_err()) {
                return mode_result;
            }
            return modify_field(afr_field, static_cast<std::uint32_t>(descriptor.value_int));
        }

        const auto pdr_register = find_runtime_register_ref_by_suffix(gpio_peripheral_id, "pdr");
        const auto abcdsr_register =
            find_runtime_register_ref_by_suffix(gpio_peripheral_id, "abcdsr__s");
        if (!pdr_register.valid || !abcdsr_register.valid) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        const auto pdr_field = find_runtime_field_ref_by_register_and_offset(
            pdr_register.register_id, static_cast<std::uint16_t>(line_index));
        const auto abcdsr_field = find_runtime_field_ref_by_register_and_offset(
            abcdsr_register.register_id, static_cast<std::uint16_t>(line_index));
        if (!pdr_field.valid || !abcdsr_field.valid) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        const auto pdr_bits = field_bits(pdr_field, 1u);
        if (pdr_bits.is_err()) {
            return core::Err(core::ErrorCode{pdr_bits.unwrap_err()});
        }
        const auto pdr_result = write_register(pdr_field.reg, pdr_bits.unwrap());
        if (pdr_result.is_err()) {
            return pdr_result;
        }

        const auto mask = field_mask(abcdsr_field);
        const auto abcdsr0_address =
            abcdsr_field.reg.base_address + abcdsr_field.reg.offset_bytes;
        const auto abcdsr1_address = abcdsr0_address + sizeof(std::uint32_t);
        auto abcdsr0 = read_mmio32(abcdsr0_address);
        auto abcdsr1 = read_mmio32(abcdsr1_address);
        const auto value = static_cast<std::uint32_t>(descriptor.value_int);
        abcdsr0 = (value & 0x1u) != 0u ? (abcdsr0 | mask) : (abcdsr0 & ~mask);
        abcdsr1 = (value & 0x2u) != 0u ? (abcdsr1 | mask) : (abcdsr1 & ~mask);
        write_mmio32(abcdsr0_address, abcdsr0);
        write_mmio32(abcdsr1_address, abcdsr1);
        return core::Ok();
    }

    return core::Err(core::ErrorCode::InvalidParameter);
}

template <typename OperationList>
inline auto apply_route_operations(const OperationList& operations)
    -> core::Result<void, core::ErrorCode> {
    for (std::size_t index = 0; index < operations.size(); ++index) {
        const auto& entry = operations[index];
        const auto result = apply_route_operation(entry);
        if (result.is_err()) {
            return result;
        }
    }
    return core::Ok();
}

}  // namespace alloy::hal::detail::runtime
