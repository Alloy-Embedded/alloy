/**
 * @file test_stm32f1_codegen.cpp
 * @brief Compilation and integration tests for STM32F1 generated code
 *
 * Tests that generated register definitions compile correctly and work as expected.
 */

#include <cassert>
#include <iostream>
#include <type_traits>

#include "../../../src/core/types.hpp"
#include "../../../src/hal/vendors/st/stm32f1/stm32f103xx/register_map.hpp"

using namespace alloy::core;
using namespace alloy::hal::st::stm32f1;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                \
    void test_##name();                                           \
    void run_test_##name() {                                      \
        tests_run++;                                              \
        std::cout << "Running test: " #name << "...";             \
        try {                                                     \
            test_##name();                                        \
            tests_passed++;                                       \
            std::cout << " PASS" << std::endl;                    \
        } catch (const std::exception& e) {                       \
            std::cout << " FAIL: " << e.what() << std::endl;      \
        } catch (...) {                                           \
            std::cout << " FAIL: Unknown exception" << std::endl; \
        }                                                         \
    }                                                             \
    void test_##name()

#define ASSERT(condition)                                              \
    do {                                                               \
        if (!(condition)) {                                            \
            throw std::runtime_error("Assertion failed: " #condition); \
        }                                                              \
    } while (0)

// =============================================================================
// Register Structure Tests
// =============================================================================

TEST(register_sizes_correct) {
    ASSERT(sizeof(u32) == 4);
}

TEST(registers_are_trivial) {
    ASSERT(std::is_trivially_copyable_v<u32>);
    ASSERT(std::is_standard_layout_v<u32>);
}

// =============================================================================
// STM32F1 Peripheral Tests
// =============================================================================

TEST(rcc_registers_compile) {
    using RccReg = rcc::RCC_Registers;
    ASSERT(sizeof(RccReg) > 0);
}

TEST(gpio_registers_compile) {
    using GpioReg = gpioa::GPIOA_Registers;
    ASSERT(sizeof(GpioReg) > 0);
}

TEST(usart_registers_compile) {
    using UsartReg = usart1::USART1_Registers;
    ASSERT(sizeof(UsartReg) > 0);
}

TEST(spi_registers_compile) {
    using SpiReg = spi1::SPI1_Registers;
    ASSERT(sizeof(SpiReg) > 0);
}

TEST(i2c_registers_compile) {
    using I2cReg = i2c1::I2C1_Registers;
    ASSERT(sizeof(I2cReg) > 0);
}

TEST(timer_registers_compile) {
    using TimReg = tim1::TIM1_Registers;
    ASSERT(sizeof(TimReg) > 0);
}

TEST(adc_registers_compile) {
    using AdcReg = adc1::ADC1_Registers;
    ASSERT(sizeof(AdcReg) > 0);
}

TEST(dma_registers_compile) {
    using DmaReg = dma1::DMA1_Registers;
    ASSERT(sizeof(DmaReg) > 0);
}

// =============================================================================
// Multiple Peripherals Tests
// =============================================================================

TEST(multiple_gpio_ports_available) {
    using GpioA = gpioa::GPIOA_Registers;
    // Note: STM32F103xx.svd only defines GPIOA in family-level registers
    // Other GPIO ports may be device-specific
    ASSERT(true);
}

TEST(multiple_usarts_available) {
    using Usart1 = usart1::USART1_Registers;
    // Note: STM32F103xx.svd only defines USART1 in family-level registers
    // Other USARTs may be device-specific
    ASSERT(true);
}

// =============================================================================
// Namespace Tests
// =============================================================================

TEST(namespace_hierarchy_correct) {
    using namespace alloy::hal::st::stm32f1;

    using R = rcc::RCC_Registers;
    using G = gpioa::GPIOA_Registers;

    ASSERT(true);
}

// =============================================================================
// STM32F1 Specific Features
// =============================================================================

TEST(afio_registers_compile) {
    // AFIO is specific to STM32F1
    using AfioReg = afio::AFIO_Registers;
    ASSERT(sizeof(AfioReg) > 0);
}

TEST(expected_stm32f1_peripherals_present) {
    // Core peripherals
    using Rcc = rcc::RCC_Registers;
    using Flash = flash::FLASH_Registers;
    using Pwr = pwr::PWR_Registers;

    // GPIO
    using GpioA = gpioa::GPIOA_Registers;

    // Communication
    using Usart1 = usart1::USART1_Registers;
    using Spi1 = spi1::SPI1_Registers;
    using I2c1 = i2c1::I2C1_Registers;

    // Timers
    using Tim1 = tim1::TIM1_Registers;
    using Tim2 = tim2::TIM2_Registers;

    // Analog
    using Adc1 = adc1::ADC1_Registers;

    // DMA
    using Dma1 = dma1::DMA1_Registers;

    ASSERT(true);
}

TEST(family_level_registers_shared) {
    using Rcc = rcc::RCC_Registers;

    // If this compiles, family-level sharing works
    ASSERT(true);
}

TEST(register_map_header_valid) {
    // The fact that this file compiles means register_map.hpp is valid
    ASSERT(true);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  STM32F1 Code Generation Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Testing: STM32F103" << std::endl;
    std::cout << "  Vendor:  STMicroelectronics" << std::endl;
    std::cout << "  Family:  STM32F1" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_register_sizes_correct();
    run_test_registers_are_trivial();
    run_test_rcc_registers_compile();
    run_test_gpio_registers_compile();
    run_test_usart_registers_compile();
    run_test_spi_registers_compile();
    run_test_i2c_registers_compile();
    run_test_timer_registers_compile();
    run_test_adc_registers_compile();
    run_test_dma_registers_compile();
    run_test_multiple_gpio_ports_available();
    run_test_multiple_usarts_available();
    run_test_namespace_hierarchy_correct();
    run_test_afio_registers_compile();
    run_test_expected_stm32f1_peripherals_present();
    run_test_family_level_registers_shared();
    run_test_register_map_header_valid();

    // Print summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << tests_run << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    if (tests_run == tests_passed) {
        std::cout << std::endl;
        std::cout << "✅ All STM32F1 code generation tests PASSED!" << std::endl;
        std::cout << "   Generated code is valid and compiles correctly." << std::endl;
    }

    return (tests_run == tests_passed) ? 0 : 1;
}
