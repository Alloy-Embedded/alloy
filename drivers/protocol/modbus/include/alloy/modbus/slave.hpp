#pragma once

// Modbus RTU slave (server) — cooperative, allocation-free.
//
// Poll model: call slave.poll(timeout_us) in a loop. Each call:
//   1. Reads one RTU frame from the byte_stream.
//   2. Ignores frames for other slave IDs.
//   3. Dispatches to the appropriate FC handler.
//   4. Writes the response RTU frame back.
//
// The Registry must be bound (constructed via bind(), not make_registry()) so
// the slave can read and write the actual variable values.
//
// Critical section: multi-word variables (float, int32, double) are updated
// under a CritSection RAII guard. Provide a platform-specific type if needed:
//   struct McuCs { McuCs() { __disable_irq(); } ~McuCs() { __enable_irq(); } };
//   Slave<Stream, N, McuCs> slave{...};
// The default NoOpCriticalSection is correct for single-threaded / test use.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "alloy/modbus/byte_stream.hpp"
#include "alloy/modbus/pdu.hpp"
#include "alloy/modbus/registry.hpp"
#include "alloy/modbus/rtu_frame.hpp"
#include "core/result.hpp"

namespace alloy::modbus {

// ============================================================================
// SlaveError: errors that poll() can surface (distinct from PDU/RTU errors)
// ============================================================================

enum class SlaveError : std::uint8_t {
    StreamError,  // underlying read/write failed
    BufferError,  // response encoding failed (should not happen with 256-B buffers)
};

// ============================================================================
// Default no-op critical section
// ============================================================================

namespace detail {
struct NoOpCriticalSection {
    NoOpCriticalSection() noexcept = default;
};
}  // namespace detail

// ============================================================================
// Slave<Stream, N, CritSection>
// ============================================================================

template <ByteStream Stream, std::size_t N,
          typename CritSection = detail::NoOpCriticalSection>
class Slave {
   public:
    Slave(Stream& stream, std::uint8_t slave_id, Registry<N>& registry) noexcept
        : stream_{stream}, slave_id_{slave_id}, registry_{registry} {}

    Slave(const Slave&) = delete;
    Slave& operator=(const Slave&) = delete;

    // Process one incoming frame. Returns true if a frame was processed and a
    // response was sent. Returns false on timeout (no data) or non-addressed frame.
    [[nodiscard]] core::Result<bool, SlaveError> poll(
        std::uint32_t timeout_us) noexcept {
        // Receive raw bytes
        std::array<std::byte, kRtuMaxFrameBytes> rx{};
        const auto n = stream_.read(rx, timeout_us);
        if (n.is_err()) return core::Err(SlaveError::StreamError);
        if (n.unwrap() == 0u) return core::Ok(false);

        // Parse RTU frame
        const auto frame_res = decode_rtu_frame(
            std::span<const std::byte>{rx.data(), n.unwrap()});
        if (frame_res.is_err()) return core::Ok(false);

        const auto& frame = frame_res.unwrap();
        if (frame.slave_id != slave_id_) return core::Ok(false);

        // Build response PDU into a local buffer
        std::array<std::byte, kMaxPduBytes> rsp_pdu{};
        const std::size_t rsp_len = dispatch(frame.pdu, rsp_pdu);
        if (rsp_len == 0u) return core::Ok(true);

        // Encode and send RTU response
        std::array<std::byte, kRtuMaxFrameBytes> tx{};
        const auto enc = encode_rtu_frame(
            tx, slave_id_, std::span<const std::byte>{rsp_pdu.data(), rsp_len});
        if (enc.is_err()) return core::Err(SlaveError::BufferError);

        const auto wr = stream_.write(
            std::span<const std::byte>{tx.data(), enc.unwrap()});
        if (wr.is_err()) return core::Err(SlaveError::StreamError);

        return core::Ok(true);
    }

    // Convenience: loop forever calling poll(timeout_us).
    [[noreturn]] void run(std::uint32_t timeout_us = 0u) noexcept {
        while (true) { (void)poll(timeout_us); }
    }

   private:
    static constexpr std::size_t kRtuMaxFrameBytes = 256u;

    Stream&      stream_;
    std::uint8_t slave_id_;
    Registry<N>& registry_;

    // -----------------------------------------------------------------------
    // Dispatch
    // -----------------------------------------------------------------------

