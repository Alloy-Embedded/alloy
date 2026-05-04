/// @file hal/syscfg.hpp
/// SYSCFG (System Configuration Controller) — lite driver.
///
/// No legacy descriptor-runtime required.
/// Include this header and use alloy::hal::syscfg::lite::controller<SyscfgBase>.
///
/// Used to route GPIO port pins to EXTI lines before enabling pin-change
/// interrupts.  Pair with hal/exti.hpp.
///
/// @see hal/syscfg/lite.hpp for the full API.
#pragma once

#include "hal/syscfg/lite.hpp"
