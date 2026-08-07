// Poll-driven Modbus RTU server (slave) over any alloy::ByteStream uart.
//
// poll() is bounded work: drain what the uart has, frame it, dispatch AT
// MOST one request against the DataModel, emit at most one response. It
// never blocks, never sleeps, never yields to an OS — drop it in a
// superloop or call it from a coroutine between co_awaits.
//
// Spec behaviors carried exactly (Modbus over Serial Line v1.02 §2.4):
//  - a request for another unit is NOT ours: total silence, not an error;
//  - unit 0 is broadcast: WRITES execute and nothing ever answers (an answer
//    would collide with every other server on the bus); broadcast READS are
//    meaningless and are dropped whole;
//  - a CRC-corrupt frame gets no reaction of any kind (the framer eats it);
//  - a parseable-but-refusable request earns the exception PDU whose code
//    says why: 0x01 unknown function, 0x02 address not in the model, 0x03
//    count/value out of range (including this server's own MaxRegisters
//    budget — the honest reply when the ask exceeds the device).
//
// Multi-write requests (FC0F/10) apply SEQUENTIALLY and stop at the first
// address the model refuses, answering 0x02; earlier items stay applied —
// the spec offers no transaction semantics and pretending otherwise would
// need a shadow buffer per space. Callers that need atomicity put it in
// their model.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"
#include "alloy/gpio.hpp"
#include "modbus/clock.hpp"
#include "modbus/crc.hpp"
#include "modbus/data_model.hpp"
#include "modbus/error.hpp"
#include "modbus/pdu.hpp"
#include "modbus/rtu_framer.hpp"
#include "modbus/rtu_timing.hpp"

namespace alloy::lib::modbus {

struct server_config {
    std::uint8_t unit = 1;      // 1..247; 0 is the broadcast id, never a server's
    std::uint32_t baud = 19'200;
};

template <alloy::ByteStream Uart, DataModel Model, std::size_t MaxRegisters = 16,
          class Clock = uptime_clock, alloy::OutputPin De = alloy::gpio::null_output>
class rtu_server {
    static_assert(MaxRegisters >= 1 && MaxRegisters <= max_read_registers,
                  "MaxRegisters is the per-request register budget: 1..125");

public:
    using config = server_config;

    constexpr rtu_server(Uart& uart, Model& model, config c, Clock clock = {},
                         De de = {})
        : uart_(uart),
          model_(model),
          framer_(direction::request, rtu_times_for(c.baud)),
          unit_(c.unit),
          clock_(clock),
          de_(de) {
        if (c.unit == 0u || c.unit > 247u) {
            __builtin_trap();  // claiming the broadcast id is a programmer error
        }
    }
    rtu_server(Uart&&, Model&, config, Clock = {}, De = {}) = delete;

    // One bounded step: call from the superloop. Returns true when a request
    // addressed to this server (or a broadcast) was dispatched — a cheap
    // activity signal for the caller, not an error channel.
    bool poll() {
        std::uint8_t b;
        while (uart_.read(b)) {
            framer_.feed(b, clock_.now_us());
            if (framer_.has_frame()) {
                break;
            }
        }
        framer_.tick(clock_.now_us());
        if (!framer_.has_frame()) {
            return false;
        }
        const auto frame = framer_.frame();
        const std::uint8_t unit = frame[0];
        if (unit != unit_ && unit != 0u) {
            framer_.consume();  // another server's conversation — stay silent
            return false;
        }
        dispatch(frame.subspan(1), /*broadcast=*/unit == 0u);
        framer_.consume();
        return true;
    }

private:
    // Response PDU worst case: fc + byte_count + 2*MaxRegisters of data.
    static constexpr std::size_t pdu_cap = 2u + 2u * MaxRegisters;

    void dispatch(std::span<const std::uint8_t> pdu, bool broadcast) {
        const auto req = parse_request(pdu);
        if (!req) {
            // Refusals still answer (that IS the protocol) — except for a
            // broadcast, where every refusal is silent by construction.
            if (!broadcast && !pdu.empty()) {
                const auto fc = static_cast<function>(pdu[0]);
                reply_exception(fc, is_exception(req.error())
                                        ? req.error()
                                        : modbus_error::exception_illegal_data_value);
            }
            return;
        }
        if (broadcast) {
            if (req->fc == function::write_single_coil ||
                req->fc == function::write_single_register ||
                req->fc == function::write_multiple_coils ||
                req->fc == function::write_multiple_registers) {
                (void)apply_write(*req);  // execute; NEVER answer, even a refusal
            }
            return;  // broadcast reads are dropped whole
        }
        switch (req->fc) {
        case function::read_coils:
        case function::read_discrete_inputs:
            reply_read_bits(*req);
            return;
        case function::read_holding_registers:
        case function::read_input_registers:
            reply_read_registers(*req);
            return;
        case function::write_single_coil:
        case function::write_single_register:
        case function::write_multiple_coils:
        case function::write_multiple_registers: {
            const auto e = apply_write(*req);
            if (e != modbus_error{}) {
                reply_exception(req->fc, e);
                return;
            }
            std::uint8_t out[5];
            const std::uint16_t echo =
                (req->fc == function::write_single_coil ||
                 req->fc == function::write_single_register)
                    ? req->value
                    : req->count;
            if (const auto n = build_write_response(req->fc, req->address, echo, out)) {
                send({out, *n});
            }
            return;
        }
        }
    }

