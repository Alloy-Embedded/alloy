/// STM32F4 Clock Configuration Implementation
///
/// Implements the clock interface for STM32F4 family (Cortex-M4F, 168MHz max)
/// Uses auto-generated peripheral definitions from SVD
///
/// Clock tree:
/// - HSI: 16MHz internal RC oscillator
/// - HSE: 8MHz external crystal (typical)
/// - PLL: VCO 192-432MHz, output 24-168MHz
/// - System: Up to 168MHz
/// - AHB: System / divider
/// - APB1: Max 42MHz
/// - APB2: Max 84MHz

#ifndef ALLOY_HAL_ST_STM32F4_CLOCK_HPP
#define ALLOY_HAL_ST_STM32F4_CLOCK_HPP

#include "hal/interface/clock.hpp"
#include "core/types.hpp"
#include "generated/unknown/stm32f4/stm32f407/peripherals.hpp"

namespace alloy::hal::st::stm32f4 {

// Import generated peripherals
using namespace alloy::generated::stm32f407;

/// Calculate flash latency based on frequency and voltage range
/// Assumes voltage range 2.7-3.6V
constexpr core::u8 calculate_flash_latency(core::u32 frequency_hz) {
    if (frequency_hz <= 30000000) return 0;
    else if (frequency_hz <= 60000000) return 1;
    else if (frequency_hz <= 90000000) return 2;
    else if (frequency_hz <= 120000000) return 3;
    else if (frequency_hz <= 150000000) return 4;
    else return 5;  // 151-168MHz
}

/// STM32F4 System Clock implementation
class SystemClock {
public:
    SystemClock() : system_frequency_(16000000) {}  // Default HSI = 16MHz

    /// Configure system clock
    core::Result<void> configure(const ClockConfig& config) {
        if (config.source == ClockSource::ExternalCrystal) {
            return configure_hse_pll(config);
        } else if (config.source == ClockSource::InternalRC) {
            return configure_hsi();
        }
        return core::Result<void>::error(core::ErrorCode::NotSupported);
    }

    /// Set system frequency (high-level API)
    core::Result<void> set_frequency(core::u32 frequency_hz) {
        if (frequency_hz <= 16000000) {
            // Use HSI directly
            return configure_hsi();
        } else if (frequency_hz == 168000000) {
            // Use standard 168MHz config (HSE 8MHz → PLL → 168MHz)
            ClockConfig config{
                .source = ClockSource::ExternalCrystal,
                .crystal_frequency_hz = 8000000,
                .pll_multiplier = 168,   // VCO = 8MHz * 168 / 4 = 336MHz
                .pll_divider = 4,        // PLLM = 4
                .ahb_divider = 1,
                .apb1_divider = 4,       // APB1 = 168/4 = 42MHz (max)
                .apb2_divider = 2        // APB2 = 168/2 = 84MHz (max)
            };
            return configure(config);
        }
        return core::Result<void>::error(core::ErrorCode::ClockInvalidFrequency);
    }

    /// Get current system frequency
    core::u32 get_frequency() const {
        return system_frequency_;
    }

    /// Get AHB frequency
    core::u32 get_ahb_frequency() const {
        return system_frequency_;  // STM32F4: AHB = SYSCLK in standard config
    }

    /// Get APB1 frequency (max 42MHz)
    core::u32 get_apb1_frequency() const {
        return system_frequency_ / 4;  // APB1 = SYSCLK / 4 @ 168MHz
    }

    /// Get APB2 frequency (max 84MHz)
    core::u32 get_apb2_frequency() const {
        return system_frequency_ / 2;  // APB2 = SYSCLK / 2 @ 168MHz
    }

    /// Get peripheral frequency
    core::u32 get_peripheral_frequency(Peripheral periph) const {
        // Simple implementation: APB1 or APB2 based on peripheral
        core::u16 periph_val = static_cast<core::u16>(periph);
        if (periph_val >= 0x0100 && periph_val < 0x0200) {
            return get_apb2_frequency();  // USART1, TIM1, TIM8, etc on APB2
        }
        return get_apb1_frequency();
    }

    /// Enable peripheral clock
    core::Result<void> enable_peripheral(Peripheral periph) {
        using namespace rcc::ahb1enr_bits;
        using namespace rcc::apb1enr_bits;
        using namespace rcc::apb2enr_bits;

        switch (periph) {
            // GPIO on AHB1
            case Peripheral::GpioA: rcc::RCC->AHB1ENR |= GPIOAEN; break;
            case Peripheral::GpioB: rcc::RCC->AHB1ENR |= GPIOBEN; break;
            case Peripheral::GpioC: rcc::RCC->AHB1ENR |= GPIOCEN; break;
            case Peripheral::GpioD: rcc::RCC->AHB1ENR |= GPIODEN; break;
            case Peripheral::GpioE: rcc::RCC->AHB1ENR |= GPIOEEN; break;

            // UARTs
            case Peripheral::Uart1: rcc::RCC->APB2ENR |= USART1EN; break;
            case Peripheral::Uart2: rcc::RCC->APB1ENR |= USART2EN; break;
            case Peripheral::Uart3: rcc::RCC->APB1ENR |= USART3EN; break;

            default: return core::Result<void>::error(core::ErrorCode::InvalidParameter);
        }
        return core::Result<void>::ok();
    }

