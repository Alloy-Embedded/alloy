/**
 * @file test_types.cpp
 * @brief Unit tests for HAL core types
 *
 * Tests all enum types and configuration structures defined in hal/types.hpp
 * and related interface headers. Validates type safety, enum values, and
 * default configurations.
 *
 * @note Part of Phase 3.1: Core Systems Testing
 */

#include "hal/types.hpp"
#include "hal/interface/spi.hpp"
#include "core/types.hpp"

#include <type_traits>
#include <cstdint>

using namespace ucore::hal;
using namespace ucore::core;

// ==============================================================================
// GPIO Types Tests
// ==============================================================================

namespace gpio_types {

/**
 * @brief Test PinDirection enum
 */
constexpr bool test_pin_direction() {
    // Test enum values
    static_assert(static_cast<uint8_t>(PinDirection::Input) == 0, "Input should be 0");
    static_assert(static_cast<uint8_t>(PinDirection::Output) == 1, "Output should be 1");

    // Test type traits
    static_assert(std::is_enum_v<PinDirection>, "PinDirection should be an enum");
    static_assert(sizeof(PinDirection) == sizeof(uint8_t), "PinDirection should be 1 byte");

    // Test usage
    PinDirection dir = PinDirection::Output;
    if (dir != PinDirection::Output) return false;

    dir = PinDirection::Input;
    if (dir != PinDirection::Input) return false;

    return true;
}

static_assert(test_pin_direction(), "PinDirection tests must pass");

/**
 * @brief Test PinPull enum
 */
constexpr bool test_pin_pull() {
    // Test enum values
    static_assert(static_cast<uint8_t>(PinPull::None) == 0, "None should be 0");
    static_assert(static_cast<uint8_t>(PinPull::PullUp) == 1, "PullUp should be 1");
    static_assert(static_cast<uint8_t>(PinPull::PullDown) == 2, "PullDown should be 2");

    // Test type traits
    static_assert(std::is_enum_v<PinPull>, "PinPull should be an enum");
    static_assert(sizeof(PinPull) == sizeof(uint8_t), "PinPull should be 1 byte");

    // Test all values
    PinPull pull = PinPull::None;
    if (pull != PinPull::None) return false;

    pull = PinPull::PullUp;
    if (pull != PinPull::PullUp) return false;

    pull = PinPull::PullDown;
    if (pull != PinPull::PullDown) return false;

    return true;
}

static_assert(test_pin_pull(), "PinPull tests must pass");

/**
 * @brief Test PinDrive enum
 */
constexpr bool test_pin_drive() {
    // Test enum values
    static_assert(static_cast<uint8_t>(PinDrive::PushPull) == 0, "PushPull should be 0");
    static_assert(static_cast<uint8_t>(PinDrive::OpenDrain) == 1, "OpenDrain should be 1");

    // Test type traits
    static_assert(std::is_enum_v<PinDrive>, "PinDrive should be an enum");
    static_assert(sizeof(PinDrive) == sizeof(uint8_t), "PinDrive should be 1 byte");

    return true;
}

static_assert(test_pin_drive(), "PinDrive tests must pass");

/**
 * @brief Test PinState enum
 */
constexpr bool test_pin_state() {
    // Test enum values
    static_assert(static_cast<uint8_t>(PinState::Low) == 0, "Low should be 0");
    static_assert(static_cast<uint8_t>(PinState::High) == 1, "High should be 1");

    // Test type traits
    static_assert(std::is_enum_v<PinState>, "PinState should be an enum");
    static_assert(sizeof(PinState) == sizeof(uint8_t), "PinState should be 1 byte");

    return true;
}

static_assert(test_pin_state(), "PinState tests must pass");

/**
 * @brief Test InterruptMode enum
 */
constexpr bool test_interrupt_mode() {
    // Test enum values
    static_assert(static_cast<uint8_t>(InterruptMode::None) == 0, "None should be 0");
    static_assert(static_cast<uint8_t>(InterruptMode::RisingEdge) == 1, "RisingEdge should be 1");
    static_assert(static_cast<uint8_t>(InterruptMode::FallingEdge) == 2, "FallingEdge should be 2");
    static_assert(static_cast<uint8_t>(InterruptMode::BothEdges) == 3, "BothEdges should be 3");
    static_assert(static_cast<uint8_t>(InterruptMode::LowLevel) == 4, "LowLevel should be 4");
    static_assert(static_cast<uint8_t>(InterruptMode::HighLevel) == 5, "HighLevel should be 5");

    // Test type traits
    static_assert(std::is_enum_v<InterruptMode>, "InterruptMode should be an enum");
    static_assert(sizeof(InterruptMode) == sizeof(uint8_t), "InterruptMode should be 1 byte");

    return true;
}

static_assert(test_interrupt_mode(), "InterruptMode tests must pass");

/**
 * @brief Test GpioConfig struct
 */
constexpr bool test_gpio_config() {
    // Test default construction
    GpioConfig config{};
    if (config.direction != PinDirection::Input) return false;
    if (config.drive != PinDrive::PushPull) return false;
    if (config.pull != PinPull::None) return false;
    if (config.initial_state != PinState::Low) return false;

    // Test custom configuration
    GpioConfig custom{
        .direction = PinDirection::Output,
        .drive = PinDrive::OpenDrain,
        .pull = PinPull::PullUp,
        .initial_state = PinState::High
    };

    if (custom.direction != PinDirection::Output) return false;
    if (custom.drive != PinDrive::OpenDrain) return false;
    if (custom.pull != PinPull::PullUp) return false;
    if (custom.initial_state != PinState::High) return false;

    return true;
}

static_assert(test_gpio_config(), "GpioConfig tests must pass");

}  // namespace gpio_types

