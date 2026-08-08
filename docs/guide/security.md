# Locking a production unit

[Firmware update](firmware-update.md) gives you signed images: a fielded device refuses an
update that was not signed with your key. That is only half a story. If the flash is readable
over SWD, an attacker with physical access reads your firmware (and any secrets in it) straight
out; if the bootloader is writable, they replace the code that *does the verifying*. A product
leaves the factory with two more locks:

* **RDP — readout protection.** The debug port loses access to flash. On ST parts this is a
  byte in the option bytes with three levels (below).
* **WRP — write protection.** Chosen flash ranges refuse erase/program. alloy protects the
  bootloader region, and derives the range from the [A/B slot layout](firmware-update.md) —
  the same one the linker and bootloader trust — never from a typed address.

Both are applied **host-side, through the debug probe, at production-flash time** — `alloy
secure` drives openocd. Firmware never self-protects in v1: a factory step you can see, log
and re-check beats a code path that runs on every unit forever.

```console
$ alloy secure status
nucleo_g071rb (st/STM32G071RB)
  RDP: level 0 (byte 0xaa) — flash readable over the debug port — NOT production-safe
  WRP: no ranges write-protected
  bootloader 0x08000000 +32 KB (pages 0-15): NOT write-protected
hint: `alloy secure apply --rdp 1 --wrp-bootloader` locks a production unit (guarded — it explains itself first)
```

`status` is always safe: it reads option registers (readable even at RDP level 1) and never
writes. Every `apply` starts by printing the same status, so you always see the current level
before anything happens.

## What the RDP levels mean

| Level | Byte | Meaning | Way back |
| --- | --- | --- | --- |
| 0 | `0xAA` | fresh from the fab — flash fully readable over SWD | — |
| 1 | any other | debugger flash access blocked; option bytes still writable | to level 0: the hardware **mass-erases the entire flash first** |
| 2 | `0xCC` | debug port **permanently disabled** | **none — ever** |

Two traps live in that table, and they are why `alloy secure apply` is deliberately loud:

!!! danger "Level 1 → 0 mass-erases the chip"
    Dropping readout protection erases **all** flash — bootloader, both firmware slots, the
    boot-state store — before protection releases. That is the feature (protected firmware
    can never be exposed by unlocking), but it means "just unlock it to debug this returned
    unit" destroys the unit's firmware. `alloy secure apply --rdp 0` on a level-1 part
    refuses unless you pass `--accept-mass-erase` **and** type `erase <part>` when prompted,
    and it prints what will be lost before touching the device.

!!! danger "Level 2 is a manufacturing decision, not a setting"
    Level 2 burns the debug port off. No probe will ever connect again — not yours, not
    ST's. Failure analysis, factory reflash, `alloy secure status` itself: all gone, forever.
    The only remaining way into the device is your own UART bootloader. Choose it for
    products where the threat model demands it, as an explicit manufacturing step. The CLI
    refuses `--rdp 2` unless you pass `--permanent` **and** type
    `permanently disable debug on <part>`. It is never a default and no other flag implies it.

Level 1 is the production baseline: field updates over UART still work (the bootloader runs
from flash, unaffected), and returned units can still be inspected at the cost of an erase.

## The bootloader write-protect derives from the slot layout

```console
$ alloy secure apply --rdp 1 --wrp-bootloader
nucleo_f722ze (st/STM32F722ZE)
  RDP: level 0 (byte 0xaa) — flash readable over the debug port — NOT production-safe
  ...
WARNING: RDP level 1 blocks debugger access to flash. Returning to level 0 later
MASS-ERASES the chip — every byte of every slot. Field updates still work (the UART
bootloader is unaffected).

WARNING: write-protecting the bootloader region — the bootloader can then only be
replaced by first clearing WRP over the probe.

proceed? [y/N] y
option bytes programmed and verified by readback
```

`--wrp-bootloader` never takes an address. The range comes from `emit/slots.py` — the same
computation that links the bootloader and partitions the slots — so the protected range and
the real bootloader cannot disagree:

