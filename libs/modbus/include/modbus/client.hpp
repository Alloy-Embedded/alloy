// Blocking Modbus RTU client (master) over any alloy::ByteStream uart.
//
// One transaction at a time, poll-driven, no heap, no ISR: send the request,
// pump the uart into the framer, hand the CRC-valid response to the PDU
// parsers, return a Result. The clock is INJECTED (house rule: inject the
// clock, never read it) — uptime_clock binds alloy::uptime_us() for boards,
// tests crank a virtual clock through timeout windows deterministically.
//
// RS-485 direction control: De is any alloy::OutputPin driven high for the
// whole TX burst; boards without a transceiver keep the default
// alloy::gpio::null_output and the calls fold to nothing. The deassert edge
// waits on uart.flush() WHERE THE DRIVER OFFERS IT (capability-gated, house
// pattern) — flush semantics per IP version are audited in the RS-485 bench
// phase, and the README's support claim says so.
//
// Correlation is enforced, not assumed: a response from the wrong unit, the
// wrong function answering, an echo that does not match what was written —
// each is its own error. Modbus RTU has no transaction id, so the OTHER half
// of correlation is hygiene: the uart RX is drained before every request, so
// a late reply to a timed-out transaction can never be parsed as this one's.
//
// MaxRegisters fixes every buffer at compile time: the largest request
// (9 + 2N bytes) lives on the stack for the call, the response framer holds
// 5 + 2N. N = 8 costs a 21-byte framer — not the reference library's fixed
// 260-byte engine plus 264-byte PDU copies.

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"
#include "alloy/gpio.hpp"
#include "alloy/util/result.hpp"
#include "modbus/clock.hpp"
#include "modbus/crc.hpp"
#include "modbus/error.hpp"
#include "modbus/pdu.hpp"
#include "modbus/rtu_framer.hpp"
#include "modbus/rtu_timing.hpp"

namespace alloy::lib::modbus {

// Namespace-scope (not nested) so one braced config serves every client
// instantiation — a nested type would make configs for clients that differ
// only in their De pin mutually unassignable.
struct client_config {
    std::uint32_t baud = 19'200;
    std::chrono::microseconds response_timeout{500'000};
};

template <alloy::ByteStream Uart, std::size_t MaxRegisters = 16,
          class Clock = uptime_clock, alloy::OutputPin De = alloy::gpio::null_output>
class rtu_client {
    static_assert(MaxRegisters >= 1 && MaxRegisters <= max_read_registers,
                  "MaxRegisters is the per-transaction register budget: "
                  "1..125 (the spec ceiling for one read)");

public:
    using config = client_config;

    constexpr rtu_client(Uart& uart, config c, Clock clock = {}, De de = {})
        : uart_(uart),
          framer_(direction::response, rtu_times_for(c.baud)),
          timeout_us_(static_cast<std::uint32_t>(c.response_timeout.count())),
          clock_(clock),
          de_(de) {}
    // A temporary uart would dangle behind the stored reference.
    rtu_client(Uart&&, config, Clock = {}, De = {}) = delete;

    // ── reads ───────────────────────────────────────────────────────────────
    // Register values land host-order in `out`; out.size() IS the count asked
    // for, and a response carrying any other count is a correlation failure.

    [[nodiscard]] Result<std::size_t, modbus_error> read_holding(
        std::uint8_t unit, std::uint16_t address, std::span<std::uint16_t> out) {
        return read_registers(function::read_holding_registers, unit, address, out);
    }
    [[nodiscard]] Result<std::size_t, modbus_error> read_input(
        std::uint8_t unit, std::uint16_t address, std::span<std::uint16_t> out) {
        return read_registers(function::read_input_registers, unit, address, out);
    }

    // Packed bits (LSB-first, as the wire carries them) land in `packed_out`;
    // returns the byte count. The bit budget is 16×MaxRegisters — same bytes.
    [[nodiscard]] Result<std::size_t, modbus_error> read_coils(
        std::uint8_t unit, std::uint16_t address, std::uint16_t count,
        std::span<std::uint8_t> packed_out) {
        return read_bits(function::read_coils, unit, address, count, packed_out);
    }
    [[nodiscard]] Result<std::size_t, modbus_error> read_discrete_inputs(
        std::uint8_t unit, std::uint16_t address, std::uint16_t count,
        std::span<std::uint8_t> packed_out) {
        return read_bits(function::read_discrete_inputs, unit, address, count,
                         packed_out);
    }

    // ── writes ──────────────────────────────────────────────────────────────
    // unit 0 is the spec's broadcast: every server executes, none responds —
    // the call returns right after TX and the caller owns the pacing.

    [[nodiscard]] Result<void, modbus_error> write_register(std::uint8_t unit,
                                                            std::uint16_t address,
                                                            std::uint16_t value) {
        return write_single(function::write_single_register, unit, address, value);
    }
    [[nodiscard]] Result<void, modbus_error> write_coil(std::uint8_t unit,
                                                        std::uint16_t address,
                                                        bool on) {
        return write_single(function::write_single_coil, unit, address,
                            on ? 0xFF00u : 0x0000u);
    }

    [[nodiscard]] Result<void, modbus_error> write_registers(
        std::uint8_t unit, std::uint16_t address, std::span<const std::uint16_t> values) {
        if (values.size() > MaxRegisters) {
            return modbus_error::too_large;
        }
        std::uint8_t pdu[6u + 2u * MaxRegisters];
        const auto n = build_write_registers_request(address, values, pdu);
        if (!n) {
            return n.error();
        }
        return write_transaction(unit, {pdu, *n}, function::write_multiple_registers,
                                 address, static_cast<std::uint16_t>(values.size()));
    }

