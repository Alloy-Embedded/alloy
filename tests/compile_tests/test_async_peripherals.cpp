// Compile test: every async_<peripheral>.hpp wrapper exposes the documented
// signature, returns `core::Result<operation<…token>, core::ErrorCode>`, and
// composes against a duck-typed mock handle. Vendor-specific ISR wiring is
// out of scope for this test — it lives in the per-board backend.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

// Pulls in every async_*.hpp header through src/async.hpp.
#include "async.hpp"

namespace {

using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

// ── async SPI ────────────────────────────────────────────────────────────────
#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
struct MockTxDmaChannel {
    static constexpr auto valid = true;
    static constexpr auto peripheral_id = alloy::hal::dma::PeripheralId::USART1;
    static constexpr auto signal_id = alloy::hal::dma::SignalId::signal_TX;
};
struct MockRxDmaChannel {
    static constexpr auto valid = true;
    static constexpr auto peripheral_id = alloy::hal::dma::PeripheralId::USART1;
    static constexpr auto signal_id = alloy::hal::dma::SignalId::signal_RX;
};

struct MockSpiPort {
    [[nodiscard]] auto write_dma(const MockTxDmaChannel&, std::span<const std::byte>) const -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto read_dma(const MockRxDmaChannel&, std::span<std::byte>) const -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto transfer_dma(const MockTxDmaChannel&, const MockRxDmaChannel&,
                                    std::span<const std::byte>, std::span<std::byte>) const -> ResultVoid {
        return alloy::core::Ok();
    }
};
#endif

// ── async I2C / ADC / Timer / GPIO mocks (interrupt-driven, no DMA shape) ────
struct MockI2cPort {
    [[nodiscard]] auto write_async(std::uint16_t, std::span<const std::uint8_t>) const -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto read_async(std::uint16_t, std::span<std::uint8_t>) const -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto write_read_async(std::uint16_t, std::span<const std::uint8_t>,
                                        std::span<std::uint8_t>) const -> ResultVoid {
        return alloy::core::Ok();
    }
};

struct MockAdcPort {
    [[nodiscard]] auto read_async() const -> ResultVoid { return alloy::core::Ok(); }
    template <typename Channel>
    [[nodiscard]] auto scan_dma_async(const Channel&, std::span<std::uint16_t>) const -> ResultVoid {
        return alloy::core::Ok();
    }
};

struct MockTimerPort {
    [[nodiscard]] auto arm_period_async() const -> ResultVoid { return alloy::core::Ok(); }
    [[nodiscard]] auto arm_oneshot_async(alloy::runtime::time::Duration) const -> ResultVoid {
        return alloy::core::Ok();
    }
};

struct MockGpioPort {
    template <alloy::device::PinId>
    [[nodiscard]] auto arm_edge_async(alloy::async::gpio::Edge) const -> ResultVoid {
        return alloy::core::Ok();
    }
};

// ── async UART wait_for (interrupt-driven) ───────────────────────────────────
struct MockUartPort {
    // peripheral_id must be constexpr for uart_event::token<P, Kind>.
    static constexpr auto peripheral_id = alloy::device::PeripheralId::none;

    [[nodiscard]] auto enable_interrupt(alloy::hal::uart::InterruptKind) const -> ResultVoid {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto disable_interrupt(alloy::hal::uart::InterruptKind) const -> ResultVoid {
        return alloy::core::Ok();
    }
};

}  // namespace

[[maybe_unused]] void compile_async_peripherals_api() {
#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
    MockSpiPort spi_port;
    MockTxDmaChannel tx_ch;
    MockRxDmaChannel rx_ch;
    std::array<std::byte, 4> tx_buf{};
    std::array<std::byte, 4> rx_buf{};

    [[maybe_unused]] const auto spi_w =
        alloy::async::spi::write_dma(spi_port, tx_ch, std::span<const std::byte>{tx_buf});
    [[maybe_unused]] const auto spi_r = alloy::async::spi::read_dma(spi_port, rx_ch, rx_buf);
    [[maybe_unused]] const auto spi_xfer = alloy::async::spi::transfer_dma(
        spi_port, tx_ch, rx_ch, std::span<const std::byte>{tx_buf}, rx_buf);
#endif

    MockI2cPort i2c_port;
    std::array<std::uint8_t, 4> i2c_tx{};
    std::array<std::uint8_t, 4> i2c_rx{};
    [[maybe_unused]] const auto i2c_w =
        alloy::async::i2c::write<alloy::device::PeripheralId::none>(
            i2c_port, 0x42u, std::span<const std::uint8_t>{i2c_tx});
    [[maybe_unused]] const auto i2c_r =
        alloy::async::i2c::read<alloy::device::PeripheralId::none>(
            i2c_port, 0x42u, std::span<std::uint8_t>{i2c_rx});
    [[maybe_unused]] const auto i2c_wr =
        alloy::async::i2c::write_read<alloy::device::PeripheralId::none>(
            i2c_port, 0x42u, std::span<const std::uint8_t>{i2c_tx},
            std::span<std::uint8_t>{i2c_rx});

    MockAdcPort adc_port;
    [[maybe_unused]] const auto adc_r =
        alloy::async::adc::read<alloy::device::PeripheralId::none>(adc_port);

    MockTimerPort tim_port;
    [[maybe_unused]] const auto tim_p =
        alloy::async::timer::wait_period<alloy::device::PeripheralId::none>(tim_port);
    [[maybe_unused]] const auto tim_d =
        alloy::async::timer::delay<alloy::device::PeripheralId::none>(
            tim_port, alloy::runtime::time::Duration::from_millis(10));

    MockGpioPort gpio_port;
    [[maybe_unused]] const auto gpio_e =
        alloy::async::gpio::wait_edge<alloy::device::PinId::none>(gpio_port,
                                                                   alloy::async::gpio::Edge::Rising);

    // ── async::uart::wait_for ────────────────────────────────────────────────
    // Pins the signature: wait_for<Kind>(port) ->
    //   Result<operation<uart_event::token<P, Kind>>, ErrorCode>.
    // The IdleLine kind is the canonical "unknown-length frame" trigger used
    // with circular RX DMA (see openspec extend-uart-coverage task 5.1 + 9.2).
    MockUartPort uart_port;
    [[maybe_unused]] const auto uart_idle =
        alloy::async::uart::wait_for<alloy::hal::uart::InterruptKind::IdleLine>(uart_port);
    // Also verify LinBreak kind compiles (task 4.2 — F401 LIN path coverage).
    [[maybe_unused]] const auto uart_lin =
        alloy::async::uart::wait_for<alloy::hal::uart::InterruptKind::LinBreak>(uart_port);
}
