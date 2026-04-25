// Modbus PDU encode/decode implementation.
//
// Layout reminders:
//   FC 0x01/0x02 request:   FC | addr_hi | addr_lo | qty_hi | qty_lo            (5 bytes)
//   FC 0x01/0x02 response:  FC | byte_count | packed_bits[ceil(qty/8)]
//   FC 0x03/0x04 request:   FC | addr_hi | addr_lo | qty_hi | qty_lo            (5 bytes)
//   FC 0x03/0x04 response:  FC | byte_count | register_bytes[qty*2]
//   FC 0x05 request/resp:   FC | addr_hi | addr_lo | val_hi | val_lo            (5 bytes)
//   FC 0x06 request/resp:   FC | addr_hi | addr_lo | val_hi | val_lo            (5 bytes)
//   FC 0x0F request:        FC | addr_hi | addr_lo | qty_hi | qty_lo | bc | bits
//   FC 0x0F response:       FC | addr_hi | addr_lo | qty_hi | qty_lo            (5 bytes)
//   FC 0x10 request:        FC | addr_hi | addr_lo | qty_hi | qty_lo | bc | regs
//   FC 0x10 response:       FC | addr_hi | addr_lo | qty_hi | qty_lo            (5 bytes)
//   FC 0x17 request:        FC | r_addr | r_qty | w_addr | w_qty | bc | w_regs
//   FC 0x17 response:       FC | byte_count | register_bytes[r_qty*2]
//   Exception:              (FC | 0x80) | exception_code                        (2 bytes)

#include "alloy/modbus/pdu.hpp"

namespace alloy::modbus {

namespace {

inline std::byte hi(std::uint16_t v) noexcept { return std::byte{static_cast<std::uint8_t>(v >> 8)}; }
inline std::byte lo(std::uint16_t v) noexcept { return std::byte{static_cast<std::uint8_t>(v & 0xFFu)}; }

inline std::uint16_t be16(std::byte hi_byte, std::byte lo_byte) noexcept {
    return static_cast<std::uint16_t>((static_cast<std::uint8_t>(hi_byte) << 8) |
                                      static_cast<std::uint8_t>(lo_byte));
}

inline std::uint16_t be16(std::span<const std::byte> bytes, std::size_t offset) noexcept {
    return be16(bytes[offset], bytes[offset + 1]);
}

constexpr bool is_register_read(FunctionCode fc) noexcept {
    return fc == FunctionCode::ReadHoldingRegisters || fc == FunctionCode::ReadInputRegisters;
}

constexpr bool is_coil_read(FunctionCode fc) noexcept {
    return fc == FunctionCode::ReadCoils || fc == FunctionCode::ReadDiscreteInputs;
}

}  // namespace

// ============================================================================
// Helpers
// ============================================================================

bool is_exception(std::span<const std::byte> pdu) noexcept {
    if (pdu.size() < 1) return false;
    return (static_cast<std::uint8_t>(pdu[0]) & 0x80u) != 0;
}

core::Result<FunctionCode, PduError> peek_function_code(std::span<const std::byte> pdu) noexcept {
    if (pdu.empty()) {
        return core::Err(PduError::Truncated);
    }
    const auto raw = static_cast<std::uint8_t>(pdu[0]) & 0x7Fu;
    if (!is_supported_function(raw)) {
        return core::Err(PduError::UnknownFunction);
    }
    return core::Ok(static_cast<FunctionCode>(raw));
}

// ============================================================================
// Read requests (FC 0x01/0x02/0x03/0x04)
// ============================================================================

core::Result<std::size_t, PduError> encode_read_request(
    std::span<std::byte> out, FunctionCode fc, ReadRequest req) noexcept {
    if (out.size() < 5) return core::Err(PduError::BufferTooSmall);
    if (req.quantity == 0) return core::Err(PduError::InvalidQuantity);
    const std::uint16_t cap =
        is_coil_read(fc) ? kMaxReadCoils : kMaxReadRegisters;
    if (req.quantity > cap) return core::Err(PduError::InvalidQuantity);

    out[0] = std::byte{static_cast<std::uint8_t>(fc)};
    out[1] = hi(req.address);
    out[2] = lo(req.address);
    out[3] = hi(req.quantity);
    out[4] = lo(req.quantity);
    return core::Ok(std::size_t{5});
}

core::Result<ReadRequest, PduError> decode_read_request(std::span<const std::byte> pdu) noexcept {
    if (pdu.size() != 5) return core::Err(PduError::Truncated);
    auto fc_result = peek_function_code(pdu);
    if (fc_result.is_err()) return core::Err(PduError{fc_result.unwrap_err()});
    const auto fc = fc_result.unwrap();
    if (!is_register_read(fc) && !is_coil_read(fc)) {
        return core::Err(PduError::UnknownFunction);
    }
    ReadRequest req{be16(pdu, 1), be16(pdu, 3)};
    if (req.quantity == 0) return core::Err(PduError::InvalidQuantity);
    const std::uint16_t cap = is_coil_read(fc) ? kMaxReadCoils : kMaxReadRegisters;
    if (req.quantity > cap) return core::Err(PduError::InvalidQuantity);
    return core::Ok(ReadRequest{req});
}

// ============================================================================
// Write Single Coil (FC 0x05)
// ============================================================================

core::Result<std::size_t, PduError> encode_write_single_coil_request(
    std::span<std::byte> out, WriteSingleCoilRequest req) noexcept {
    if (out.size() < 5) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(FunctionCode::WriteSingleCoil)};
    out[1] = hi(req.address);
    out[2] = lo(req.address);
    out[3] = std::byte{static_cast<std::uint8_t>(req.value ? 0xFFu : 0x00u)};
    out[4] = std::byte{static_cast<std::uint8_t>(0x00u)};
    return core::Ok(std::size_t{5});
}

