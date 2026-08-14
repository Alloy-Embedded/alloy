# Changelog

Versions are `MAJOR.MINOR.PATCH`. What each number is allowed to break, which
headers and commands are covered, and how much notice a removal gets are
written down in [docs/reference/stability.md](docs/reference/stability.md).
Read that before planning a product around a version — in particular the part
about pinning `alloy-devices`, which carries the register facts and lives in
its own repo on its own tags.

Entries marked **Deprecated** keep working for at least one MINOR release and
are removed no earlier than the next MAJOR; each one names its replacement.

## Unreleased

### New

- **`alloy symbols` and a `[budget]` table — proof that the code landed where
  the linker script said.** `size` reports four numbers; they tell you the
  image fits and nothing about placement. `alloy symbols` reads the ELF, sorts
  by address, demangles, and shows each section's run address beside its load
  address — so a section that startup must copy is visibly distinct from one
  that runs where it is stored:

  ```
  section          size      vma        lma
  .text              3776  0x080000c0 0x080000c0
  .fastcode            20  0x20000000 0x08000f88  copied from flash at startup
  .bss               1152  0x20000018 0x08000f9c  zeroed/reserved, not loaded
  ```

  `--section .fastcode` answers "did my hot function actually get there" in one
  command; `--json` is the `alloy.symbols.v1` envelope for the IDE.

  An optional `[budget]` table in `alloy.toml` turns a section ceiling into a
  build failure:

  ```toml
  [budget]
  ".fastcode" = 64
  ```

  A budgeted section that is missing **or empty** fails too. That is the case
  worth guarding: deleting an `ALLOY_FASTCODE` attribute leaves the section
  behind at zero bytes, every total still looks right, and the hot path
  quietly runs from flash for the rest of the product's life. `examples/
  fastcode` now carries a budget, and CI asserts its symbol really sits at a
  RAM address loaded from flash.

  This came out of reading an eight-repository firmware estate whose linker
  script placed sixteen hot functions in tightly-coupled memory that no
  startup ever copied — documented intent, absent mechanism, and no artefact
  anywhere that could have shown it.

- **DMA streams, phase 2: `uart.rx_ring()`, `alloy::dma::claim(route)`, and
  the Modbus RTU server on the ring** (docs/design/dma-streams.md §2.2/§2.3).
  Where the board assigns `debug_uart.rx`, the uart handle grows the anchor
  shape — received bytes land in a caller-owned ring BY DMA, no interrupt per
  byte, no CPU read of the data register:

      static alloy::dma::ring_storage<std::uint8_t, 256> rxbuf;
      auto ring = uart.rx_ring(rxbuf);   // claims, arms, raises DMAR + IDLE
      for (;;) {
          alloy::sleep_until_event();
          auto bytes = ring.readable();  // live count register, no ISR ran
          if (!bytes.empty() && server.on_bytes(bytes)) {
              ring.consume(bytes.size());
          }
      }

  The generator attaches the assignment to the binder (`rx_dma<>`/`tx_dma<>`
  tags -> `rx_route`/`tx_route`), so boards that assign nothing fold the
  methods away at a named requires-gate, and `alloy::dma::claim(route)` turns
  the board's TX assignment into the channel token the shipped
  `write_dma_begin/end` + async `dma_waiter` pair consume (anchor 2.3 — the
  async_io example now claims its channel from the board fact). libs/modbus
  0.5.0 adds `rtu_server::on_bytes(span)` — the batch entry sharing one
  delivery path with `poll()`. Both anchors are asserted under emulation on
  the wired G0 Nucleos by the EXISTING `modbus_rtu.robot` (every assertion
  unchanged, plus a ring-path marker the polled fallback cannot print) and
  `async_io.robot` (route-claim marker; dma1 ch3's shared-IRQ line). Stated
  limits: the pinned Renode USART model implements no IDLE, so the IDLE
  frame-gap wake is silicon-only (SysTick wakes the loop under emulation),
  and t3.5 framing never depended on sub-millisecond wakes.

- **DMA streams, phase 1: `alloy::dma::ring`, board-assigned routes, and
  `adc.ring()`** (docs/design/dma-streams.md). A DMA route is now a generated
  fact: the board states which channel serves which role signal —

      "dma": { "adc.conv": {"controller": "dma1", "channel": 1} }

  — the generator validates it against the chip's routing data (collisions,
  out-of-range channels, and signals without a request are refused with the
  legal alternatives; `alloy board-validate` reports the same problems with
  locations) and emits a typed `board::dma` constant. The request id never
  appears in board.json — it is the chip's fact. On top of the route,
  `ring<T>`/`ring_storage<T, N>` stream circular DMA into a caller-owned
  buffer with half/full boundary events, and the ADC facade composes the two:

      alloy::dma::ring_storage<std::uint16_t, 256> samples;
      auto adc = board::adc::open();
      auto ring = adc.ring(samples, /*channel=*/3);
      // consumer: ring.pending() / ring.take() — hardware-stable halves

  Destroying the ring stops the hardware and releases the channel claim.
  Boards that assign nothing still build, and a program that streams nothing
  pays zero bytes (measured: byte-identical images). The half-before-full
  event order and the delivered values are asserted under emulation on
  `nucleo_g0b1re` and `nucleo_g071rb` (`examples/adc_stream`); DMAMUX request
  routing itself is not witnessable under Renode and awaits silicon. One
  route ships assigned (`adc.conv`); UART/SPI streams are the next phase.

- **`alloy::tick` — a hardware timer as a periodic time base, over four ST IP
  versions with one driver.** The STM32G0's TIM6, TIM7, TIM14, TIM15, TIM16 and
  TIM17 were `uncurated` and unreachable; they are curated in `alloy-devices`
  now, and this is what reaches them.

      auto t = board::tick::open({.hz = 10});
      while (true) { if (t.expired()) { led.toggle(); } }

  The handle answers `expired()` (consuming, so a polling loop counts periods),
  `count()`, `restart()`, `stop()`, `start()`, `irq_on_update()`,
  `dma_on_update()`, and `achieved_hz()` — the rate the block ACTUALLY runs at,
  which is rarely the requested one because the division is integer.

  `trigger_on_update()` — point the timer's TRGO at the update event, the
  standard way to clock an ADC or a DAC — is constrained on
  `Inst::feat::trgo`, a GENERATED number that is 0 on TIM14, TIM16 and TIM17
  (those blocks have no MMS field: no CR2 at all, or a CR2 whose bits 6:4 are
  the output-idle state). Calling it there is a compile error naming the
  instance; the hardware's answer would have been a trigger that never fires.

  **A new `tick` board role**, with no pins — the only board fact is which
  block is free — and `hz` as a project field, overridable from `alloy.toml`.
  `nucleo_g0b1re` binds it to TIM6. **A new `personality::tick`**, so a tick
  and a PWM channel on one timer trap instead of silently agreeing; `pwm`
  claims a timer SHARED on a frequency because four channels are four owners
  of one personality, and `tick` claims it EXCLUSIVE because a time base has
  no channels to share.

  `examples/tick` builds on **9 of 9 boards** with no preprocessor, and a
  program that never opens a tick pays nothing: `examples/blink` on
  `nucleo_g0b1re` is byte-identical before and after (3396 text / 8 data /
  3240 bss), measured rather than asserted.

  **Not witnessed on silicon.** Renode 1.16.1 binds no model to any of these
  instances on this die, so there is no emulation leg to add and none is
  claimed; no board was attached. The 11 new host tests cover the divisor
  arithmetic and the ownership rule, and nothing executes the register
  sequence.

### Fixed

- **`alloy::tick` admitted rates ABOVE the timer's kernel clock**, found by its
  own test sweep before the facade shipped. The divider is rounded to nearest,
  so a request of `kernel_hz + 1` came back as a divider of one: a legal
  register pair, a timer running at the kernel clock, and a reported success.
  Every rate between 1x and 2x the kernel had that shape.

- **The ADC analog watchdog (`alloy/adc.hpp`), a SUB-RESOURCE with its own
  handle — and the emulation leg that was written for it, now executed.**
  `adc.watchdog<0>({.channel = 3, .low = 1000, .high = 3000})` arms one of the
  three watchdogs this IP carries, long after `open()`; the handle answers
  `tripped()`, `clear()`, `rearm()` and `disarm()`. The ordinal is checked
  against the GENERATED `Inst::feat::analog_watchdogs`, so `watchdog<3>()` is a
  compile error carrying both numbers rather than three registers nobody owns —
  and the ordinal has to be a template parameter because the silicon says so:
  watchdog 1's enable and channel are CFGR1 fields, watchdogs 2 and 3 own a
  19-bit channel bitmask each where a non-zero mask IS the enable, and the three
  threshold registers sit at 0x20, 0x24 and 0x2C, which is not a stride.
  `tests/emulation/adc_watchdog.robot` runs it on `nucleo_g0b1re` against
  Renode's own `Analog.STM32G0_ADC`: **PASS in 1.61 s** on pinned Renode 1.16.1,
  four lines that contradict each other unless the watchdog works. It is
  load-bearing, not decorative — with `AWD1EN` deleted from the driver the suite
  FAILS in 31.73 s on the "out-of-window ... TRIPPED" line. What it does NOT
  prove is written in the robot's own documentation: Renode's model is built
  with `watchdogCount: 1`, so ordinals 1 and 2 — which this die has and this
  driver programs — are never exercised, and neither is the AWD interrupt. No
  silicon has run any of it.
- **An EXTI line gets an owner, and the sub-resource scope gets the `shared`
  shape it was argued out of (`alloy/core/claim.hpp`,
  `alloy/hal/exti/exti_impl.hpp`).** The two entries below closed instance
  ownership and then sub-resource ownership, and the second one wrote down what
  a sub-resource is: *"no block-scoped register for two claimants to disagree
  about."* That was derived from a timer channel and a DMA channel, and the
  EXTI line — in the tree the whole time — refutes it. One line serves the same
  index on every port, so PA5 and PB5 are both line 5, sharing one EXTICR
  field, one trigger pair and one callback slot. Arming both compiled clean and
  said nothing: the second pin took the line and the FIRST pin's handle was left
  reporting the second pin's edges as its own. Proven on emulated silicon in
  9.1 s with exact counters (`a_edges=00000001` after an edge that only ever
  happened on PB5, `cb_a` never run). New `claim::sub_shared<Inst, Sub, P>(w)`
  keys the line on the instance and the ordinal with the PORT INDEX as witness,
  so re-arming one pin stays legal and a second pin traps
  `trap_code::sub_config_conflict`; new `claim::sub_release<Inst, Sub, P>(w)` is
  the mechanism's one release, because `gpio::input::clear_on_edge()` genuinely
  frees a line and refusing the handover would have been a new wrong answer in
  place of the old one — it also catches `pb5.clear_on_edge()` silently
  disarming PA5. The handover is proven positively under Renode (7.8 s). Cost on
  `nucleo_g0b1re`: `.text` **unchanged** for every example that arms no pin
  interrupt, +40 bytes and +8 bytes of `.bss` for `pin_irq`, which gains the
  claim. `gpio::input::disable_edge_irq()` now passes the port index to the
  driver's `disarm<PortIndex, Line>()`; the portable API is unchanged.
- **The claim inventory is a test, and the claim is shown to cross translation
  units.** Hole (A2) — a facade the reference page said claimed and did not —
  was found by reading the facades one at a time, which is not a thing that
  happens twice. `tools/alloy/tests/test_claim_surface.py` parses the "which
  facade claims what" table out of `docs/reference/peripheral-surface.md` and
  checks every row against the file it names, in both directions, plus a check
  that no peripheral class ships a driver directory without a row.
  `tests/test_claim_tu2.cpp` is a second translation unit naming the same
  instances as `tests/test_claim.cpp`: it proves that `claim::owner<Inst>` is
  one object across the image and not one per compilation — the property the
  whole repair rests on, and the one no single-`.cpp` test could distinguish
  from the per-binder `static` it replaced.
- **CAN acceptance filters (`alloy/can.hpp`), and what a two-peripheral
  feature demanded.** `board::can.accept_only(alloy::can::match(0x123),
  alloy::can::match_masked(0x200, 0x7F0))` tells the controller to drop
  everything else in hardware; `accept_all()` restores the bring-up default.
  One line of user code, two blocks of silicon: the match ELEMENTS are words
  in the FDCAN's **companion message RAM**, the list size and the
  unmatched-frame policy are a register in the **controller** that only accepts
  writes inside a config window. The driver writes the elements first — a size
  published before its elements exist is a core scanning garbage — and takes
  the window exactly once, which is why the call is variadic rather than a
  builder.

  Three things a caller can see:

  - **Capacity is not where the register that counts it is.** `RXGFC.LSS` is
    five bits and would admit 31; the real number is 28 and it is the
    companion's `FLSSA_count`. `accept_only` static_asserts against the
    companion's number, and both numbers appear in the diagnostic because they
    are template arguments (`filters_fit<29, 28>`).
  - **An identifier wider than eleven bits is refused**, and not for the
    reason the first draft gave: such a filter does not match *nothing*, it
    matches a *different* identifier — `match(0x800)` is `match(0x000)` under
    another name. Constant at the call site, it is a compile error; computed,
    it traps.
  - **It costs nothing if you do not use it.** `.text` for the pre-existing
    `examples/can` main is byte-identical before and after (1467 B, `-Os`,
    Cortex-M0+, disassembly diffed); adding the two-filter call costs 112 B.
    Re-measured at the IMAGE level since, against a `git archive` of the parent
    commit with its matching `alloy-devices`: the old `main.cpp` built against
    the new headers gives `.text` 4044 bytes, MD5-identical to the old tree, and
    `blink`, `uart_echo`, `pwm_fade` and `adc_read` are unmoved to the byte —
    see [the surface page](docs/reference/peripheral-surface.md#cost-zero-for-unused-features-measured-to-the-byte),
    which does the same for the encoder and the analog watchdog.

  **Not proven in emulation, and it cannot be**: Renode's own `stm32g0.repl`
  maps FDCAN to `CAN.STMCAN`, a bxCAN model whose register map differs — a
  write of M_CAN's `CCCR.INIT` reads back 0 (measured), so the driver's
  init handshake would spin forever, and the message RAM at the companion's
  base is not a peripheral in that platform at all. Alloy's own platform
  emitter models no CAN and emits neither block. The loopback self-check in
  `examples/can` is a real witness on real silicon and nothing weaker
  substitutes for it.

- **Instance ownership gets its second scope, and the other seven facades
  (`alloy/core/claim.hpp`).** The entry below closed the double-open hole for
  five of alloy's twelve peripheral facades and for one of the defect's two
  scopes; the identical bug was still live where it was not looking.
  `pwm::bind` guarded the CHANNEL with a `static bool` of its own — and `bind`
  is templated on the pin, so one channel had one flag per route. TIM2_CH1 has
  four routes on the `nucleo_g0b1re`, so two binds on one channel compiled
  clean, both opened, both muxed a pin onto one output compare, and the block
  claim admitted them because they agreed on the frequency. Booted in Renode,
  the pre-fix image printed its "second bind also opened" banner in 1.4 s. New
  `alloy::claim::sub_exclusive<Inst, Sub, P>()` is keyed on the instance and
  the ordinal and nothing else; `dma::channel`'s private `claimed_` — correctly
  scoped but a third mechanism with an untyped trap — now uses it too, and
  `trap_code::sub_resource_owned` tells a channel conflict from a port
  conflict. `wdt::start` gained the `shared` claim it never had (two `start()`
  calls with different timeouts silently kept the last one); `dac`, `can` and
  `rtc` claim their personality with a constant witness. `flash` and `gpio`
  deliberately claim nothing — a facade claims at its configuring entry point,
  and neither has one; `gpio` is a third (pin) scope this mechanism does not
  model, which
  [the reference page](docs/reference/peripheral-surface.md#which-facade-claims-what)
  now states rather than leaves to be discovered. `ROLE_PERSONALITY` and the
  C++ `personality` enum are checked against each other by a new test, after
  `ethernet` turned out to exist only in the Python half and `dma` in neither.
  Cost, measured on `nucleo_g0b1re`: `.text` **unchanged** for nine examples
  that claim nothing new, **unchanged with 8 bytes of RAM returned** for
  `pwm_fade`, and +8…+48 bytes for the six that gained a claim.
- **Peripheral instance ownership (`alloy/core/claim.hpp`).** A peripheral is
  owned by one binder, in one *personality*. `alloy::claim::owner<Inst>` is an
  inline variable template, so it is one byte per instance across the whole
  image — where the old `detail_opened` was a static of the *binder type*, and
  two `bind<>`s naming one `usart2_t` therefore had two flags and both
  "succeeded", the second silently reprogramming the port under the first
  handle. `uart`, `i2c`, `spi` and `adc` claim exclusively; `pwm` claims
  *shared*, because four channels of one timer are legitimate — and pass the
  block-scoped frequency as a witness, which turns "two channels asked this
  timer for two frequencies and the second one silently won" from a live defect
  into a trap. `bind::reconfigure<Opts>()` now refuses a port nobody opened and
  refuses `Opts` that disagree with the ones `open()` programmed. Each guard
  puts an `alloy::trap_code` in a register before trapping, so a fault report
  can tell them apart (it does **not** give each one its own PC — GCC
  tail-merges the trap). Cost: **+8 bytes of `.text`** on every ARM board's
  `uart_echo`, all of it in the branch that traps; the success path is
  byte-identical.
- **Board roles get the same rule at generation time.**
  `emit/board.py::ROLE_PERSONALITY` refuses a `board.json` that hands one
  peripheral to two mutually exclusive personalities, naming the peripheral and
  both roles. `nvm` and `fs` deliberately share the `flash` personality — two
  regions on one controller are one driver serving two roles, which
  `nucleo_g0b1re` declares today.
- **Layer 1 admits VALUES, not only fields (`alloy/core/admit.hpp`).**
  `open({.baud = 0})` used to compile with no diagnostic, fold the constant
  division to unreachable, and fall into the double-open trap — so even the
  crash named the wrong guard. `open()` now refuses any rate no divisor can
  represent (zero, or above the peripheral's own kernel clock): a **compile
  error** naming the instance when the value is a literal, a named trap when it
  is computed. Same two lines guard `i2c::open`, `spi::open` and `pwm::open`.
  Zero bytes when the value is constant, 20 when it is not. Accuracy stays
  `open_checked<Baud>`'s job, deliberately — a tolerance applied silently to a
  runtime value is a policy, not a check.
- **The rule that routes a knob to a layer is at revision 3, and this time it
  was derived from three peripherals that were BUILT**
  ([docs/reference/peripheral-surface.md](docs/reference/peripheral-surface.md)).
  Two a-priori rules were written and both were killed by a reviewer who simply
  applied them, so the method changed: `can` (a cross-peripheral feature),
  `encoder` (a personality) and `adc` (a sub-resource) were built first, and v3
  is the generalisation of what those three builds demanded — nothing else.
  **Question 0 survives and now has five rows**: the FDCAN filters found that a
  curated FIELD whose *encoding* is not curated is a magic number wearing an
  accessor (`RXGFC.ANFS`), and each row's OUTPUT is stated — for four of the
  five it is a task in `alloy-devices`, not a layer. **Question 5 is new** and is
  the axis that killed v2: a feature living in two blocks stays on the facade of
  the block the user names, the other block is an edge on the instance
  descriptor, curation is a closure over the pair, and the ordering and
  config-window obligations are the driver's because no layer can state them.
  **Where a maximum comes from** is restated with three witnesses, because the
  page had been wrong in both directions (v1 put it in `feat`; v2 said "always
  the field's `raw_mask`", and `RXGFC.LSS` says 31 where the capacity is 28 —
  stated by the *companion*). Personality gains three clauses from the encoder
  build (the data must declare it; whole-register writes, never RMW;
  a binder takes only tags carrying a fact it programs), sub-resource gains the
  three the watchdog answered differently from v2's guesses, and `feat` gains
  its second home. What v3 **refuses to decide** is a table of nine cases it has
  no evidence about, plus three open questions it inherits; five data-model
  changes are stated as **proposals for the maintainer, not made**. The page
  ends with an audit trail from each clause to the commit that demanded it —
  including the four clauses whose witness is a comment rather than a test — and
  with the one thing revision 3 has not survived: nobody has yet applied it to a
  feature it was not derived from.

- **Revision 2 of the same page, superseded and kept for the record**
  ([docs/reference/peripheral-surface.md](docs/reference/peripheral-surface.md)).
  v1 was derived from UART and failed all three of the features an adversarial
  pass tried on it — I2C 10-bit addressing, the ADC analog watchdog, timer
  encoder mode — because it asked *which layer* without first asking *what kind
  of thing*. v2 adds a gate (**question 0: what does the database already
  know**, four rows, because `alloy::dev::` is *not* the unconditional escape
  hatch v1 advertised) and three categories in front of the layers:
  **personality**, **sub-resource**, and **per-transfer value**. All three
  counterexamples are answered with the code a user would write; three more
  features are stressed against v2 and **two of them break it**, which the page
  records rather than smooths over; and two categories v2 deliberately does not
  decide are stated as questions a maintainer must answer.

### Changed

- **`bind::open_checked<Baud>()` takes `alloy::uart::frame`, not
  `alloy::uart::config`.** `open_checked<115'200_baud>({.baud = 9'600})`
  compiled clean and ran at 115 200, with the loser never mentioned; `frame` is
  Layer 1 minus the rate, so that call is now a compile error naming the
  member. `open_checked<A>()` and `open_checked<A>({.parity = …})` are
  unchanged. This is a Tier-1 signature change with no in-tree caller, taken
  rather than deprecated: a window in which both spellings work is a window in
  which the disagreement is still silent.

- **The peripheral surface, implemented on UART.** The three layers
  [decided last commit](docs/reference/peripheral-surface.md) now exist in
  code, across all six UART drivers and four vendors.
  `alloy::uart::config` (Layer 1) gains `parity` and `stop`, both defaulted, so
  every `open({.baud = …})` call site — all 59 in the 47 examples — compiles
  untouched. `alloy::uart::opts<Inst>` (Layer 2) is new: a compile-time value
  whose members are declared beside each driver, so a knob the silicon lacks is
  not a member and cannot be asked for. `uart::de<Pin>` is a new binder tag —
  hardware RS-485 driver-enable needs a pin, so the tag that names the pin is
  what switches it on, never a config bool. `Inst::feat::<name>` is new and
  generated: degree numbers a register map cannot state (`rx_fifo_depth`,
  `tx_fifo_depth` today), where **0 means absent**. `examples/uart_frame` shows
  all three in one portable file with zero preprocessor, and builds for 9 of 9
  boards. Costs measured on `examples/uart_echo` (the example that configures
  nothing): four boards byte-identical, the two STM32F7 boards **164 bytes
  smaller** and the two RP2040 boards 80 bytes smaller — nothing grew.
  Three parts of the decision did not survive contact with the code and are
  recorded in the page rather than edited out: word length is not a Layer-1
  field (the six drivers' value domains intersect at `{8}` only), the
  cross-vendor naming rule's falsification test came back **negative** (the SAM
  USART has no DE assert/deassert time under any name), and a maximum
  programmable value comes from the generated register field width
  (`IP::deat.raw_mask`) rather than from `feat`.
- **`scripts/check_compile_errors.py` grows three cases**, one per failure mode
  the surface promises: a Layer-2 knob absent on this IP, a Layer-2 value wider
  than the generated register field (GCC prints `the comparison reduces to
  '(40 <= 31)'`), and a `uart::de<pin>` tag on an IP with no driver-enable.
- **`tests/emulation/uart_frame_surface.robot`** — degree asserted on emulated
  silicon rather than argued: the same `main.cpp` prints `rx-fifo: shallow` on
  the STM32G0 (`feat::rx_fifo_depth == 8`) and `rx-fifo: none` on the STM32F7
  (`== 0`). Two CI legs; if the generated number never reached the image, or
  reached it as a silent default zero, one of them fails.
- **`check_contract.sh` gains one grep**: `struct feat` may not appear under
  `src/`. A hand-written degree number is guard #1 with a decimal literal.

- **`docs/reference/peripheral-surface.md`** — the decision that governs the
  config surface of the remaining 28 uncurated peripherals, taken before those
  drivers are written rather than after. Three layers (`alloy::<periph>::config`
  frozen at what every driver honours, `alloy::<periph>::opts<Inst>` whose
  *members* are declared per IP version, `alloy::dev::` for the rest), degree as
  a generated `Inst::feat::` number where 0 means absent, and a five-question
  rule that routes any future knob to exactly one of them. Costs measured
  (+4 bytes for a user who configures nothing; +8 for five vendor features;
  +80 when the config is not a compile-time literal) and the give-up named: a
  Layer-2 knob cannot be changed at run time. UART converts first, because it
  is the one peripheral where the shape fixes a live defect —
  `hal::serial_config` promises nine fields on behalf of six drivers, one of
  which implements `configure()`.
- **`alloy chip-status <chip>`** (`--json`) — the peripheral coverage
  scoreboard: per peripheral instance, is there curated register data, a HAL
  driver, a Renode model, and is it bound by a board role? Every column is
  DERIVED (chip yaml, `alloy-devices/registers/`, `src/alloy/hal/`,
  `emit/renode.py`'s model tables, `boards/` + `roles.py`), so it cannot rot
  the way a checklist does. The headline — "X of Y peripherals curated, Z with
  drivers" — is stated with what it does and does not count. See
  [docs/reference/chip-coverage.md](docs/reference/chip-coverage.md), which
  records the STM32G0B1RE baseline the coverage push is measured against.
- **`alloy devices`** — report the chip database this project resolves: root,
  declared version, and a content digest over every schema/register/chip file.
  `--pin` writes both into `alloy.toml` under `[devices]`, and from then on any
  verb that loads the project refuses a database that does not match. A
  `[devices] path` also redirects discovery, ahead of `ALLOY_DEVICES_ROOT`.
  This closes the hole where a shipped product rebuilt against a moved sibling
  checkout silently compiled different register offsets.
- **`docs/reference/stability.md`** — what is public API, what may break in
  which release, the deprecation window, and how to pin both repos.
- **`docs/reference/safety.md`** — the properties that hold by construction
  (no heap, no exceptions/RTTI, no recursion, bounded stack), each with the
  command that checks it, plus the ones that do **not** hold today.
- **`docs/guide/escape-hatch.md`** + **`examples/escape_hatch/`** — calling a
  vendor HAL, poking a register through `alloy::dev::`, and what does and does
  not collide with CMSIS (nothing at include time; `Reset_Handler` and
  `SysTick_Handler` at link time; peripheral IRQ handlers silently).
- **`scripts/check_static_limits.sh`** + **`scripts/static_limits.py`** —
  recursion, heap, exception/RTTI machinery and a worst-case stack bound read
  out of a linked ELF, each with a negative control. Firmware is now compiled
  with `-fstack-usage`, which changes no code bytes.
- A `static-limits` CI job runs all of the above, plus the two escape-hatch
  link experiments, on every push.

### Changed

- **A route naming a signal `alloy::signal` does not model is skipped, not an
  error.** A chip's full alternate-function table names far more signals than
  the framework models — complementary PWM outputs, timer break and ETR inputs,
  RS-485 DE, I2S, comparator inputs, USB DM/DP. `emit_routes_header` used to
  refuse the whole build over one of them, which forced the chip database to
  carry only signals alloy already understood. It now applies the same rule it
  already applied to routes into an uncurated peripheral — the fact stays in the
  database, codegen emits nothing — and the generated `routes.hpp` **lists what
  it skipped**, so the gap is readable in the artefact instead of invisible.
  Adding an enumerator in `core/types.hpp` (plus `emit/common.py`'s `SIGNALS`)
  is all it takes to turn one of those lines into a compile-checked route.
- **`nucleo_g0b1re` now points at `st/stm32g0b1re`**, the graduated
  full-silicon chip file (65 peripherals) that replaces the hand-written
  `st/stm32g0b1` (23). The `adc` and `dac` roles name `adc1`/`dac1`, the
  instance names that die actually has. Verified: `blink` and `uart_echo` build,
  and the `firmware_boots` emulation leg passes on the regenerated platform —
  which now also models GPIOE, the EXTI controller and the IWDG.

### Fixed

- **`alloy sbom` no longer relabels an undeclared vendored tree as alloy's own.**
  A third-party source under `<alloy>/src/**/vendor/` that matched no entry in
  `sbom.py`'s `_VENDORED` table fell through to the framework bucket and was
  reported as part of "alloy, MIT" — silently, with its own licence file sitting
  beside it. Both packages alloy vendors today live under exactly that path, so
  the next one would have inherited the bug. Such a file is now an **undeclared
  component**, which is also what makes `--strict` refuse it.
- **`docs/guide/async.md` figures re-measured, and one claim withdrawn.** The
  guide said a task parked on an event *or a timer* costs nothing per superstep.
  Measured: eight event-parked tasks cost nothing (34.75 ieq empty poll, same as
  none), but eight parked on `delay()` cost 131.25 — `run_once()` walks the timer
  list every superstep, ≈12 ieq per sleeping task. `examples/concurrency_probe`
  now measures it, the emulation leg gates it as `verdict parked`, and the page
  says how far to read a figure (±1 ieq; a rebuild moves the last digit).
- **`alloy setup` now prints the `export PATH=…` its own install requires.**
  An offline install put the cross compiler in `~/.alloy/tools`, `alloy setup
  --check` called it `ok`, and `alloy build` then failed with "The CXX compiler
  identification is unknown" — because the build looks for the compiler on
  `PATH` only. On an air-gapped host those two messages have nothing connecting
  them. The line is now printed at the end of any install, and
  `docs/guide/supply-chain.md` says to run it.
- Two factual corrections on `docs/reference/stability.md`: `alloy image` was
  listed among the verbs a `[devices]` mismatch refuses, but it takes a built
  binary and never opens a project; and the `### Deprecated` changelog policy
  starts here, not retroactively at 0.1.0. The stale sample `.elf` digest on
  `docs/guide/supply-chain.md` was refreshed and labelled as one build's.

