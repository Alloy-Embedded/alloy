/**
 * @file clock_platform.hpp
 * @brief STM32F7 Clock Configuration Policy
 *
 * Provides compile-time clock configuration for STM32F7 family.
 * Implements the ClockImpl policy interface required by clock_simple.hpp API.
 *
 * ## Design
 *
 * - **Policy-Based Design**: Template-based compile-time configuration
 * - **Zero Runtime Overhead**: All configuration is constexpr
 * - **Type-Safe**: Strong typing prevents configuration errors
 * - **Documented**: References to STM32F7 Reference Manual (RM0410)
 *
 * ## Usage
 *
 * @code
 * #include "hal/api/clock_simple.hpp"
 * #include "hal/platform/st/stm32f7/clock_platform.hpp"
 *
 * // Define board-specific clock configuration
 * struct BoardClockConfig {
 *     static constexpr uint32_t hse_hz = 8'000'000;
 *     static constexpr uint32_t system_clock_hz = 180'000'000;
 *     static constexpr uint32_t pll_m = 4;
 *     static constexpr uint32_t pll_n = 180;
 *     static constexpr uint32_t pll_p_div = 2;  // Actual divider value (not register value)
 *     static constexpr uint32_t pll_q = 8;
 *     static constexpr uint32_t flash_latency = 5;
 *     static constexpr uint32_t ahb_prescaler = 1;
 *     static constexpr uint32_t apb1_prescaler = 4;
 *     static constexpr uint32_t apb2_prescaler = 2;
 * };
 *
 * using BoardClock = Stm32f7Clock<BoardClockConfig>;
 *
 * // Initialize system clock
 * BoardClock::initialize();
 *
 * // Enable GPIO clocks
 * BoardClock::enable_gpio_clocks();
 * @endcode
 */

#pragma once

#include "core/result.hpp"
#include "core/error.hpp"
#include "hal/vendors/st/stm32f7/generated/registers/rcc_registers.hpp"
#include "hal/vendors/st/stm32f7/generated/registers/flash_registers.hpp"
#include "hal/vendors/st/stm32f7/generated/bitfields/rcc_bitfields.hpp"
#include "hal/vendors/st/stm32f7/generated/bitfields/flash_bitfields.hpp"
#include <cstdint>

#if __cplusplus >= 202002L
#include "hal/core/concepts.hpp"
#endif

namespace alloy::hal::st::stm32f7 {

using namespace alloy::core;

/**
 * @brief STM32F7 Clock Policy Template
 *
 * Implements clock configuration for STM32F7 family using Policy-Based Design.
 *
 * @tparam Config Clock configuration struct with the following required fields:
 *   - `hse_hz`: HSE crystal frequency in Hz
 *   - `system_clock_hz`: Target system clock frequency in Hz
 *   - `pll_m`: PLL input divider (HSE / pll_m)
 *   - `pll_n`: PLL multiplier (VCO = input × pll_n)
 *   - `pll_p_div`: PLL output divider for system clock (actual divider, not register value)
 *   - `pll_q`: PLL output divider for USB/SDMMC
 *   - `flash_latency`: Flash wait states for target frequency
 *   - `ahb_prescaler`: AHB clock divider
 *   - `apb1_prescaler`: APB1 clock divider
 *   - `apb2_prescaler`: APB2 clock divider
 */
template <typename Config>
class Stm32f7Clock {
private:
    // Validate configuration at compile-time
    static_assert(Config::pll_m >= 2 && Config::pll_m <= 63, "PLL_M must be 2-63");
    static_assert(Config::pll_n >= 50 && Config::pll_n <= 432, "PLL_N must be 50-432");
    static_assert(Config::pll_p_div == 2 || Config::pll_p_div == 4 ||
                  Config::pll_p_div == 6 || Config::pll_p_div == 8,
                  "PLL_P must be 2, 4, 6, or 8");
    static_assert(Config::pll_q >= 2 && Config::pll_q <= 15, "PLL_Q must be 2-15");
    static_assert(Config::flash_latency <= 15, "Flash latency must be 0-15");