* **STM32G0**: the bootloader is 2 KB pages 0–15; `WRP1AR` gets `STRT=0, END=15`.
* **STM32F722**: the bootloader is the first two 16 KB sectors; `nWRP` bits 0 and 1 are
  cleared (the field is active-low).

Everything is a read-modify-write under field masks from the device database: option bits
this tool does not manage (brown-out level, watchdog mode, reset behaviour, boot addresses)
are read back exactly as they were.

!!! danger "On the G0, this range also contains the factory identity page"
    The slot layout carves per-device identity out of the **last page of the bootloader region**
    on uniform-page flash — page 15 on the G0. `--wrp-bootloader` therefore freezes it too, so
    `alloy provision write` must run **before** `alloy secure apply`, not after. Get it backwards
    and the write silently does nothing; the provisioning verb's readback is what catches it.

    On the F722 the identity page is its own sector, outside the WRP range, so it stays writable
    — but RDP level 1 locks the probe out either way. The line order is the same on both:
    provision, then secure. See [Firmware update](firmware-update.md#mass-programming-the-production-line).

A note on scope: `apply` supports STM32G0 and STM32F72x/F73x. On STM32F76x/F77x, `status`
works but `apply` refuses — those parts carry a wider `nWRP` field and an `nDBANK` bit that
changes the whole sector map, and alloy does not model that yet. The refusal says so.

## What `alloy secure` refuses, and why

| You ask | It does |
| --- | --- |
| `apply` with no flags | prints status + advice; **never** acts |
| `apply --rdp 1` (from 0) | warns (regression = mass erase), then y/N or `--yes` |
| `apply --rdp 0` (from 1) | **refuses** without `--accept-mass-erase` + typed `erase <part>` |
| `apply --rdp 2` | **refuses** without `--permanent` + typed `permanently disable debug on <part>` |
| anything on a level-2 part | refuses — nothing can change a level-2 part (the probe cannot even connect) |
| `--wrp-bootloader` on a chip without a slot layout | refuses — there is nothing to derive the range from |
| unsupported family or probe | refuses, naming the missing data or mapping |

The typed phrases are read from stdin, so a factory line can pipe them — the warnings print
regardless, into the factory log. `--yes` only answers the routine y/N; it never stands in
for a typed phrase. `--dry-run` prints the exact openocd commands and exits.

One quirk worth knowing: on STM32G0 the final step (`OBL_LAUNCH`) **resets the chip**, which
kills the debug session mid-run. The CLI expects that, says so, and tells you to re-run
`alloy secure status` to see the new state.

## What is proven, and what is not

Honesty section, per alloy's usual rule: claims carry their evidence.

* **Witnessed in emulation (Renode 1.16.1, F7 model)**: the option-byte register interface —
  a locked `OPTCR` ignores writes, the `OPTKEYR` key sequence unlocks it, the planner's own
  poke list programs the expected value, relocking holds. The test drives the *same* write
  list `apply` sends to openocd (`tools/alloy/tests/test_secure.py`).
* **Community-proven tooling**: the G0 path uses openocd's `stm32l4x option_write` /
  `option_load` driver, which is what the openocd project ships for G0 silicon.
* **Not witnessed by alloy on silicon**: actual readout blocking, the 1 → 0 mass erase, RDP2
  permanence, WRP refusing an erase, persistence across power cycles — and the claim that
  option registers stay *readable* under level 1 (what makes `status` "always safe" there)
  is likewise RM-only. These rest on
  RM0444/RM0431 and ST's own tooling behaviour. No board was sacrificed to verify them —
  RDP is exactly the thing you do not test casually on your only board. Renode's G0 has no
  flash-controller model at all, and its F7 model has no protection *semantics* (RDP has no
  behavioural effect there), so emulation cannot close this gap. If you have a unit to
  spare, `alloy secure apply --rdp 1` followed by a failed flash read, and `--rdp 0
  --accept-mass-erase` followed by `alloy secure status` showing a blank level-0 part, is
  the full silicon witness — we would love the report.
