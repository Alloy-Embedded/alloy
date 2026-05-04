/// @file tests/compile_tests/test_lite_irq_bridge.cpp
/// Compile-time verification of the device-data bridge (irq_number / irq_count)
/// added to all Tier 2 lite drivers in task 1.2.x of refactor-lite-hal-surface.
///
/// Uses self-contained mock peripheral structs that satisfy the alloy.device.v2.1
/// PeripheralSpec concept, including the kIrqLines / kIrqCount fields.
/// No real device artifact is required — this test is fully host-buildable.

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "hal/uart/lite.hpp"
#include "hal/spi/lite.hpp"
#include "hal/i2c/lite.hpp"
#include "hal/gpio/lite.hpp"
#include "hal/timer/lite.hpp"
#include "hal/adc/lite.hpp"
#include "hal/dac/lite.hpp"
#include "hal/rtc/lite.hpp"
#include "hal/watchdog/lite.hpp"
#include "hal/lptim/lite.hpp"

// ============================================================================
// Mock peripheral specs (simulate alloy.device.v2.1 flat-struct)
// ============================================================================

namespace mock {

/// STM32G0-like USART1 (SCI3, modern)
struct usart1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40013800u;
    static constexpr const char*    kName        = "usart1";
    static constexpr const char*    kTemplate    = "usart";
    static constexpr const char*    kIpVersion   = "sci3_v1_3";
    static constexpr unsigned       kIrqLines[]  = { 27u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// STM32G0-like SPI1
struct spi1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40013000u;
    static constexpr const char*    kName        = "spi1";
    static constexpr const char*    kTemplate    = "spi";
    static constexpr const char*    kIpVersion   = "spi2s1_v3_5";
    static constexpr unsigned       kIrqLines[]  = { 25u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// STM32G0-like I2C1
struct i2c1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40005400u;
    static constexpr const char*    kName        = "i2c1";
    static constexpr const char*    kTemplate    = "i2c";
    static constexpr const char*    kIpVersion   = "i2c2_v1_1";
    static constexpr unsigned       kIrqLines[]  = { 23u, 24u };  // event + error
    static constexpr unsigned       kIrqCount    = 2u;
};

/// STM32G0-like GPIOA
struct gpioa {
    static constexpr std::uintptr_t kBaseAddress = 0x50000000u;
    static constexpr const char*    kName        = "gpioa";
    static constexpr const char*    kTemplate    = "gpio";
    static constexpr const char*    kIpVersion   = "gpio2_v1_0";
    static constexpr unsigned       kIrqLines[]  = { 7u };  // EXTI0_1 on G0
    static constexpr unsigned       kIrqCount    = 1u;
};

/// STM32G0-like TIM1 (advanced — 4 IRQ lines: BRK/UP, CC, TRG, COM)
struct tim1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40012C00u;
    static constexpr const char*    kName        = "tim1";
    static constexpr const char*    kTemplate    = "tim";
    static constexpr const char*    kIpVersion   = "tim_mptm_v2_0";
    static constexpr unsigned       kIrqLines[]  = { 13u, 14u };  // BRK/UP/TRG, CC
    static constexpr unsigned       kIrqCount    = 2u;
};

/// STM32G0-like ADC1 (modern)
struct adc1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40012400u;
    static constexpr const char*    kName        = "adc1";
    static constexpr const char*    kTemplate    = "adc";
    static constexpr const char*    kIpVersion   = "adc_g0_v1_0";
    static constexpr unsigned       kIrqLines[]  = { 12u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// STM32G0-like DAC1
struct dac1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40007400u;
    static constexpr const char*    kName        = "dac1";
    static constexpr const char*    kTemplate    = "dac";
    static constexpr const char*    kIpVersion   = "dac_v3_0";
    static constexpr unsigned       kIrqLines[]  = { 19u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// STM32G0-like RTC (layout A)
struct rtc {
    static constexpr std::uintptr_t kBaseAddress = 0x40002800u;
    static constexpr const char*    kName        = "rtc";
    static constexpr const char*    kTemplate    = "rtc";
    static constexpr const char*    kIpVersion   = "rtc_v2_3";
    static constexpr unsigned       kIrqLines[]  = { 2u, 3u };  // alarm, wakeup
    static constexpr unsigned       kIrqCount    = 2u;
};

/// STM32G0-like IWDG
struct iwdg {
    static constexpr std::uintptr_t kBaseAddress = 0x40003000u;
    static constexpr const char*    kName        = "iwdg";
    static constexpr const char*    kTemplate    = "iwdg";
    static constexpr const char*    kIpVersion   = "iwdg_v2_0";
    static constexpr unsigned       kIrqLines[]  = { 16u };  // EWI on select STM32
    static constexpr unsigned       kIrqCount    = 1u;
};

/// STM32G0-like WWDG
struct wwdg {
    static constexpr std::uintptr_t kBaseAddress = 0x40002C00u;
    static constexpr const char*    kName        = "wwdg";
    static constexpr const char*    kTemplate    = "wwdg";
    static constexpr const char*    kIpVersion   = "wwdg_v2_0";
    static constexpr unsigned       kIrqLines[]  = { 0u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// STM32G0-like LPTIM1
struct lptim1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40007C00u;
    static constexpr const char*    kName        = "lptim1";
    static constexpr const char*    kTemplate    = "lptim";
    static constexpr const char*    kIpVersion   = "lptimv2_v1_0";
    static constexpr unsigned       kIrqLines[]  = { 17u };
    static constexpr unsigned       kIrqCount    = 1u;
};

}  // namespace mock

// ============================================================================
// irq_number() / irq_count() — return-type checks
// ============================================================================

namespace {

using Uart1   = alloy::hal::uart::lite::port<mock::usart1>;
using Spi1    = alloy::hal::spi::lite::port<mock::spi1>;
using I2c1    = alloy::hal::i2c::lite::port<mock::i2c1>;
using GpioA   = alloy::hal::gpio::lite::port<mock::gpioa>;
using Tim1    = alloy::hal::timer::lite::port<mock::tim1>;
using Adc1    = alloy::hal::adc::lite::port<mock::adc1>;
using Dac1    = alloy::hal::dac::lite::port<mock::dac1>;
using Rtc1    = alloy::hal::rtc::lite::port<mock::rtc>;
using Iwdg1   = alloy::hal::watchdog::lite::iwdg_port<mock::iwdg>;
using Wwdg1   = alloy::hal::watchdog::lite::wwdg_port<mock::wwdg>;
using Lptim1  = alloy::hal::lptim::lite::port<mock::lptim1>;

// irq_number() returns uint32_t
static_assert(std::is_same_v<decltype(Uart1::irq_number()),  std::uint32_t>);
static_assert(std::is_same_v<decltype(Spi1::irq_number()),   std::uint32_t>);
static_assert(std::is_same_v<decltype(I2c1::irq_number()),   std::uint32_t>);
static_assert(std::is_same_v<decltype(GpioA::irq_number()),  std::uint32_t>);
static_assert(std::is_same_v<decltype(Tim1::irq_number()),   std::uint32_t>);
static_assert(std::is_same_v<decltype(Adc1::irq_number()),   std::uint32_t>);
static_assert(std::is_same_v<decltype(Dac1::irq_number()),   std::uint32_t>);
static_assert(std::is_same_v<decltype(Rtc1::irq_number()),   std::uint32_t>);
static_assert(std::is_same_v<decltype(Iwdg1::irq_number()),  std::uint32_t>);
static_assert(std::is_same_v<decltype(Wwdg1::irq_number()),  std::uint32_t>);
static_assert(std::is_same_v<decltype(Lptim1::irq_number()), std::uint32_t>);

// irq_count() returns size_t
static_assert(std::is_same_v<decltype(Uart1::irq_count()),   std::size_t>);
static_assert(std::is_same_v<decltype(I2c1::irq_count()),    std::size_t>);
static_assert(std::is_same_v<decltype(Tim1::irq_count()),    std::size_t>);
static_assert(std::is_same_v<decltype(Lptim1::irq_count()),  std::size_t>);

// ============================================================================
// irq_number() value matches kIrqLines (compile-time)
// ============================================================================

static_assert(Uart1::irq_number()   == 27u);
static_assert(Spi1::irq_number()    == 25u);
static_assert(I2c1::irq_number()    == 23u);   // idx=0: event IRQ
static_assert(I2c1::irq_number(1u)  == 24u);   // idx=1: error IRQ
static_assert(GpioA::irq_number()   == 7u);
static_assert(Tim1::irq_number()    == 13u);
static_assert(Tim1::irq_number(1u)  == 14u);
static_assert(Adc1::irq_number()    == 12u);
static_assert(Dac1::irq_number()    == 19u);
static_assert(Rtc1::irq_number()    == 2u);
static_assert(Rtc1::irq_number(1u)  == 3u);
static_assert(Iwdg1::irq_number()   == 16u);
static_assert(Wwdg1::irq_number()   == 0u);
static_assert(Lptim1::irq_number()  == 17u);

// ============================================================================
// irq_count() value matches kIrqCount
// ============================================================================

static_assert(Uart1::irq_count()   == 1u);
static_assert(I2c1::irq_count()    == 2u);
static_assert(Tim1::irq_count()    == 2u);
static_assert(Rtc1::irq_count()    == 2u);
static_assert(Lptim1::irq_count()  == 1u);

// ============================================================================
// irq_number() / irq_count() are constexpr (usable in constant expressions)
// ============================================================================

static_assert(noexcept(Uart1::irq_number()));
static_assert(noexcept(Uart1::irq_count()));
static_assert(noexcept(I2c1::irq_number(1u)));

}  // namespace
