#pragma once

#include "hal/interface/clock.hpp"
#include "core/error.hpp"
#include "core/types.hpp"

// Forward declare generated peripheral access
namespace alloy::generated::stm32f746 {
    namespace rcc { struct Registers; extern Registers* const RCC; }
    namespace flash { struct Registers; extern Registers* const FLASH; }
    namespace pwr { struct Registers; extern Registers* const PWR; }
}

namespace alloy::hal::st::stm32f7 {

using namespace alloy::core;

/**
 * STM32F7 Clock Configuration
 *
 * Type-safe clock configuration for STM32F7 family (up to 216 MHz).
 * Enhanced features compared to F4:
 * - Higher maximum frequency (216 MHz vs 180 MHz)
 * - Over-drive mode for 216 MHz operation
 * - Improved flash controller with ART accelerator
 *
 * Usage:
 *   using Clock = Stm32F7Clock<25000000>;  // 25 MHz HSE crystal
 *   auto result = Clock::configure_216mhz();
 */
template<u32 ExternalCrystalHz = 25000000>
class Stm32F7Clock {
public:
    static constexpr u32 HSI_FREQ_HZ = 16000000;
    static constexpr u32 HSE_FREQ_HZ = ExternalCrystalHz;
    static constexpr u32 MAX_FREQ_HZ = 216000000;

    /**
     * Configure for maximum performance: 216 MHz
     *
     * Standard configuration with 25 MHz HSE:
     * - VCO = 25MHz / 25 * 432 = 432 MHz
     * - System = 432 MHz / 2 = 216 MHz
     * - AHB = 216 MHz
     * - APB1 = 54 MHz
     * - APB2 = 108 MHz
     *
     * Enables overdrive mode required for 216 MHz.
     */
    static Result<void> configure_216mhz() {
        using namespace alloy::generated::stm32f746;

        // Enable power interface clock
        rcc::RCC->APB1ENR |= (1 << 28);  // PWREN

        // Enable Over-drive mode (required for 216 MHz)
        pwr::PWR->CR1 |= (1 << 16);  // ODEN
        while (!(pwr::PWR->CSR1 & (1 << 16))) {}  // Wait ODRDY

        // Enable Over-drive switching
        pwr::PWR->CR1 |= (1 << 17);  // ODSWEN
        while (!(pwr::PWR->CSR1 & (1 << 17))) {}  // Wait ODSWRDY

        // Configure voltage regulator to Scale 1
        pwr::PWR->CR1 |= (3 << 14);  // VOS = Scale 1

        // Configure flash latency for 216 MHz @ 3.3V (7 wait states)
        flash::FLASH->ACR = 7 |        // 7 wait states
                           (1 << 8) |  // PRFTEN
                           (1 << 9) |  // ART enable
                           (1 << 10);  // ART reset

        // Enable HSE
        rcc::RCC->CR |= (1 << 16);
        u32 timeout = 100000;
        while (!(rcc::RCC->CR & (1 << 17)) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::ClockSourceNotReady);

        // Configure PLL: 25MHz / 25 * 432 / 2 = 216 MHz
        rcc::RCC->PLLCFGR = (1 << 22) |          // PLLSRC = HSE
                           25 |                  // PLLM = 25
                           (432 << 6) |          // PLLN = 432
                           (0 << 16) |           // PLLP = 2 (00b)
                           (9 << 24);            // PLLQ = 9 (for 48MHz USB)

        // Enable PLL
        rcc::RCC->CR |= (1 << 24);
        timeout = 100000;
        while (!(rcc::RCC->CR & (1 << 25)) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::PllLockFailed);

        // Configure bus dividers
        rcc::RCC->CFGR = (0b0000 << 4) |   // AHB = /1 = 216 MHz
                        (0b101 << 10) |    // APB1 = /4 = 54 MHz
                        (0b100 << 13);     // APB2 = /2 = 108 MHz

        // Switch to PLL
        rcc::RCC->CFGR |= 2;  // SW = PLL
        while ((rcc::RCC->CFGR & (3 << 2)) != (2 << 2)) {}

        // Update frequencies
        current_frequencies_.system = 216000000;
        current_frequencies_.ahb = 216000000;
        current_frequencies_.apb1 = 54000000;
        current_frequencies_.apb2 = 108000000;

        return Ok();
    }

    /**
     * Configure for 180 MHz (no overdrive needed)
     */
    static Result<void> configure_180mhz() {
        using namespace alloy::generated::stm32f746;

        // Similar to 216 MHz but simpler
        rcc::RCC->APB1ENR |= (1 << 28);
        pwr::PWR->CR1 |= (3 << 14);  // Scale 1

        flash::FLASH->ACR = 6 | (1 << 8) | (1 << 9);  // 6 WS

        rcc::RCC->CR |= (1 << 16);
        u32 timeout = 100000;
        while (!(rcc::RCC->CR & (1 << 17)) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::ClockSourceNotReady);

        // PLL for 180 MHz
        rcc::RCC->PLLCFGR = (1 << 22) | 25 | (360 << 6) | (0 << 16) | (8 << 24);

        rcc::RCC->CR |= (1 << 24);
        timeout = 100000;
        while (!(rcc::RCC->CR & (1 << 25)) && timeout--) {}
        if (timeout == 0) return Error(ErrorCode::PllLockFailed);

        rcc::RCC->CFGR = (0b0000 << 4) | (0b101 << 10) | (0b100 << 13);
        rcc::RCC->CFGR |= 2;
        while ((rcc::RCC->CFGR & (3 << 2)) != (2 << 2)) {}

        current_frequencies_.system = 180000000;
        current_frequencies_.ahb = 180000000;
        current_frequencies_.apb1 = 45000000;
        current_frequencies_.apb2 = 90000000;

        return Ok();
    }

    static u32 get_frequency() { return current_frequencies_.system; }
    static u32 get_ahb_frequency() { return current_frequencies_.ahb; }
    static u32 get_apb1_frequency() { return current_frequencies_.apb1; }
    static u32 get_apb2_frequency() { return current_frequencies_.apb2; }

    static Result<void> enable_peripheral(hal::Peripheral peripheral) {
        using namespace alloy::generated::stm32f746;
        u16 id = static_cast<u16>(peripheral);
        u8 bit = id & 0xFF;

        if (id < 0x0100) rcc::RCC->AHB1ENR |= (1 << bit);
        else if (id < 0x0200) rcc::RCC->APB2ENR |= (1 << (bit % 32));
        else rcc::RCC->APB1ENR |= (1 << (bit % 32));

        return Ok();
    }

    static Result<void> disable_peripheral(hal::Peripheral peripheral) {
        using namespace alloy::generated::stm32f746;
        u16 id = static_cast<u16>(peripheral);
        u8 bit = id & 0xFF;

        if (id < 0x0100) rcc::RCC->AHB1ENR &= ~(1 << bit);
        else if (id < 0x0200) rcc::RCC->APB2ENR &= ~(1 << (bit % 32));
        else rcc::RCC->APB1ENR &= ~(1 << (bit % 32));

        return Ok();
    }

private:
    struct Frequencies {
        u32 system = HSI_FREQ_HZ;
        u32 ahb = HSI_FREQ_HZ;
        u32 apb1 = HSI_FREQ_HZ;
        u32 apb2 = HSI_FREQ_HZ;
    };

    static inline Frequencies current_frequencies_;
};

using Stm32F746Clock = Stm32F7Clock<25000000>;

} // namespace alloy::hal::st::stm32f7