    void reply_read_bits(const request_view& req) {
        std::uint8_t packed[2u * MaxRegisters]{};
        if (detail::bit_bytes(req.count) > sizeof packed) {
            reply_exception(req.fc, modbus_error::exception_illegal_data_value);
            return;
        }
        const bool discrete = req.fc == function::read_discrete_inputs;
        for (std::uint16_t i = 0; i < req.count; ++i) {
            bool bit = false;
            const auto addr = static_cast<std::uint16_t>(req.address + i);
            const bool ok = discrete ? model_.read_discrete_input(addr, bit)
                                     : model_.read_coil(addr, bit);
            if (!ok) {
                reply_exception(req.fc, modbus_error::exception_illegal_data_address);
                return;
            }
            if (bit) {
                packed[i / 8u] |= static_cast<std::uint8_t>(1u << (i % 8u));
            }
        }
        std::uint8_t out[pdu_cap];
        if (const auto n = build_read_bits_response(
                req.fc, {packed, detail::bit_bytes(req.count)}, out)) {
            send({out, *n});
        }
    }

    void reply_read_registers(const request_view& req) {
        std::uint16_t regs[MaxRegisters];
        if (req.count > MaxRegisters) {
            reply_exception(req.fc, modbus_error::exception_illegal_data_value);
            return;
        }
        const bool input = req.fc == function::read_input_registers;
        for (std::uint16_t i = 0; i < req.count; ++i) {
            const auto addr = static_cast<std::uint16_t>(req.address + i);
            const bool ok = input ? model_.read_input(addr, regs[i])
                                  : model_.read_holding(addr, regs[i]);
            if (!ok) {
                reply_exception(req.fc, modbus_error::exception_illegal_data_address);
                return;
            }
        }
        std::uint8_t out[pdu_cap];
        if (const auto n =
                build_read_registers_response(req.fc, {regs, req.count}, out)) {
            send({out, *n});
        }
    }

    // Applies any of the four write shapes. Default-constructed modbus_error
    // (value 0) is "no error" here — a private sentinel, never on the wire.
    [[nodiscard]] modbus_error apply_write(const request_view& req) {
        switch (req.fc) {
        case function::write_single_coil:
            return model_.write_coil(req.address, req.value == 0xFF00u)
                       ? modbus_error{}
                       : modbus_error::exception_illegal_data_address;
        case function::write_single_register:
            return model_.write_holding(req.address, req.value)
                       ? modbus_error{}
                       : modbus_error::exception_illegal_data_address;
        case function::write_multiple_coils:
            for (std::uint16_t i = 0; i < req.count; ++i) {
                const bool on =
                    (req.payload[i / 8u] >> (i % 8u) & 1u) != 0u;
                if (!model_.write_coil(static_cast<std::uint16_t>(req.address + i),
                                       on)) {
                    return modbus_error::exception_illegal_data_address;
                }
            }
            return modbus_error{};
        case function::write_multiple_registers:
            for (std::uint16_t i = 0; i < req.count; ++i) {
                const std::uint16_t v = static_cast<std::uint16_t>(
                    static_cast<std::uint16_t>(req.payload[2u * i]) << 8 |
                    req.payload[2u * i + 1u]);
                if (!model_.write_holding(
                        static_cast<std::uint16_t>(req.address + i), v)) {
                    return modbus_error::exception_illegal_data_address;
                }
            }
            return modbus_error{};
        default:
            return modbus_error::exception_illegal_function;  // unreachable
        }
    }

    void reply_exception(function fc, modbus_error e) {
        std::uint8_t out[2];
        if (const auto n = build_exception(fc, e, out)) {
            send({out, *n});
        }
    }

    void send(std::span<const std::uint8_t> pdu) {
        std::uint8_t adu[3u + pdu_cap];
        adu[0] = unit_;
        for (std::size_t i = 0; i < pdu.size(); ++i) {
            adu[1u + i] = pdu[i];
        }
        const std::size_t len = append_crc(adu, 1u + pdu.size());
        de_.set_high();
        for (std::size_t i = 0; i < len; ++i) {
            uart_.write(adu[i]);
        }
        if constexpr (requires { uart_.flush(); }) {
            uart_.flush();
        }
        de_.set_low();
    }

    Uart& uart_;
    Model& model_;
    rtu_framer<max_adu> framer_;
    std::uint8_t unit_;
    Clock clock_;
    De de_;
};

}  // namespace alloy::lib::modbus
