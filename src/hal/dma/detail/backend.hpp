#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/dma/types.hpp"

namespace alloy::hal::dma::detail {

namespace rt = alloy::hal::detail::runtime;

enum class DmaSchema : std::uint8_t {
    unknown,
    st_dma,
    microchip_xdmac_k,
};

template <typename SchemaId>
[[nodiscard]] constexpr auto to_dma_schema(SchemaId schema_id) -> DmaSchema {
    if constexpr (requires { SchemaId::schema_alloy_dma_st_dma; }) {
        if (schema_id == SchemaId::schema_alloy_dma_st_dma) {
            return DmaSchema::st_dma;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_dma_microchip_xdmac_k; }) {
        if (schema_id == SchemaId::schema_alloy_dma_microchip_xdmac_k) {
            return DmaSchema::microchip_xdmac_k;
        }
    }
    return DmaSchema::unknown;
}

[[nodiscard]] constexpr auto priority_bits(Priority priority) -> std::uint32_t {
    switch (priority) {
        case Priority::low:
            return 0u;
        case Priority::medium:
            return 1u;
        case Priority::high:
            return 2u;
        case Priority::very_high:
            return 3u;
    }
    return 0u;
}

[[nodiscard]] constexpr auto data_width_bits(DataWidth data_width) -> std::uint32_t {
    switch (data_width) {
        case DataWidth::bits8:
            return 0u;
        case DataWidth::bits16:
            return 1u;
        case DataWidth::bits32:
            return 2u;
    }
    return 0u;
}

[[nodiscard]] constexpr auto data_width_bytes(DataWidth data_width) -> std::size_t {
    return static_cast<std::size_t>(data_width);
}

template <typename ChannelHandle>
[[nodiscard]] auto resolve_channel_index(const ChannelHandle& handle)
    -> core::Result<std::size_t, core::ErrorCode> {
    if constexpr (ChannelHandle::fixed_channel_index >= 0) {
        const auto requested = handle.config().channel_index;
        if (requested >= 0 && requested != ChannelHandle::fixed_channel_index) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        return core::Ok<std::size_t>(
            static_cast<std::size_t>(ChannelHandle::fixed_channel_index));
    } else {
        if (handle.config().channel_index < 0) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        return core::Ok<std::size_t>(static_cast<std::size_t>(handle.config().channel_index));
    }
}

template <typename ChannelHandle>
inline auto enable_runtime_dependencies() -> core::Result<void, core::ErrorCode> {
    if constexpr (ChannelHandle::controller_peripheral_id != device::runtime::PeripheralId::none) {
        const auto controller_result =
            rt::enable_peripheral_runtime_typed<ChannelHandle::controller_peripheral_id>();
        if (controller_result.is_err()) {
            return controller_result;
        }
    }

    if constexpr (ChannelHandle::router_peripheral_id != device::runtime::PeripheralId::none) {
        const auto router_result =
            rt::enable_peripheral_runtime_typed<ChannelHandle::router_peripheral_id>();
        if (router_result.is_err()) {
            return router_result;
        }
    }

    // Enable NVIC interrupt for this DMA channel/stream so the ISR fires on completion.
    // kNvicIrq >= 0 only for embedded device specializations; base template uses -1.
    if constexpr (ChannelHandle::semantic_traits::kNvicIrq >= 0) {
        constexpr auto irqn = static_cast<std::uint32_t>(ChannelHandle::semantic_traits::kNvicIrq);
        // ARM Cortex-M NVIC ISER: base 0xE000E100, 32 IRQs per register
        volatile auto* nvic_iser = reinterpret_cast<volatile std::uint32_t*>(0xE000E100u);
        nvic_iser[irqn / 32u] |= (1u << (irqn % 32u));
    }

    return core::Ok();
}

template <typename PeripheralEnum>
[[nodiscard]] consteval auto st_dmamux_base_address() -> std::uintptr_t {
    if constexpr (requires { PeripheralEnum::DMAMUX1; }) {
        using traits = device::runtime::PeripheralInstanceTraits<PeripheralEnum::DMAMUX1>;
        if constexpr (traits::kPresent) {
            return traits::kBaseAddress;
        }
    }
    return 0u;
}

inline constexpr auto kStDmamuxBase = st_dmamux_base_address<device::runtime::PeripheralId>();

template <typename ChannelHandle>
inline auto configure_st_dmamux_route(std::size_t channel_index)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (ChannelHandle::request_value < 0 || ChannelHandle::channel_selector >= 0) {
        return core::Ok();
    } else {
        if constexpr (kStDmamuxBase == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            constexpr auto request_id_field = rt::IndexedFieldRef{
                .base_address = kStDmamuxBase,
                .base_offset_bytes = 0u,
                .stride_bytes = sizeof(std::uint32_t),
                .bit_offset = 0u,
                .bit_width = 7u,
                .valid = true,
            };
            return rt::modify_indexed_field(request_id_field, channel_index,
                                            static_cast<std::uint32_t>(ChannelHandle::request_value));
        }
    }
}

