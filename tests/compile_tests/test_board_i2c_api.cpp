#include <array>
#include <cstdint>

#include "board.hpp"

namespace {

[[maybe_unused]] void compile_probe_board_i2c_api() {
    auto bus = board::make_i2c();
    std::array<std::uint8_t, 2> write_buffer{0x00u, 0x55u};
    std::array<std::uint8_t, 4> read_buffer{};
    std::array<std::uint8_t, 16> found_devices{};

    [[maybe_unused]] const auto configure_result = bus.configure();
    [[maybe_unused]] const auto write_result = bus.write(0x50u, write_buffer);
    [[maybe_unused]] const auto read_result = bus.read(0x50u, read_buffer);
    [[maybe_unused]] const auto write_read_result = bus.write_read(0x50u, write_buffer, read_buffer);
    [[maybe_unused]] const auto scan_result = bus.scan_bus(found_devices);
}

}  // namespace
