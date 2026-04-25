#pragma once

// Modbus RTU master (client) — cooperative, allocation-free.
//
// Usage (production):
//   Master master{stream, now_us_fn};
//   master.add_poll(temp_var, temp_mirror, 0x01, 500'000u); // poll every 500ms
//   while (true) { master.poll_once(10'000u); }
//
// Usage (loopback test — interleave slave between send and recv):
//   master.send_due_request();   // sends FC03 to slave
//   slave.poll(0u);              // slave processes and puts response in loopback
//   master.recv_due_response(0u); // master reads response, updates mirror
//
// Mirror update: atomic under CritSection RAII guard (same as slave.hpp).
// Stale-data callback: optional; called when recv_due_response() times out.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "alloy/modbus/byte_stream.hpp"
#include "alloy/modbus/pdu.hpp"
#include "alloy/modbus/registry.hpp"  // VarTraits, WordOrder, VarValueType
#include "alloy/modbus/rtu_frame.hpp"
#include "alloy/modbus/var.hpp"
#include "core/result.hpp"

namespace alloy::modbus {

// ============================================================================
// MasterError
// ============================================================================

enum class MasterError : std::uint8_t {
    StreamError,     // send or receive failed
    ResponseError,   // CRC mismatch, wrong slave ID, or exception response
    Timeout,         // no response within deadline
    BufferError,     // internal encoding overflow (should not happen)
    SlotsFull,       // add_poll() called when MaxPolls entries already exist
};

// ============================================================================
// Internal poll descriptor (type-erased)
// ============================================================================

namespace detail {

struct PollDescriptor {
    std::uint16_t reg_address;
    std::uint8_t  reg_count;
    std::uint8_t  slave_id;
    WordOrder     word_order;
    std::uint32_t interval_us;
    std::uint64_t next_deadline_us;
    bool          stale{true};

    void* mirror{nullptr};
    void (*update_fn)(const std::uint16_t* words, void* mirror,
                      WordOrder) noexcept{nullptr};
};

}  // namespace detail

// ============================================================================
// Master<Stream, MaxPolls, NowFn, CritSection>
// ============================================================================

template <ByteStream Stream, std::size_t MaxPolls, typename NowFn,
          typename CritSection = detail::NoOpCriticalSection>
class Master {
    static_assert(MaxPolls > 0u);

   public:
    // NowFn must be callable with no args and return uint64_t (microseconds).
    Master(Stream& stream, NowFn now_us) noexcept
        : stream_{stream}, now_us_{now_us} {}

    Master(const Master&) = delete;
    Master& operator=(const Master&) = delete;

    // -----------------------------------------------------------------------
    // Registration
    // -----------------------------------------------------------------------

    // Register a variable for periodic polling. The master will read the
    // variable from the slave at the given interval and store the result in
    // mirror. mirror must outlive this Master.
    template <VarValueType T>
    [[nodiscard]] core::Result<void, MasterError> add_poll(
        const Var<T>& var, T& mirror, std::uint8_t slave_id,
        std::uint32_t interval_us) noexcept {
        if (poll_count_ >= MaxPolls) return core::Err(MasterError::SlotsFull);

        detail::PollDescriptor d{};
        d.reg_address      = var.address;
        d.reg_count        = static_cast<std::uint8_t>(Var<T>::kRegCount);
        d.slave_id         = slave_id;
        d.word_order       = var.word_order;
        d.interval_us      = interval_us;
        d.next_deadline_us = now_us_();  // due immediately on first call
        d.stale            = true;
        d.mirror           = &mirror;
        d.update_fn        = VarTraits<T>::decode;

        polls_[poll_count_++] = d;
        return core::Ok();
    }

    // -----------------------------------------------------------------------
    // Stale-data callback
    // -----------------------------------------------------------------------

    void set_stale_callback(
        void (*cb)(std::uint16_t reg_address,
                   std::uint8_t slave_id) noexcept) noexcept {
        stale_cb_ = cb;
    }

    // -----------------------------------------------------------------------
    // Two-phase poll API (fine-grained control for tests)
    // -----------------------------------------------------------------------

