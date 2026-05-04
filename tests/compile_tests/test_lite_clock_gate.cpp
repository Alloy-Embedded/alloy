/// @file tests/compile_tests/test_lite_clock_gate.cpp
/// Compile-time verification of clock_on() / clock_off() on all Tier 2 lite drivers (task 3.3.1).
///
/// Verifies:
///   - clock_on() / clock_off() exist and return void on every Tier 2 driver
///   - Methods are only available when P::kRccEnable is present (concept-gated)
///   - Methods compile without including dev::peripheral_on<>
///   - Without ALLOY_DEVICE_RCC_TABLE_AVAILABLE the bodies are no-ops (compile only)
///
/// All drivers are exercised with self-contained mock PeripheralSpecs.
/// No real device artifact or RCC table is needed for this compile test.

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
// Mock peripheral specs — all have kRccEnable so clock_on/clock_off compile
// ============================================================================

namespace mock {

struct usart1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40013800u;
    static constexpr const char*    kName        = "usart1";
    static constexpr const char*    kTemplate    = "usart";
    static constexpr const char*    kIpVersion   = "sci3_v1_3";
    static constexpr const char*    kRccEnable   = "rcc.apbenr2.usart1en";
    static constexpr const char*    kRccReset    = "rcc.apbrstr2.usart1rst";
    static constexpr unsigned       kIrqLines[]  = { 27u };
    static constexpr unsigned       kIrqCount    = 1u;
};

struct spi1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40013000u;
    static constexpr const char*    kName        = "spi1";
    static constexpr const char*    kTemplate    = "spi";
    static constexpr const char*    kIpVersion   = "spi2s1_v3_3";
    static constexpr const char*    kRccEnable   = "rcc.apbenr2.spi1en";
    static constexpr const char*    kRccReset    = "rcc.apbrstr2.spi1rst";
    static constexpr unsigned       kIrqLines[]  = { 25u };
    static constexpr unsigned       kIrqCount    = 1u;
};

struct i2c1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40005400u;
    static constexpr const char*    kName        = "i2c1";
    static constexpr const char*    kTemplate    = "i2c";
    static constexpr const char*    kIpVersion   = "i2c2_v1_1";
    static constexpr const char*    kRccEnable   = "rcc.apbenr1.i2c1en";
    static constexpr const char*    kRccReset    = "rcc.apbrstr1.i2c1rst";
    static constexpr unsigned       kIrqLines[]  = { 23u, 24u };
    static constexpr unsigned       kIrqCount    = 2u;
};

struct gpioa {
    static constexpr std::uintptr_t kBaseAddress = 0x50000000u;
    static constexpr const char*    kName        = "gpioa";
    static constexpr const char*    kTemplate    = "gpio";
    static constexpr const char*    kIpVersion   = "gpio2_v1_0";
    static constexpr const char*    kRccEnable   = "rcc.iopenr.gpioaen";
    static constexpr const char*    kRccReset    = "rcc.ioprstr.gpioarst";
    static constexpr unsigned       kPort        = 0u;
    static constexpr unsigned       kIrqLines[]  = { 7u };
    static constexpr unsigned       kIrqCount    = 1u;
};

struct tim1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40012C00u;
    static constexpr const char*    kName        = "tim1";
    static constexpr const char*    kTemplate    = "timer";
    static constexpr const char*    kIpVersion   = "gptimer_v2_0";
    static constexpr const char*    kRccEnable   = "rcc.apbenr2.tim1en";
    static constexpr const char*    kRccReset    = "rcc.apbrstr2.tim1rst";
    static constexpr unsigned       kIrqLines[]  = { 13u, 14u };
    static constexpr unsigned       kIrqCount    = 2u;
};

struct adc1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40012400u;
    static constexpr const char*    kName        = "adc1";
    static constexpr const char*    kTemplate    = "adc";
    static constexpr const char*    kIpVersion   = "aditf4_v2_3";
    static constexpr const char*    kRccEnable   = "rcc.apbenr2.adcen";
    static constexpr const char*    kRccReset    = "rcc.apbrstr2.adcrst";
    static constexpr unsigned       kIrqLines[]  = { 12u };
    static constexpr unsigned       kIrqCount    = 1u;
};

struct dac1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40007400u;
    static constexpr const char*    kName        = "dac1";
    static constexpr const char*    kTemplate    = "dac";
    static constexpr const char*    kIpVersion   = "dacif_v2_0";
    static constexpr const char*    kRccEnable   = "rcc.apbenr1.dac1en";
    static constexpr const char*    kRccReset    = "rcc.apbrstr1.dac1rst";
    static constexpr unsigned       kIrqLines[]  = { 19u };
    static constexpr unsigned       kIrqCount    = 1u;
};

struct rtc1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40002800u;
    static constexpr const char*    kName        = "rtc";
    static constexpr const char*    kTemplate    = "rtc";
    static constexpr const char*    kIpVersion   = "rtc_v2_2";
    static constexpr const char*    kRccEnable   = "rcc.bdcr.rtcen";
    static constexpr const char*    kRccReset    = "rcc.bdcr.bdrst";
    static constexpr unsigned       kIrqLines[]  = { 2u };
    static constexpr unsigned       kIrqCount    = 1u;
};

