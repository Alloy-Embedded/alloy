/// @file tests/compile_tests/test_lite_tier1_guard.cpp
/// Compile-time verification that ALLOY_ASSERT_VENDOR_STM32 guard compiles
/// cleanly when ALLOY_DEVICE_VENDOR_STM32=1 is also defined.
///
/// Task 1.4.2 of refactor-lite-hal-surface.
///
/// To verify the NEGATIVE case (guard fires when target is not STM32):
///   Build with -DALLOY_ASSERT_VENDOR_STM32 but WITHOUT -DALLOY_DEVICE_VENDOR_STM32=1
///   Expected: static_assert failure naming the mis-used driver file.

#define ALLOY_ASSERT_VENDOR_STM32
#define ALLOY_DEVICE_VENDOR_STM32 1

#include "hal/dma/lite.hpp"
#include "hal/exti/lite.hpp"
#include "hal/flash/lite.hpp"
#include "hal/crc/lite.hpp"
#include "hal/rng/lite.hpp"
#include "hal/pwr/lite.hpp"
#include "hal/syscfg/lite.hpp"
#include "hal/opamp/lite.hpp"
#include "hal/comp/lite.hpp"

// Instantiate each Tier 1 template so the static_asserts fire at instantiation.
// Using STM32G0 base addresses as representative values.

namespace {

using Dma1    = alloy::hal::dma::lite::controller<0x40020000u>;
using Mux1    = alloy::hal::dma::lite::dmamux<0x40020800u>;
using Exti    = alloy::hal::exti::lite::controller<0x40010400u>;
using Flash   = alloy::hal::flash::lite::controller<0x40022000u>;
using Crc     = alloy::hal::crc::lite::engine<0x40023000u>;
using Rng     = alloy::hal::rng::lite::engine<0x40025000u>;
using Pwr     = alloy::hal::pwr::lite::controller<0x40007000u>;
using Syscfg  = alloy::hal::syscfg::lite::controller<0x40010000u>;
using Opamp1  = alloy::hal::opamp::lite::engine<0x40007800u>;
using Comp1   = alloy::hal::comp::lite::engine<0x40010200u>;

// Basic sanity: each type is complete (sizeof does not trigger an error).
static_assert(sizeof(Dma1)   == 1u);  // zero-size tag type
static_assert(sizeof(Mux1)   == 1u);
static_assert(sizeof(Exti)   == 1u);
static_assert(sizeof(Flash)  == 1u);
static_assert(sizeof(Crc)    == 1u);
static_assert(sizeof(Rng)    == 1u);
static_assert(sizeof(Pwr)    == 1u);
static_assert(sizeof(Syscfg) == 1u);
static_assert(sizeof(Opamp1) == 1u);
static_assert(sizeof(Comp1)  == 1u);

}  // namespace