    std::size_t dispatch(std::span<const std::byte> pdu,
                         std::span<std::byte> rsp) noexcept {
        const auto fc_res = peek_function_code(pdu);
        if (fc_res.is_err()) {
            // Can't determine FC — send IllegalFunction with a placeholder FC.
            return encode_exception_into(
                rsp, static_cast<FunctionCode>(0x00),
                ExceptionCode::IllegalFunction);
        }
        const FunctionCode fc = fc_res.unwrap();
        switch (fc) {
            case FunctionCode::ReadCoils:
            case FunctionCode::ReadDiscreteInputs:
                return handle_read_coils(pdu, rsp, fc);
            case FunctionCode::ReadHoldingRegisters:
            case FunctionCode::ReadInputRegisters:
                return handle_read_registers(pdu, rsp, fc);
            case FunctionCode::WriteSingleCoil:
                return handle_write_single_coil(pdu, rsp);
            case FunctionCode::WriteSingleRegister:
                return handle_write_single_register(pdu, rsp);
            case FunctionCode::WriteMultipleCoils:
                return handle_write_multiple_coils(pdu, rsp);
            case FunctionCode::WriteMultipleRegisters:
                return handle_write_multiple_registers(pdu, rsp);
            case FunctionCode::ReadWriteMultipleRegisters:
                return handle_read_write_multiple_registers(pdu, rsp);
        }
        return encode_exception_into(rsp, fc, ExceptionCode::IllegalFunction);
    }

    // -----------------------------------------------------------------------
    // Register-level read/write helpers
    // -----------------------------------------------------------------------

    // Read a single 16-bit register from the registry. Returns nullopt when
    // the address is unmapped or the variable is write-only.
    [[nodiscard]] std::optional<std::uint16_t> read_register_word(
        std::uint16_t addr) const noexcept {
        const auto* d = registry_.find(addr);
        if (!d || !d->encode_fn) return std::nullopt;
        if (d->access == Access::WriteOnly) return std::nullopt;

        std::array<std::uint16_t, 4u> words{};
        d->encode_fn(d->data, words.data(), d->word_order);
        return words[addr - d->address];
    }

    // Write a single 16-bit register into the registry. Uses a read-modify-write
    // under a critical section to prevent tearing of multi-word variables.
    // Returns false if the address is unmapped or read-only.
    [[nodiscard]] bool write_register_word(std::uint16_t addr,
                                           std::uint16_t value) noexcept {
        const auto* d = registry_.find(addr);
        if (!d || !d->decode_fn) return false;
        if (d->access == Access::ReadOnly) return false;

        [[maybe_unused]] const CritSection guard{};  // RAII critical section

        // Read-modify-write: preserve words not covered by this single write.
        std::array<std::uint16_t, 4u> words{};
        if (d->encode_fn) d->encode_fn(d->data, words.data(), d->word_order);
        words[addr - d->address] = value;
        d->decode_fn(words.data(), d->data, d->word_order);
        return true;
    }

    // -----------------------------------------------------------------------
    // FC 0x03 / 0x04 — Read Holding / Input Registers
    // -----------------------------------------------------------------------

    std::size_t handle_read_registers(std::span<const std::byte> pdu,
                                      std::span<std::byte> rsp,
                                      FunctionCode fc) noexcept {
        const auto req = decode_read_request(pdu);
        if (req.is_err()) {
            return encode_exception_into(rsp, fc, ExceptionCode::IllegalDataValue);
        }
        const auto [start, qty] = req.unwrap();
        if (qty == 0u || qty > kMaxReadRegisters) {
            return encode_exception_into(rsp, fc, ExceptionCode::IllegalDataValue);
        }

        // Collect register words
        std::array<std::uint16_t, kMaxReadRegisters> words{};
        for (std::uint16_t i = 0u; i < qty; ++i) {
            const auto w = read_register_word(
                static_cast<std::uint16_t>(start + i));
            if (!w) {
                return encode_exception_into(
                    rsp, fc, ExceptionCode::IllegalDataAddress);
            }
            words[i] = *w;
        }

        // Convert to big-endian bytes
        std::array<std::byte, kMaxReadRegisters * 2u> reg_bytes{};
        for (std::uint16_t i = 0u; i < qty; ++i) {
            reg_bytes[i * 2u] =
                std::byte{static_cast<std::uint8_t>(words[i] >> 8u)};
            reg_bytes[i * 2u + 1u] =
                std::byte{static_cast<std::uint8_t>(words[i] & 0xFFu)};
        }

        const auto enc = encode_read_registers_response(
            rsp, fc, std::span<const std::byte>{reg_bytes.data(), qty * 2u});
        if (enc.is_err()) {
            return encode_exception_into(rsp, fc, ExceptionCode::SlaveDeviceFailure);
        }
        return enc.unwrap();
    }

    // -----------------------------------------------------------------------
    // FC 0x06 — Write Single Register
    // -----------------------------------------------------------------------

    std::size_t handle_write_single_register(std::span<const std::byte> pdu,
                                              std::span<std::byte> rsp) noexcept {
        constexpr FunctionCode kFc = FunctionCode::WriteSingleRegister;
        const auto req = decode_write_single_register_request(pdu);
        if (req.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataValue);
        }
        const auto [addr, value] = req.unwrap();