[[nodiscard]] constexpr auto st_channel_cr_value(const Config& config) -> std::uint32_t {
    auto value = std::uint32_t{0u};
    value |= static_cast<std::uint32_t>(priority_bits(config.priority) << 12u);
    value |= static_cast<std::uint32_t>(data_width_bits(config.data_width) << 8u);
    value |= static_cast<std::uint32_t>(data_width_bits(config.data_width) << 10u);
    value |= (1u << 7u);  // MINC

    switch (config.direction) {
        case Direction::memory_to_peripheral:
            value |= (1u << 4u);
            break;
        case Direction::peripheral_to_memory:
            break;
        case Direction::memory_to_memory:
            value |= (1u << 14u);
            break;
    }

    if (config.mode == Mode::circular) {
        value |= (1u << 5u);
    }

    return value;
}

[[nodiscard]] constexpr auto st_stream_cr_value(const Config& config, int channel_selector)
    -> std::uint32_t {
    auto value = std::uint32_t{0u};
    value |= static_cast<std::uint32_t>(priority_bits(config.priority) << 16u);
    value |= static_cast<std::uint32_t>(data_width_bits(config.data_width) << 11u);
    value |= static_cast<std::uint32_t>(data_width_bits(config.data_width) << 13u);
    value |= (1u << 10u);  // MINC
    value |= static_cast<std::uint32_t>(channel_selector & 0x7) << 25u;

    switch (config.direction) {
        case Direction::peripheral_to_memory:
            value |= (0u << 6u);
            break;
        case Direction::memory_to_peripheral:
            value |= (1u << 6u);
            break;
        case Direction::memory_to_memory:
            value |= (2u << 6u);
            break;
    }

    if (config.mode == Mode::circular) {
        value |= (1u << 8u);
    }

    return value;
}

[[nodiscard]] constexpr auto xdmac_cc_value(const Config& config, std::uint32_t request_value)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (config.mode == Mode::circular || config.direction == Direction::memory_to_memory) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto value = std::uint32_t{0u};
    value |= (1u << 0u);  // TYPE = PER_TRAN
    value |= (0u << 1u);  // MBSIZE = SINGLE
    value |= ((config.direction == Direction::memory_to_peripheral ? 1u : 0u) << 4u);  // DSYNC
    value |= (0u << 6u);  // SWREQ = hardware
    value |= (0u << 7u);  // MEMSET = normal
    value |= (0u << 8u);  // CSIZE = 1
    value |= static_cast<std::uint32_t>(data_width_bits(config.data_width) << 11u);
    value |= (0u << 13u);  // SIF = AHB_IF0
    value |= (0u << 14u);  // DIF = AHB_IF0

    if (config.direction == Direction::memory_to_peripheral) {
        value |= (1u << 16u);  // SAM incremented
        value |= (0u << 18u);  // DAM fixed
    } else {
        value |= (0u << 16u);  // SAM fixed
        value |= (1u << 18u);  // DAM incremented
    }

    value |= ((request_value & 0x7Fu) << 24u);
    return core::Ok<std::uint32_t>(std::uint32_t{value});
}

