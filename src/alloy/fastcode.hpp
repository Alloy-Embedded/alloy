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

// Put a hot loop's STATE in the chip's coupled RAM.
//
//   ALLOY_FASTDATA float g_integrator = 0.0f;   // initialised: copied at boot
//   ALLOY_FASTBSS  float g_history[64];         // zeroed at boot
//
// Where a part has tightly- or core-coupled memory — DTCM on a Cortex-M7, CCM
// on an F3/F4/G4 — this lands there, and the core reaches it with no wait
// states and no contention with the flash bus. On a part with none, it lands in
// ordinary RAM and costs nothing: the section is emitted either way.
//
// OPT-IN PER SYMBOL, ON PURPOSE. Coupled memory hangs off the core rather than
// the bus matrix the peripheral DMA sees, so THE DMA CANNOT REACH IT. A buffer
// marked ALLOY_FASTDATA and handed to a DMA transfer moves nothing and reports
// nothing. That is why alloy does not sweep `.data` and `.bss` in here
// wholesale, which would break every DMA buffer in a program at once — it is
// also why the decision is yours per object rather than the framework's.
//
// Use it for what a control ISR touches every cycle: accumulator state, filter
// history, a lookup table read on the hot path. Not for anything a peripheral
// writes.
#define ALLOY_FASTDATA [[gnu::section(".fastdata")]]
#define ALLOY_FASTBSS [[gnu::section(".fastbss")]]