core::Result<WriteSingleCoilRequest, PduError> decode_write_single_coil_request(
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() != 5) return core::Err(PduError::Truncated);
    if (static_cast<std::uint8_t>(pdu[0]) !=
        static_cast<std::uint8_t>(FunctionCode::WriteSingleCoil)) {
        return core::Err(PduError::UnknownFunction);
    }
    const auto vh = static_cast<std::uint8_t>(pdu[3]);
    const auto vl = static_cast<std::uint8_t>(pdu[4]);
    bool value;
    if (vh == 0xFFu && vl == 0x00u) value = true;
    else if (vh == 0x00u && vl == 0x00u) value = false;
    else return core::Err(PduError::InvalidValue);
    return core::Ok(WriteSingleCoilRequest{be16(pdu, 1), value});
}

// ============================================================================
// Write Single Register (FC 0x06)
// ============================================================================

core::Result<std::size_t, PduError> encode_write_single_register_request(
    std::span<std::byte> out, WriteSingleRegisterRequest req) noexcept {
    if (out.size() < 5) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(FunctionCode::WriteSingleRegister)};
    out[1] = hi(req.address);
    out[2] = lo(req.address);
    out[3] = hi(req.value);
    out[4] = lo(req.value);
    return core::Ok(std::size_t{5});
}

core::Result<WriteSingleRegisterRequest, PduError> decode_write_single_register_request(
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() != 5) return core::Err(PduError::Truncated);
    if (static_cast<std::uint8_t>(pdu[0]) !=
        static_cast<std::uint8_t>(FunctionCode::WriteSingleRegister)) {
        return core::Err(PduError::UnknownFunction);
    }
    return core::Ok(WriteSingleRegisterRequest{be16(pdu, 1), be16(pdu, 3)});
}

// ============================================================================
// Write Multiple Coils (FC 0x0F)
// ============================================================================

core::Result<std::size_t, PduError> encode_write_multiple_coils_request(
    std::span<std::byte> out, WriteMultipleCoilsRequest req) noexcept {
    if (req.quantity == 0 || req.quantity > kMaxWriteCoils) {
        return core::Err(PduError::InvalidQuantity);
    }
    const std::size_t expected_bytes = (req.quantity + 7u) / 8u;
    if (req.packed_bits.size() != expected_bytes) {
        return core::Err(PduError::InvalidByteCount);
    }
    const std::size_t total = 6 + expected_bytes;
    if (out.size() < total) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(FunctionCode::WriteMultipleCoils)};
    out[1] = hi(req.address);
    out[2] = lo(req.address);
    out[3] = hi(req.quantity);
    out[4] = lo(req.quantity);
    out[5] = std::byte{static_cast<std::uint8_t>(expected_bytes)};
    for (std::size_t i = 0; i < expected_bytes; ++i) out[6 + i] = req.packed_bits[i];
    return core::Ok(std::size_t{total});
}