    // Find the next overdue poll entry and send a FC03 request for it.
    // Returns true if a request was sent, false if nothing is due yet.
    [[nodiscard]] bool send_due_request() noexcept {
        detail::PollDescriptor* due = find_next_due();
        if (!due) return false;

        inflight_idx_ = static_cast<std::size_t>(due - polls_.data());
        inflight_active_ = true;

        // Encode FC03 request PDU
        std::array<std::byte, kMaxPduBytes> req_pdu{};
        const auto enc = encode_read_request(
            req_pdu, FunctionCode::ReadHoldingRegisters,
            ReadRequest{.address = due->reg_address,
                        .quantity = due->reg_count});
        if (enc.is_err()) {
            inflight_active_ = false;
            return false;
        }

        // Wrap in RTU frame and send
        std::array<std::byte, 256u> tx{};
        const auto frm = encode_rtu_frame(
            tx, due->slave_id,
            std::span<const std::byte>{req_pdu.data(), enc.unwrap()});
        if (frm.is_err()) {
            inflight_active_ = false;
            return false;
        }

        const auto wr = stream_.write(
            std::span<const std::byte>{tx.data(), frm.unwrap()});
        if (wr.is_err()) {
            inflight_active_ = false;
        }
        return inflight_active_;
    }

    // Read the response for the in-flight request and update the mirror.
    // Returns Ok(true) on success, Ok(false) if nothing was in-flight.
    [[nodiscard]] core::Result<bool, MasterError> recv_due_response(
        std::uint32_t timeout_us) noexcept {
        if (!inflight_active_) return core::Ok(false);
        inflight_active_ = false;

        detail::PollDescriptor& due = polls_[inflight_idx_];
        const std::uint64_t now = now_us_();

        // Read RTU response frame
        std::array<std::byte, 256u> rx{};
        const auto n = stream_.read(rx, timeout_us);
        if (n.is_err() || n.unwrap() == 0u) {
            due.stale = true;
            if (stale_cb_) stale_cb_(due.reg_address, due.slave_id);
            return core::Err(MasterError::Timeout);
        }

        const auto frame_res = decode_rtu_frame(
            std::span<const std::byte>{rx.data(), n.unwrap()});
        if (frame_res.is_err()) {
            due.stale = true;
            return core::Err(MasterError::ResponseError);
        }

        const auto& frame = frame_res.unwrap();
        if (frame.slave_id != due.slave_id) {
            due.stale = true;
            return core::Err(MasterError::ResponseError);
        }

        // Parse FC03/04 response
        if (is_exception(frame.pdu)) {
            due.stale = true;
            return core::Err(MasterError::ResponseError);
        }

        const auto rsp_res = decode_read_registers_response(frame.pdu);
        if (rsp_res.is_err()) {
            due.stale = true;
            return core::Err(MasterError::ResponseError);
        }

        const auto& rsp = rsp_res.unwrap();
        const std::size_t expected_bytes =
            static_cast<std::size_t>(due.reg_count) * 2u;
        if (rsp.register_bytes.size() < expected_bytes) {
            due.stale = true;
            return core::Err(MasterError::ResponseError);
        }

        // Convert big-endian register bytes → uint16_t words
        std::array<std::uint16_t, 4u> words{};
        for (std::uint8_t i = 0u; i < due.reg_count; ++i) {
            const auto hi =
                static_cast<std::uint8_t>(rsp.register_bytes[i * 2u]);
            const auto lo =
                static_cast<std::uint8_t>(rsp.register_bytes[i * 2u + 1u]);
            words[i] = (static_cast<std::uint16_t>(hi) << 8u) | lo;
        }

        // Update mirror under critical section
        {
            [[maybe_unused]] const CritSection guard{};
            if (due.update_fn) {
                due.update_fn(words.data(), due.mirror, due.word_order);
            }
        }

        due.stale             = false;
        due.next_deadline_us  = now + due.interval_us;
        return core::Ok(true);
    }

    // -----------------------------------------------------------------------
    // Combined poll (production use): send + blocking recv
    // -----------------------------------------------------------------------

    [[nodiscard]] core::Result<bool, MasterError> poll_once(
        std::uint32_t response_timeout_us) noexcept {
        if (!send_due_request()) return core::Ok(false);
        return recv_due_response(response_timeout_us);
    }

    // -----------------------------------------------------------------------
    // Immediate write: FC06 (single register) or FC10 (multiple registers)
    // -----------------------------------------------------------------------

