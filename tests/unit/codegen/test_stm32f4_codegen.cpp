/**
 * @file test_stm32f4_codegen.cpp
 * @brief Compilation and integration tests for STM32F4 generated code
 */

#include "../../../src/hal/vendors/st/stm32f4/stm32f407/register_map.hpp"
#include "../../../src/core/types.hpp"
#include <cassert>
#include <iostream>
#include <type_traits>

using namespace alloy::core;
using namespace alloy::hal::st::stm32f4;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        tests_run++; \
        std::cout << "Running test: " #name << "..."; \
        try { \
            test_##name(); \
            tests_passed++; \
            std::cout << " PASS" << std::endl; \
        } catch (const std::exception& e) { \
            std::cout << " FAIL: " << e.what() << std::endl; \
        } catch (...) { \
            std::cout << " FAIL: Unknown exception" << std::endl; \
        } \
    } \
    void test_##name()

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Assertion failed: " #condition); \
        } \
    } while(0)

TEST(register_sizes_correct) {
    ASSERT(sizeof(u32) == 4);
}

TEST(rcc_registers_compile) {
    using RccReg = rcc::RCC_Registers;
    ASSERT(sizeof(RccReg) > 0);
}

TEST(gpio_registers_compile) {
    using GpioReg = gpioa::GPIOA_Registers;
    ASSERT(sizeof(GpioReg) > 0);
}

TEST(usart_registers_compile) {
    using UsartReg = usart6::USART6_Registers;
    ASSERT(sizeof(UsartReg) > 0);
}

TEST(spi_registers_compile) {
    using SpiReg = spi1::SPI1_Registers;
    ASSERT(sizeof(SpiReg) > 0);
}

TEST(i2c_registers_compile) {
    using I2cReg = i2c3::I2C3_Registers;
    ASSERT(sizeof(I2cReg) > 0);
}

TEST(dma_registers_compile) {
    using DmaReg = dma2::DMA2_Registers;
    ASSERT(sizeof(DmaReg) > 0);
}

TEST(fmc_registers_compile) {
    // FSMC is specific to STM32F4
    using FsmcReg = fsmc::FSMC_Registers;
    ASSERT(sizeof(FsmcReg) > 0);
}

TEST(ethernet_registers_compile) {
    // Ethernet MAC available on STM32F4
    using EthReg = ethernet_mac::Ethernet_MAC_Registers;
    ASSERT(sizeof(EthReg) > 0);
}

TEST(expected_stm32f4_peripherals_present) {
    using Rcc = rcc::RCC_Registers;
    using Flash = flash::FLASH_Registers;
    using Pwr = pwr::PWR_Registers;
    using GpioA = gpioa::GPIOA_Registers;
    using GpioI = gpioi::GPIOI_Registers;
    using Usart6 = usart6::USART6_Registers;
    using Spi1 = spi1::SPI1_Registers;
    using I2c3 = i2c3::I2C3_Registers;
    using Tim3 = tim3::TIM3_Registers;
    using Dma2 = dma2::DMA2_Registers;
    using Fsmc = fsmc::FSMC_Registers;

    ASSERT(true);
}

TEST(namespace_hierarchy_correct) {
    using namespace alloy::hal::st::stm32f4;
    using R = rcc::RCC_Registers;
    ASSERT(true);
}

TEST(register_map_header_valid) {
    ASSERT(true);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  STM32F4 Code Generation Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Testing: STM32F407" << std::endl;
    std::cout << "  Vendor:  STMicroelectronics" << std::endl;
    std::cout << "  Family:  STM32F4" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    run_test_register_sizes_correct();
    run_test_rcc_registers_compile();
    run_test_gpio_registers_compile();
    run_test_usart_registers_compile();
    run_test_spi_registers_compile();
    run_test_i2c_registers_compile();
    run_test_dma_registers_compile();
    run_test_fmc_registers_compile();
    run_test_ethernet_registers_compile();
    run_test_expected_stm32f4_peripherals_present();
    run_test_namespace_hierarchy_correct();
    run_test_register_map_header_valid();

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
        std::cout << "✅ All STM32F4 code generation tests PASSED!" << std::endl;
    }

    return (tests_run == tests_passed) ? 0 : 1;
}