    // Convert PLL_P divider to register value
    // Register encoding: 0=/2, 1=/4, 2=/6, 3=/8
    static constexpr uint32_t pll_p_reg_value() {
        return (Config::pll_p_div / 2) - 1;
    }

    // Convert prescaler values to register values
    // APB prescaler encoding: 0=div1, 4=div2, 5=div4, 6=div8, 7=div16
    static constexpr uint32_t apb_prescaler_to_reg(uint32_t div) {
        switch (div) {
            case 1:  return 0;
            case 2:  return 4;
            case 4:  return 5;
            case 8:  return 6;
            case 16: return 7;
            default: return 0;  // Will trigger static_assert
        }
    }

    static_assert(apb_prescaler_to_reg(Config::apb1_prescaler) != 0 || Config::apb1_prescaler == 1,
                  "APB1 prescaler must be 1, 2, 4, 8, or 16");
    static_assert(apb_prescaler_to_reg(Config::apb2_prescaler) != 0 || Config::apb2_prescaler == 1,
                  "APB2 prescaler must be 1, 2, 4, 8, or 16");

    // PLLCFGR register bit positions (STM32F7 RM0410)
    static constexpr uint32_t PLLM_POS = 0;
    static constexpr uint32_t PLLN_POS = 6;
    static constexpr uint32_t PLLP_POS = 16;
    static constexpr uint32_t PLLSRC_POS = 22;
    static constexpr uint32_t PLLQ_POS = 24;

    // CFGR register bit positions
    static constexpr uint32_t SW_POS = 0;
    static constexpr uint32_t SWS_POS = 2;
    static constexpr uint32_t HPRE_POS = 4;
    static constexpr uint32_t PPRE1_POS = 10;
    static constexpr uint32_t PPRE2_POS = 13;

    // Clock source values
    static constexpr uint32_t PLL_SRC_HSE = 1;
    static constexpr uint32_t SW_PLL = 2;

    // Bit masks
    static constexpr uint32_t SW_MASK = 0x3 << SW_POS;
    static constexpr uint32_t SWS_MASK = 0x3 << SWS_POS;
    static constexpr uint32_t HPRE_MASK = 0xF << HPRE_POS;
    static constexpr uint32_t PPRE1_MASK = 0x7 << PPRE1_POS;
    static constexpr uint32_t PPRE2_MASK = 0x7 << PPRE2_POS;

public:
    /**
     * @brief Initialize system clock with configured parameters
     *
     * Configures the STM32F7 clock tree:
     * 1. Enable HSE
     * 2. Configure Flash latency
     * 3. Configure PLL
     * 4. Enable PLL
     * 5. Configure bus prescalers
     * 6. Switch to PLL
     *
     * @return Result indicating success or error
     */
    static Result<void, ErrorCode> initialize() {
        using namespace rcc;

        // 1. Enable HSE (High-Speed External oscillator)
        rcc::RCC()->CR |= cr::HSEON::mask;

        // Wait for HSE ready with timeout
        uint32_t timeout = 10000;
        while (!(rcc::RCC()->CR & cr::HSERDY::mask) && --timeout);
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }

        // 2. Configure Flash latency BEFORE increasing frequency
        flash::FLASH()->ACR = flash::acr::LATENCY::write(flash::FLASH()->ACR, Config::flash_latency);

        // 3. Configure PLL
        uint32_t pllcfgr_value = (Config::pll_m << PLLM_POS) |
                                  (Config::pll_n << PLLN_POS) |
                                  (pll_p_reg_value() << PLLP_POS) |
                                  (PLL_SRC_HSE << PLLSRC_POS) |
                                  (Config::pll_q << PLLQ_POS);
        rcc::RCC()->PLLCFGR = pllcfgr_value;

        // 4. Enable PLL
        rcc::RCC()->CR |= cr::PLLON::mask;