template <typename ChannelHandle>
inline auto configure_st_dma_channel(const ChannelHandle& handle, std::size_t channel_index)
    -> core::Result<void, core::ErrorCode> {
    const auto controller_base = ChannelHandle::controller_base_address;
    const auto channel_offset = 0x08u + (static_cast<std::uint32_t>(channel_index) * 0x14u);
    const auto cr = rt::RegisterRef{
        .register_id = device::runtime::RegisterId::none,
        .base_address = controller_base,
        .offset_bytes = channel_offset,
        .valid = true,
    };

    if (const auto disable_result = rt::write_register(cr, 0u); disable_result.is_err()) {
        return disable_result;
    }

    if (const auto route_result = configure_st_dmamux_route<ChannelHandle>(channel_index);
        route_result.is_err()) {
        return route_result;
    }

    return rt::write_register(cr, st_channel_cr_value(handle.config()));
}

template <typename ChannelHandle>
inline auto configure_st_dma_stream(const ChannelHandle& handle, std::size_t channel_index)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (ChannelHandle::channel_selector < 0) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        const auto controller_base = ChannelHandle::controller_base_address;
        const auto stream_offset = 0x10u + (static_cast<std::uint32_t>(channel_index) * 0x18u);
        const auto cr = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = stream_offset,
            .valid = true,
        };
        const auto fcr = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = stream_offset + 0x14u,
            .valid = true,
        };

        if (const auto disable_result = rt::write_register(cr, 0u); disable_result.is_err()) {
            return disable_result;
        }
        if (const auto fifo_result = rt::write_register(fcr, 0x21u); fifo_result.is_err()) {
            return fifo_result;
        }

        return rt::write_register(
            cr, st_stream_cr_value(handle.config(), ChannelHandle::channel_selector));
    }
}

template <typename ChannelHandle>
inline auto configure_microchip_xdmac(const ChannelHandle& handle, std::size_t channel_index)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (ChannelHandle::request_value < 0) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        const auto controller_base = ChannelHandle::controller_base_address;
        const auto disable_reg = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = 0x20u,
            .valid = true,
        };
        const auto cc_reg = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = 0x78u + (static_cast<std::uint32_t>(channel_index) * 0x40u),
            .valid = true,
        };

        if (const auto disable_result =
                rt::write_register(disable_reg,
                                   static_cast<std::uint32_t>(1u << channel_index));
            disable_result.is_err()) {
            return disable_result;
        }

        const auto cc_value =
            xdmac_cc_value(handle.config(), static_cast<std::uint32_t>(ChannelHandle::request_value));
        if (cc_value.is_err()) {
            return core::Err(core::ErrorCode{cc_value.unwrap_err()});
        }
        return rt::write_register(cc_reg, cc_value.unwrap());
    }
}

template <typename ChannelHandle>
[[nodiscard]] inline auto resolve_transfer_count(const ChannelHandle& handle,
                                                 std::uintptr_t peripheral_address,
                                                 std::uintptr_t memory_address,
                                                 std::size_t size_bytes)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (size_bytes == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto unit_size = data_width_bytes(handle.config().data_width);
    if ((size_bytes % unit_size) != 0u || (peripheral_address % unit_size) != 0u ||
        (memory_address % unit_size) != 0u) {
        return core::Err(core::ErrorCode::DmaAlignmentError);
    }

    const auto transfer_count = size_bytes / unit_size;
    if (transfer_count > std::numeric_limits<std::uint16_t>::max()) {
        return core::Err(core::ErrorCode::DmaTransferError);
    }

    return core::Ok(static_cast<std::uint32_t>(transfer_count));
}

[[nodiscard]] constexpr auto st_dma_channel_clear_mask(std::size_t channel_index) -> std::uint32_t {
    return static_cast<std::uint32_t>(0x0Fu << (channel_index * 4u));
}

[[nodiscard]] constexpr auto st_dma_stream_clear_mask(std::size_t channel_index) -> std::uint32_t {
    constexpr auto kMasks = std::uint32_t{0x3Du};
    constexpr auto kShifts = std::array<std::uint32_t, 4u>{0u, 6u, 16u, 22u};
    return kMasks << kShifts[channel_index % 4u];
}