### Deprecated

- **`alloy::uart::serial_config`** and `bind::reconfigure(serial_config)` —
  the nine-field struct that one driver in six honoured. Replaced by
  `alloy::uart::config` (portable fields) plus `Role::opts` (vendor knobs);
  the live-port path is now `reconfigure<Opts>(config)`. The old spelling
  survives as `reconfigure_legacy()` for the window. There are no in-tree
  callers.


## 0.3.0 — 2026-08-07

Everything an editor needs to describe a board without reimplementing any of
it. Each verb below has a versioned `--json` envelope, and a readable default
so the same command is useful in a terminal.

**Requires `alloy-devices` >= 0.3.0.** Not a formality: the interrupt-driven
I2C driver reads `CR1.STOPIE`, which 0.2.0 does not carry — against it the
generated IP header silently lacks the accessor and the failure lands as a
template error inside framework headers you do not own.

### New: describe and check a board

- **`alloy board-info [<id>] --json`** — roles, capabilities, used pins and
  problems of **any** board, curated or project-local. Capabilities come from
  the same `role_caps()` the emitter uses, so this cannot disagree with the
  `board::caps` the generated header will carry.
- **`alloy board-validate [<id>] [--file f|-] --json`** — every problem at
  once, located (role, field, pin) and answered with the values that would
  work. The headline rule moves a `static_assert` forward: `alloy::i2c::bind`
  already refuses a pin with no route to its peripheral, but only when the app
  instantiates the bus, and only at build time. This asks at config time.
  Exits non-zero, so it works as a CI gate.