core::Result<WriteMultipleCoilsRequest, PduError> decode_write_multiple_coils_request(
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() < 6) return core::Err(PduError::Truncated);
    if (static_cast<std::uint8_t>(pdu[0]) !=
        static_cast<std::uint8_t>(FunctionCode::WriteMultipleCoils)) {
        return core::Err(PduError::UnknownFunction);
    }
    const std::uint16_t quantity = be16(pdu, 3);
    if (quantity == 0 || quantity > kMaxWriteCoils) {
        return core::Err(PduError::InvalidQuantity);
    }
    const auto byte_count = static_cast<std::uint8_t>(pdu[5]);
    const std::size_t expected = (quantity + 7u) / 8u;
    if (byte_count != expected || pdu.size() != 6 + expected) {
        return core::Err(PduError::InvalidByteCount);
    }
    return core::Ok(WriteMultipleCoilsRequest{be16(pdu, 1), quantity, pdu.subspan(6, expected)});
}

// ============================================================================
// Write Multiple Registers (FC 0x10)
// ============================================================================

core::Result<std::size_t, PduError> encode_write_multiple_registers_request(
    std::span<std::byte> out, WriteMultipleRegistersRequest req) noexcept {
    if (req.quantity == 0 || req.quantity > kMaxWriteRegisters) {
        return core::Err(PduError::InvalidQuantity);
    }
    const std::size_t expected_bytes = static_cast<std::size_t>(req.quantity) * 2u;
    if (req.register_bytes.size() != expected_bytes) {
        return core::Err(PduError::InvalidByteCount);
    }
    const std::size_t total = 6 + expected_bytes;
    if (out.size() < total) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(FunctionCode::WriteMultipleRegisters)};
    out[1] = hi(req.address);
    out[2] = lo(req.address);
    out[3] = hi(req.quantity);
    out[4] = lo(req.quantity);
    out[5] = std::byte{static_cast<std::uint8_t>(expected_bytes)};
    for (std::size_t i = 0; i < expected_bytes; ++i) out[6 + i] = req.register_bytes[i];
    return core::Ok(std::size_t{total});
}

core::Result<WriteMultipleRegistersRequest, PduError> decode_write_multiple_registers_request(
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() < 6) return core::Err(PduError::Truncated);
    if (static_cast<std::uint8_t>(pdu[0]) !=
        static_cast<std::uint8_t>(FunctionCode::WriteMultipleRegisters)) {
        return core::Err(PduError::UnknownFunction);
    }
    const std::uint16_t quantity = be16(pdu, 3);
    if (quantity == 0 || quantity > kMaxWriteRegisters) {
        return core::Err(PduError::InvalidQuantity);
    }
    const auto byte_count = static_cast<std::uint8_t>(pdu[5]);
    const std::size_t expected = static_cast<std::size_t>(quantity) * 2u;
    if (byte_count != expected || pdu.size() != 6 + expected) {
        return core::Err(PduError::InvalidByteCount);
    }
    return core::Ok(
        WriteMultipleRegistersRequest{be16(pdu, 1), quantity, pdu.subspan(6, expected)});
}

// ============================================================================
// Read/Write Multiple Registers (FC 0x17)
// ============================================================================

core::Result<std::size_t, PduError> encode_read_write_multiple_registers_request(
    std::span<std::byte> out, ReadWriteMultipleRegistersRequest req) noexcept {
    if (req.read_quantity == 0 || req.read_quantity > kMaxReadWriteReadRegisters) {
        return core::Err(PduError::InvalidQuantity);
    }
    if (req.write_quantity == 0 || req.write_quantity > kMaxReadWriteWriteRegisters) {
        return core::Err(PduError::InvalidQuantity);
    }
    const std::size_t expected_w = static_cast<std::size_t>(req.write_quantity) * 2u;
    if (req.write_register_bytes.size() != expected_w) {
        return core::Err(PduError::InvalidByteCount);
    }
    const std::size_t total = 10 + expected_w;
    if (out.size() < total) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(FunctionCode::ReadWriteMultipleRegisters)};
    out[1] = hi(req.read_address);
    out[2] = lo(req.read_address);
    out[3] = hi(req.read_quantity);
    out[4] = lo(req.read_quantity);
    out[5] = hi(req.write_address);
    out[6] = lo(req.write_address);
    out[7] = hi(req.write_quantity);
    out[8] = lo(req.write_quantity);
    out[9] = std::byte{static_cast<std::uint8_t>(expected_w)};
    for (std::size_t i = 0; i < expected_w; ++i) out[10 + i] = req.write_register_bytes[i];
    return core::Ok(std::size_t{total});
}