template <typename ChannelHandle>
inline auto start_st_dma_channel_transfer(const ChannelHandle& handle, std::size_t channel_index,
                                          std::uintptr_t peripheral_address,
                                          std::uintptr_t memory_address, std::size_t size_bytes)
    -> core::Result<void, core::ErrorCode> {
    const auto transfer_count =
        resolve_transfer_count(handle, peripheral_address, memory_address, size_bytes);
    if (transfer_count.is_err()) {
        return core::Err(core::ErrorCode{transfer_count.unwrap_err()});
    }

    const auto controller_base = ChannelHandle::controller_base_address;
    const auto channel_offset = 0x08u + (static_cast<std::uint32_t>(channel_index) * 0x14u);
    const auto cr = rt::RegisterRef{
        .register_id = device::runtime::RegisterId::none,
        .base_address = controller_base,
        .offset_bytes = channel_offset,
        .valid = true,
    };
    const auto cndtr = rt::RegisterRef{
        .register_id = device::runtime::RegisterId::none,
        .base_address = controller_base,
        .offset_bytes = channel_offset + 0x04u,
        .valid = true,
    };
    const auto cpar = rt::RegisterRef{
        .register_id = device::runtime::RegisterId::none,
        .base_address = controller_base,
        .offset_bytes = channel_offset + 0x08u,
        .valid = true,
    };
    const auto cmar = rt::RegisterRef{
        .register_id = device::runtime::RegisterId::none,
        .base_address = controller_base,
        .offset_bytes = channel_offset + 0x0Cu,
        .valid = true,
    };
    const auto ifcr = rt::RegisterRef{
        .register_id = device::runtime::RegisterId::none,
        .base_address = controller_base,
        .offset_bytes = 0x04u,
        .valid = true,
    };

    const auto current_cr = rt::read_register(cr);
    if (current_cr.is_err()) {
        return core::Err(core::ErrorCode{current_cr.unwrap_err()});
    }
    if ((current_cr.unwrap() & 0x1u) != 0u) {
        return core::Err(core::ErrorCode::DmaChannelBusy);
    }

    if (const auto disable_result = rt::write_register(cr, 0u); disable_result.is_err()) {
        return disable_result;
    }
    if (const auto route_result = configure_st_dmamux_route<ChannelHandle>(channel_index);
        route_result.is_err()) {
        return route_result;
    }
    if (const auto clear_result = rt::write_register(ifcr, st_dma_channel_clear_mask(channel_index));
        clear_result.is_err()) {
        return clear_result;
    }
    if (const auto peripheral_result =
            rt::write_register(cpar, static_cast<std::uint32_t>(peripheral_address));
        peripheral_result.is_err()) {
        return peripheral_result;
    }
    if (const auto memory_result =
            rt::write_register(cmar, static_cast<std::uint32_t>(memory_address));
        memory_result.is_err()) {
        return memory_result;
    }
    if (const auto count_result = rt::write_register(cndtr, transfer_count.unwrap());
        count_result.is_err()) {
        return count_result;
    }

    auto cr_value = st_channel_cr_value(handle.config());
    cr_value |= (1u << 1u);  // TCIE
    cr_value |= (1u << 0u);  // EN
    return rt::write_register(cr, cr_value);
}

