#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/spi.hpp"

#include "device/traits.hpp"

namespace {

constexpr auto spi_peripheral_clock_hz() -> std::uint32_t {
#if defined(ALLOY_BOARD_NUCLEO_G071RB)
    return 64'000'000u;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    return 84'000'000u;
#elif defined(ALLOY_BOARD_SAME70_XPLD)
    return 12'000'000u;
#else
    return 0u;
#endif
}

template <typename SpiHandle>
consteval auto spi_is_usable() -> bool {
    if constexpr (!SpiHandle::valid) {
        return false;
    }

    return !SpiHandle::peripheral_name.empty() && SpiHandle::base_address() != 0u &&
           SpiHandle::operations().size() >= 3;
}

template <typename SpiHandle>
void exercise_spi_backend() {
    auto port = alloy::hal::spi::open<typename SpiHandle::connector_type>(alloy::hal::spi::Config{
        alloy::hal::SpiMode::Mode0, 1'000'000u, alloy::hal::SpiBitOrder::MsbFirst,
        alloy::hal::SpiDataSize::Bits8, spi_peripheral_clock_hz()});

    std::array<std::uint8_t, 4> tx_buffer{0x9Fu, 0x00u, 0x00u, 0x00u};
    std::array<std::uint8_t, 4> rx_buffer{};

    [[maybe_unused]] const auto configure_result = port.configure();
    [[maybe_unused]] const auto transfer_result = port.transfer(tx_buffer, rx_buffer);
    [[maybe_unused]] const auto transmit_result = port.transmit(tx_buffer);
    [[maybe_unused]] const auto receive_result = port.receive(rx_buffer);
    [[maybe_unused]] const auto busy = port.is_busy();

    // ── extend-spi-coverage: Phase 1 — variable data size / clock speed ─────
    [[maybe_unused]] const auto data_size_8 = port.set_data_size(8u);
    [[maybe_unused]] const auto data_size_16 = port.set_data_size(16u);
    [[maybe_unused]] const auto data_size_static_8 =
        port.template set_data_size_static<8u>();
    [[maybe_unused]] const auto clock_speed = port.set_clock_speed(1'000'000u);
    [[maybe_unused]] const std::uint32_t realised = port.realised_clock_speed();
    [[maybe_unused]] const std::uint32_t kernel_hz = port.kernel_clock_hz();

    // ── extend-spi-coverage: Phase 2 — frame format / CRC / bidi / NSS ──────
    [[maybe_unused]] const auto frame_motorola =
        port.set_frame_format(alloy::hal::spi::FrameFormat::Motorola);
    [[maybe_unused]] const auto frame_ti =
        port.set_frame_format(alloy::hal::spi::FrameFormat::TI);
    [[maybe_unused]] const auto crc_enable = port.enable_crc(true);
    [[maybe_unused]] const auto crc_poly = port.set_crc_polynomial(0x1021u);
    [[maybe_unused]] const std::uint16_t crc_value = port.read_crc();
    [[maybe_unused]] const bool crc_err = port.crc_error();
    [[maybe_unused]] const auto crc_clear = port.clear_crc_error();
    [[maybe_unused]] const auto bidi_on = port.set_bidirectional(true);
    [[maybe_unused]] const auto bidi_dir =
        port.set_bidirectional_direction(alloy::hal::spi::BiDir::Receive);
    [[maybe_unused]] const auto nss_sw =
        port.set_nss_management(alloy::hal::spi::NssManagement::Software);
    [[maybe_unused]] const auto nss_hw_out =
        port.set_nss_management(alloy::hal::spi::NssManagement::HardwareOutput);
    [[maybe_unused]] const auto nss_pulse = port.set_nss_pulse_per_transfer(true);

    // ── extend-spi-coverage: Phase 3 — SAM-style per-CS timing ─────────────
    [[maybe_unused]] const auto cs_decoded = port.set_cs_decode_mode(false);
    [[maybe_unused]] const auto cs_dlybct =
        port.set_cs_delay_between_consecutive(std::uint16_t{0});
    [[maybe_unused]] const auto cs_dlybs =
        port.set_cs_delay_clock_to_active(std::uint16_t{0});
    [[maybe_unused]] const auto cs_dlybcs =
        port.set_cs_delay_active_to_clock(std::uint16_t{0});

    // ── extend-spi-coverage: Phase 4 — status flags + interrupts + IRQ ─────
    [[maybe_unused]] const bool tx_empty = port.tx_register_empty();
    [[maybe_unused]] const bool rx_ne = port.rx_register_not_empty();
    [[maybe_unused]] const bool busy_flag = port.busy();
    [[maybe_unused]] const bool modf = port.mode_fault();
    [[maybe_unused]] const auto modf_clear = port.clear_mode_fault();
    [[maybe_unused]] const bool fre = port.frame_format_error();
    [[maybe_unused]] const auto irq_txe =
        port.enable_interrupt(alloy::hal::spi::InterruptKind::Txe);
    [[maybe_unused]] const auto irq_rxne =
        port.enable_interrupt(alloy::hal::spi::InterruptKind::Rxne);
    [[maybe_unused]] const auto irq_err =
        port.enable_interrupt(alloy::hal::spi::InterruptKind::Error);
    [[maybe_unused]] const auto irq_modf =
        port.enable_interrupt(alloy::hal::spi::InterruptKind::ModeFault);
    [[maybe_unused]] const auto irq_crc_err =
        port.enable_interrupt(alloy::hal::spi::InterruptKind::CrcError);
    [[maybe_unused]] const auto irq_fre =
        port.enable_interrupt(alloy::hal::spi::InterruptKind::FrameError);
    [[maybe_unused]] const auto disarm_txe =
        port.disable_interrupt(alloy::hal::spi::InterruptKind::Txe);
    [[maybe_unused]] const auto irq_list = SpiHandle::irq_numbers();
    static_assert(std::is_same_v<decltype(irq_list), const std::span<const std::uint32_t>>);
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI1,
    alloy::hal::connection::sck<alloy::device::PinId::PA5, alloy::device::SignalId::signal_sck>,
    alloy::hal::connection::miso<alloy::device::PinId::PA6,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PA7,
                                 alloy::device::SignalId::signal_mosi>>;
using SpiPort = decltype(alloy::hal::spi::open<SpiConnector>());
static_assert(SpiPort::valid);
static_assert(SpiPort::peripheral_name == std::string_view{"SPI1"});
static_assert(spi_is_usable<SpiPort>());
[[maybe_unused]] void compile_g071_spi_backend() {
    exercise_spi_backend<SpiPort>();
}
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI1,
    alloy::hal::connection::sck<alloy::device::PinId::PA5, alloy::device::SignalId::signal_sck>,
    alloy::hal::connection::miso<alloy::device::PinId::PA6,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PA7,
                                 alloy::device::SignalId::signal_mosi>>;
using SpiPort = decltype(alloy::hal::spi::open<SpiConnector>());
static_assert(SpiPort::valid);
static_assert(SpiPort::peripheral_name == std::string_view{"SPI1"});
static_assert(spi_is_usable<SpiPort>());
[[maybe_unused]] void compile_f401_spi_backend() {
    exercise_spi_backend<SpiPort>();
}
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI0,
    alloy::hal::connection::sck<alloy::device::PinId::PD22, alloy::device::SignalId::signal_spck>,
    alloy::hal::connection::miso<alloy::device::PinId::PD20,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PD21,
                                 alloy::device::SignalId::signal_mosi>>;
using SpiPort = decltype(alloy::hal::spi::open<SpiConnector>());
static_assert(SpiPort::valid);
static_assert(SpiPort::peripheral_name == std::string_view{"SPI0"});
static_assert(spi_is_usable<SpiPort>());
[[maybe_unused]] void compile_same70_spi_backend() {
    exercise_spi_backend<SpiPort>();
}
#endif
