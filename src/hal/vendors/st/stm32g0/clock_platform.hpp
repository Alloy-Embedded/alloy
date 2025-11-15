/**
 * @file clock_platform.hpp
 * @brief STM32G0 Clock Configuration Policy
 *
 * Provides compile-time clock configuration for STM32G0 family.
 * Implements the ClockPlatform concept interface.
 *
 * ## Design
 *
 * - **Policy-Based Design**: Template-based compile-time configuration
 * - **Zero Runtime Overhead**: All configuration is constexpr
 * - **Type-Safe**: Strong typing prevents configuration errors
 * - **Concept Compliant**: Satisfies ClockPlatform concept
 *
 * ## Usage
 *
 * @code
 * #include "hal/api/clock_simple.hpp"
 * #include "hal/vendors/st/stm32g0/clock_platform.hpp"
 *
 * // Define board-specific clock configuration
 * struct BoardClockConfig {
 *     static constexpr uint32_t system_clock_hz = 64'000'000;
 *     static constexpr uint32_t pll_m = 0;  // /1
 *     static constexpr uint32_t pll_n = 8;  // ×8
 *     static constexpr uint32_t pll_r = 0;  // /2
 *     static constexpr uint32_t flash_latency = 2;
 * };
 *
 * using BoardClock = Stm32g0Clock<BoardClockConfig>;
 *
 * // Initialize system clock
 * BoardClock::initialize();
 * @endcode
 */

#pragma once

#include "core/result.hpp"
#include "core/error.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/rcc_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/flash_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/rcc_bitfields.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/flash_bitfields.hpp"
#include <cstdint>

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

/**
 * @brief STM32G0 Clock Policy Template
 *
 * Implements clock configuration for STM32G0 family.
 * Satisfies the ClockPlatform concept.
 */
template <typename Config>
class Stm32g0Clock {
public:
    /**
     * @brief Initialize system clock with configured parameters
     *
     * Configures the STM32G0 clock tree using HSI16 and PLL.
     *
     * @return Result indicating success or error
     */
    static Result<void, ErrorCode> initialize() {
        using namespace rcc;

        // 1. Enable HSI16 (should already be enabled after reset)
        rcc::RCC()->CR |= cr::HSION::mask;
        
        uint32_t timeout = 10000;
        while (!(rcc::RCC()->CR & cr::HSIRDY::mask) && --timeout);
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }

        // 2. Configure flash latency BEFORE increasing frequency
        flash::FLASH()->ACR = flash::acr::LATENCY::write(flash::FLASH()->ACR, Config::flash_latency);

        // 3. Configure PLL: HSI16 as source
        rcc::RCC()->PLLCFGR = pllcfgr::PLLSRC::write(0, 2) |   // HSI16 = 0b10
                              pllcfgr::PLLM::write(0, Config::pll_m) |
                              pllcfgr::PLLN::write(0, Config::pll_n) |
                              pllcfgr::PLLR::write(0, Config::pll_r) |
                              pllcfgr::PLLREN::mask;           // Enable PLLR output

        // 4. Enable PLL
        rcc::RCC()->CR |= cr::PLLON::mask;
        