core::Result<ReadWriteMultipleRegistersRequest, PduError>
decode_read_write_multiple_registers_request(std::span<const std::byte> pdu) noexcept {
    if (pdu.size() < 10) return core::Err(PduError::Truncated);
    if (static_cast<std::uint8_t>(pdu[0]) !=
        static_cast<std::uint8_t>(FunctionCode::ReadWriteMultipleRegisters)) {
        return core::Err(PduError::UnknownFunction);
    }
    const std::uint16_t read_qty = be16(pdu, 3);
    const std::uint16_t write_qty = be16(pdu, 7);
    if (read_qty == 0 || read_qty > kMaxReadWriteReadRegisters) {
        return core::Err(PduError::InvalidQuantity);
    }
    if (write_qty == 0 || write_qty > kMaxReadWriteWriteRegisters) {
        return core::Err(PduError::InvalidQuantity);
    }
    const auto byte_count = static_cast<std::uint8_t>(pdu[9]);
    const std::size_t expected_w = static_cast<std::size_t>(write_qty) * 2u;
    if (byte_count != expected_w || pdu.size() != 10 + expected_w) {
        return core::Err(PduError::InvalidByteCount);
    }
    return core::Ok(ReadWriteMultipleRegistersRequest{be16(pdu, 1), read_qty, be16(pdu, 5),
                                                      write_qty, pdu.subspan(10, expected_w)});
}

// ============================================================================
// Read responses (FC 0x01/0x02/0x03/0x04 + 0x17)
// ============================================================================

core::Result<std::size_t, PduError> encode_read_registers_response(
    std::span<std::byte> out, FunctionCode fc,
    std::span<const std::byte> register_bytes) noexcept {
    if (!is_register_read(fc) && fc != FunctionCode::ReadWriteMultipleRegisters) {
        return core::Err(PduError::UnknownFunction);
    }
    if ((register_bytes.size() % 2u) != 0u) return core::Err(PduError::InvalidByteCount);
    if (register_bytes.size() > 250u) return core::Err(PduError::InvalidByteCount);
    const std::size_t total = 2 + register_bytes.size();
    if (out.size() < total) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(fc)};
    out[1] = std::byte{static_cast<std::uint8_t>(register_bytes.size())};
    for (std::size_t i = 0; i < register_bytes.size(); ++i) out[2 + i] = register_bytes[i];
    return core::Ok(std::size_t{total});
}

core::Result<ReadRegistersResponse, PduError> decode_read_registers_response(
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() < 2) return core::Err(PduError::Truncated);
    auto fc_result = peek_function_code(pdu);
    if (fc_result.is_err()) return core::Err(PduError{fc_result.unwrap_err()});
    const auto fc = fc_result.unwrap();
    if (!is_register_read(fc) && fc != FunctionCode::ReadWriteMultipleRegisters) {
        return core::Err(PduError::UnknownFunction);
    }
    const auto byte_count = static_cast<std::uint8_t>(pdu[1]);
    if ((byte_count % 2u) != 0u) return core::Err(PduError::InvalidByteCount);
    if (pdu.size() != static_cast<std::size_t>(2u) + byte_count) {
        return core::Err(PduError::InvalidByteCount);
    }
    return core::Ok(ReadRegistersResponse{pdu.subspan(2, byte_count)});
}

core::Result<std::size_t, PduError> encode_read_coils_response(
    std::span<std::byte> out, FunctionCode fc,
    std::span<const std::byte> packed_bits) noexcept {
    if (!is_coil_read(fc)) return core::Err(PduError::UnknownFunction);
    if (packed_bits.size() > 250u) return core::Err(PduError::InvalidByteCount);
    const std::size_t total = 2 + packed_bits.size();
    if (out.size() < total) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(fc)};
    out[1] = std::byte{static_cast<std::uint8_t>(packed_bits.size())};
    for (std::size_t i = 0; i < packed_bits.size(); ++i) out[2 + i] = packed_bits[i];
    return core::Ok(std::size_t{total});
}