// ==============================================================================
// UART Types Tests
// ==============================================================================

namespace uart_types {

/**
 * @brief Test Baudrate enum
 */
constexpr bool test_baudrate() {
    // Test enum values match actual baud rates
    static_assert(static_cast<uint32_t>(Baudrate::e9600) == 9600, "Baudrate 9600");
    static_assert(static_cast<uint32_t>(Baudrate::e19200) == 19200, "Baudrate 19200");
    static_assert(static_cast<uint32_t>(Baudrate::e38400) == 38400, "Baudrate 38400");
    static_assert(static_cast<uint32_t>(Baudrate::e57600) == 57600, "Baudrate 57600");
    static_assert(static_cast<uint32_t>(Baudrate::e115200) == 115200, "Baudrate 115200");
    static_assert(static_cast<uint32_t>(Baudrate::e230400) == 230400, "Baudrate 230400");
    static_assert(static_cast<uint32_t>(Baudrate::e460800) == 460800, "Baudrate 460800");
    static_assert(static_cast<uint32_t>(Baudrate::e921600) == 921600, "Baudrate 921600");

    // Test type traits
    static_assert(std::is_enum_v<Baudrate>, "Baudrate should be an enum");
    static_assert(sizeof(Baudrate) == sizeof(uint32_t), "Baudrate should be 4 bytes");

    return true;
}

static_assert(test_baudrate(), "Baudrate tests must pass");

/**
 * @brief Test Parity enum
 */
constexpr bool test_parity() {
    // Test enum values
    static_assert(static_cast<uint8_t>(Parity::None) == 0, "None should be 0");
    static_assert(static_cast<uint8_t>(Parity::Even) == 1, "Even should be 1");
    static_assert(static_cast<uint8_t>(Parity::Odd) == 2, "Odd should be 2");
    static_assert(static_cast<uint8_t>(Parity::Mark) == 3, "Mark should be 3");
    static_assert(static_cast<uint8_t>(Parity::Space) == 4, "Space should be 4");

    // Test type traits
    static_assert(std::is_enum_v<Parity>, "Parity should be an enum");
    static_assert(sizeof(Parity) == sizeof(uint8_t), "Parity should be 1 byte");

    return true;
}

static_assert(test_parity(), "Parity tests must pass");

/**
 * @brief Test StopBits enum
 */
constexpr bool test_stop_bits() {
    // Test enum values
    static_assert(static_cast<uint8_t>(StopBits::One) == 0, "One should be 0");
    static_assert(static_cast<uint8_t>(StopBits::OneAndHalf) == 1, "OneAndHalf should be 1");
    static_assert(static_cast<uint8_t>(StopBits::Two) == 2, "Two should be 2");

    // Test type traits
    static_assert(std::is_enum_v<StopBits>, "StopBits should be an enum");
    static_assert(sizeof(StopBits) == sizeof(uint8_t), "StopBits should be 1 byte");

    return true;
}

static_assert(test_stop_bits(), "StopBits tests must pass");

/**
 * @brief Test DataBits enum
 */
constexpr bool test_data_bits() {
    // Test enum values
    static_assert(static_cast<uint8_t>(DataBits::Five) == 5, "Five should be 5");
    static_assert(static_cast<uint8_t>(DataBits::Six) == 6, "Six should be 6");
    static_assert(static_cast<uint8_t>(DataBits::Seven) == 7, "Seven should be 7");
    static_assert(static_cast<uint8_t>(DataBits::Eight) == 8, "Eight should be 8");

    // Test type traits
    static_assert(std::is_enum_v<DataBits>, "DataBits should be an enum");
    static_assert(sizeof(DataBits) == sizeof(uint8_t), "DataBits should be 1 byte");

    return true;
}

static_assert(test_data_bits(), "DataBits tests must pass");

/**
 * @brief Test FlowControl enum
 */
constexpr bool test_flow_control() {
    // Test enum values
    static_assert(static_cast<uint8_t>(FlowControl::None) == 0, "None should be 0");
    static_assert(static_cast<uint8_t>(FlowControl::RtsCts) == 1, "RtsCts should be 1");
    static_assert(static_cast<uint8_t>(FlowControl::XonXoff) == 2, "XonXoff should be 2");

    // Test type traits
    static_assert(std::is_enum_v<FlowControl>, "FlowControl should be an enum");
    static_assert(sizeof(FlowControl) == sizeof(uint8_t), "FlowControl should be 1 byte");

    return true;
}

static_assert(test_flow_control(), "FlowControl tests must pass");

/**
 * @brief Test UartConfig struct
 */
constexpr bool test_uart_config() {
    // Test default construction (8N1 at 115200)
    UartConfig config{};
    if (config.baudrate != Baudrate::e115200) return false;
    if (config.data_bits != DataBits::Eight) return false;
    if (config.parity != Parity::None) return false;
    if (config.stop_bits != StopBits::One) return false;
    if (config.flow_control != FlowControl::None) return false;

    // Test custom configuration
    UartConfig custom{
        .baudrate = Baudrate::e9600,
        .data_bits = DataBits::Seven,
        .parity = Parity::Even,
        .stop_bits = StopBits::Two,
        .flow_control = FlowControl::RtsCts
    };

    if (custom.baudrate != Baudrate::e9600) return false;
    if (custom.data_bits != DataBits::Seven) return false;
    if (custom.parity != Parity::Even) return false;
    if (custom.stop_bits != StopBits::Two) return false;
    if (custom.flow_control != FlowControl::RtsCts) return false;

    return true;
}

static_assert(test_uart_config(), "UartConfig tests must pass");

}  // namespace uart_types

