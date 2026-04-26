#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_SPI_HEADER
    #error "spi_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_SPI_HEADER

#ifdef BOARD_UART_HEADER
    #include BOARD_UART_HEADER
#endif

#include "examples/common/uart_console.hpp"
#include "hal/gpio.hpp"
#include "hal/systick.hpp"

namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

}  // namespace

int main() {
    board::init();

#ifdef BOARD_UART_HEADER
    auto uart = board::make_debug_uart();
    const auto uart_ready = uart.configure().is_ok();
#else
    constexpr auto uart_ready = false;
#endif

    auto bus = board::make_spi(alloy::hal::spi::Config{
        alloy::hal::SpiMode::Mode0,
        1'000'000u,
        alloy::hal::SpiBitOrder::MsbFirst,
        alloy::hal::SpiDataSize::Bits8,
        board::kBoardSpiPeripheralClockHz,
    });
    if (const auto result = bus.configure(); result.is_err()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "spi configure failed");
        }
#endif
        blink_error(200);
    }

#ifdef BOARD_UART_HEADER
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "spi probe ready");
    }
#endif

    // Drive PD25 low as plain GPIO to assert W25Q /CS (no CsPolicy abstraction).
    using CsPin = alloy::hal::gpio::pin_handle<
        alloy::hal::gpio::pin<alloy::device::PinId::PD25>>;
    CsPin cs{{.direction     = alloy::hal::PinDirection::Output,
              .initial_state = alloy::hal::PinState::High}};
    (void)cs.configure();

    // Send JEDEC-ID command: 0x9F followed by 3 dummy bytes.
    // W25Q128 expected response: EF 40 18.
    (void)cs.set_low();
    std::array<std::uint8_t, 4> tx{0x9F, 0x00, 0x00, 0x00};
    std::array<std::uint8_t, 4> rx{};
    const auto result = bus.transfer(tx, rx);
    (void)cs.set_high();

#ifdef BOARD_UART_HEADER
    if (uart_ready) {
        if (result.is_err()) {
            alloy::examples::uart_console::write_line(uart, "transfer err");
        } else {
            alloy::examples::uart_console::write_text(uart, "jedec: ");
            alloy::examples::uart_console::write_hex_byte(uart, rx[1]);
            alloy::examples::uart_console::write_text(uart, " ");
            alloy::examples::uart_console::write_hex_byte(uart, rx[2]);
            alloy::examples::uart_console::write_text(uart, " ");
            alloy::examples::uart_console::write_hex_byte(uart, rx[3]);
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
    }
#endif

    blink_error(500);
}
