/// @file hal/rcc.hpp
/// Clock-gate and reset helpers for alloy.device.v2.1 peripherals.
///
/// Provides typed, compile-time-dispatched `peripheral_on<P>()`,
/// `clk_enable<P>()`, `clk_disable<P>()`, `rst_assert<P>()`, and
/// `rst_release<P>()` for every peripheral that has RCC metadata.
///
/// All functions are generated per-device by alloy-codegen into
/// `device/<vendor>/<family>/<device>/rcc_enable.hpp` and live in the
/// device namespace (e.g. `alloy::st::stm32g0::stm32g071rb`).
/// They are accessible via the `alloy::device::traits` alias when
/// `ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE` is true.
///
/// Typical usage:
/// @code
///   #include "hal/gpio.hpp"
///   #include "hal/uart.hpp"
///   #include "hal/rcc.hpp"
///   #include "device/runtime.hpp"
///
///   namespace dev = alloy::device::traits;
///
///   void board_init() {
///       // Enable clocks and deassert resets before configuring peripherals.
///       dev::peripheral_on<dev::gpioa>();
///       dev::peripheral_on<dev::usart1>();
///
///       using Gpioa = alloy::hal::gpio::lite::port<dev::gpioa>;
///       Gpioa::configure_af(2, 1);   // PA2 = USART2 TX on G071: AF1
///
///       using Uart1 = alloy::hal::uart::lite::port<dev::usart1>;
///       Uart1::configure({.baudrate = 115200, .clock_hz = 16'000'000u});
///   }
/// @endcode
///
/// Available API per peripheral (when `ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE`):
///   - `dev::peripheral_on<P>()` — enable clock + deassert reset.
///   - `dev::clk_enable<P>()`    — enable the peripheral clock only.
///   - `dev::clk_disable<P>()`   — gate off the peripheral clock.
///   - `dev::rst_assert<P>()`    — assert (hold in) reset.
///   - `dev::rst_release<P>()`   — release from reset.
///
/// Family-specific notes
/// ----------------------
/// **STM32**: `peripheral_on` calls `clk_enable` + `rst_release` in that order.
///   Both target distinct RCC registers (APBxENR / APBxRSTR).
///
/// **RP2040**: The RESETS peripheral uses inverted polarity — clearing the bit
///   unresets (enables) the peripheral.  `peripheral_on` calls `rst_release`
///   only (which is equivalent to `clk_enable` on this family).
///
/// **Microchip SAMx** (`mclk` / `pm`): Set-bit = enable; no per-peripheral
///   reset path.  Use the peripheral's own `CTRLA.SWRST` for reset.
///
/// **Microchip SAME70 / SAMV71** (`pmc`): Write-1-to-PCER = enable,
///   write-1-to-PCDR = disable; no template reset path.
///
#pragma once

// The generated rcc_enable.hpp is included transitively via
// device/runtime.hpp → selected_config.hpp when
// ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE is set.
// Including device/runtime.hpp here ensures every TU that includes
// hal/rcc.hpp has access to the typed helpers without additional includes.
#include "device/runtime.hpp"