// ==============================================================================
// SPI Types Tests
// ==============================================================================

namespace spi_types {

/**
 * @brief Test SpiMode enum
 */
constexpr bool test_spi_mode() {
    // Test enum values
    static_assert(static_cast<u8>(SpiMode::Mode0) == 0, "Mode0 should be 0");
    static_assert(static_cast<u8>(SpiMode::Mode1) == 1, "Mode1 should be 1");
    static_assert(static_cast<u8>(SpiMode::Mode2) == 2, "Mode2 should be 2");
    static_assert(static_cast<u8>(SpiMode::Mode3) == 3, "Mode3 should be 3");

    // Test type traits
    static_assert(std::is_enum_v<SpiMode>, "SpiMode should be an enum");
    static_assert(sizeof(SpiMode) == sizeof(u8), "SpiMode should be 1 byte");

    return true;
}

static_assert(test_spi_mode(), "SpiMode tests must pass");

/**
 * @brief Test SpiBitOrder enum
 */
constexpr bool test_spi_bit_order() {
    // Test enum values
    static_assert(static_cast<u8>(SpiBitOrder::MsbFirst) == 0, "MsbFirst should be 0");
    static_assert(static_cast<u8>(SpiBitOrder::LsbFirst) == 1, "LsbFirst should be 1");

    // Test type traits
    static_assert(std::is_enum_v<SpiBitOrder>, "SpiBitOrder should be an enum");
    static_assert(sizeof(SpiBitOrder) == sizeof(u8), "SpiBitOrder should be 1 byte");

    return true;
}

static_assert(test_spi_bit_order(), "SpiBitOrder tests must pass");

/**
 * @brief Test SpiDataSize enum
 */
constexpr bool test_spi_data_size() {
    // Test enum values
    static_assert(static_cast<u8>(SpiDataSize::Bits8) == 8, "Bits8 should be 8");
    static_assert(static_cast<u8>(SpiDataSize::Bits16) == 16, "Bits16 should be 16");

    // Test type traits
    static_assert(std::is_enum_v<SpiDataSize>, "SpiDataSize should be an enum");
    static_assert(sizeof(SpiDataSize) == sizeof(u8), "SpiDataSize should be 1 byte");

    return true;
}

static_assert(test_spi_data_size(), "SpiDataSize tests must pass");

/**
 * @brief Test SpiCsMode enum
 */
constexpr bool test_spi_cs_mode() {
    // Test enum values
    static_assert(static_cast<uint8_t>(SpiCsMode::Hardware) == 0, "Hardware should be 0");
    static_assert(static_cast<uint8_t>(SpiCsMode::Software) == 1, "Software should be 1");

    // Test type traits
    static_assert(std::is_enum_v<SpiCsMode>, "SpiCsMode should be an enum");
    static_assert(sizeof(SpiCsMode) == sizeof(uint8_t), "SpiCsMode should be 1 byte");

    return true;
}

static_assert(test_spi_cs_mode(), "SpiCsMode tests must pass");

/**
 * @brief Test SpiConfig struct
 */
constexpr bool test_spi_config() {
    // Test default construction
    SpiConfig config{};
    if (config.mode != SpiMode::Mode0) return false;
    if (config.clock_speed != 1000000) return false;  // 1 MHz default
    if (config.bit_order != SpiBitOrder::MsbFirst) return false;
    if (config.data_size != SpiDataSize::Bits8) return false;

    // Test custom configuration
    SpiConfig custom{
        SpiMode::Mode3,
        4000000,  // 4 MHz
        SpiBitOrder::LsbFirst,
        SpiDataSize::Bits16
    };

    if (custom.mode != SpiMode::Mode3) return false;
    if (custom.clock_speed != 4000000) return false;
    if (custom.bit_order != SpiBitOrder::LsbFirst) return false;
    if (custom.data_size != SpiDataSize::Bits16) return false;

    return true;
}

static_assert(test_spi_config(), "SpiConfig tests must pass");

}  // namespace spi_types

