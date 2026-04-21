#pragma once

#include <cstddef>
#include <cstdint>

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::dac {

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
using PeripheralId = device::runtime::PeripheralId;

struct Config {
    bool enable_on_configure = true;
    bool write_initial_value = false;
    std::uint32_t initial_value = 0u;
};

template <PeripheralId Peripheral, std::size_t Channel>
class handle {
  public:
    using peripheral_traits = device::runtime::DacSemanticTraits<Peripheral>;
    using channel_traits = device::runtime::DacChannelSemanticTraits<Peripheral, Channel>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr auto channel_index = Channel;
    static constexpr bool valid = peripheral_traits::kPresent && channel_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.enable_on_configure) {
            result = enable();
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.write_initial_value) {
            result = write(config_.initial_value);
        }
        return result;
    }

    [[nodiscard]] auto enable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (channel_traits::kEnableField.valid) {
            return detail::runtime::modify_field(channel_traits::kEnableField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto disable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (channel_traits::kDisableField.valid) {
            return detail::runtime::modify_field(channel_traits::kDisableField, 1u);
        }
        if constexpr (channel_traits::kEnableField.valid) {
            return detail::runtime::modify_field(channel_traits::kEnableField, 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto ready() const -> bool {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (channel_traits::kReadyField.valid) {
            const auto state = detail::runtime::read_field(channel_traits::kReadyField);
            return state.is_ok() && state.unwrap() != 0u;
        }
        return true;
    }

    [[nodiscard]] auto write(std::uint32_t value) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (channel_traits::kDataField.valid) {
            return detail::runtime::modify_field(channel_traits::kDataField, value);
        }
        if constexpr (peripheral_traits::kDataRegister.valid) {
            return detail::runtime::write_register(peripheral_traits::kDataRegister, value);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    template <typename DmaChannel>
    [[nodiscard]] auto configure_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        return channel.configure();
    }

    [[nodiscard]] static constexpr auto data_register_address() -> std::uintptr_t {
        if constexpr (channel_traits::kDataField.valid) {
            return channel_traits::kDataField.reg.base_address +
                   channel_traits::kDataField.reg.offset_bytes;
        } else if constexpr (peripheral_traits::kDataRegister.valid) {
            return peripheral_traits::kDataRegister.base_address +
                   peripheral_traits::kDataRegister.offset_bytes;
        } else {
            return 0u;
        }
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral, std::size_t Channel>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral, Channel> {
    static_assert(handle<Peripheral, Channel>::valid,
                  "Requested DAC channel is not published for the selected device.");
    return handle<Peripheral, Channel>{config};
}
#endif

}  // namespace alloy::hal::dac