template <typename ChannelHandle>
inline auto start_st_dma_stream_transfer(const ChannelHandle& handle, std::size_t channel_index,
                                         std::uintptr_t peripheral_address,
                                         std::uintptr_t memory_address, std::size_t size_bytes)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (ChannelHandle::channel_selector < 0) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        const auto transfer_count =
            resolve_transfer_count(handle, peripheral_address, memory_address, size_bytes);
        if (transfer_count.is_err()) {
            return core::Err(core::ErrorCode{transfer_count.unwrap_err()});
        }

        const auto controller_base = ChannelHandle::controller_base_address;
        const auto stream_offset = 0x10u + (static_cast<std::uint32_t>(channel_index) * 0x18u);
        const auto cr = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = stream_offset,
            .valid = true,
        };
        const auto ndtr = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = stream_offset + 0x04u,
            .valid = true,
        };
        const auto par = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = stream_offset + 0x08u,
            .valid = true,
        };
        const auto m0ar = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = stream_offset + 0x0Cu,
            .valid = true,
        };
        const auto fcr = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = stream_offset + 0x14u,
            .valid = true,
        };
        const auto ifcr = rt::RegisterRef{
            .register_id = device::runtime::RegisterId::none,
            .base_address = controller_base,
            .offset_bytes = channel_index < 4u ? 0x08u : 0x0Cu,
            .valid = true,
        };

        const auto current_cr = rt::read_register(cr);
        if (current_cr.is_err()) {
            return core::Err(core::ErrorCode{current_cr.unwrap_err()});
        }
        if ((current_cr.unwrap() & 0x1u) != 0u) {
            return core::Err(core::ErrorCode::DmaChannelBusy);
        }

        if (const auto disable_result = rt::write_register(cr, 0u); disable_result.is_err()) {
            return disable_result;
        }
        if (const auto fifo_result = rt::write_register(fcr, 0x21u); fifo_result.is_err()) {
            return fifo_result;
        }
        if (const auto clear_result = rt::write_register(ifcr, st_dma_stream_clear_mask(channel_index));
            clear_result.is_err()) {
            return clear_result;
        }
        if (const auto peripheral_result =
                rt::write_register(par, static_cast<std::uint32_t>(peripheral_address));
            peripheral_result.is_err()) {
            return peripheral_result;
        }
        if (const auto memory_result =
                rt::write_register(m0ar, static_cast<std::uint32_t>(memory_address));
            memory_result.is_err()) {
            return memory_result;
        }
        if (const auto count_result = rt::write_register(ndtr, transfer_count.unwrap());
            count_result.is_err()) {
            return count_result;
        }

        auto cr_value = st_stream_cr_value(handle.config(), ChannelHandle::channel_selector);
        cr_value |= (1u << 4u);  // TCIE
        cr_value |= (1u << 0u);  // EN
        return rt::write_register(cr, cr_value);
    }
}

template <typename ChannelHandle>
inline auto start_transfer(const ChannelHandle& handle, std::uintptr_t peripheral_address,
                           std::uintptr_t memory_address, std::size_t size_bytes)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!ChannelHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    } else {
        const auto dependency_result = enable_runtime_dependencies<ChannelHandle>();
        if (dependency_result.is_err()) {
            return dependency_result;
        }

        const auto resolved_channel_index = resolve_channel_index(handle);
        if (resolved_channel_index.is_err()) {
            return core::Err(core::ErrorCode{resolved_channel_index.unwrap_err()});
        }

        if constexpr (ChannelHandle::schema == DmaSchema::st_dma) {
            if constexpr (ChannelHandle::channel_selector >= 0) {
                return start_st_dma_stream_transfer(handle, resolved_channel_index.unwrap(),
                                                    peripheral_address, memory_address,
                                                    size_bytes);
            } else {
                return start_st_dma_channel_transfer(handle, resolved_channel_index.unwrap(),
                                                     peripheral_address, memory_address,
                                                     size_bytes);
            }
        }

        return core::Err(core::ErrorCode::NotSupported);
    }
}

template <typename ChannelHandle>
inline auto configure_dma(const ChannelHandle& handle) -> core::Result<void, core::ErrorCode> {
    if constexpr (!ChannelHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    } else {
        const auto dependency_result = enable_runtime_dependencies<ChannelHandle>();
        if (dependency_result.is_err()) {
            return dependency_result;
        }

        const auto resolved_channel_index = resolve_channel_index(handle);
        if (resolved_channel_index.is_err()) {
            return core::Err(core::ErrorCode{resolved_channel_index.unwrap_err()});
        }

        if constexpr (ChannelHandle::schema == DmaSchema::st_dma) {
            if constexpr (ChannelHandle::channel_selector >= 0) {
                return configure_st_dma_stream(handle, resolved_channel_index.unwrap());
            } else {
                return configure_st_dma_channel(handle, resolved_channel_index.unwrap());
            }
        }

        if constexpr (ChannelHandle::schema == DmaSchema::microchip_xdmac_k) {
            return configure_microchip_xdmac(handle, resolved_channel_index.unwrap());
        }

        return core::Err(core::ErrorCode::NotSupported);
    }
}

}  // namespace alloy::hal::dma::detail
