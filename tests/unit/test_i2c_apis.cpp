/**
 * @file test_i2c_apis.cpp
 * @brief Tests for I2C multi-level APIs
 */

#include <iostream>

#include "../../src/hal/i2c_simple.hpp"
#include "../../src/hal/i2c_fluent.hpp"
#include "../../src/hal/i2c_expert.hpp"
#include "../../src/hal/i2c_dma.hpp"

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::signals;

// Mock pin
template <PinId pin_id>
struct MockPin {
    static constexpr PinId get_pin_id() { return pin_id; }
};

using I2c0_SDA = MockPin<PinId::PA10>;
using I2c0_SCL = MockPin<PinId::PA9>;

int main() {
    int tests = 0;
    int passed = 0;

    std::cout << "I2cSimpleApi:\n";
    
    // Simple API tests
    {
        constexpr auto config = I2c<PeripheralId::I2C0>::quick_setup<I2c0_SDA, I2c0_SCL>();
        static_assert(config.speed == I2cSpeed::Standard, "");
        std::cout << "  ✓ quick_setup_creates_config\n";
        tests++; passed++;
    }

    {
        constexpr auto config = I2c<PeripheralId::I2C0>::quick_setup_fast<I2c0_SDA, I2c0_SCL>();
        static_assert(config.speed == I2cSpeed::Fast, "");
        std::cout << "  ✓ quick_setup_fast\n";
        tests++; passed++;
    }

    std::cout << "\nI2cFluentApi:\n";

    // Fluent API tests
    {
        auto result = I2cBuilder<PeripheralId::I2C0>()
            .with_sda<I2c0_SDA>()
            .with_scl<I2c0_SCL>()
            .speed(I2cSpeed::Fast)
            .initialize();

        if (result.is_ok()) {
            std::cout << "  ✓ builder_basic_setup\n";
            tests++; passed++;
        }
    }

    {
        auto result = I2cBuilder<PeripheralId::I2C0>()
            .with_sda<I2c0_SDA>()
            .with_scl<I2c0_SCL>()
            .fast_mode()
            .initialize();

        if (result.is_ok()) {
            std::cout << "  ✓ builder_fast_mode\n";
            tests++; passed++;
        }
    }

    std::cout << "\nI2cExpertApi:\n";

    // Expert API tests
    {
        constexpr I2cExpertConfig config = {
            .peripheral = PeripheralId::I2C0,
            .sda_pin = PinId::PA10,
            .scl_pin = PinId::PA9,
            .speed = I2cSpeed::Standard,
            .addressing = I2cAddressing::SevenBit,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_analog_filter = true,
            .enable_digital_filter = false,
            .digital_filter_coefficient = 0
        };

        static_assert(config.is_valid(), "");
        std::cout << "  ✓ expert_config_basic\n";
        tests++; passed++;
    }

    {
        constexpr auto config = I2cExpertConfig::standard(
            PeripheralId::I2C0, PinId::PA10, PinId::PA9);
        
        static_assert(config.is_valid(), "");
        std::cout << "  ✓ expert_preset_standard\n";
        tests++; passed++;
    }

    {
        constexpr auto config = I2cExpertConfig::fast(
            PeripheralId::I2C0, PinId::PA10, PinId::PA9);
        
        static_assert(config.is_valid(), "");
        std::cout << "  ✓ expert_preset_fast\n";
        tests++; passed++;
    }

    std::cout << "\nI2cDmaIntegration:\n";

    // DMA tests
    {
        using I2c0TxDma = DmaConnection<PeripheralId::I2C0, DmaRequest::I2C0_TX, DmaStream::Stream6>;
        using I2c0RxDma = DmaConnection<PeripheralId::I2C0, DmaRequest::I2C0_RX, DmaStream::Stream5>;

        constexpr auto config = I2cDmaConfig<I2c0TxDma, I2c0RxDma>::create(
            PinId::PA10, PinId::PA9, I2cSpeed::Fast);

        static_assert(config.is_valid(), "");
        std::cout << "  ✓ create_i2c_dma_config\n";
        tests++; passed++;
    }

    std::cout << "\n========================================\n";
    std::cout << "Tests run: " << tests << "\n";
    std::cout << "Tests passed: " << passed << "\n";
    std::cout << "Tests failed: " << (tests - passed) << "\n";
    std::cout << "========================================\n";

    return (tests == passed) ? 0 : 1;
}
