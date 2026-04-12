#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

class SystemClock {
   public:
    template <typename ClockImpl>
    [[nodiscard]] static auto use_default() -> Result<void, ErrorCode> {
        return ClockImpl::initialize();
    }

    template <typename ClockImpl>
    [[nodiscard]] static auto use_safe_default() -> Result<void, ErrorCode> {
        return ClockImpl::initialize(ClockImpl::CLOCK_CONFIG_SAFE_DEFAULT);
    }

    template <typename ClockImpl>
    [[nodiscard]] static auto use_high_performance() -> Result<void, ErrorCode> {
        return ClockImpl::initialize(ClockImpl::CLOCK_CONFIG_HIGH_PERFORMANCE);
    }

    template <typename ClockImpl>
    [[nodiscard]] static auto use_medium_performance() -> Result<void, ErrorCode> {
        return ClockImpl::initialize(ClockImpl::CLOCK_CONFIG_MEDIUM_PERFORMANCE);
    }

    template <typename ClockImpl, typename ConfigType>
    [[nodiscard]] static auto use_custom(const ConfigType& config) -> Result<void, ErrorCode> {
        return ClockImpl::initialize(config);
    }

    template <typename ClockImpl>
    [[nodiscard]] static auto enable_peripheral(u8 peripheral_id) -> Result<void, ErrorCode> {
        return ClockImpl::enablePeripheralClock(peripheral_id);
    }

    template <typename ClockImpl>
    [[nodiscard]] static auto disable_peripheral(u8 peripheral_id) -> Result<void, ErrorCode> {
        return ClockImpl::disablePeripheralClock(peripheral_id);
    }

    template <typename ClockImpl>
    [[nodiscard]] static auto enable_peripherals(const u8* peripheral_ids,
                                                 u8 count) -> Result<void, ErrorCode> {
        for (u8 index = 0; index < count; ++index) {
            if (auto result = ClockImpl::enablePeripheralClock(peripheral_ids[index]);
                !result.is_ok()) {
                return result;
            }
        }
        return Ok();
    }

    template <typename ClockImpl>
    [[nodiscard]] static auto get_frequency_hz() -> u32 {
        return ClockImpl::getMasterClockFrequency();
    }
};

}  // namespace alloy::hal
