// Compile test: TcpStream satisfies modbus::ByteStream.
// Uses a mock TcpStream (no lwIP dependency) to verify the concept check
// in tcp_stream.hpp is achievable without hardware or TCP stack.

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "drivers/protocol/modbus/include/alloy/modbus/byte_stream.hpp"

namespace {

// Mock TcpStream that satisfies ByteStream without needing lwIP.
struct MockTcpStream {
    [[nodiscard]] alloy::core::Result<std::size_t, alloy::modbus::StreamError>
    read(std::span<std::byte> /*buf*/, std::uint32_t /*timeout_us*/) noexcept {
        return alloy::core::Ok(std::size_t{0u});
    }
    [[nodiscard]] alloy::core::Result<void, alloy::modbus::StreamError>
    write(std::span<const std::byte> /*buf*/) noexcept {
        return alloy::core::Ok();
    }
    [[nodiscard]] alloy::core::Result<void, alloy::modbus::StreamError>
    flush(std::uint32_t /*timeout_us*/) noexcept {
        return alloy::core::Ok();
    }
    [[nodiscard]] alloy::core::Result<void, alloy::modbus::StreamError>
    wait_idle(std::uint32_t /*silence_us*/) noexcept {
        return alloy::core::Ok();
    }
};

static_assert(alloy::modbus::ByteStream<MockTcpStream>);

[[maybe_unused]] void compile_mock_tcp_stream_usage() {
    MockTcpStream stream;
    std::byte buf[64]{};
    (void)stream.read(std::span{buf}, 1'000u);
    (void)stream.write(std::span{static_cast<const std::byte*>(buf), 64u});
    (void)stream.flush(1'000u);
    (void)stream.wait_idle(3'500u);
}

}  // namespace
