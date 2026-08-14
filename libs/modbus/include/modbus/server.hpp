// Poll-driven Modbus RTU server (slave) over any alloy::ByteStream uart.
//
// Two RX entries, one delivery path:
//  - poll(): bounded work — drain what the uart has, frame it, dispatch AT
//    MOST one request against the DataModel, emit at most one response. It
//    never blocks, never sleeps, never yields to an OS — drop it in a
//    superloop or call it from a coroutine between co_awaits.
//  - on_bytes(span): the DMA-ring shape (docs/design/dma-streams.md §2.2) —
//    the caller's ring collected the bytes with no per-byte ISR and no
//    per-byte read; each wake hands the batch over whole, and EVERY request
//    inside it is dispatched as it closes. TX (the response) still goes out
//    through the uart the server holds.
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

#include <concepts>
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

// The escape hatch for function codes the eight-FC core does not speak
// (vendor FCs, diagnostics, file records): consulted ONLY when parse_request
// refuses with illegal-function, i.e. the standard dispatch always wins for
// the codes it owns. Contract: (request ADU-less PDU, response PDU buffer,
// broadcast?) -> Result<bytes_written, modbus_error>:
//   - value N>0: the server frames+sends out[0..N) (unless broadcast);
//   - value 0: deliberate silence (the hook executed, nothing to say);
//   - error unexpected_function: "not mine" — the server answers 0x01 as if
//     no hook existed;
//   - any other error: exception reply — wire codes pass through, local
//     errors become 0x04 (server failure), the honest catch-all.
// Writing more than the response buffer holds is a programmer error (trap).
struct no_dispatch {
    [[nodiscard]] Result<std::size_t, modbus_error> operator()(
        std::span<const std::uint8_t>, std::span<std::uint8_t>, bool) const {
        return modbus_error::unexpected_function;
    }
};

template <class D>
concept UserDispatch = requires(D& d, std::span<const std::uint8_t> req,
                                std::span<std::uint8_t> resp) {
    { d(req, resp, true) } -> std::same_as<Result<std::size_t, modbus_error>>;
};

struct server_config {
    std::uint8_t unit = 1;      // 1..247; 0 is the broadcast id, never a server's
    std::uint32_t baud = 19'200;
};

template <alloy::ByteStream Uart, DataModel Model, std::size_t MaxRegisters = 16,
          class Clock = uptime_clock, alloy::OutputPin De = alloy::gpio::null_output,
          UserDispatch Dispatch = no_dispatch>
class rtu_server {
    static_assert(MaxRegisters >= 1 && MaxRegisters <= max_read_registers,
                  "MaxRegisters is the per-request register budget: 1..125");

public:
    using config = server_config;

    constexpr rtu_server(Uart& uart, Model& model, config c, Clock clock = {},
                         De de = {}, Dispatch dispatch = {})
        : uart_(uart),
          model_(model),
          dispatch_(dispatch),
          framer_(direction::request, rtu_times_for(c.baud)),
          unit_(c.unit),
          clock_(clock),
          de_(de) {
        if (c.unit == 0u || c.unit > 247u) {
            __builtin_trap();  // claiming the broadcast id is a programmer error
        }
    }
    rtu_server(Uart&&, Model&, config, Clock = {}, De = {}, Dispatch = {}) = delete;

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
        return deliver();
    }

    // The DMA-ring entry (docs/design/dma-streams.md §2.2): a BATCH of
    // received bytes — everything the ring's readable() returned at this
    // wake — fed in one call, every complete request inside it dispatched
    // (and answered) as it closes. The uart is never read here; RX bytes
    // arrive by DMA and this span is the only input path.
    //
    // Returns the count of bytes ACCEPTED, which is always bytes.size(): the
    // framer owns assembly and discards internally (noise, oversize claims,
    // CRC failures all land in its t3.5 discard mode), so the caller must
    // ring.consume() exactly what it fed — re-feeding a byte on a later wake
    // is a double-feed corruption, not a retry. Non-zero for a non-empty
    // span, so the anchor's `if (on_bytes(frame)) ring.consume(...)`
    // spelling reads naturally.
    //
    // One timestamp stamps the whole batch: the bytes were already on the
    // wire before this call, and intra-batch gaps are unknowable after DMA
    // batched them — frame separation inside a batch comes from LENGTH
    // PREDICTION (a frame closes on its last byte, then the next byte starts
    // fresh because deliver() consumed), while t3.5 silence separates the
    // batches themselves.
    std::size_t on_bytes(std::span<const std::uint8_t> bytes) {
        const std::uint32_t now = clock_.now_us();
        for (std::uint8_t b : bytes) {
            framer_.feed(b, now);
            if (framer_.has_frame()) {
                (void)deliver();
            }
        }
        framer_.tick(clock_.now_us());
        if (framer_.has_frame()) {
            (void)deliver();  // a silence-framed close on the trailing tick
        }
        return bytes.size();
    }

