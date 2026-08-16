# API stability and support

What a company can plan against: which parts of alloy are promised, what a
version number is allowed to break, how much notice a removal gets, and — the
sharp one — how a shipped product stops its *chip facts* from moving.

| Question | Answer |
|---|---|
| What is public API? | The `alloy::` and `board::` headers, the CLI verbs, the `--json` envelopes, and `alloy.toml`. Enumerated below. |
| What is internal? | `alloy::arch::`, `alloy::hal::`, any `detail` namespace, `src/alloy/*/vendor/`. |
| What may break in a MINOR? | Nothing public, except by deprecation-then-removal across two releases. |
| How long is the deprecation window? | At least one MINOR release, removal no earlier than the next MAJOR. |
| Can we pin the chip database? | Yes — `[devices]` in `alloy.toml`, by version **and** by content digest. |
| Is any of this enforced? | The surface is recorded in `tools/alloy/tests/test_stability.py`; deleting a public name fails CI. |

Alloy is `0.x`. That is a real statement, not modesty: until `1.0` a MINOR
release **may** make a breaking change, and the promise below is that it will
be announced and given a window, not that it will never happen. Everything on
this page is what the project holds itself to today; nothing here is a
contractual support agreement.

---

## The public surface

### Tier 1 — application headers

Headers under `src/alloy/` that application code reaches — whether it includes
them itself or gets them through `alloy/board.hpp`. The list is every
application-facing header that ships today; it is checked against
`ls src/alloy/*.hpp` rather than against memory, because it drifted once by
ten names when the peripheral surface grew:

```
alloy/board.hpp     alloy/time.hpp      alloy/delay.hpp     alloy/gpio.hpp
alloy/uart.hpp      alloy/i2c.hpp       alloy/spi.hpp       alloy/adc.hpp
alloy/dac.hpp       alloy/pwm.hpp       alloy/bridge.hpp    alloy/encoder.hpp
alloy/tick.hpp      alloy/can.hpp       alloy/rtc.hpp       alloy/dma.hpp
alloy/irq.hpp       alloy/wdt.hpp       alloy/wwdt.hpp      alloy/flash.hpp
alloy/crc.hpp       alloy/uid.hpp       alloy/log.hpp       alloy/sched.hpp
alloy/fault.hpp     alloy/secure.hpp    alloy/ota.hpp       alloy/provision.hpp
alloy/concepts.hpp  alloy/fastcode.hpp
alloy/async/{task,executor,delay,uart,i2c,spi,dma,event,periodic,waiter}.hpp
alloy/net/{socket,http,lwip}.hpp
alloy/ota/{signed,uart_transport,crc32}.hpp
alloy/drivers/…     alloy/dsp/…         alloy/util/…        alloy/control/…
```

