## Why

Bring-up of the seed drivers on SAME70 Xplained Ultra exposed a pattern: every
probe has to bypass the published device contract and poke MMIO directly
because the generated runtime artifacts do not carry enough semantic info for
the HAL to drive the peripheral. Concrete evidence collected in
`driver_at24mac402_probe`, `driver_is42s16100f_probe`, and
`driver_ksz8081_probe`:

- **I2C driver semantics are 151/151 fields emitted as `kInvalidFieldRef`.**
  The TWIHS backend could not reference `CR.START`, `CR.STOP`, or `CR.MSEN`
  through `RuntimeFieldRef`, forcing raw bit writes in
  `src/hal/i2c/detail/backend.hpp` (`kTwihsCrStart`, `kTwihsCrStop`, etc.).
- **Pin multiplexing has no C++ helper.** `routes.hpp` carries the correct
  `(PinId, PeripheralId, SignalId) → selector` table, but there is no
  generated `alloy::pinmux::route<Pin, Peripheral, Signal>()` function, so
  every probe reimplements `ABCDSR1 / ABCDSR2 / PDR` by hand — and gets it
  wrong (IS42S16100F probe muxed PIOD to peripheral A when the board wires it
  to peripheral C; missed PIOE entirely for D8..D13; the first attempt also
  muxed PD30 for NBS1 which does not exist on this part).
- **PMC `enable_peripheral` has no typed caller.** Every probe writes
  `PMC_PCER0 / PMC_PCER1` directly with magic PID numbers. The generated
  `ClockGateId` + `clock_bindings.hpp` artifacts are already present; what is
  missing is a `alloy::clock::enable(PeripheralId::X)` free function that
  uses them.
- **Peripheral base addresses leak into user code.** KSZ8081 probe hardcoded
  `kGmacBase = 0x40034000u` (XDMAC). The correct base was always available in
  `PeripheralInstanceTraits<PeripheralId::GMAC>::kBaseAddress`
  (`0x40050000u`), but there is no `alloy::device::base<PeripheralId>()`
  accessor that would have made the wrong literal impossible to write.
- **PIO GPIO output for "release this reset pin" style one-offs** bypasses
  `alloy::hal::gpio` because the reset pin has no associated peripheral in
  the published contract — the HAL has no idiomatic way to say "PIOC bit 10,
  push-pull output, drive high".

The impact on adopters is concrete: three of the three successful probes
needed between 20 and 80 lines of undocumented MMIO each, and at least two
bring-up bugs (IS42S16100F pinmux, KSZ8081 base address) were caused
specifically because the HAL let the probe author retype information that
already existed in the generated contract.

## What Changes

Codegen work (lands in `alloy-codegen` + re-published `alloy-devices`):

1. **Emit semantic `FieldRef` for every known CR / SR / MR / MAN field of
   TWIHS, SPI, USART, GMAC, SDRAMC, PMC.** Specifically: the register bit
   ranges are already documented in the SVD/YAML; the generator must wire
   them through the semantic-traits layer instead of falling back to
   `kInvalidFieldRef`. Minimum coverage required to retire every hardcoded
   bit mask currently in `src/hal/**/detail/backend.hpp`.
2. **Emit a typed pin-route helper** per `PeripheralId + SignalId + PinId`
   triple. Shape:
   `alloy::pinmux::route<PinId::PD0, PeripheralId::GMAC, SignalId::signal_gtxck>()`
   SHALL write the correct `ABCDSR1`, `ABCDSR2`, and `PDR` bits for the
   selector value the contract already stores in `routes.hpp`. The helper is
   `constexpr`-friendly so failed routes fail at compile time.
3. **Emit `alloy::clock::enable(PeripheralId)` / `disable(PeripheralId)`**
   free functions that consume `ClockGateId` + `clock_bindings.hpp` and
   write the correct `PMC_PCER0/PCER1` (or vendor equivalent) bit. No magic
   PID numbers in user code.
4. **Emit `alloy::device::base<PeripheralId>()`** returning
   `PeripheralInstanceTraits<Id>::kBaseAddress`. Probes and HAL backends stop
   typing base-address literals.
5. **Add a GPIO-only peripheral binding** so `alloy::hal::gpio::configure`
   accepts pins like "PIOC bit 10" without needing a peripheral signal.

Alloy-side follow-ups (land in this repo once the above republishes):

6. Refactor `src/hal/i2c/detail/backend.hpp` (TWIHS path) to consume the
   emitted `CR.START/STOP/MSEN/SVDIS/MSDIS` FieldRefs and drop the
   `kTwihsCr*` / `kTwihsSr*` raw masks.
7. Refactor the three passing probes (`driver_at24mac402_probe`,
   `driver_is42s16100f_probe`, `driver_ksz8081_probe`) so the only code
   outside the public HAL is peripheral-specific logic (JEDEC init sequence
   for SDRAM, MDIO transaction framing, etc.). All pinmux, PMC enable, and
   base-address references go through the generated helpers from items
   (2)–(4).
8. Extend `tests/compile/contract_smoke.cmake` with a new smoke group that
   asserts, for every published device, that the I2C / SPI / USART / GMAC /
   SDRAMC / PMC semantic traits have zero `kInvalidFieldRef` entries in the
   field sets the HAL declares a dependency on.

Out of scope for this change:

- New peripheral drivers beyond what items (1)–(8) already touch.
- Low-power / sleep-mode semantics — tracked separately under §2 of
  `promote-foundational-peripheral-coverage`.
- Any SAM E70–specific corrections beyond what the generator change
  automatically produces when re-running against the existing YAML.

## Outcome

After this change:

- Every probe example in `examples/driver_*/main.cpp` reads as "use the
  driver + use the HAL", with no MMIO pokes outside the peripheral
  bring-up sequences that genuinely require raw register ordering (SDRAMC
  JEDEC init, GMAC MDIO frame construction).
- The runtime contract carries enough semantic info that an adopter writing
  a new driver can build against the public HAL without reading the SAM E70
  datasheet to recover bit positions the generator already knew about.
- The "I2C semantic traits are 100% invalid" regression cannot come back:
  the compile-time smoke check fails if the generator emits another
  `kInvalidFieldRef` wave.

## Impact

- Affected specs:
  - modified: `runtime-device-boundary`
- Affected code (upstream):
  - `alloy-codegen/src/alloy_codegen/runtime_driver_semantics.py` (field
    emission)
  - `alloy-codegen/src/alloy_codegen/runtime_pinmux.py` (new pinmux helper
    emission)
  - `alloy-codegen/src/alloy_codegen/runtime_clock_bindings.py` (enable/
    disable helper emission)
  - `alloy-codegen/src/alloy_codegen/runtime_peripheral_instances.py`
    (`base<>` accessor emission)
  - `alloy-devices/microchip/same70/generated/**` (re-emitted)
- Affected code (this repo, once republished):
  - `src/hal/i2c/detail/backend.hpp` (drop raw CR/SR masks)
  - `examples/driver_at24mac402_probe/main.cpp`
  - `examples/driver_is42s16100f_probe/main.cpp`
  - `examples/driver_ksz8081_probe/main.cpp`
  - `tests/compile/contract_smoke.cmake`