private:
    // One complete CRC-valid frame is waiting in the framer: answer it (or
    // honor the silences) and release the assembly buffer. The single
    // delivery path poll() and on_bytes() share, so the unit filter and the
    // broadcast rule cannot drift between the polled and the DMA-ring shape.
    bool deliver() {
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

    // Response PDU worst case: fc + byte_count + 2*MaxRegisters of data.
    static constexpr std::size_t pdu_cap = 2u + 2u * MaxRegisters;

    void dispatch(std::span<const std::uint8_t> pdu, bool broadcast) {
        const auto req = parse_request(pdu);
        if (!req) {
            // Unknown function code: the hook speaks before the refusal does.
            if (req.error() == modbus_error::exception_illegal_function &&
                !pdu.empty()) {
                std::uint8_t out[pdu_cap];
                const auto handled = dispatch_(pdu, {out, sizeof out}, broadcast);
                if (handled) {
                    if (*handled > sizeof out) {
                        __builtin_trap();  // hook overran its buffer: programmer error
                    }
                    if (*handled != 0u && !broadcast) {
                        send({out, *handled});
                    }
                    return;
                }
                if (handled.error() != modbus_error::unexpected_function) {
                    if (!broadcast) {
                        const auto fc = static_cast<function>(pdu[0]);
                        reply_exception(fc, is_exception(handled.error())
                                                ? handled.error()
                                                : modbus_error::exception_server_failure);
                    }
                    return;
                }
            }
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

    // One write call, either verdict shape (data_model.hpp): bool folds to
    // 0x02, a rich Result passes its wire exception through — and a LOCAL
    // error becomes 0x04, the honest catch-all, never a fabricated 02/03.
    template <class F>
    [[nodiscard]] modbus_error one_write(F&& call) {
        if constexpr (std::same_as<decltype(call()), bool>) {
            return call() ? modbus_error{}
                          : modbus_error::exception_illegal_data_address;
        } else {
            const auto r = call();
            if (r) {
                return modbus_error{};
            }
            return is_exception(r.error()) ? r.error()
                                           : modbus_error::exception_server_failure;
        }
    }

    // Applies any of the four write shapes. Default-constructed modbus_error
    // (value 0) is "no error" here — a private sentinel, never on the wire.
    [[nodiscard]] modbus_error apply_write(const request_view& req) {
        switch (req.fc) {
        case function::write_single_coil:
            return one_write(
                [&] { return model_.write_coil(req.address, req.value == 0xFF00u); });
        case function::write_single_register:
            return one_write(
                [&] { return model_.write_holding(req.address, req.value); });
        case function::write_multiple_coils:
            for (std::uint16_t i = 0; i < req.count; ++i) {
                const bool on =
                    (req.payload[i / 8u] >> (i % 8u) & 1u) != 0u;
                const auto a = static_cast<std::uint16_t>(req.address + i);
                if (const auto e = one_write([&] { return model_.write_coil(a, on); });
                    e != modbus_error{}) {
                    return e;
                }
            }
            return modbus_error{};
        case function::write_multiple_registers:
            for (std::uint16_t i = 0; i < req.count; ++i) {
                const std::uint16_t v = static_cast<std::uint16_t>(
                    static_cast<std::uint16_t>(req.payload[2u * i]) << 8 |
                    req.payload[2u * i + 1u]);
                const auto a = static_cast<std::uint16_t>(req.address + i);
                if (const auto e = one_write([&] { return model_.write_holding(a, v); });
                    e != modbus_error{}) {
                    return e;
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
    Dispatch dispatch_;
    rtu_framer<max_adu> framer_;
    std::uint8_t unit_;
    Clock clock_;
    De de_;
};

}  // namespace alloy::lib::modbus
