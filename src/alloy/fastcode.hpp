// Run a hot function from fast RAM instead of flash.
//
//   ALLOY_FASTCODE void control_isr() { ... }   // no flash wait states
//
// Mark a control loop or ISR ALLOY_FASTCODE and it lands in the `.fastcode`
// linker section, copied from flash to fast RAM at startup (like `.data`), so it
// executes without the flash wait states a fast core pays at speed (e.g. 2 WS on
// a 64 MHz STM32G0). On Cortex-M the section maps into SRAM; on the classic
// ESP32 into IRAM. Zero cost when unused — the section is empty and the startup
// copy loop runs zero iterations.
//
// noinline is part of the contract: an inlined copy would run from the caller's
// (flash) section, defeating the point — so the out-of-line body stays the one
// that runs. Keep ALLOY_FASTCODE functions leaf-ish; anything they call that
// isn't also ALLOY_FASTCODE still runs from flash.

#pragma once

#define ALLOY_FASTCODE [[gnu::noinline, gnu::section(".fastcode")]]