    /// Disable peripheral clock
    core::Result<void> disable_peripheral(Peripheral periph) {
        using namespace rcc::ahb1enr_bits;
        using namespace rcc::apb1enr_bits;
        using namespace rcc::apb2enr_bits;

        switch (periph) {
            case Peripheral::GpioA: rcc::RCC->AHB1ENR &= ~GPIOAEN; break;
            case Peripheral::GpioB: rcc::RCC->AHB1ENR &= ~GPIOBEN; break;
            case Peripheral::GpioC: rcc::RCC->AHB1ENR &= ~GPIOCEN; break;
            case Peripheral::GpioD: rcc::RCC->AHB1ENR &= ~GPIODEN; break;
            case Peripheral::GpioE: rcc::RCC->AHB1ENR &= ~GPIOEEN; break;
            case Peripheral::Uart1: rcc::RCC->APB2ENR &= ~USART1EN; break;
            case Peripheral::Uart2: rcc::RCC->APB1ENR &= ~USART2EN; break;
            case Peripheral::Uart3: rcc::RCC->APB1ENR &= ~USART3EN; break;
            default: return core::Result<void>::error(core::ErrorCode::InvalidParameter);
        }
        return core::Result<void>::ok();
    }

    /// Set flash latency
    core::Result<void> set_flash_latency(core::u32 frequency_hz) {
        core::u8 latency = calculate_flash_latency(frequency_hz);
        flash::FLASH->ACR = (flash::FLASH->ACR & ~flash::acr_bits::LATENCY) | latency;
        return core::Result<void>::ok();
    }

    /// Configure PLL (not fully implemented in minimal version)
    core::Result<void> configure_pll(const PllConfig& config) {
        return core::Result<void>::error(core::ErrorCode::NotSupported);
    }

private:
    core::u32 system_frequency_;

    /// Configure HSI (16MHz internal oscillator)
    core::Result<void> configure_hsi() {
        using namespace rcc::cfgr_bits;

        // HSI is always on, just switch to it
        rcc::RCC->CFGR = (rcc::RCC->CFGR & ~SW) | 0x0;  // SW = 0b00 = HSI

        // Wait for switch (SWS should be 0b00)
        while ((rcc::RCC->CFGR & SWS) != 0x0);

        system_frequency_ = 16000000;
        return core::Result<void>::ok();
    }

    /// Configure HSE + PLL (8MHz → 168MHz)
    core::Result<void> configure_hse_pll(const ClockConfig& config) {
        using namespace rcc::cr_bits;
        using namespace rcc::cfgr_bits;
        using namespace rcc::pllcfgr_bits;

        // 1. Enable HSE
        rcc::RCC->CR |= HSEON;

        // Wait for HSE ready (timeout after ~1000 iterations)
        core::u32 timeout = 1000;
        while (!(rcc::RCC->CR & HSERDY) && timeout--) {
            if (timeout == 0) {
                return core::Result<void>::error(core::ErrorCode::ClockSourceNotReady);
            }
        }

        // 2. Set flash latency BEFORE increasing frequency
        set_flash_latency(config.crystal_frequency_hz * config.pll_multiplier / config.pll_divider);

        // 3. Configure PLL
        // VCO = HSE * (PLLN / PLLM)
        // System clock = VCO / PLLP
        // For 168MHz: VCO = 8MHz * (168/4) = 336MHz, SYSCLK = 336MHz / 2 = 168MHz

        core::u32 pllm = config.pll_divider;        // PLLM = 4 (HSE divider)
        core::u32 plln = config.pll_multiplier;     // PLLN = 168 (VCO multiplier)
        core::u32 pllp = 0;                         // PLLP = 0 means /2 (336MHz/2 = 168MHz)
        core::u32 pllq = 7;                         // PLLQ = 7 (for USB: 336MHz/7 = 48MHz)

        rcc::RCC->PLLCFGR = (pllm << 0)   |  // PLLM bits [5:0]
                           (plln << 6)    |  // PLLN bits [14:6]
                           (pllp << 16)   |  // PLLP bits [17:16]
                           PLLSRC         |  // HSE as source
                           (pllq << 24);     // PLLQ bits [27:24]

        // 4. Configure bus dividers
        // AHB prescaler = 1 (HPRE = 0b0000)
        // APB1 prescaler = 4 (PPRE1 = 0b101) -> 42MHz
        // APB2 prescaler = 2 (PPRE2 = 0b100) -> 84MHz
        core::u32 cfgr = rcc::RCC->CFGR;
        cfgr &= ~(HPRE | PPRE1 | PPRE2);
        cfgr |= (0x5 << 10);  // PPRE1 = 0b101 = /4
        cfgr |= (0x4 << 13);  // PPRE2 = 0b100 = /2
        rcc::RCC->CFGR = cfgr;

        // 5. Enable PLL
        rcc::RCC->CR |= PLLON;

        // Wait for PLL ready
        timeout = 1000;
        while (!(rcc::RCC->CR & PLLRDY) && timeout--) {
            if (timeout == 0) {
                return core::Result<void>::error(core::ErrorCode::PllLockFailed);
            }
        }

        // 6. Switch system clock to PLL
        rcc::RCC->CFGR = (rcc::RCC->CFGR & ~SW) | 0x2;  // SW = 0b10 = PLL

        // Wait for switch (SWS should be 0b10)
        while ((rcc::RCC->CFGR & SWS) != (0x2 << 2));

        system_frequency_ = (config.crystal_frequency_hz * config.pll_multiplier) / config.pll_divider / 2;
        return core::Result<void>::ok();
    }
};

// Static assertions to verify concept compliance
static_assert(hal::SystemClock<SystemClock>, "STM32F4 SystemClock must satisfy SystemClock concept");

} // namespace alloy::hal::st::stm32f4

#endif // ALLOY_HAL_ST_STM32F4_CLOCK_HPP