        if (!write_register_word(addr, value)) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataAddress);
        }

        const auto enc = encode_write_single_response(
            rsp, kFc, WriteSingleResponse{.address = addr, .value = value});
        if (enc.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::SlaveDeviceFailure);
        }
        return enc.unwrap();
    }

    // -----------------------------------------------------------------------
    // FC 0x10 — Write Multiple Registers
    // -----------------------------------------------------------------------

    std::size_t handle_write_multiple_registers(std::span<const std::byte> pdu,
                                                 std::span<std::byte> rsp) noexcept {
        constexpr FunctionCode kFc = FunctionCode::WriteMultipleRegisters;
        const auto req = decode_write_multiple_registers_request(pdu);
        if (req.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataValue);
        }
        const auto& r = req.unwrap();
        if (r.quantity == 0u || r.quantity > kMaxWriteRegisters) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataValue);
        }

        // First pass: validate all addresses are writable.
        for (std::uint16_t i = 0u; i < r.quantity; ++i) {
            const auto* d = registry_.find(
                static_cast<std::uint16_t>(r.address + i));
            if (!d || d->access == Access::ReadOnly || !d->decode_fn) {
                return encode_exception_into(
                    rsp, kFc, ExceptionCode::IllegalDataAddress);
            }
        }

        // Second pass: write each register (RMW under critical section per word).
        for (std::uint16_t i = 0u; i < r.quantity; ++i) {
            const auto hi =
                static_cast<std::uint8_t>(r.register_bytes[i * 2u]);
            const auto lo =
                static_cast<std::uint8_t>(r.register_bytes[i * 2u + 1u]);
            const std::uint16_t val =
                (static_cast<std::uint16_t>(hi) << 8u) | lo;
            (void)write_register_word(static_cast<std::uint16_t>(r.address + i), val);
        }

        const auto enc = encode_write_multiple_response(
            rsp, kFc,
            WriteMultipleResponse{.address = r.address, .quantity = r.quantity});
        if (enc.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::SlaveDeviceFailure);
        }
        return enc.unwrap();
    }

    // -----------------------------------------------------------------------
    // FC 0x17 — Read/Write Multiple Registers
    // -----------------------------------------------------------------------

    std::size_t handle_read_write_multiple_registers(
        std::span<const std::byte> pdu, std::span<std::byte> rsp) noexcept {
        constexpr FunctionCode kFc = FunctionCode::ReadWriteMultipleRegisters;
        const auto req = decode_read_write_multiple_registers_request(pdu);
        if (req.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataValue);
        }
        const auto& r = req.unwrap();

        // Validate quantities
        if (r.read_quantity == 0u || r.read_quantity > kMaxReadWriteReadRegisters ||
            r.write_quantity == 0u ||
            r.write_quantity > kMaxReadWriteWriteRegisters) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataValue);
        }

        // Writes first (per spec)
        for (std::uint16_t i = 0u; i < r.write_quantity; ++i) {
            const auto hi =
                static_cast<std::uint8_t>(r.write_register_bytes[i * 2u]);
            const auto lo =
                static_cast<std::uint8_t>(r.write_register_bytes[i * 2u + 1u]);
            const std::uint16_t val =
                (static_cast<std::uint16_t>(hi) << 8u) | lo;
            if (!write_register_word(
                    static_cast<std::uint16_t>(r.write_address + i), val)) {
                return encode_exception_into(
                    rsp, kFc, ExceptionCode::IllegalDataAddress);
            }
        }

        // Then reads
        std::array<std::uint16_t, kMaxReadWriteReadRegisters> words{};
        for (std::uint16_t i = 0u; i < r.read_quantity; ++i) {
            const auto w = read_register_word(
                static_cast<std::uint16_t>(r.read_address + i));
            if (!w) {
                return encode_exception_into(
                    rsp, kFc, ExceptionCode::IllegalDataAddress);
            }
            words[i] = *w;
        }

        std::array<std::byte, kMaxReadWriteReadRegisters * 2u> reg_bytes{};
        for (std::uint16_t i = 0u; i < r.read_quantity; ++i) {
            reg_bytes[i * 2u] =
                std::byte{static_cast<std::uint8_t>(words[i] >> 8u)};
            reg_bytes[i * 2u + 1u] =
                std::byte{static_cast<std::uint8_t>(words[i] & 0xFFu)};
        }

        const auto enc = encode_read_registers_response(
            rsp, kFc,
            std::span<const std::byte>{reg_bytes.data(), r.read_quantity * 2u});
        if (enc.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::SlaveDeviceFailure);
        }
        return enc.unwrap();
    }

    // -----------------------------------------------------------------------
    // FC 0x01 / 0x02 — Read Coils / Discrete Inputs
    // Coils are mapped to bool variables in the registry.
    // -----------------------------------------------------------------------

    std::size_t handle_read_coils(std::span<const std::byte> pdu,
                                  std::span<std::byte> rsp,
                                  FunctionCode fc) noexcept {
        const auto req = decode_read_request(pdu);
        if (req.is_err()) {
            return encode_exception_into(rsp, fc, ExceptionCode::IllegalDataValue);
        }
        const auto [start, qty] = req.unwrap();
        if (qty == 0u || qty > kMaxReadCoils) {
            return encode_exception_into(rsp, fc, ExceptionCode::IllegalDataValue);
        }

        // Packed bits: LSB of first byte = coil at start address
        constexpr std::size_t kMaxCoilBytes = (kMaxReadCoils + 7u) / 8u;
        std::array<std::byte, kMaxCoilBytes> bits{};

        for (std::uint16_t i = 0u; i < qty; ++i) {
            const auto w = read_register_word(
                static_cast<std::uint16_t>(start + i));
            if (!w) {
                return encode_exception_into(
                    rsp, fc, ExceptionCode::IllegalDataAddress);
            }
            if (*w != 0u) {
                bits[i / 8u] |= std::byte{
                    static_cast<std::uint8_t>(1u << (i % 8u))};
            }
        }

        const std::size_t byte_count = (qty + 7u) / 8u;
        const auto enc = encode_read_coils_response(
            rsp, fc, std::span<const std::byte>{bits.data(), byte_count});
        if (enc.is_err()) {
            return encode_exception_into(rsp, fc, ExceptionCode::SlaveDeviceFailure);
        }
        return enc.unwrap();
    }

    // -----------------------------------------------------------------------
    // FC 0x05 — Write Single Coil
    // -----------------------------------------------------------------------

    std::size_t handle_write_single_coil(std::span<const std::byte> pdu,
                                          std::span<std::byte> rsp) noexcept {
        constexpr FunctionCode kFc = FunctionCode::WriteSingleCoil;
        const auto req = decode_write_single_coil_request(pdu);
        if (req.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataValue);
        }
        const auto [addr, value] = req.unwrap();

        // Coil ON = 0xFF00, OFF = 0x0000 on the wire.
        const std::uint16_t wire_val = value ? 0xFF00u : 0x0000u;

        if (!write_register_word(addr, wire_val)) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataAddress);
        }

        const auto enc = encode_write_single_response(
            rsp, kFc, WriteSingleResponse{.address = addr, .value = wire_val});
        if (enc.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::SlaveDeviceFailure);
        }
        return enc.unwrap();
    }

    // -----------------------------------------------------------------------
    // FC 0x0F — Write Multiple Coils
    // -----------------------------------------------------------------------

    std::size_t handle_write_multiple_coils(std::span<const std::byte> pdu,
                                             std::span<std::byte> rsp) noexcept {
        constexpr FunctionCode kFc = FunctionCode::WriteMultipleCoils;
        const auto req = decode_write_multiple_coils_request(pdu);
        if (req.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataValue);
        }
        const auto& r = req.unwrap();
        if (r.quantity == 0u || r.quantity > kMaxWriteCoils) {
            return encode_exception_into(rsp, kFc, ExceptionCode::IllegalDataValue);
        }

        for (std::uint16_t i = 0u; i < r.quantity; ++i) {
            const bool bit_set =
                (static_cast<std::uint8_t>(r.packed_bits[i / 8u]) >>
                 (i % 8u)) & 0x01u;
            const std::uint16_t wire_val = bit_set ? 0xFF00u : 0x0000u;
            if (!write_register_word(
                    static_cast<std::uint16_t>(r.address + i), wire_val)) {
                return encode_exception_into(
                    rsp, kFc, ExceptionCode::IllegalDataAddress);
            }
        }

        const auto enc = encode_write_multiple_response(
            rsp, kFc,
            WriteMultipleResponse{.address = r.address, .quantity = r.quantity});
        if (enc.is_err()) {
            return encode_exception_into(rsp, kFc, ExceptionCode::SlaveDeviceFailure);
        }
        return enc.unwrap();
    }

    // -----------------------------------------------------------------------
    // Exception helper
    // -----------------------------------------------------------------------

    [[nodiscard]] static std::size_t encode_exception_into(
        std::span<std::byte> out, FunctionCode fc, ExceptionCode ec) noexcept {
        const auto res = encode_exception(out, fc, ec);
        return res.is_ok() ? res.unwrap() : 0u;
    }
};

}  // namespace alloy::modbus