        timeout = 10000;
        while (!(rcc::RCC()->CR & cr::PLLRDY::mask) && --timeout);
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }

        // 5. Switch system clock to PLL
        rcc::RCC()->CFGR = cfgr::SW::write(rcc::RCC()->CFGR, 2);  // SW = 2 (PLL)
        
        timeout = 10000;
        while (cfgr::SWS::read(rcc::RCC()->CFGR) != 2 && --timeout);  // Wait for SWS = PLL
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }

        return Ok();
    }

    /**
     * @brief Enable all GPIO peripheral clocks
     *
     * @return Result indicating success
     */
    static Result<void, ErrorCode> enable_gpio_clocks() {
        using namespace rcc;

        rcc::RCC()->IOPENR |= iopenr::GPIOAEN::mask |
                              iopenr::GPIOBEN::mask |
                              iopenr::GPIOCEN::mask |
                              iopenr::GPIODEN::mask |
                              iopenr::GPIOEEN::mask |
                              iopenr::GPIOFEN::mask;

        return Ok();
    }

    /**
     * @brief Enable UART/USART/LPUART peripheral clock
     *
     * @param uart_base UART peripheral base address
     * @return Result indicating success or error if invalid peripheral
     */
    static Result<void, ErrorCode> enable_uart_clock(uint32_t uart_base) {
        using namespace rcc;

        // USART1 is on APB2
        if (uart_base == 0x40013800) {  // USART1
            rcc::RCC()->APBENR2 |= apbenr2::USART1EN::mask;
            return Ok();
        }

        // USART2, USART3, USART4, LPUART1 are on APB1
        if (uart_base == 0x40004400) {  // USART2
            rcc::RCC()->APBENR1 |= apbenr1::USART2EN::mask;
            return Ok();
        }
        if (uart_base == 0x40004800) {  // USART3
            rcc::RCC()->APBENR1 |= apbenr1::USART3EN::mask;
            return Ok();
        }
        if (uart_base == 0x40004C00) {  // USART4
            rcc::RCC()->APBENR1 |= apbenr1::USART4EN::mask;
            return Ok();
        }
        if (uart_base == 0x40008000) {  // LPUART1
            rcc::RCC()->APBENR1 |= apbenr1::LPUART1EN::mask;
            return Ok();
        }

        return Err(ErrorCode::InvalidParameter);
    }

    /**
     * @brief Enable SPI peripheral clock
     *
     * @param spi_base SPI peripheral base address
     * @return Result indicating success or error if invalid peripheral
     */
    static Result<void, ErrorCode> enable_spi_clock(uint32_t spi_base) {
        using namespace rcc;

        // SPI1 is on APB2
        if (spi_base == 0x40013000) {  // SPI1
            rcc::RCC()->APBENR2 |= apbenr2::SPI1EN::mask;
            return Ok();
        }

        // SPI2 is on APB1
        if (spi_base == 0x40003800) {  // SPI2
            rcc::RCC()->APBENR1 |= apbenr1::SPI2EN::mask;
            return Ok();
        }

        return Err(ErrorCode::InvalidParameter);
    }

    /**
     * @brief Enable I2C peripheral clock
     *
     * @param i2c_base I2C peripheral base address
     * @return Result indicating success or error if invalid peripheral
     */
    static Result<void, ErrorCode> enable_i2c_clock(uint32_t i2c_base) {
        using namespace rcc;

        // All I2C peripherals are on APB1
        if (i2c_base == 0x40005400) {  // I2C1
            rcc::RCC()->APBENR1 |= apbenr1::I2C1EN::mask;
            return Ok();
        }
        if (i2c_base == 0x40005800) {  // I2C2
            rcc::RCC()->APBENR1 |= apbenr1::I2C2EN::mask;
            return Ok();
        }

        return Err(ErrorCode::InvalidParameter);
    }

    /**
     * @brief Get system clock frequency
     * @return Frequency in Hz
     */
    static constexpr uint32_t get_system_clock_hz() {
        return Config::system_clock_hz;
    }
};

// ============================================================================
// Concept Validation (C++20)
// ============================================================================

#if __cplusplus >= 202002L
// Example config for validation:
struct ExampleG0ClockConfig {
    static constexpr uint32_t system_clock_hz = 64'000'000;
    static constexpr uint32_t pll_m = 0;  // /1
    static constexpr uint32_t pll_n = 8;  // ×8
    static constexpr uint32_t pll_r = 0;  // /2
    static constexpr uint32_t flash_latency = 2;
};

// Uncomment when concepts.hpp is included
// static_assert(alloy::hal::concepts::ClockPlatform<Stm32g0Clock<ExampleG0ClockConfig>>,
//               "Stm32g0Clock must satisfy ClockPlatform concept");
#endif

} // namespace alloy::hal::st::stm32g0
