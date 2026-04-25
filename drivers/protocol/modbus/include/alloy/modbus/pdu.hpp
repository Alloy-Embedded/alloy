#pragma once

// Modbus PDU encode/decode -- pure logic, no I/O.
//
// The PDU is the function-code byte plus a payload. RTU and TCP framers wrap PDUs
// with their own headers and trailers; the codec here neither knows nor cares.
// Every encoder writes into a caller-provided buffer; every decoder consumes a
// caller-provided byte view. No allocation. No exceptions.
//
// Coverage matches the Modbus Application Protocol Specification v1.1b3:
//   FC 0x01 Read Coils
//   FC 0x02 Read Discrete Inputs
//   FC 0x03 Read Holding Registers
//   FC 0x04 Read Input Registers
//   FC 0x05 Write Single Coil
//   FC 0x06 Write Single Register
//   FC 0x0F Write Multiple Coils
//   FC 0x10 Write Multiple Registers
//   FC 0x17 Read/Write Multiple Registers
// Plus exception responses for any function code.
//
// Quantities are bounded by the spec: reads of registers <= 125, writes of
// registers <= 123, coil reads <= 2000, coil writes <= 1968, read+write 0x17
// reads <= 125 and writes <= 121.

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"

namespace alloy::modbus {

enum class FunctionCode : std::uint8_t {
    ReadCoils = 0x01,
    ReadDiscreteInputs = 0x02,
    ReadHoldingRegisters = 0x03,
    ReadInputRegisters = 0x04,
    WriteSingleCoil = 0x05,
    WriteSingleRegister = 0x06,
    WriteMultipleCoils = 0x0F,
    WriteMultipleRegisters = 0x10,
    ReadWriteMultipleRegisters = 0x17,
};

enum class ExceptionCode : std::uint8_t {
    IllegalFunction = 0x01,
    IllegalDataAddress = 0x02,
    IllegalDataValue = 0x03,
    SlaveDeviceFailure = 0x04,
    Acknowledge = 0x05,
    SlaveDeviceBusy = 0x06,
    NegativeAcknowledge = 0x07,
    MemoryParityError = 0x08,
    GatewayPathUnavailable = 0x0A,
    GatewayTargetDeviceFailedToRespond = 0x0B,
};

enum class PduError : std::uint8_t {
    BufferTooSmall,    // caller-supplied output buffer cannot hold the PDU
    Truncated,         // input bytes ended before the PDU was complete
    UnknownFunction,   // function code is outside the implemented set
    InvalidByteCount,  // explicit byte_count does not match payload length
    InvalidQuantity,   // quantity field is zero or above the spec maximum
    InvalidValue,      // a constrained field has a forbidden value (e.g. coil != 0xFF00/0x0000)
    BadFraming,        // structural error not covered by another category
};

// --- spec limits ---------------------------------------------------------------------

inline constexpr std::uint16_t kMaxReadCoils = 2000;
inline constexpr std::uint16_t kMaxReadDiscreteInputs = 2000;
inline constexpr std::uint16_t kMaxReadRegisters = 125;
inline constexpr std::uint16_t kMaxWriteCoils = 1968;
inline constexpr std::uint16_t kMaxWriteRegisters = 123;
inline constexpr std::uint16_t kMaxReadWriteReadRegisters = 125;
inline constexpr std::uint16_t kMaxReadWriteWriteRegisters = 121;

// Maximum number of bytes a Modbus PDU can occupy on the wire (function code +
// payload). The spec caps the ADU at 256 bytes; subtract slave id + CRC for RTU.
inline constexpr std::size_t kMaxPduBytes = 253;

// --- typed request / response views --------------------------------------------------

struct ReadRequest {
    std::uint16_t address;
    std::uint16_t quantity;
};

struct WriteSingleCoilRequest {
    std::uint16_t address;
    bool value;
};

struct WriteSingleRegisterRequest {
    std::uint16_t address;
    std::uint16_t value;
};

struct WriteMultipleCoilsRequest {
    std::uint16_t address;
    std::uint16_t quantity;
    std::span<const std::byte> packed_bits;  // spec-packed coil values, ceil(quantity/8) bytes
};

struct WriteMultipleRegistersRequest {
    std::uint16_t address;
    std::uint16_t quantity;
    std::span<const std::byte> register_bytes;  // big-endian, quantity*2 bytes
};

struct ReadWriteMultipleRegistersRequest {
    std::uint16_t read_address;
    std::uint16_t read_quantity;
    std::uint16_t write_address;
    std::uint16_t write_quantity;
    std::span<const std::byte> write_register_bytes;
};

struct ReadRegistersResponse {
    std::span<const std::byte> register_bytes;  // big-endian, quantity*2 bytes
};

struct ReadCoilsResponse {
    std::span<const std::byte> packed_bits;
};

struct WriteSingleResponse {
    std::uint16_t address;
    std::uint16_t value;
};

struct WriteMultipleResponse {
    std::uint16_t address;
    std::uint16_t quantity;
};

// --- helpers -------------------------------------------------------------------------

// Returns true if `fc` is one of the standard function codes the codec implements.
[[nodiscard]] constexpr bool is_supported_function(std::uint8_t fc) noexcept {
    switch (static_cast<FunctionCode>(fc)) {
        case FunctionCode::ReadCoils:
        case FunctionCode::ReadDiscreteInputs:
        case FunctionCode::ReadHoldingRegisters:
        case FunctionCode::ReadInputRegisters:
        case FunctionCode::WriteSingleCoil:
        case FunctionCode::WriteSingleRegister:
        case FunctionCode::WriteMultipleCoils:
        case FunctionCode::WriteMultipleRegisters:
        case FunctionCode::ReadWriteMultipleRegisters:
            return true;
    }
    return false;
}

// --- request encoders (master side) --------------------------------------------------

core::Result<std::size_t, PduError> encode_read_request(
    std::span<std::byte> out, FunctionCode fc, ReadRequest req) noexcept;

core::Result<std::size_t, PduError> encode_write_single_coil_request(
    std::span<std::byte> out, WriteSingleCoilRequest req) noexcept;

core::Result<std::size_t, PduError> encode_write_single_register_request(
    std::span<std::byte> out, WriteSingleRegisterRequest req) noexcept;

core::Result<std::size_t, PduError> encode_write_multiple_coils_request(
    std::span<std::byte> out, WriteMultipleCoilsRequest req) noexcept;

core::Result<std::size_t, PduError> encode_write_multiple_registers_request(
    std::span<std::byte> out, WriteMultipleRegistersRequest req) noexcept;

core::Result<std::size_t, PduError> encode_read_write_multiple_registers_request(
    std::span<std::byte> out, ReadWriteMultipleRegistersRequest req) noexcept;

// --- request decoders (slave side) ---------------------------------------------------

// Generic dispatch helper: returns the FC byte, or UnknownFunction if outside the
// implemented set. Useful for slave loops that switch on FC before calling the
// typed decoder.
[[nodiscard]] core::Result<FunctionCode, PduError> peek_function_code(
    std::span<const std::byte> pdu) noexcept;

core::Result<ReadRequest, PduError> decode_read_request(
    std::span<const std::byte> pdu) noexcept;

core::Result<WriteSingleCoilRequest, PduError> decode_write_single_coil_request(
    std::span<const std::byte> pdu) noexcept;

core::Result<WriteSingleRegisterRequest, PduError> decode_write_single_register_request(
    std::span<const std::byte> pdu) noexcept;

core::Result<WriteMultipleCoilsRequest, PduError> decode_write_multiple_coils_request(
    std::span<const std::byte> pdu) noexcept;

core::Result<WriteMultipleRegistersRequest, PduError> decode_write_multiple_registers_request(
    std::span<const std::byte> pdu) noexcept;

core::Result<ReadWriteMultipleRegistersRequest, PduError>
decode_read_write_multiple_registers_request(std::span<const std::byte> pdu) noexcept;

// --- response encoders (slave side) --------------------------------------------------

core::Result<std::size_t, PduError> encode_read_registers_response(
    std::span<std::byte> out, FunctionCode fc,
    std::span<const std::byte> register_bytes) noexcept;

core::Result<std::size_t, PduError> encode_read_coils_response(
    std::span<std::byte> out, FunctionCode fc,
    std::span<const std::byte> packed_bits) noexcept;

core::Result<std::size_t, PduError> encode_write_single_response(
    std::span<std::byte> out, FunctionCode fc, WriteSingleResponse resp) noexcept;

core::Result<std::size_t, PduError> encode_write_multiple_response(
    std::span<std::byte> out, FunctionCode fc, WriteMultipleResponse resp) noexcept;

// --- response decoders (master side) -------------------------------------------------

core::Result<ReadRegistersResponse, PduError> decode_read_registers_response(
    std::span<const std::byte> pdu) noexcept;

core::Result<ReadCoilsResponse, PduError> decode_read_coils_response(
    std::span<const std::byte> pdu) noexcept;

core::Result<WriteSingleResponse, PduError> decode_write_single_response(
    std::span<const std::byte> pdu) noexcept;

core::Result<WriteMultipleResponse, PduError> decode_write_multiple_response(
    std::span<const std::byte> pdu) noexcept;

// --- exception responses -------------------------------------------------------------

// Exception PDUs are always 2 bytes: (fc | 0x80), exception_code.
inline constexpr std::size_t kExceptionPduBytes = 2;

core::Result<std::size_t, PduError> encode_exception(
    std::span<std::byte> out, FunctionCode fc, ExceptionCode ec) noexcept;

struct ExceptionPdu {
    FunctionCode function;  // original function code (high bit cleared)
    ExceptionCode code;
};

// Returns Ok(ExceptionPdu) when the high bit of the FC is set; Err(BadFraming)
// when this PDU is a normal (non-exception) response.
core::Result<ExceptionPdu, PduError> decode_exception(
    std::span<const std::byte> pdu) noexcept;

// True if this PDU's function-code byte has the high bit set. Useful for the
// master to dispatch to `decode_exception` vs the typed decoders.
[[nodiscard]] bool is_exception(std::span<const std::byte> pdu) noexcept;

}  // namespace alloy::modbus