core::Result<ReadCoilsResponse, PduError> decode_read_coils_response(
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() < 2) return core::Err(PduError::Truncated);
    auto fc_result = peek_function_code(pdu);
    if (fc_result.is_err()) return core::Err(PduError{fc_result.unwrap_err()});
    if (!is_coil_read(fc_result.unwrap())) return core::Err(PduError::UnknownFunction);
    const auto byte_count = static_cast<std::uint8_t>(pdu[1]);
    if (pdu.size() != static_cast<std::size_t>(2u) + byte_count) {
        return core::Err(PduError::InvalidByteCount);
    }
    return core::Ok(ReadCoilsResponse{pdu.subspan(2, byte_count)});
}

// ============================================================================
// Write Single / Multiple responses
// ============================================================================

core::Result<std::size_t, PduError> encode_write_single_response(
    std::span<std::byte> out, FunctionCode fc, WriteSingleResponse resp) noexcept {
    if (fc != FunctionCode::WriteSingleCoil && fc != FunctionCode::WriteSingleRegister) {
        return core::Err(PduError::UnknownFunction);
    }
    if (out.size() < 5) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(fc)};
    out[1] = hi(resp.address);
    out[2] = lo(resp.address);
    out[3] = hi(resp.value);
    out[4] = lo(resp.value);
    return core::Ok(std::size_t{5});
}

core::Result<WriteSingleResponse, PduError> decode_write_single_response(
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() != 5) return core::Err(PduError::Truncated);
    auto fc_result = peek_function_code(pdu);
    if (fc_result.is_err()) return core::Err(PduError{fc_result.unwrap_err()});
    const auto fc = fc_result.unwrap();
    if (fc != FunctionCode::WriteSingleCoil && fc != FunctionCode::WriteSingleRegister) {
        return core::Err(PduError::UnknownFunction);
    }
    return core::Ok(WriteSingleResponse{be16(pdu, 1), be16(pdu, 3)});
}

core::Result<std::size_t, PduError> encode_write_multiple_response(
    std::span<std::byte> out, FunctionCode fc, WriteMultipleResponse resp) noexcept {
    if (fc != FunctionCode::WriteMultipleCoils && fc != FunctionCode::WriteMultipleRegisters) {
        return core::Err(PduError::UnknownFunction);
    }
    if (out.size() < 5) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(fc)};
    out[1] = hi(resp.address);
    out[2] = lo(resp.address);
    out[3] = hi(resp.quantity);
    out[4] = lo(resp.quantity);
    return core::Ok(std::size_t{5});
}

core::Result<WriteMultipleResponse, PduError> decode_write_multiple_response(
    std::span<const std::byte> pdu) noexcept {
    if (pdu.size() != 5) return core::Err(PduError::Truncated);
    auto fc_result = peek_function_code(pdu);
    if (fc_result.is_err()) return core::Err(PduError{fc_result.unwrap_err()});
    const auto fc = fc_result.unwrap();
    if (fc != FunctionCode::WriteMultipleCoils && fc != FunctionCode::WriteMultipleRegisters) {
        return core::Err(PduError::UnknownFunction);
    }
    return core::Ok(WriteMultipleResponse{be16(pdu, 1), be16(pdu, 3)});
}

// ============================================================================
// Exceptions
// ============================================================================

core::Result<std::size_t, PduError> encode_exception(
    std::span<std::byte> out, FunctionCode fc, ExceptionCode ec) noexcept {
    if (out.size() < kExceptionPduBytes) return core::Err(PduError::BufferTooSmall);
    out[0] = std::byte{static_cast<std::uint8_t>(static_cast<std::uint8_t>(fc) | 0x80u)};
    out[1] = std::byte{static_cast<std::uint8_t>(ec)};
    return core::Ok(std::size_t{kExceptionPduBytes});
}

core::Result<ExceptionPdu, PduError> decode_exception(std::span<const std::byte> pdu) noexcept {
    if (pdu.size() != kExceptionPduBytes) return core::Err(PduError::Truncated);
    const auto raw_fc = static_cast<std::uint8_t>(pdu[0]);
    if ((raw_fc & 0x80u) == 0u) return core::Err(PduError::BadFraming);
    const std::uint8_t fc_bits = raw_fc & 0x7Fu;
    if (!is_supported_function(fc_bits)) return core::Err(PduError::UnknownFunction);
    return core::Ok(
        ExceptionPdu{static_cast<FunctionCode>(fc_bits),
                     static_cast<ExceptionCode>(static_cast<std::uint8_t>(pdu[1]))});
}

}  // namespace alloy::modbus
