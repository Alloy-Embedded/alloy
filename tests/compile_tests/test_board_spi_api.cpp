#include <array>
#include <cstdint>

#include "board.hpp"

namespace {

[[maybe_unused]] void compile_probe_board_spi_api() {
    auto bus = board::make_spi();
    std::array<std::uint8_t, 4> tx_buffer{0x9Fu, 0x00u, 0x00u, 0x00u};
    std::array<std::uint8_t, 4> rx_buffer{};

    [[maybe_unused]] const auto configure_result = bus.configure();
    [[maybe_unused]] const auto transfer_result = bus.transfer(tx_buffer, rx_buffer);
    [[maybe_unused]] const auto transmit_result = bus.transmit(tx_buffer);
    [[maybe_unused]] const auto receive_result = bus.receive(rx_buffer);
    [[maybe_unused]] const auto busy = bus.is_busy();
}

}  // namespace
