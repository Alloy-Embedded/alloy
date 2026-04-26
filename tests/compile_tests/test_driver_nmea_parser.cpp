// Compile test: nmea_parser seed driver instantiates against the documented
// public UART HAL surface. Exercises read_line(), parse_gga(), and parse_rmc()
// so that any drift in the bus handle's signature fails the build.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/net/nmea_parser/nmea_parser.hpp"

namespace {

struct MockUartBus {
    /// transmit is part of the UART contract even though the parser never
    /// sends bytes; drivers must compile against the full surface.
    [[nodiscard]] auto transmit(std::span<const std::uint8_t> /*tx*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    /// receive: fill every byte with 0 (simulates a stream of NUL bytes which
    /// will cause read_line to complete on the first '\n' == 0x0A — this is a
    /// compile-only check, not a correctness test).
    [[nodiscard]] auto receive(std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_nmea_parser_against_public_uart_handle() {
    MockUartBus bus;
    alloy::drivers::net::nmea_parser::Parser<MockUartBus> parser{bus};

    // read_line() must compile and return a Result<void, ErrorCode>.
    auto line_result = parser.read_line();
    (void)line_result.is_ok();

    // last_line() must compile and return a span<const char>.
    auto raw = parser.last_line();
    (void)raw.size();

    // parse_gga() must compile and return a Result<GgaFix, ErrorCode>.
    auto gga = parser.parse_gga();
    (void)gga.is_ok();

    // parse_rmc() must compile and return a Result<RmcFix, ErrorCode>.
    auto rmc = parser.parse_rmc();
    (void)rmc.is_ok();
}

}  // namespace
