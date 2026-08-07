# Products

One firmware codebase usually ships as **many products** — an inverter family driving
different compressors, a fan drive sold in an economy and a pro trim. The industry default
is `#ifdef` spaghetti. alloy already solved this exact problem for hardware: `board.json` →
validated → generated `board.hpp`, `if constexpr` in app code, no `#if` anywhere. Products
are the second instance of the same pattern, for the SKU dimension:

```
products/family.toml     the parameter space every product shares
products/<name>.toml     ONE product = one file = one clean git diff
alloy.toml [product]     which one this checkout builds by default
<alloy/product.hpp>      generated: constexpr params, caps, strategy aliases
```

A product is pure data. No hand-written C++ lives in a product, and no `#if` lives in the
app.

## The family declares the space

`products/family.toml` names every knob a product may turn — typed, bounded, with defaults
(`examples/product_demo` carries this one):

```toml
schema = "alloy.family.v1"
name = "fan_drive"

[params.rated_power_w]
type = "int"
default = 750
min = 50
max = 15000

[params.pwm]                   # emitted as alloy::frequency (u32 Hz)
type = "frequency"
default = 20000

[params.motor]                 # wire values EXPLICIT — never auto-assigned
type = "enum"
values = { induction = 1, pmsm = 2 }
default = "induction"

[caps]
has_night_mode = false

[strategies.control]
options = ["vf_scalar", "sensorless_foc"]
default = "vf_scalar"
```

Enum **wire values are explicit** in the data, never assigned by position — reordering the
table can never silently renumber what is already deployed in the field, logged over Modbus,
or stored in NVM.

## A product is the diff, and nothing else

`products/pump_pro.toml` states only what differs from the family:

```toml
schema = "alloy.product.v1"
name = "pump_pro"
family = "fan_drive"

[params]
motor = "pmsm"
rated_power_w = 1500
bus_voltage_mv = 48000

[strategies]
control = "sensorless_foc"
```

That is the whole file, and that is the point: `git log products/pump_pro.toml` is the
complete history of one SKU, a review of a new product is a one-file diff that a domain
expert can read without knowing C++, and `git blame` answers "who changed the pump's bus
voltage, and when" in one command. Compare that to the same fact smeared across a
`#ifdef PUMP_PRO` in six translation units.

Overlays are **one level deep by design** — a product extends the family, never another
product. Product-extends-product chains are how "the diff" stops being readable, so the
validator rejects them outright.

## Selecting a product

The default lives in `alloy.toml`:

```toml
[product]
name = "fan_eco"
```

Override it per command — every build verb takes it:

```console
$ alloy build --board nucleo_g071rb --product pump_pro
```

Each combination builds in its own tree, keyed `<board>+<product>`
(`.alloy/build-tree/nucleo_g071rb+pump_pro/`), so switching products never reuses a stale
object file or the other product's generated headers. Building the same product twice —
even after building the other one in between — reproduces the same binary.

The full sweep is one command:

```console
$ alloy matrix --products          # boards x products build table
```

## What gets generated

`alloy gen` (or any build) validates the product and emits `<alloy/product.hpp>`:

```cpp
namespace product {
inline constexpr char name[] = "pump_pro";
inline constexpr char family[] = "fan_drive";

enum class motor : std::uint32_t { induction = 1, pmsm = 2 };

namespace params {
inline constexpr std::int32_t rated_power_w = 1500;
inline constexpr alloy::frequency pwm = alloy::frequency{16000u};
inline constexpr ::product::motor motor = ::product::motor::pmsm;
}

namespace caps {
inline constexpr bool has_night_mode = false;
}

using control = ::product_strategy::sensorless_foc;
}
```

Params are typed — a `frequency` param is an `alloy::frequency`, not a bare int. The data
holds physical quantities in base units; unit scaling stays in C++ `constexpr`, never in
TOML arithmetic. Caps are booleans for `if constexpr`:

```cpp
if constexpr (product::caps::has_night_mode) {
    // compiled only into products that have it — and still type-checked in all of them
}
```

## Strategies are types, not flags

