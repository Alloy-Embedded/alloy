#pragma once

// Async I2C runtime adapter — interrupt-driven completion (not DMA).
//
// I2C on the foundational chips raises an event interrupt
// (BTF / TC on STM32, TXCOMP / RXRDY on SAME70 TWIHS) at end of
// transfer; that ISR signals the corresponding `i2c_event::token`. DMA
// is intentionally NOT used here because some chips (e.g. STM32G0
// variants) don't wire I2C through a DMA channel — interrupt-driven is
// the portable lowest common denominator.
//
// Three operations:
//
//   async::i2c::write<Peripheral>(port, address, tx_buffer)
//      -> Result<operation<i2c_event::token<Peripheral>>, ErrorCode>
//
//   async::i2c::read<Peripheral>(port, address, rx_buffer)
//      -> Result<operation<i2c_event::token<Peripheral>>, ErrorCode>
//
//   async::i2c::write_read<Peripheral>(port, address, tx, rx)
//      -> Result<operation<i2c_event::token<Peripheral>>, ErrorCode>
//
// NACK and bus-error are reported as `core::ErrorCode::Nack` /
// `core::ErrorCode::BusError` from the await result, not from the
// transfer-start return.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "runtime/async.hpp"
#include "runtime/i2c_event.hpp"

namespace alloy::runtime::async::i2c {

template <device::PeripheralId Peripheral, typename PortHandle>
[[nodiscard]] auto write(const PortHandle& port, std::uint16_t address,
                         std::span<const std::uint8_t> tx)
    -> core::Result<operation<alloy::runtime::i2c_event::token<Peripheral>>,
                    core::ErrorCode> {
    using operation_type = operation<alloy::runtime::i2c_event::token<Peripheral>>;
    const auto start_result = port.write_async(address, tx);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

template <device::PeripheralId Peripheral, typename PortHandle>
[[nodiscard]] auto read(const PortHandle& port, std::uint16_t address,
                        std::span<std::uint8_t> rx)
    -> core::Result<operation<alloy::runtime::i2c_event::token<Peripheral>>,
                    core::ErrorCode> {
    using operation_type = operation<alloy::runtime::i2c_event::token<Peripheral>>;
    const auto start_result = port.read_async(address, rx);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

template <device::PeripheralId Peripheral, typename PortHandle>
[[nodiscard]] auto write_read(const PortHandle& port, std::uint16_t address,
                              std::span<const std::uint8_t> tx,
                              std::span<std::uint8_t> rx)
    -> core::Result<operation<alloy::runtime::i2c_event::token<Peripheral>>,
                    core::ErrorCode> {
    using operation_type = operation<alloy::runtime::i2c_event::token<Peripheral>>;
    const auto start_result = port.write_read_async(address, tx, rx);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

}  // namespace alloy::runtime::async::i2c
