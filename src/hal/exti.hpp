/// @file hal/exti.hpp
/// STM32 EXTI (External Interrupt/Event Controller) — lite driver.
///
/// No legacy descriptor-runtime required.
/// Include this header and use alloy::hal::exti::lite::controller<ExtiBase>.
///
/// Pair with hal/syscfg.hpp for GPIO→EXTI line routing.
///
/// @see hal/exti/lite.hpp for the full API.
#pragma once

#include "hal/exti/lite.hpp"
