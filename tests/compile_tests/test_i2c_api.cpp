#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/i2c.hpp"

#include "device/traits.hpp"

namespace {

constexpr auto i2c_peripheral_clock_hz() -> std::uint32_t {
#if defined(ALLOY_BOARD_NUCLEO_G071RB)
    return 64'000'000u;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    return 42'000'000u;
#elif defined(ALLOY_BOARD_SAME70_XPLD)
    return 12'000'000u;
#else
    return 0u;
#endif
}

template <typename I2cHandle>
consteval auto i2c_is_usable() -> bool {
    if constexpr (!I2cHandle::valid) {
        return false;
    }
    return !I2cHandle::peripheral_name.empty() && I2cHandle::base_address() != 0u &&
           I2cHandle::operations().size() >= 2;
}

template <typename I2cHandle>
void exercise_i2c_backend() {
    auto port = alloy::hal::i2c::open<typename I2cHandle::connector_type>(
        alloy::hal::i2c::Config{alloy::hal::I2cSpeed::Fast, alloy::hal::I2cAddressing::SevenBit,
                                i2c_peripheral_clock_hz()});

    std::array<std::uint8_t, 2> write_buffer{0x00u, 0x55u};
    std::array<std::uint8_t, 4> read_buffer{};
    std::array<std::uint8_t, 16> found_devices{};

    [[maybe_unused]] const auto configure_result   = port.configure();
    [[maybe_unused]] const auto write_result       = port.write(0x50u, write_buffer);
    [[maybe_unused]] const auto read_result        = port.read(0x50u, read_buffer);
    [[maybe_unused]] const auto write_read_result  =
        port.write_read(0x50u, write_buffer, read_buffer);
    [[maybe_unused]] const auto scan_result = port.scan_bus(found_devices);
}

template <typename I2cHandle>
void exercise_i2c_extended() {
    auto port = alloy::hal::i2c::open<typename I2cHandle::connector_type>(
        alloy::hal::i2c::Config{alloy::hal::I2cSpeed::Fast, alloy::hal::I2cAddressing::SevenBit,
                                i2c_peripheral_clock_hz()});

    // ---- task 1.1: set_clock_speed ----
    [[maybe_unused]] const auto spd_r  = port.set_clock_speed(400'000u);

    // ---- task 1.2: set_speed_mode ----
    using SM = alloy::hal::i2c::SpeedMode;
    [[maybe_unused]] const auto sm_std = port.set_speed_mode(SM::Standard100kHz);
    [[maybe_unused]] const auto sm_fst = port.set_speed_mode(SM::Fast400kHz);
    [[maybe_unused]] const auto sm_fp  = port.set_speed_mode(SM::FastPlus1MHz);

    // ---- task 1.3: set_duty_cycle ----
    using DC = alloy::hal::i2c::DutyCycle;
    [[maybe_unused]] const auto dc2   = port.set_duty_cycle(DC::Duty2);
    [[maybe_unused]] const auto dc169 = port.set_duty_cycle(DC::Duty169);

    // ---- task 1.4: set_addressing_mode ----
    using AM = alloy::hal::i2c::AddressingMode;
    [[maybe_unused]] const auto am7  = port.set_addressing_mode(AM::Bits7);
    [[maybe_unused]] const auto am10 = port.set_addressing_mode(AM::Bits10);

    // ---- task 1.5: own + dual address ----
    [[maybe_unused]] const auto oa  = port.set_own_address(0x42u, AM::Bits7);
    [[maybe_unused]] const auto oa2 = port.set_dual_address(0x43u);

    // ---- task 1.6: clock stretching ----
    [[maybe_unused]] const auto cs_on  = port.set_clock_stretching(true);
    [[maybe_unused]] const auto cs_off = port.set_clock_stretching(false);

    // ---- task 2.1: status + clear ----
    [[maybe_unused]] const bool nack  = port.nack_received();
    [[maybe_unused]] const bool arlo  = port.arbitration_lost();
    [[maybe_unused]] const bool berr  = port.bus_error();
    [[maybe_unused]] const auto cn    = port.clear_nack();
    [[maybe_unused]] const auto ca    = port.clear_arbitration_lost();
    [[maybe_unused]] const auto cb    = port.clear_bus_error();

    // ---- task 2.2: typed interrupts ----
    using K = alloy::hal::i2c::InterruptKind;
    [[maybe_unused]] const auto en_tx   = port.enable_interrupt(K::Tx);
    [[maybe_unused]] const auto en_rx   = port.enable_interrupt(K::Rx);
    [[maybe_unused]] const auto en_stop = port.enable_interrupt(K::Stop);
    [[maybe_unused]] const auto en_tc   = port.enable_interrupt(K::Tc);
    [[maybe_unused]] const auto en_nack = port.enable_interrupt(K::Nack);
    [[maybe_unused]] const auto dis_tx  = port.disable_interrupt(K::Tx);

    // ---- task 2.3: irq_numbers ----
    [[maybe_unused]] const auto irqs = I2cHandle::irq_numbers();
    static_assert(std::is_same_v<decltype(irqs), const std::span<const std::uint32_t>>);

    // ---- task 2.4: kernel clock source ----
    [[maybe_unused]] const auto kcs = port.set_kernel_clock_source(
        alloy::hal::i2c::KernelClockSource::Default);

    // ---- task 3.1: SMBus ----
    using SR = alloy::hal::i2c::SmbusRole;
    [[maybe_unused]] const auto sm_en  = port.enable_smbus(true);
    [[maybe_unused]] const auto sm_dis = port.enable_smbus(false);
    [[maybe_unused]] const auto sm_h   = port.set_smbus_role(SR::Host);
    [[maybe_unused]] const auto sm_d   = port.set_smbus_role(SR::Device);

    // ---- task 3.2: PEC ----
    [[maybe_unused]] const auto pec_en  = port.enable_pec(true);
    [[maybe_unused]] const auto pec_dis = port.enable_pec(false);
    [[maybe_unused]] const std::uint8_t lpec = port.last_pec();
    [[maybe_unused]] const bool pec_err       = port.pec_error();
    [[maybe_unused]] const auto clr_pec       = port.clear_pec_error();
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

// ---- SAME70 (original section) ----
#if defined(ALLOY_BOARD_SAME70_XPLD)
using I2cConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::TWIHS0,
    alloy::hal::connection::scl<alloy::device::PinId::PA4, alloy::device::SignalId::signal_twck0>,
    alloy::hal::connection::sda<alloy::device::PinId::PA3, alloy::device::SignalId::signal_twd0>>;
using I2cPort = decltype(alloy::hal::i2c::open<I2cConnector>());
static_assert(I2cPort::valid);
static_assert(I2cPort::peripheral_name == std::string_view{"TWIHS0"});
static_assert(i2c_is_usable<I2cPort>());
[[maybe_unused]] void compile_same70_i2c_backend() {
    exercise_i2c_backend<I2cPort>();
    exercise_i2c_extended<I2cPort>();
}
#endif

// ---- F401 (task 4.2) ----
#if defined(ALLOY_BOARD_NUCLEO_F401RE)
using F401I2cConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::I2C1,
    alloy::hal::connection::scl<alloy::device::PinId::PB8, alloy::device::SignalId::signal_scl>,
    alloy::hal::connection::sda<alloy::device::PinId::PB9, alloy::device::SignalId::signal_sda>>;
using F401I2cPort = decltype(alloy::hal::i2c::open<F401I2cConnector>());
static_assert(F401I2cPort::valid);
static_assert(F401I2cPort::peripheral_name == std::string_view{"I2C1"});
static_assert(i2c_is_usable<F401I2cPort>());
[[maybe_unused]] void compile_f401_i2c_backend() {
    exercise_i2c_backend<F401I2cPort>();
    exercise_i2c_extended<F401I2cPort>();
}
#endif