        // Wait for PLL ready with timeout
        timeout = 10000;
        while (!(rcc::RCC()->CR & cr::PLLRDY::mask) && --timeout);
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }

        // 5. Configure bus prescalers
        uint32_t cfgr_value = rcc::RCC()->CFGR;
        cfgr_value &= ~(HPRE_MASK | PPRE1_MASK | PPRE2_MASK);
        cfgr_value |= (0 << HPRE_POS) |  // AHB prescaler = 1
                      (apb_prescaler_to_reg(Config::apb1_prescaler) << PPRE1_POS) |
                      (apb_prescaler_to_reg(Config::apb2_prescaler) << PPRE2_POS);
        rcc::RCC()->CFGR = cfgr_value;

        // 6. Switch system clock to PLL
        cfgr_value = rcc::RCC()->CFGR;
        cfgr_value &= ~SW_MASK;
        cfgr_value |= (SW_PLL << SW_POS);
        rcc::RCC()->CFGR = cfgr_value;

        // Wait for clock switch with timeout
        timeout = 10000;
        while (((rcc::RCC()->CFGR & SWS_MASK) >> SWS_POS) != SW_PLL && --timeout);
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }

        return Ok();
    }

    /**
     * @brief Enable all GPIO peripheral clocks
     *
     * Enables clock for GPIOA through GPIOH.
     *
     * @return Result indicating success
     */
    static Result<void, ErrorCode> enable_gpio_clocks() {
        using namespace rcc;

        rcc::RCC()->AHB1ENR |= ahb1enr::GPIOAEN::mask |
                               ahb1enr::GPIOBEN::mask |
                               ahb1enr::GPIOCEN::mask |
                               ahb1enr::GPIODEN::mask |
                               ahb1enr::GPIOEEN::mask |
                               ahb1enr::GPIOFEN::mask |
                               ahb1enr::GPIOGEN::mask |
                               ahb1enr::GPIOHEN::mask;

        return Ok();
    }

    /**
     * @brief Enable UART/USART peripheral clock
     *
     * Enables clock for the specified UART/USART peripheral.
     *
     * @param uart_base UART/USART peripheral base address
     * @return Result indicating success or error if invalid peripheral
     */
    static Result<void, ErrorCode> enable_uart_clock(uint32_t uart_base) {
        using namespace rcc;

        // USART1 and USART6 are on APB2
        if (uart_base == 0x40011000) {  // USART1
            rcc::RCC()->APB2ENR |= apb2enr::USART1EN::mask;
            return Ok();
        }
        if (uart_base == 0x40011400) {  // USART6
            rcc::RCC()->APB2ENR |= apb2enr::USART6EN::mask;
            return Ok();
        }

        // USART2, USART3, UART4, UART5, UART7, UART8 are on APB1
        if (uart_base == 0x40004400) {  // USART2
            rcc::RCC()->APB1ENR |= apb1enr::USART2EN::mask;
            return Ok();
        }
        if (uart_base == 0x40004800) {  // USART3
            rcc::RCC()->APB1ENR |= apb1enr::USART3EN::mask;
            return Ok();
        }
        if (uart_base == 0x40004C00) {  // UART4
            rcc::RCC()->APB1ENR |= apb1enr::UART4EN::mask;
            return Ok();
        }
        if (uart_base == 0x40005000) {  // UART5
            rcc::RCC()->APB1ENR |= apb1enr::UART5EN::mask;
            return Ok();
        }
        if (uart_base == 0x40007800) {  // UART7
            rcc::RCC()->APB1ENR |= apb1enr::UART7EN::mask;
            return Ok();
        }
        if (uart_base == 0x40007C00) {  // UART8
            rcc::RCC()->APB1ENR |= apb1enr::UART8EN::mask;
            return Ok();
        }

        return Err(ErrorCode::InvalidParameter);
    }

    /**
     * @brief Enable SPI peripheral clock
     *
     * Enables clock for the specified SPI peripheral.
     *
     * @param spi_base SPI peripheral base address
     * @return Result indicating success or error if invalid peripheral
     */
    static Result<void, ErrorCode> enable_spi_clock(uint32_t spi_base) {
        using namespace rcc;

        // SPI1, SPI4, SPI5 are on APB2
        if (spi_base == 0x40013000) {  // SPI1
            rcc::RCC()->APB2ENR |= apb2enr::SPI1EN::mask;
            return Ok();
        }
        if (spi_base == 0x40013400) {  // SPI4
            rcc::RCC()->APB2ENR |= apb2enr::SPI4EN::mask;
            return Ok();
        }
        if (spi_base == 0x40015000) {  // SPI5
            rcc::RCC()->APB2ENR |= apb2enr::SPI5EN::mask;
            return Ok();
        }

        // SPI2 and SPI3 are on APB1
        if (spi_base == 0x40003800) {  // SPI2
            rcc::RCC()->APB1ENR |= apb1enr::SPI2EN::mask;
            return Ok();
        }
        if (spi_base == 0x40003C00) {  // SPI3
            rcc::RCC()->APB1ENR |= apb1enr::SPI3EN::mask;
            return Ok();
        }

        return Err(ErrorCode::InvalidParameter);
    }

    /**
     * @brief Enable I2C peripheral clock
     *
     * Enables clock for the specified I2C peripheral.
     *
     * @param i2c_base I2C peripheral base address
     * @return Result indicating success or error if invalid peripheral
     */
    static Result<void, ErrorCode> enable_i2c_clock(uint32_t i2c_base) {
        using namespace rcc;

        // All I2C peripherals are on APB1
        if (i2c_base == 0x40005400) {  // I2C1
            rcc::RCC()->APB1ENR |= apb1enr::I2C1EN::mask;
            return Ok();
        }
        if (i2c_base == 0x40005800) {  // I2C2
            rcc::RCC()->APB1ENR |= apb1enr::I2C2EN::mask;
            return Ok();
        }
        if (i2c_base == 0x40005C00) {  // I2C3
            rcc::RCC()->APB1ENR |= apb1enr::I2C3EN::mask;
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

    /**
     * @brief Get AHB clock frequency
     * @return Frequency in Hz
     */
    static constexpr uint32_t get_ahb_clock_hz() {
        return Config::system_clock_hz / Config::ahb_prescaler;
    }

    /**
     * @brief Get APB1 clock frequency
     * @return Frequency in Hz
     */
    static constexpr uint32_t get_apb1_clock_hz() {
        return get_ahb_clock_hz() / Config::apb1_prescaler;
    }

    /**
     * @brief Get APB2 clock frequency
     * @return Frequency in Hz
     */
    static constexpr uint32_t get_apb2_clock_hz() {
        return get_ahb_clock_hz() / Config::apb2_prescaler;
    }
};

// ============================================================================
// Concept Validation (C++20)
// ============================================================================

#if __cplusplus >= 202002L
// Example config for validation:
struct ExampleF7ClockConfig {
    static constexpr uint32_t hse_hz = 8'000'000;
    static constexpr uint32_t system_clock_hz = 180'000'000;
    static constexpr uint32_t pll_m = 4;
    static constexpr uint32_t pll_n = 180;
    static constexpr uint32_t pll_p_div = 2;
    static constexpr uint32_t pll_q = 8;
    static constexpr uint32_t flash_latency = 5;
    static constexpr uint32_t ahb_prescaler = 1;
    static constexpr uint32_t apb1_prescaler = 4;
    static constexpr uint32_t apb2_prescaler = 2;
};

// Compile-time validation: Verify that Stm32f7Clock satisfies ClockPlatform concept
static_assert(alloy::hal::concepts::ClockPlatform<Stm32f7Clock<ExampleF7ClockConfig>>,
              "Stm32f7Clock must satisfy ClockPlatform concept - missing required methods");
#endif

} // namespace alloy::hal::st::stm32f7
