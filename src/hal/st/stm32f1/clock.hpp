/// STM32F1 Clock Configuration Implementation
///
/// Implements the clock interface for STM32F1 family (Cortex-M3, 72MHz max)
/// Uses auto-generated peripheral definitions from SVD

#ifndef ALLOY_HAL_ST_STM32F1_CLOCK_HPP
#define ALLOY_HAL_ST_STM32F1_CLOCK_HPP

#include "hal/interface/clock.hpp"
#include "core/types.hpp"
#include "generated/st/stm32f1/stm32f103c8/peripherals.hpp"

namespace alloy::hal::st::stm32f1 {

// Import generated peripherals
using namespace alloy::generated::stm32f103c8;

/// STM32F1 System Clock implementation
class SystemClock {
public:
    SystemClock() : system_frequency_(8000000) {}  // Default HSI = 8MHz

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
        if (frequency_hz <= 8000000) {
            // Use HSI directly
            return configure_hsi();
        } else if (frequency_hz == 72000000) {
            // Use standard 72MHz config (HSE 8MHz * 9)
            ClockConfig config{
                .source = ClockSource::ExternalCrystal,
                .crystal_frequency_hz = 8000000,
                .pll_multiplier = 9,
                .ahb_divider = 1,
                .apb1_divider = 2,
                .apb2_divider = 1
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
        return system_frequency_;  // STM32F1: AHB = SYSCLK (no divider in simple config)
    }

    /// Get APB1 frequency (max 36MHz)
    core::u32 get_apb1_frequency() const {
        return system_frequency_ / 2;  // APB1 = SYSCLK / 2 @ 72MHz
    }

    /// Get APB2 frequency
    core::u32 get_apb2_frequency() const {
        return system_frequency_;  // APB2 = SYSCLK
    }

    /// Get peripheral frequency
    core::u32 get_peripheral_frequency(Peripheral periph) const {
        // Simple implementation: APB1 or APB2 based on peripheral
        core::u16 periph_val = static_cast<core::u16>(periph);
        if (periph_val >= 0x0100 && periph_val < 0x0200) {
            return get_apb2_frequency();  // UARTs on APB2
        }
        return get_apb1_frequency();
    }

    /// Enable peripheral clock
    core::Result<void> enable_peripheral(Peripheral periph) {
        using namespace rcc::apb2enr_bits;
        using namespace rcc::apb1enr_bits;

        switch (periph) {
            case Peripheral::GpioA: rcc::RCC->APB2ENR |= IOPAEN; break;
            case Peripheral::GpioB: rcc::RCC->APB2ENR |= IOPBEN; break;
            case Peripheral::GpioC: rcc::RCC->APB2ENR |= IOPCEN; break;
            case Peripheral::GpioD: rcc::RCC->APB2ENR |= IOPDEN; break;
            case Peripheral::GpioE: rcc::RCC->APB2ENR |= IOPEEN; break;
            case Peripheral::Uart1: rcc::RCC->APB2ENR |= USART1EN; break;
            case Peripheral::Uart2: rcc::RCC->APB1ENR |= USART2EN; break;
            case Peripheral::Uart3: rcc::RCC->APB1ENR |= USART3EN; break;
            default: return core::Result<void>::error(core::ErrorCode::InvalidParameter);
        }
        return core::Result<void>::ok();
    }

    /// Disable peripheral clock
    core::Result<void> disable_peripheral(Peripheral periph) {
        using namespace rcc::apb2enr_bits;
        using namespace rcc::apb1enr_bits;

        switch (periph) {
            case Peripheral::GpioA: rcc::RCC->APB2ENR &= ~IOPAEN; break;
            case Peripheral::GpioB: rcc::RCC->APB2ENR &= ~IOPBEN; break;
            case Peripheral::GpioC: rcc::RCC->APB2ENR &= ~IOPCEN; break;
            case Peripheral::GpioD: rcc::RCC->APB2ENR &= ~IOPDEN; break;
            case Peripheral::GpioE: rcc::RCC->APB2ENR &= ~IOPEEN; break;
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

    /// Configure HSI (8MHz internal oscillator)
    core::Result<void> configure_hsi() {
        using namespace rcc::cfgr_bits;

        // HSI is always on, just switch to it
        rcc::RCC->CFGR = (rcc::RCC->CFGR & ~SW) | 0x0;  // SW = 0b00 = HSI

        // Wait for switch (SWS should be 0b00)
        while ((rcc::RCC->CFGR & SWS) != 0x0);

        system_frequency_ = 8000000;
        return core::Result<void>::ok();
    }

    /// Configure HSE + PLL (8MHz → 72MHz)
    core::Result<void> configure_hse_pll(const ClockConfig& config) {
        using namespace rcc::cr_bits;
        using namespace rcc::cfgr_bits;

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
        set_flash_latency(config.crystal_frequency_hz * config.pll_multiplier);

        // 3. Configure PLL: HSE as source, multiplier
        // PLLMUL bits: multiplier-2 (e.g., 9x = 0b0111 = 7)
        core::u32 pll_mul = ((config.pll_multiplier - 2) << 18);
        rcc::RCC->CFGR = (rcc::RCC->CFGR & ~PLLMUL) | pll_mul | PLLSRC;

        // 4. Configure bus dividers (APB1 must be ≤ 36MHz)
        if (config.apb1_divider >= 2) {
            rcc::RCC->CFGR |= (0x4 << 8);  // PPRE1 = 0b100 = /2
        }

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

        system_frequency_ = config.crystal_frequency_hz * config.pll_multiplier;
        return core::Result<void>::ok();
    }
};

// Static assertions to verify concept compliance
static_assert(hal::SystemClock<SystemClock>, "STM32F1 SystemClock must satisfy SystemClock concept");

} // namespace alloy::hal::st::stm32f1

#endif // ALLOY_HAL_ST_STM32F1_CLOCK_HPP