`alloy/bridge.hpp`, `alloy/encoder.hpp` and `alloy/tick.hpp` are the timer's
three non-PWM [personalities](peripheral-surface.md#personalities-a-block-runs-in-one-mode-at-a-time);
`alloy/wwdt.hpp` is a separate role from `alloy/wdt.hpp`, not a mode of it.
All four are taught in the guide and all four are Tier 1.

`alloy/board.hpp`, `alloy/device.hpp`, `alloy/product.hpp`,
`alloy/product_nvm.hpp`, `alloy/slots.hpp`, `alloy/ota_key.hpp` and
`alloy/bus_messages.hpp` have no file in `src/alloy` at all — codegen writes
them per board into `.alloy/generated/`. Their **names** are public; the
contents of the first six are the chip database's promise, which is what
`[devices]` below pins. `bus_messages.hpp` is different: its contents are the
**project's own** promise — the `bus.toml` wire contract — and the emitted
binding shape (the `WireBinding` form `libs/bus` consumes) is Tier 1, because
two boards compiled from one `bus.toml` must stay byte-compatible on the wire.

Two more names are public with **IP-shaped contents**, on the same footing:

- `alloy::<periph>::opts<Inst>` (surfaced as `Role::opts`) — Layer 2 of the
  [peripheral surface](peripheral-surface.md). The template's *name* is Tier 1
  and will not move. Its *members* are declared per IP version beside the
  driver, so which knobs exist changes with the chip, by design. Adding a
  member is MINOR; renaming or removing one follows the deprecation window for
  the drivers that had it.
- `Inst::feat::<name>` — degree numbers, generated from `alloy-devices`. Their
  contents are the chip database's promise, exactly like `alloy::dev::`, and
  are pinned by `[devices]`.

One more header is Tier 1 by intent rather than by the examples using it:

- `alloy/core/claim.hpp` — `alloy::claim::exclusive/shared/held`,
  `alloy::claim::sub_exclusive/sub_held`, `alloy::claim::personality`,
  `alloy::trap_code`. A facade written outside this repo has to be able to
  participate in
  [instance ownership](peripheral-surface.md#personalities-a-block-runs-in-one-mode-at-a-time)
  at both scopes — a whole peripheral and one numbered part of it — so these
  are public names and not a `detail` namespace. `personality` gains
  enumerators as alloy ships facades — additive, and it carries `user_a`/
  `user_b` so an out-of-tree facade never has to patch the header.

### Tier 1 — the CLI

Every verb `alloy --help` lists, and every `--json` envelope. Envelopes carry
their own version in the payload:

```json
{"schema": "alloy.board_info.v1", …}
```

A breaking change to an envelope bumps `.v1` to `.v2` and both are emitted for
the window. A tool that checks the `schema` field will never be silently handed
a different shape.

### Tier 1 — `alloy.toml`

The tables a project may write: `[project]`, `[board]`, `[alloy]`,
`[devices]`, `[product]`, `[libs]`, `[ota]`, `[roles.*]`, `[clock]`. Unknown
keys inside `[devices]` are refused rather than ignored — a typo in a pin is
the last thing that should fail open.

`bus.toml` (beside `alloy.toml`, when a project has wire messages) is on the
same footing: `schema = "alloy.bus.v1"`, unknown keys refused everywhere — a
typo in a wire id is the last thing that should fail open — and the id rules
(explicit, never auto-assigned, retired not deleted) are enforced by
`alloy bus validate` and by generation.

### Not public

| Surface | Why |
|---|---|
| `alloy::arch::` (`src/alloy/arch/…`) | the per-ISA backend; portable code never names it, and a new backend reshapes it |
| `alloy::hal::` (`src/alloy/hal/…`) | the per-IP peripheral drivers the generated `board.hpp` instantiates; they move whenever the data does |
| any `detail` namespace | implementation of the header it sits in |
| `src/alloy/net/vendor/`, `src/alloy/fs/vendor/`, `third_party/` | upstream trees, on upstream's terms — see the [NOTICE](https://github.com/Alloy-Embedded/alloy/blob/main/NOTICE) |
| `alloy::dev::` | generated register facts — stable in *shape*, but its content is the chip database's version, not alloy's |

A test asserts that `alloy::arch` is not declared outside `arch/`, so the
boundary cannot erode quietly.

---

## What each number means

| Change | Bumps |
|---|---|
| new board, new chip, new peripheral, new verb, new `--flag` | MINOR |
| bug fix that does not change a documented behaviour | PATCH |
| removing or renaming a Tier-1 name; changing an envelope's shape without a new `.vN`; making a previously accepted `alloy.toml` invalid | MAJOR |
| anything under "Not public" | any release, no notice |
| a new chip fact, a corrected register offset | **alloy-devices**, not alloy — see below |

### The deprecation window

1. The release that deprecates a name keeps it working, marks it
   **Deprecated** in `CHANGELOG.md`, and names the replacement.
2. It keeps working for at least one further MINOR release.
3. It is removed no earlier than the next MAJOR.

From the next release on, `CHANGELOG.md` carries a `### Deprecated` section in
every entry, even when the answer is "nothing" — an empty section is a claim; a
missing one is a question. `Unreleased` has one today; `0.1.0`–`0.3.0` predate
the policy and do not, so read those three as "no deprecations were tracked",
not as "none happened".

### The docs are versioned too

`mkdocs` publishes with `mike`: a tag becomes `/<version>/` and `latest`, and
`main` becomes `/dev/`. Documentation for the version you pinned does not
disappear when the next one ships.

---

## Pinning the chip database

This is the part that matters most and is easiest to miss.

The register offsets, memory maps, IRQ numbers and clock trees a build compiles
do **not** live in this repo. They live in `alloy-devices`, which is resolved at
build time — from `ALLOY_DEVICES_ROOT`, from a sibling checkout, or from an
installed wheel, in that order. A product built from a repo checkout therefore
gets *whatever that checkout is today*.

That is not hypothetical. `alloy-devices` `0.3.0` is tagged, and its main branch
has already changed schema since the tag while still calling itself `0.3.0`.

### The failure it causes

Move one register offset in the database and rebuild:

```console
$ sed -i 's/offset: "0x14"/offset: "0x40"/' registers/st/gpio_v2.yaml   # ODR
$ cd my-project && alloy gen
generated 20 file(s) -> .alloy/generated/nucleo_g071rb
```

Nothing complained. The generated header now says:

```cpp title="illustrative: one line of a generated register header, quoted"
static_assert(offsetof(regs, ODR) == 0x40);
```

Every write to that pin now lands on a different register. The firmware builds,
flashes, and behaves differently on the bench with no version, no warning and
no diff in your own repository.

### The pin

The path, the digest and the file count below are **an example** — yours will
differ, and a digest that does not match this one is not a sign that anything
is wrong. Only the *shape* is the claim.

```console
$ alloy devices
database  /opt/alloy-devices
version   0.3.0
digest    sha256:77e851f7b10b82b123fb6ca459a2c0243dd739b11a8b40c42fa1092d54875551  (477 files)

this project pins nothing — its facts follow whatever database is
resolved at build time. `alloy devices --pin` freezes them.

$ alloy devices --pin
pinned in /home/me/my-project/alloy.toml:
  [devices]
  version = "0.3.0"
  digest = "sha256:77e851f7b10b82b123fb6ca459a2c0243dd739b11a8b40c42fa1092d54875551"
```

Now the same edit is caught before a single file is generated:

```console
$ alloy gen
error: chip database CONTENT mismatch: alloy.toml pins
  [devices] digest = "sha256:77e851f7…"
but /opt/alloy-devices hashes to
  sha256:5d6007db…
The facts this project would compile are not the facts it was pinned to.
Check out the pinned database, or re-pin with `alloy devices --pin` if the
move was intended.
```

### The three keys

```toml
[devices]
path    = "/opt/alloy-devices"   # optional: WHERE. Wins over ALLOY_DEVICES_ROOT.
version = "0.3.0"                # optional: what it calls itself. Also ">=0.3.0".
digest  = "sha256:…"             # optional: what it actually IS.
```

* `path` only redirects discovery. It takes precedence over the environment,
  exactly as `[alloy] root` does — a shipped product says where its facts come
  from, and a shell variable does not get to move them.
* `version` and `digest` are **assertions**, checked against whatever was
  resolved, by any route. An operator who points `ALLOY_DEVICES_ROOT` at a
  different database still trips the pin. That is what makes it load-bearing
  rather than advisory.
* `version` compares release numbers (`0.3.0`, `>=0.3.0`) — it is not PEP 440,
  and anything else is refused rather than guessed.
* `digest` covers every file under `schema/`, `registers/` and `chips/`, path
  and content both, so a rename is caught even when no byte changed. It ignores
  READMEs, changelogs and editor droppings, so a prose commit does not
  invalidate a shipped product's pin.

The check runs in `load_project`, so **every** verb that opens a project —
`gen`, `build`, `flash`, `monitor`, `size`, `sbom`, `boards`, `board-info`,
`matrix`, `debug-info`, `devices` itself — refuses together, with exit 1 and
before a single file is generated. There is no route that quietly builds
against the wrong facts.

Two verbs deliberately do not: `clean` only removes build trees, and `image`
takes an already-built binary as an argument and never opens a project or reads
the database at all.

### Which pin to use

| Situation | Pin |
|---|---|
| a released product, a certification package, a build you must reproduce | `digest` (with `version` for readability) |
| a library or example that should track the database | `version = ">=0.3.0"` |
| a corporate build server with the database at a fixed path | `path` + `digest` |
| day-to-day development against a moving sibling checkout | none |

### And the framework itself

The Python side pins the database the ordinary way — `tools/alloy/pyproject.toml`
depends on `alloy-devices==0.3.0`, an exact `==`. Be aware of one development
wart: `[tool.uv.sources]` overrides that dependency with the sibling checkout
in editable mode, so **a development checkout is never version-checked by pip
resolution**. The `[devices]` pin is what covers that case, which is why it
asserts against the resolved root rather than against installed metadata.

To pin the framework too, `[alloy] root` records the checkout `alloy new`
scaffolded against; an installed wheel embeds framework and boards as package
data, so pinning the wheel version pins both.

---

## What this page does not promise

* **No LTS branch and no security-fix backports.** Fixes land on `main` and
  ship in the next release. If you need a supported old line, that is a
  conversation, not a policy.
* **No ABI stability.** Everything is headers and templates; there is no
  pre-built alloy library to link against, so "ABI" has no meaning here yet.
* **No promise about `alloy-devices` schema.** It has its own repo, its own
  tags and its own changelog, and it changes more often than alloy does. Pin
  it; do not assume it.
* **Nothing about certification.** See [Safety posture](safety.md) for what is
  and is not true today.