    [[nodiscard]] Result<void, modbus_error> write_coils(
        std::uint8_t unit, std::uint16_t address, std::uint16_t count,
        std::span<const std::uint8_t> packed) {
        if (packed.size() > 2u * MaxRegisters) {
            return modbus_error::too_large;
        }
        std::uint8_t pdu[6u + 2u * MaxRegisters];
        const auto n = build_write_coils_request(address, count, packed, pdu);
        if (!n) {
            return n.error();
        }
        return write_transaction(unit, {pdu, *n}, function::write_multiple_coils,
                                 address, count);
    }

private:
    static constexpr std::size_t rx_cap =
        (5u + 2u * MaxRegisters) < 8u ? 8u : 5u + 2u * MaxRegisters;
    static constexpr std::size_t tx_cap = 9u + 2u * MaxRegisters;

    [[nodiscard]] Result<std::size_t, modbus_error> read_registers(
        function fc, std::uint8_t unit, std::uint16_t address,
        std::span<std::uint16_t> out) {
        if (unit == 0u) {
            return modbus_error::malformed;  // a broadcast cannot answer
        }
        if (out.size() > MaxRegisters) {
            return modbus_error::too_large;
        }
        std::uint8_t pdu[5];
        const auto n = build_read_request(fc, address,
                                          static_cast<std::uint16_t>(out.size()), pdu);
        if (!n) {
            return n.error();
        }
        send_adu(unit, {pdu, *n});
        const auto f = await_response(unit);
        if (!f) {
            return f.error();
        }
        const auto got = parse_read_registers_response(f->subspan(1), fc, out);
        framer_.consume();
        if (got && *got != out.size()) {
            return modbus_error::malformed;  // right shape, wrong count
        }
        return got;
    }

    [[nodiscard]] Result<std::size_t, modbus_error> read_bits(
        function fc, std::uint8_t unit, std::uint16_t address, std::uint16_t count,
        std::span<std::uint8_t> packed_out) {
        if (unit == 0u) {
            return modbus_error::malformed;
        }
        const std::size_t need = (static_cast<std::size_t>(count) + 7u) / 8u;
        if (need > 2u * MaxRegisters) {
            return modbus_error::too_large;
        }
        std::uint8_t pdu[5];
        const auto n = build_read_request(fc, address, count, pdu);
        if (!n) {
            return n.error();
        }
        send_adu(unit, {pdu, *n});
        const auto f = await_response(unit);
        if (!f) {
            return f.error();
        }
        const auto got = parse_read_bits_response(f->subspan(1), fc, packed_out);
        framer_.consume();
        if (got && *got != need) {
            return modbus_error::malformed;
        }
        return got;
    }

    [[nodiscard]] Result<void, modbus_error> write_single(function fc,
                                                          std::uint8_t unit,
                                                          std::uint16_t address,
                                                          std::uint16_t value) {
        std::uint8_t pdu[5];
        const auto n = build_write_single(fc, address, value, pdu);
        if (!n) {
            return n.error();
        }
        return write_transaction(unit, {pdu, *n}, fc, address, value);
    }

    // Shared tail of every write: send, then either broadcast-return or
    // verify the echo — FC05/06 echo the value, FC0F/10 echo the count.
    [[nodiscard]] Result<void, modbus_error> write_transaction(
        std::uint8_t unit, std::span<const std::uint8_t> pdu, function fc,
        std::uint16_t address, std::uint16_t value_or_count) {
        send_adu(unit, pdu);
        if (unit == 0u) {
            return ok<modbus_error>();
        }
        const auto f = await_response(unit);
        if (!f) {
            return f.error();
        }
        const auto r =
            parse_write_response(f->subspan(1), fc, address, value_or_count);
        framer_.consume();
        return r;
    }

    void send_adu(std::uint8_t unit, std::span<const std::uint8_t> pdu) {
        // Hygiene half of correlation: kill any late reply still in the RX
        // path before this request exists, so it cannot answer it.
        std::uint8_t stale;
        while (uart_.read(stale)) {
        }
        framer_.reset();

        std::uint8_t adu[tx_cap];
        adu[0] = unit;
        for (std::size_t i = 0; i < pdu.size(); ++i) {
            adu[1u + i] = pdu[i];
        }
        const std::size_t len = append_crc(adu, 1u + pdu.size());

        de_.set_high();
        for (std::size_t i = 0; i < len; ++i) {
            uart_.write(adu[i]);
        }
        // TX-complete before dropping the transceiver enable — only where the
        // driver can say so. A flush() that returns on TDR-empty instead of
        // TC truncates the CRC on the wire; see the README's RS-485 claim.
        if constexpr (requires { uart_.flush(); }) {
            uart_.flush();
        }
        de_.set_low();
    }

    // Pump bytes until one CRC-valid frame lands or the deadline passes.
    // Returns the ADU [unit | pdu...]; the caller parses, then consume()s.
    [[nodiscard]] Result<std::span<const std::uint8_t>, modbus_error> await_response(
        std::uint8_t unit) {
        const std::uint32_t t0 = clock_.now_us();
        for (;;) {
            std::uint8_t b;
            while (uart_.read(b)) {
                framer_.feed(b, clock_.now_us());
                if (framer_.has_frame()) {
                    break;
                }
            }
            if (framer_.has_frame()) {
                const auto f = framer_.frame();
                if (f[0] != unit) {
                    framer_.consume();
                    return modbus_error::unexpected_unit;
                }
                return f;
            }
            framer_.tick(clock_.now_us());
            if (clock_.now_us() - t0 >= timeout_us_) {
                return modbus_error::timeout;
            }
        }
    }

    Uart& uart_;
    rtu_framer<rx_cap> framer_;
    std::uint32_t timeout_us_;
    Clock clock_;
    De de_;
};

}  // namespace alloy::lib::modbus