- **`alloy board-clone <src> <new>`** — copy a curated board into your project
  as an editable one.
- **A `board.json` value is a default; `alloy.toml` chooses.** The fields a
  project may pick — `debug_uart.baud`, `watchdog.timeout_ms`, `nvm`/`fs`
  `bytes`, and `[clock]` — can be set per project, so a framework board keeps
  receiving upstream fixes instead of being forked to change a number. Which
  fields those are is declared once, in `roles.py`; anything else is refused
  with the reason and the command that would actually do it, and an override
  for a role the board does not define is reported as inert rather than
  silently dropped. `board-info` reports the effective board *and* a
  `project_overrides` block with the board's own value beside each change.

### New: see what you built

- **`alloy size --json`** — flash and RAM of the last build against the chip's
  real memories, and, on a board with an A/B layout, whether the packed image
  fits each slot. Measured against the regions the linker actually used.
- **`alloy build --json`** — the build result with that size summary attached.
- **`alloy matrix [--boards a,b] --json`** — build this project for every
  board and table the result. One source tree, nine boards, two architectures.
  A board that fails is a row with a reason, not an aborted sweep.
- **`alloy svd [--chip <id>] [-o out]`** — write a CMSIS-SVD file so a debugger
  shows peripheral registers by name. Generated from register maps that were
  already curated; no new data. Refuses a non-ARM chip rather than emitting a
  file no debugger can use.