struct iwdg1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40003000u;
    static constexpr const char*    kName        = "iwdg";
    static constexpr const char*    kTemplate    = "iwdg";
    static constexpr const char*    kIpVersion   = "iwdg_v2_0";
    static constexpr const char*    kRccEnable   = "rcc.csr.lsion";  // IWDG clocked from LSI
    static constexpr const char*    kRccReset    = "";
    static constexpr unsigned       kIrqLines[]  = { 0u };
    static constexpr unsigned       kIrqCount    = 0u;
};

struct wwdg1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40002C00u;
    static constexpr const char*    kName        = "wwdg";
    static constexpr const char*    kTemplate    = "wwdg";
    static constexpr const char*    kIpVersion   = "wwdg_v2_0";
    static constexpr const char*    kRccEnable   = "rcc.apbenr1.wwdgen";
    static constexpr const char*    kRccReset    = "";
    static constexpr unsigned       kIrqLines[]  = { 0u };
    static constexpr unsigned       kIrqCount    = 0u;
};

struct lptim1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40007C00u;
    static constexpr const char*    kName        = "lptim1";
    static constexpr const char*    kTemplate    = "lptim";
    static constexpr const char*    kIpVersion   = "lptimer_v1_1";
    static constexpr const char*    kRccEnable   = "rcc.apbenr1.lptim1en";
    static constexpr const char*    kRccReset    = "rcc.apbrstr1.lptim1rst";
    static constexpr unsigned       kIrqLines[]  = { 17u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// Peripheral WITHOUT kRccEnable — clock_on/clock_off must NOT be callable.
struct usart_no_rcc {
    static constexpr std::uintptr_t kBaseAddress = 0x40014000u;
    static constexpr const char*    kName        = "usart2_no_rcc";
    static constexpr const char*    kTemplate    = "usart";
    static constexpr const char*    kIpVersion   = "sci3_v1_3";
    // No kRccEnable — clock_on/clock_off must be constrained away.
    static constexpr unsigned       kIrqLines[]  = { 28u };
    static constexpr unsigned       kIrqCount    = 1u;
};

}  // namespace mock

// ============================================================================
// Instantiate all drivers
// ============================================================================

using Uart1  = alloy::hal::uart::lite::port<mock::usart1>;
using Spi1   = alloy::hal::spi::lite::port<mock::spi1>;
using I2c1   = alloy::hal::i2c::lite::port<mock::i2c1>;
using GpioA  = alloy::hal::gpio::lite::port<mock::gpioa>;
using Tim1   = alloy::hal::timer::lite::port<mock::tim1>;
using Adc1   = alloy::hal::adc::lite::port<mock::adc1>;
using Dac1   = alloy::hal::dac::lite::port<mock::dac1>;
using Rtc1   = alloy::hal::rtc::lite::port<mock::rtc1>;
using Iwdg1  = alloy::hal::watchdog::lite::iwdg_port<mock::iwdg1>;
using Wwdg1  = alloy::hal::watchdog::lite::wwdg_port<mock::wwdg1>;
using Lptim1 = alloy::hal::lptim::lite::port<mock::lptim1>;

// ============================================================================
// Return-type checks — clock_on / clock_off → void
// ============================================================================

namespace {

static_assert(std::is_same_v<void, decltype(Uart1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Uart1::clock_off())>);

static_assert(std::is_same_v<void, decltype(Spi1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Spi1::clock_off())>);

static_assert(std::is_same_v<void, decltype(I2c1::clock_on())>);
static_assert(std::is_same_v<void, decltype(I2c1::clock_off())>);

static_assert(std::is_same_v<void, decltype(GpioA::clock_on())>);
static_assert(std::is_same_v<void, decltype(GpioA::clock_off())>);

static_assert(std::is_same_v<void, decltype(Tim1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Tim1::clock_off())>);

static_assert(std::is_same_v<void, decltype(Adc1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Adc1::clock_off())>);

static_assert(std::is_same_v<void, decltype(Dac1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Dac1::clock_off())>);

static_assert(std::is_same_v<void, decltype(Rtc1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Rtc1::clock_off())>);

static_assert(std::is_same_v<void, decltype(Iwdg1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Iwdg1::clock_off())>);

static_assert(std::is_same_v<void, decltype(Wwdg1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Wwdg1::clock_off())>);

static_assert(std::is_same_v<void, decltype(Lptim1::clock_on())>);
static_assert(std::is_same_v<void, decltype(Lptim1::clock_off())>);

// ============================================================================
// Concept-gating: peripheral WITHOUT kRccEnable must NOT expose clock_on/off
// ============================================================================

using UartNoRcc = alloy::hal::uart::lite::port<mock::usart_no_rcc>;

static_assert(!requires { UartNoRcc::clock_on(); },
    "clock_on() must not exist when P::kRccEnable is absent");
static_assert(!requires { UartNoRcc::clock_off(); },
    "clock_off() must not exist when P::kRccEnable is absent");

}  // namespace