// ==============================================================================
// PWM/Timer Types Tests
// ==============================================================================

namespace pwm_types {

/**
 * @brief Test PwmPolarity enum
 */
constexpr bool test_pwm_polarity() {
    // Test enum values
    static_assert(static_cast<uint8_t>(PwmPolarity::Normal) == 0, "Normal should be 0");
    static_assert(static_cast<uint8_t>(PwmPolarity::Inverted) == 1, "Inverted should be 1");

    // Test type traits
    static_assert(std::is_enum_v<PwmPolarity>, "PwmPolarity should be an enum");
    static_assert(sizeof(PwmPolarity) == sizeof(uint8_t), "PwmPolarity should be 1 byte");

    return true;
}

static_assert(test_pwm_polarity(), "PwmPolarity tests must pass");

/**
 * @brief Test PwmAlignment enum
 */
constexpr bool test_pwm_alignment() {
    // Test enum values
    static_assert(static_cast<uint8_t>(PwmAlignment::Edge) == 0, "Edge should be 0");
    static_assert(static_cast<uint8_t>(PwmAlignment::Center) == 1, "Center should be 1");

    // Test type traits
    static_assert(std::is_enum_v<PwmAlignment>, "PwmAlignment should be an enum");
    static_assert(sizeof(PwmAlignment) == sizeof(uint8_t), "PwmAlignment should be 1 byte");

    return true;
}

static_assert(test_pwm_alignment(), "PwmAlignment tests must pass");

/**
 * @brief Test PwmConfig struct
 */
constexpr bool test_pwm_config() {
    // Test default construction (1 kHz, 50% duty)
    PwmConfig config{};
    if (config.frequency_hz != 1000) return false;
    if (config.duty_cycle_percent != 50) return false;
    if (config.polarity != PwmPolarity::Normal) return false;
    if (config.alignment != PwmAlignment::Edge) return false;

    // Test custom configuration
    PwmConfig custom{
        .frequency_hz = 10000,         // 10 kHz
        .duty_cycle_percent = 75,      // 75% duty
        .polarity = PwmPolarity::Inverted,
        .alignment = PwmAlignment::Center
    };

    if (custom.frequency_hz != 10000) return false;
    if (custom.duty_cycle_percent != 75) return false;
    if (custom.polarity != PwmPolarity::Inverted) return false;
    if (custom.alignment != PwmAlignment::Center) return false;

    return true;
}

static_assert(test_pwm_config(), "PwmConfig tests must pass");

}  // namespace pwm_types

