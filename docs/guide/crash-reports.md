# Crash reports

A device that hangs in the field is a support call with no evidence. The generated vector
table points every fault at a weak handler whose body is `for (;;) {}` — a dead device that
says nothing. alloy ships the other half: a **fault handler** that records where the firmware
died into RAM that survives a reset, resets, and lets the **next boot** report it — over the
UART, into flash, wherever the application decides. A host verb, **`alloy crash`**, turns the
raw addresses back into file:line.

```console
$ alloy monitor
alloy crash_report ready
RECOVERED FROM A FAULT  pc=0x08000578 lr=0x08000119 status=0x00000000 (1 in a row)
$ alloy crash --line "RECOVERED FROM A FAULT  pc=0x08000578 lr=0x08000119 status=0x00000000 (1 in a row)"
pc      0x08000578  alloy_irq_dispatch  src/alloy/arch/cortex_m/irq_dispatch.cpp:10
lr      0x08000119  put_hex(...)  examples/crash_report/src/main.cpp:48  (call site — lr is where execution would have returned)
status  0x00000000  no fault status recorded — normal on Cortex-M0/M0+ (ARMv6-M has no CFSR)
        1 crash(es) in a row
```

The whole loop is `examples/crash_report`, and it runs — five real reboots of one emulated
machine — as a blocking CI leg on Cortex-M0 and Cortex-M7.

## The handler runs on a broken machine

By the time `HardFault_Handler` executes, the firmware has already proven it cannot be
trusted: a pointer was wild, a stack was torn, an instruction was not an instruction. So the
handler does the least that still leaves evidence:

1. Read the exception frame the core stacked automatically (pc, lr, psr — the same eight
   words on ARMv6-M and ARMv7-M).
2. On ARMv7-M, read the fault status registers (CFSR, and MMFAR/BFAR when valid). ARMv6-M
   has none; the record honestly says `0`.
3. Copy those few words into a `.noinit` section — RAM the linker script marks NOLOAD and
   startup never zeroes, so it survives the warm reset.
4. Bump a **consecutive-crash counter** *before anything else can go wrong*, and reset via
   `SYSRESETREQ`.

No driver, no formatting, no flash programming, no clock work — anything that could fault
again would turn one crash into a silent reboot loop. Everything else waits for the next
boot, which has a healthy machine and all the time in the world.

## The next boot reports — and decides

```cpp
#include <alloy/fault.hpp>

int main() {
    board::init();
    if (alloy::fault::record crash; alloy::fault::take(crash)) {
        // crash.pc, crash.lr, crash.psr, crash.sp, crash.status, crash.address,
        // crash.consecutive — print them, store them, count them.
        if (crash.consecutive >= 3) { /* stop doing the thing that kills us */ }
    }
    // ... later, once the application has proven it actually works:
    alloy::fault::healthy();
}
```

Three calls carry the policy:

| Call | What it does |
| --- | --- |
| `take(record&)` | the previous boot's crash, **once** — it clears the record |
| `consecutive()` | boots in a row that ended in a fault; survives `take()` |
| `healthy()` | this boot got far enough to count as working — clears the counter |

`healthy()` is deliberately the application's call, not the framework's — same shape as an
OTA trial confirming itself, for the same reason: only the application knows what "working"
means. Without it the count only grows, and a crash-looping device can notice and park
itself in safe mode instead of faulting forever.

## Persisting the record: next boot's job, not the handler's

`.noinit` RAM survives a warm reset but **not power-off**. If the crash evidence must
outlive a power cycle — a field unit someone unplugs before the truck arrives — persist it
on the boot *after* the crash, where flash programming is safe:

```cpp
if (alloy::fault::record crash; alloy::fault::take(crash)) {
    if constexpr (board::caps::nvm) {
        board::nvm.set(kCrashPc, crash.pc);
        board::nvm.set(kCrashStatus, crash.status);
        board::nvm.set(kCrashTotal, board::nvm.get(kCrashTotal, 0u) + 1u);
    }
}
```

This is doctrine, not a missing feature: the handler must never touch the flash controller
(a half-programmed page during a crash is worse than no record), so persistence belongs to
`take()`'s caller. The `if constexpr` keeps one source portable — on a board without the
`nvm` role the branch is not compiled at all. `examples/crash_report` carries exactly this
shape.

## `alloy crash` — addresses back into source

The device reports raw addresses because that is all a broken machine can afford. The host
has the ELF and the toolchain, so the decoding happens there:

```console
$ alloy crash --line "<paste the RECOVERED line>"     # or --pc/--lr/--status/--address
$ alloy crash --pc 0x08000578 --elf builds/v1.4.2/app.elf
$ alloy crash --status 0x00008200 --address 0x20030000
status  0x00008200
        BusFault PRECISERR — precise data bus error — pc IS the faulting instruction
        faulting address 0x20030000 (exact, from BFAR)
```

- **ELF resolution**: defaults to the current board's last build (same rule as
  `alloy size`). For a report from the field, pass `--elf` with the build that actually
  shipped — symbolizing against the wrong binary produces confident nonsense.
- **Toolchain**: `arm-none-eabi-addr2line` from PATH first, then the managed install under
  `~/.alloy/tools` (the `alloy setup` doctrine). Absent both, the verb says so, exits
  nonzero — and still decodes the status word, which needs no toolchain.
- **CFSR decode** (ARMv7-M): every set bit becomes words — which fault, what it means, and
  whether the recorded faulting address is exact (MMARVALID/BFARVALID). The table is
  architectural (ARMv7-M ARM, DDI 0403E.b §B3.2.15), so it needs no chip data.
- `lr` is symbolized at the **call site** (two bytes back from the return address), which is
  the line a human wants; an `lr` that is an EXC_RETURN value is labelled as such, not
  symbolized into nonsense.
- `--json` emits a versioned `alloy.crash.v1` envelope for editors.

## What is witnessed, and what is not

Honesty over reassurance — the claims above are not all equal:

**Witnessed under emulation** (a blocking CI leg from the next push on; Renode 1.16.1,
Cortex-M0 and M7): handler entry through
both trampolines (NMI pended by firmware, HardFault pended through the NVIC), the capture,
the `.noinit` record surviving real `SYSRESETREQ` reboots, the consecutive counter counting
1-2-3-4 across four resets, and the safe-mode park. The deliberate crash in the example is
`alloy::fault::trigger()` — a software-pended NMI, architectural on every Cortex-M — because
it takes the identical capture/persist/reset path *and* is provable in emulation.

**Silicon-only, unwitnessed**: *organic* fault entry — a real wild jump or undefined
instruction escalating into `HardFault_Handler`. Renode's core does not perform M-profile
fault exception entry at all (a wild jump kills the emulated core with the vector table
unread), so that first link of the chain rests on the ARM architecture manual, not on a test
this repo ran. Everything downstream of handler entry is exercised.

**Unwitnessed in emulation**: the `nvm` persistence branch (no Renode leg exercises flash
writes through the nvm role) — it compiles both ways in CI, guarded by `if constexpr`.
