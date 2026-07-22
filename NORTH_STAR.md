# North Star — non-negotiables

This repo is a from-scratch rebuild of the Alloy framework, informed by a full audit of the
previous ecosystem (2026-07-22) and by how modm, embassy/stm32-data, Zephyr and PlatformIO
actually scale. This file is the contract that keeps us honest. **When a shortcut conflicts
with this file, the shortcut loses** — even when the shortcut is faster, even mid-migration.

## The four goals

1. **Coherent**: one architecture, two repos (`alloy`, `alloy-devices`), one CI story.
2. **Automated families**: adding a chip that reuses known peripheral IP versions costs
   **zero new C++ and zero new emitter branches** — only data.
3. **Portable user code**: the SAME `main.cpp` recompiles for another board/MCU by changing
   one config line. Zero preprocessor conditionals in app code, ever.
4. **Trivially easy**: `pipx install alloy && alloy new && alloy run` → blinking LED in
   minutes. Errors at compile time, readable by a beginner.

## The governing rule

> **FACTS ARE GENERATED. BEHAVIOR IS HAND-WRITTEN.**

Facts: addresses, register offsets, bit positions, pin routes/AF numbers, clock gates, IRQ
numbers, memory sizes, clock frequencies, vector tables, linker layouts.
Behavior: driver sequencing, quirks, errata handling, the reset handler, API design.

Facts live in `alloy-devices` data and reach C++ only through codegen. Behavior is human
code that consumes generated facts by name.

## Non-negotiable guards (each one is a CI gate, not a wish)

1. **No silicon fact in hand-written code.** No hex peripheral address, register offset,
   bit position, AF number, IRQ number or memory size in `src/`, `tools/`, or `examples/`.
   Enforced: `scripts/check_contract.sh` (grep gate) — runs in CI from day one.
2. **One driver per peripheral IP version**, selected by tag type
   (`requires std::same_as<typename Inst::ip, ip::st::usart_v4>`). Never per-chip drivers.
   Never consteval string comparison for dispatch.
3. **Zero `#if`/`#ifdef` in examples/ and in any user-facing snippet.** Portability comes
   from board roles + `if constexpr (board::caps::x)` + concepts. Enforced by grep gate.
4. **Every fix is reproducible by the pipeline** (modm's rule). Hand-editing generated
   output is forbidden; fix the data or the emitter and regenerate. Generated trees carry a
   `// GENERATED — DO NOT EDIT` header and live only under `.alloy/` (gitignored).
5. **Route model carries `route_kind` from day one**: `af_fixed | funcsel | full_matrix |
   psel`. The STM32 AF model is NOT universal (ESP32 matrix, nRF PSEL, RP2040 FUNCSEL);
   pretending it is was a root cause of the old ecosystem's failure.
6. **Fixed board-role schema.** Every generated board provides the full contract:
   `board::init()`, `board::led`, `board::caps::*`, and always-declared role objects
   (no-op stubs when absent) so `if constexpr` compiles everywhere. An app that uses a
   role a board truly lacks fails at compile time with a static_assert message.
7. **Honest compile-time claims.** Wrong pin route → `static_assert` naming pin and
   peripheral. Double-open → runtime debug assert (C++ cannot make it a link error across
   TUs; do not pretend otherwise). Error messages must fit on one screen — that is an
   acceptance test, not a nice-to-have.
8. **No feature ships unless CI compiles what it emits.** The old `alloy add` emitted
   decorative C++ and destroyed trust. Scaffold output is compiled in CI or the command
   does not exist.
9. **Honest support claims.** A board/family is "supported" only when CI compiles
   blink + uart_echo for it and its data passed plausibility lints. The capability matrix
   is generated from data, never hand-written.
10. **Never break the working set.** Once a board builds in CI, every commit keeps it
    building. Validation before continuation — no error accumulation.

## Scope discipline (v1)

ST STM32G0 first, then SAME70, then RP2040 — all three hardware-validated 2026-07-22.
nRF52840 remains the proof that "new family = data only" works. AVR (needs rw8 overlays +
freestanding audit) stays deferred. Depth before breadth: boards that fully work beat 588
admitted devices with 3 working.

**Amendment (2026-07-22, owner decision):** classic ESP32 (Xtensa LX6) is promoted to the
active queue — the owner's validation hardware is an ESP-WROVER-KIT. The schedule-killer
risks the audit flagged are contained by explicit v1 constraints, honestly surfaced in
caps: the app is RAM-only and loaded by the boot ROM (no second-stage bootloader, no
XIP/flash cache), clocks stay at ROM/XTAL defaults (no clock-tree bring-up; timebase =
CCOUNT/CCOMPARE), and only GPIO-matrix routing + UART0 + GPIO are in scope. Anything
beyond that (XIP, PSRAM, Wi-Fi, 240 MHz) is explicitly out until the family earns it.

## Known traps (from the audit — do not repeat)

- Duplicating a silicon fact "just for now" in an emitter → became 918-line emit_board.py.
- Shipping a scaffold that isn't compiled by CI → broken `alloy new` for months.
- Validating against a stale schema → 3,126 files silently failing CI.
- Hand-editing generated output to fix a hardware bug → fix lost, data still wrong.
- Board-specific helpers leaking into user code (`make_gpio_pd25`) → 5 dead examples.
- Toolchain SHAs left as placeholders → installer that can never work.

Full audit & design report: https://claude.ai/code/artifact/71838bf1-ed24-4357-8802-76e78fa32552