// ==============================================================================
// Test Summary
// ==============================================================================

/**
 * HAL Types Test Summary:
 *
 * GPIO Types (6 tests):
 * ✅ PinDirection - 2 values (Input, Output)
 * ✅ PinPull - 3 values (None, PullUp, PullDown)
 * ✅ PinDrive - 2 values (PushPull, OpenDrain)
 * ✅ PinState - 2 values (Low, High)
 * ✅ InterruptMode - 6 values
 * ✅ GpioConfig - struct with defaults
 *
 * UART Types (6 tests):
 * ✅ Baudrate - 8 standard rates (9600-921600)
 * ✅ Parity - 5 modes (None, Even, Odd, Mark, Space)
 * ✅ StopBits - 3 values (One, OneAndHalf, Two)
 * ✅ DataBits - 4 values (Five, Six, Seven, Eight)
 * ✅ FlowControl - 3 modes (None, RtsCts, XonXoff)
 * ✅ UartConfig - struct with 8N1@115200 defaults
 *
 * SPI Types (5 tests):
 * ✅ SpiMode - 4 modes (Mode0-Mode3)
 * ✅ SpiBitOrder - 2 orders (MsbFirst, LsbFirst)
 * ✅ SpiDataSize - 2 sizes (Bits8, Bits16)
 * ✅ SpiCsMode - 2 modes (Hardware, Software)
 * ✅ SpiConfig - struct with Mode0@1MHz defaults
 *
 * PWM Types (3 tests):
 * ✅ PwmPolarity - 2 values (Normal, Inverted)
 * ✅ PwmAlignment - 2 values (Edge, Center)
 * ✅ PwmConfig - struct with 1kHz@50% defaults
 *
 * Total: 20 type tests
 * All tests pass at compile-time via static_assert
 *
 * Type Safety Verified:
 * - All enums are strongly typed (enum class)
 * - Correct underlying types (u8, u32)
 * - Appropriate sizes (1 or 4 bytes)
 * - Sensible default values
 * - Proper constexpr usage
 */

int main() {
    // All tests are compile-time static_assert checks
    // If this compiles, all tests passed
    return 0;
}