### New: the whole clock, and CI

- **`alloy clock --graph [--profile p] --json`** — sources, bus prescalers, and
  the kernel clock each peripheral is fed, with what it implies: a UART's baud
  error computed with the driver's own rounding, a timer's reachable range.
- **`alloy ci-init [--boards a,b] [--all] [--force]`** — write a GitHub Actions
  workflow for your project. Targets the boards it actually targets, and
  installs only the toolchains those need.
- **`alloy monitor --json`** — the serial link as NDJSON, for a caller with no
  terminal. End of stdin means "nothing more to send", not "close the link".

### Extended

- `alloy chip-info` gains a **role catalogue** (per role: required and optional
  fields, candidate peripherals by IP class with every pin each signal can
  reach, whether each is curated) and the chip's `package`, when its data
  carries one. Still `alloy.chip_info.v1` — every field is additive.

### Fixed

- `alloy size` measured against the first memory of each kind. Correct on
  Cortex-M, wrong on Xtensa, where `.text` goes to IROM and `.data`/`.bss` to
  DRAM while "the first ram" is a 2 KiB vector window — an ESP32 build reported
  0.3K of 2.0K RAM instead of 0.3K of 272K. Regions now come from the linker
  emitter, and every figure names the memory it refers to.

## 0.2.0 — 2026-07-24

`alloy chips` / `new --chip` for any MCU in the database, the parametric clock
solver, `alloy lib`, and the OTA verbs (`keygen`, `image`, `ports`, `update`).

## 0.1.0

First release: `new`, `build`, `flash`, `monitor`, `run` with `--board`
switching, and the data → codegen → HAL loop behind them.
