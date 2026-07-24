# Error handling

alloy has no exceptions in firmware. Failures are reported in one of three deliberate ways,
depending on *who* made the mistake.

## 1. Recoverable I/O errors → `bool` (and, later, `Result`)

A transfer that can legitimately fail at runtime — an I²C NACK, a stuck bus, a flash program
error — returns `false`. The caller decides what to do:

```cpp
if (!i2c.write(0x68, data)) {
    // NACK or bus error — retry, log, or give up
}
```

Drivers bound their polls, so a missing pull-up or a jammed bus returns `false` instead of
hanging forever.

For calls that want to carry a *reason*, alloy ships a `-fno-exceptions`-safe
`Result<T, E>` — a cousin of `std::expected` (with `has_value` / `value_or` / `and_then` /
`transform`) whose misused `value()` traps rather than throws:

```cpp
alloy::Result<config, alloy::error> r = device.read_config();
if (r) {
    use(*r);
} else {
    log(r.error());          // error() on a success value traps — that's a bug
}
```

## 2. Programmer mistakes → compile error, where possible

The best error is the one you can't ship. Wrong pin routes, using a role a board lacks, calling
a capability a controller doesn't have — all of these are **`static_assert` failures at compile
time**, with messages a beginner can read:

```
error: static assertion failed: TX pin has no route to this UART on the selected chip
```

## 3. Contract violations → a debug trap

Some misuse can only be caught at runtime — opening the same peripheral twice, claiming an
already-claimed DMA channel. These hit `__builtin_trap()` (an honest debug assert), so the
program stops at the fault instead of silently corrupting state:

```cpp
auto a = board::spi::open();
auto b = board::spi::open();   // double-open → trap, not a silent re-clock
```

## Choosing between them

| Situation | Mechanism |
| --- | --- |
| The hardware or the wire can fail | `bool` / `Result` return |
| The *code* is wrong (route, missing role, capability) | `static_assert` — compile error |
| A resource contract is violated at runtime | `__builtin_trap()` debug assert |

The guiding idea: **push every failure as early as the language allows** — compile time first,
a bounded runtime error next, and never a silent hang.