    template <VarValueType T>
    [[nodiscard]] core::Result<void, MasterError> write_now(
        const Var<T>& var, T value, std::uint8_t slave_id,
        std::uint32_t response_timeout_us) noexcept {
        constexpr std::size_t kRegCount = Var<T>::kRegCount;
        const auto words = var.encode(value);

        std::array<std::byte, kMaxPduBytes> req_pdu{};
        std::size_t req_len{};

        if constexpr (kRegCount == 1u) {
            // FC06: write single register
            const auto enc = encode_write_single_register_request(
                req_pdu,
                WriteSingleRegisterRequest{.address = var.address,
                                           .value   = words[0]});
            if (enc.is_err()) return core::Err(MasterError::BufferError);
            req_len = enc.unwrap();
        } else {
            // FC10: write multiple registers
            std::array<std::byte, kRegCount * 2u> reg_bytes{};
            for (std::size_t i = 0u; i < kRegCount; ++i) {
                reg_bytes[i * 2u] =
                    std::byte{static_cast<std::uint8_t>(words[i] >> 8u)};
                reg_bytes[i * 2u + 1u] =
                    std::byte{static_cast<std::uint8_t>(words[i] & 0xFFu)};
            }
            const auto enc = encode_write_multiple_registers_request(
                req_pdu,
                WriteMultipleRegistersRequest{
                    .address        = var.address,
                    .quantity       = kRegCount,
                    .register_bytes = reg_bytes});
            if (enc.is_err()) return core::Err(MasterError::BufferError);
            req_len = enc.unwrap();
        }

        // Send RTU frame
        std::array<std::byte, 256u> tx{};
        const auto frm = encode_rtu_frame(
            tx, slave_id,
            std::span<const std::byte>{req_pdu.data(), req_len});
        if (frm.is_err()) return core::Err(MasterError::BufferError);

        const auto wr = stream_.write(
            std::span<const std::byte>{tx.data(), frm.unwrap()});
        if (wr.is_err()) return core::Err(MasterError::StreamError);

        // Read echo response
        std::array<std::byte, 256u> rx{};
        const auto n = stream_.read(rx, response_timeout_us);
        if (n.is_err() || n.unwrap() == 0u) {
            return core::Err(MasterError::Timeout);
        }

        const auto frame_res = decode_rtu_frame(
            std::span<const std::byte>{rx.data(), n.unwrap()});
        if (frame_res.is_err()) return core::Err(MasterError::ResponseError);
        if (frame_res.unwrap().slave_id != slave_id) {
            return core::Err(MasterError::ResponseError);
        }
        if (is_exception(frame_res.unwrap().pdu)) {
            return core::Err(MasterError::ResponseError);
        }

        return core::Ok();
    }

    // -----------------------------------------------------------------------
    // Query: is the given poll entry's mirror stale?
    // -----------------------------------------------------------------------

    [[nodiscard]] bool is_stale(std::size_t poll_idx) const noexcept {
        if (poll_idx >= poll_count_) return true;
        return polls_[poll_idx].stale;
    }

    [[nodiscard]] std::size_t poll_count() const noexcept { return poll_count_; }

   private:
    Stream& stream_;
    NowFn   now_us_;

    void (*stale_cb_)(std::uint16_t, std::uint8_t) noexcept {nullptr};

    std::array<detail::PollDescriptor, MaxPolls> polls_{};
    std::size_t poll_count_{0u};

    std::size_t inflight_idx_{0u};
    bool        inflight_active_{false};

    // Find the poll entry with the earliest overdue deadline.
    // Skips the currently in-flight entry (if any) to prevent duplicate sends.
    [[nodiscard]] detail::PollDescriptor* find_next_due() noexcept {
        const std::uint64_t now = now_us_();
        detail::PollDescriptor* best = nullptr;

        for (std::size_t i = 0u; i < poll_count_; ++i) {
            if (inflight_active_ && i == inflight_idx_) continue;
            auto& p = polls_[i];
            if (now < p.next_deadline_us) continue;  // not due yet
            if (!best || p.next_deadline_us < best->next_deadline_us) {
                best = &p;
            }
        }
        return best;
    }
};

}  // namespace alloy::modbus