The family names the options; **the app provides the types**. Declare them in namespace
`product_strategy` *before* including `<alloy/product.hpp>`, and the generated
`using control = ::product_strategy::sensorless_foc;` binds the product's choice:

```cpp
namespace app {
template <class S>
concept control_loop = requires(S s, std::int32_t rpm_target) {
    { S::banner } -> std::convertible_to<const char*>;
    { s.step(rpm_target) } -> std::same_as<std::int32_t>;
};
}

namespace product_strategy {
struct vf_scalar       { /* ... */ };
struct sensorless_foc  { /* ... */ };
}

#include <alloy/product.hpp>

static_assert(app::control_loop<product::control>,
              "the product's control strategy must satisfy app::control_loop");
```

Two failure modes, both at compile time, both readable:

- a product picks an option the app never defined → the compile fails **at the alias line**
  with `'sensorless_foc' in namespace 'product_strategy' does not name a type`;
- the type exists but doesn't satisfy the app's concept → the `static_assert` fails naming
  the concept, the concrete type, and each missing requirement
  (`the required expression 's.step(rpm_target)' is invalid`).

`examples/product_demo` is the working end-to-end demo: two mock control loops satisfying
one concept, two products picking different ones, and an emulation leg that boots each
product and asserts the *strategy's own arithmetic* on the UART.

## Rules: derivations live in data

Facts that follow from other facts are derived **once**, in the family — not repeated (and
eventually mistyped) in every product file:

```toml
[[rules]]
when = { motor = "pmsm" }
set = "pwm"
value = 16000
```

`pump_pro` states only `motor = "pmsm"` and gets 16 kHz PWM; every future PMSM product
agrees automatically. An explicit value in a product file still wins over a rule — a rule
is a default derivation, not a decree.

## Constraints: combinations that must not ship

```toml
[constraints.foc_needs_pmsm]
when = { control = "sensorless_foc" }
require = { motor = "pmsm" }
```

A product violating this cannot be generated, cannot build, and fails validation by name:

```console
$ alloy product-validate --file products/bad.toml
error: constraints.foc_needs_pmsm: constraint 'foc_needs_pmsm': with control =
'sensorless_foc', this product must have motor = 'pmsm' (it has motor = 'induction')
```

`alloy product-validate` is the mirror of `board-validate`: **every** problem at once, each
located to a field, with suggestions where there is a way out — a form, an editor, or a CI
gate needs the full list, not the first failure.

## What goes in the product, the board, and NVM

The three files answer three different questions, and the discipline is worth keeping:

| Question | Lives in | Example |
| --- | --- | --- |
| What PCB is this? | `board.json` | which pin the LED is on, debug UART wiring |
| What SKU is this? | `products/<name>.toml` | compressor size, control strategy, current limit |
| What did commissioning set? | NVM (runtime) | Modbus baud, field-tuned setpoints |

A **board** fact changes when the hardware changes. A **product** fact changes when
marketing ships a new SKU on the same hardware. An **NVM** fact changes per installed unit,
with a screwdriver — it must never be a `constexpr`, because the compile-time constant and
the flash value would disagree.

The product file handles that last case explicitly — mark the field `nvm = true`:

```toml
[params.modbus_baud]
type = "int"
nvm = true
key = 0x10          # the nvm_kv key, explicit like every wire value
default = 19200     # per-product DEFAULT — the runtime value lives in flash
```

It is then emitted into a separate `<alloy/product_nvm.hpp>` as a key + default — consumed
as `board::nvm.get(product::nvm::k_modbus_baud, product::nvm::modbus_baud_default)` — and
deliberately **not** as a constexpr param. `examples/product_line` shows the full pattern.

## The plain-C twin

The same data also emits a `product.h` of `#define` lines for legacy C codebases migrating
off their `#ifdef` matrix one file at a time. It is a secondary output — the C++ header is
the product surface.

## CI

Products multiply build time, so they ride their own narrow CI jobs rather than the main
board×example matrix: `build-products` validates and builds each product of both product
examples in its own `<board>+<product>` tree, and `emulate-products` boots each
`product_demo` product under Renode and asserts the firmware names its product *and* runs
its strategy's arithmetic on the emulated UART.
