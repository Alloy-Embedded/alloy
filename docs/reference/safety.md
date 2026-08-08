# Safety posture

What is already true of an alloy image by construction, what the number
actually is, **how each one was checked**, and — the part that matters more —
what is *not* true today.

Nothing on this page is a certification claim. Alloy has never been assessed
against IEC 61508, ISO 26262 or DO-178C, uses no qualified tool chain, and
carries no MISRA conformance statement. What it has is a set of properties you
can re-verify yourself in about two minutes:

```console
$ export PATH="$HOME/.local/bin:$PATH"          # and the arm-none-eabi toolchain
$ scripts/check_static_limits.sh
== blink@nucleo_g071rb
ok   blink@nucleo_g071rb: no heap, no exceptions/RTTI, no recursion; peak stack 180 bytes (thread 132 + handler 16 + 32 entry), 3 indirect call(s) not followed, 10 frame(s) estimated, 0 vtable(s)
== async_blink@nucleo_g071rb
ok   async_blink@nucleo_g071rb: … peak stack 128 bytes …
== fs@nucleo_g0b1re
ok   fs@nucleo_g0b1re: … peak stack 1776 bytes …
== http_server@nucleo_f767zi
ok   http_server@nucleo_f767zi: … peak stack 688 bytes …
== negative control: a project that DOES recurse and DOES allocate
ok   negative control: recursion detected — _Z4pingi -> _Z4pongi -> _Z4pingi
ok   negative control: heap detected — ['_free_r', '_malloc_r', '_sbrk', '_sbrk_r', 'free', 'malloc']
== negative control: -fno-exceptions / -fno-rtti actually refuse the code
ok   control: the same code compiles when the flags are absent
ok   negative control: refused — 4 compile error(s)
static limits: every claim on docs/reference/safety.md holds
```

The negative controls are the point. A checker that only ever prints "clean"
proves nothing, so every property below is paired with a deliberately broken
build that the same checker **must** catch.

---

## Scope of the measurements

| | |
|---|---|
| Analysed | **146 linked ELF images** — every example × board build tree in this repo |
| Boards | 7: `nucleo_g071rb` (29), `same70_xplained` (28), `nucleo_g0b1re` (25), `nucleo_f722ze` (21), `rp2040_zero` (19), `raspberry_pi_pico` (16), `nucleo_f767zi` (8) |
| Architecture | **Cortex-M only** |
| Not measured | **Xtensa (ESP32) and RL78.** The RL78 backend is new and has no released binary toolchain here; no number on this page covers either. |
| Tool | `scripts/static_limits.py`, ~450 lines, no dependencies beyond binutils |

---

## 1. No exceptions, no RTTI

**True, and enforced at compile time — which is stronger than "the image
happens to contain no unwinder".**

How it is checked, three ways:

1. **The flags.** `tools/alloy/alloy_cli/build.py` puts `-fno-exceptions`,
   `-fno-rtti`, `-fno-threadsafe-statics` and `-fno-use-cxa-atexit` on every
   C++ translation unit of every firmware build. The host test suite
   (`tests/CMakeLists.txt`) uses the same first two, so a test cannot pass
   using machinery the firmware does not have.
2. **The compiler refuses.** A translation unit containing `throw` and
   `typeid` compiles cleanly with default flags and fails with **4 errors**
   under alloy's — `cannot use 'typeid' with '-fno-rtti'` and the exception
   equivalents. That is negative control 2 above.
3. **The image.** No `__cxa_throw`, `__cxa_allocate_exception`,
   `__gxx_personality_v0` or `_Unwind_RaiseException` in any of the 146
   images; `.eh_frame` is 0 bytes; the linker script `/DISCARD/`s
   `.ARM.exidx` and `.ARM.extab` outright.

Two honest footnotes:

* `-fno-rtti` does **not** forbid virtual functions. Virtual dispatch is legal
  and the checker reports vtables (`_ZTV…`) rather than failing on them. All
  146 images happen to carry **zero** vtables today, but that is a property of
  the code written so far, not a rule.
* Error handling therefore uses return values and `expected`-shaped types
  throughout — see [Error handling](error-handling.md).

## 2. No libc heap

**True, with a qualification that matters.**

* No `malloc`, `calloc`, `realloc`, `free`, `_sbrk`, `operator new` or
  `operator delete` is defined or referenced in any of the 146 images.
* The linker script is heapless by design but still defines `end` /
  `__heap_start`, so a stray `printf` gets a bounded region and a link-time
  error instead of `undefined reference to 'end'`.
* Static footprint that does not fit fails the **link** — the `._heap_stack`
  region is placed `> RAM` and reserves `_Min_Stack_Size` (0x800 by default,
  overridable with `--defsym`), so an overflow is a readable "region RAM
  overflowed", not silent corruption at runtime.

**The qualification:** "no heap" is not "no allocation". The lwIP images
define `mem_malloc`, `memp_malloc`, `mem_free` and friends — those are lwIP's
own allocators over **fixed static pools** sized in the generated
`lwipopts.h` (`MEM_SIZE`, `PBUF_POOL_SIZE`, `MEMP_NUM_*`). They cannot grow,
they never call `sbrk`, and they fail by returning null. If your posture
requires *no dynamic allocation of any kind*, an lwIP image does not qualify;
a non-networked one does.

Negative control: a project that calls `std::malloc` links, and the checker
reports `['_free_r', '_malloc_r', '_sbrk', '_sbrk_r', 'free', 'malloc']`.

## 3. No recursion

**True in the direct-call graph of all 146 images. Conditional on indirect
calls, and that condition is stated with every result.**

Method: disassemble the linked image, build a call graph from `bl`/`blx` to a
symbol plus `b`/`b.w` landing on another function's entry (that is how `-Os`
spells a tail call), then look for cycles with a DFS. Only `STT_FUNC` symbols
are considered, so const tables emitted into `.text` are not mistaken for code.

Result: **zero cycles, in every image, on every board.** That includes the
lwIP and littlefs images, which are where recursion normally hides.

What this does **not** cover: calls made through a register or a table are
invisible to the method. They are counted and printed with every result —
3 in a plain `blink`, 5 with the async executor, 18 in the littlefs image,
25–28 in the lwIP ones. Two of the three in `blink` are `Reset_Handler`
walking `__init_array_start`…`__init_array_end`; the third is
`alloy_irq_dispatch` reaching the registered handler. A recursion that exists
only through a function pointer would not be found.

The usual trap is littlefs, and it is clean for a reason worth knowing: the
recursive `lfs_dir_traverse` was replaced upstream by an explicit bounded
stack — `struct lfs_dir_traverse stack[LFS_DIR_TRAVERSE_DEPTH-1]` with
`LFS_DIR_TRAVERSE_DEPTH 3` (`src/alloy/fs/vendor/lfs.c:889,920`).

Negative control: a mutually recursive pair marked `noinline,noipa` (so `-Os`
cannot turn it into a loop) is reported as
`_Z4pingi -> _Z4pongi -> _Z4pingi`.

## 4. Bounded stack — with real numbers

**A bound can be computed, and here it is. It is an engineering estimate, not
a certified WCET result.**

Every C and C++ file is now compiled with `-fstack-usage`, so GCC writes each
function's frame size beside its object file. `static_limits.py` matches those
to the linked symbols by demangled name and sums along the longest path in the
call graph.

| Image | Thread (from `main`) | Worst handler | + exception entry | **Peak** | RAM left for stack | Peak as % |
|---|---|---|---|---|---|---|
| `blink@nucleo_g071rb` | 132 B | 16 B | 32 B | **180 B** | 36 696 B | 0.5 % |
| `async_blink@nucleo_g071rb` | 80 B | 16 B | 32 B | **128 B** | 36 112 B | 0.4 % |
| `http_server@nucleo_f767zi` (lwIP) | 648 B | 8 B | 32 B | **688 B** | 460 376 B | 0.1 % |
| `fs@nucleo_g0b1re` (littlefs) | 1 728 B | 16 B | 32 B | **1 776 B** | 146 264 B | 1.2 % |

Deepest across all 146 images: `fs` at 1 776 B; then `dhcp_echo` 952 B,
`tcp_echo` 936 B, `modbus_rtu_server` 752 B. "RAM left for stack" is
`_estack` minus `_end` from the image's own symbol table.

The bound also reports a **reset path** (`Reset_Handler` → `main`) separately,
because it runs before `main` and then hands over — 140 B in `blink`.

### What the number includes, and what it does not

Included: the deepest chain of compiler-declared frames from `main`, plus the
deepest handler chain, plus the 8 words (32 B) Cortex-M pushes on exception
entry.

**Not** included, and each one can only make the real figure larger:

* **Indirect calls are not followed** (see above), so a deeper path reachable
  only through a function pointer is not in the sum.
* **One level of interrupt nesting** is assumed. If you enable priorities that
  allow a second preemption, add that handler's path too.
* **No FP exception frame.** These builds are `-mfloat-abi=soft`; a build with
  lazy FP stacking pushes 18 more words.
* **Library functions the build did not compile** have no `.su` file — 10 in
  `blink`, all libgcc integer helpers (`__divdi3`, `__aeabi_ldivmod`, …).
  Their frames are estimated from the disassembled body, which *over*-counts a
  function with two alternative prologues. They are listed and marked `~` in
  the output, and they are on the deepest `blink` path, which is why its 132 B
  is flagged as containing an estimate.
* **No `alloca`, no VLAs** are modelled. None appear in `src/alloy`; a project
  that adds one is on its own.

This is a design-review number. It is not produced by a qualified tool and
should not be pasted into a safety case as though it were.

## 5. Deterministic control flow

**True of the concurrency model as written; verify the parts you rely on.**

* One cooperative executor, one stack, single core. `alloy::scheduler`
  (`src/alloy/sched.hpp`) is a fixed compile-time table of tasks run by a
  `run_once(now)` superstep — no preemption between tasks, no context switch,
  no heap. `alloy::async::executor_core` is the coroutine equivalent, with a
  fixed ready queue; a second instance is a `__builtin_trap`, and a ready-queue
  overflow traps rather than dropping work.
* Coroutine frames are statically sized: `task_storage<N>` declares the size
  and `alloy frame-audit` reads the real frame sizes back out of the built
  ELF's DWARF and compares. That is a genuine static memory check — of
  coroutine frames, **not** of stack depth. Do not conflate the two.
* Interrupts are the only preemption. `run_once` is pure enough to be driven
  with a fake clock on the host, which is how the scheduling logic is tested
  without hardware.
* **Startup is not free of dynamic initialisation.** 30 of the 146 images have
  a non-empty `.init_array`, and `Reset_Handler`
  (`src/alloy/arch/cortex_m/startup.cpp`) walks `__preinit_array` and
  `__init_array` through function pointers before calling `main`. In the
  littlefs image the two entries are the generated `board.hpp`'s static
  objects. `-fno-use-cxa-atexit` means static **destructors** are never
  registered and never run, which is correct for a `main` that does not return.

## 6. Facts come from data, not from typing

Two mechanical guards, run by `scripts/check_contract.sh` in CI:

* no hardware address literal may appear in hand-written C++ under
  `src/alloy` (they must come from the chip database), and
* a 32-bit mask may not be built from `1u`, whose width the target decides.

These are not safety standards, but they remove two of the classic sources of
"works on this chip" defects. See [Stability](stability.md) for how to pin the
database those facts come from — an unpinned database is a bigger correctness
risk than anything on this page.

---

## What is NOT true today

Written plainly, because the value of this page is that you can trust the
parts above.

| Claim you might expect | Reality |
|---|---|
| **MISRA C++ conformance or a deviation record** | **None.** There is no MISRA configuration, no deviation register, and no static-analysis run of any kind in this repo. There is no `.clang-tidy` and no `.clang-format` either. Any MISRA posture would have to be built from scratch. |
| Certified or qualified tooling | None. The compiler is stock GCC; `static_limits.py` is a 450-line script with no qualification evidence. |
| A safety manual, FMEDA, or failure-rate data | None. |
| Formal WCET or a sound stack analysis | No. Section 4 states its own limits. |
| The properties above on Xtensa or RL78 | **Unmeasured.** Cortex-M only. |
| "No dynamic allocation anywhere" | False for lwIP images — static pools, see §2. |
| "No virtual functions" | Not a rule; today's images happen to have no vtables. |
| Freedom from interference between tasks | There is one stack and no MPU use. A task that overruns its arithmetic corrupts its neighbours like any other single-stack system. |
| Redundancy, lockstep, or diagnostic coverage of the CPU | Nothing of the kind exists. |

What *does* exist next to this page: a crash path that captures faults and
decodes them (`alloy crash`), signed A/B updates with anti-rollback, RDP/WRP
locking, and roughly fifteen blocking emulation legs in CI. Those are
reliability engineering, not functional safety, and the difference is the
subject of this page.

---

## Re-running any of it

```console
# every property, four images, all negative controls
scripts/check_static_limits.sh

# a specific image
scripts/check_static_limits.sh http_server nucleo_f767zi

# the raw report for one ELF, including the deepest path
python3 scripts/static_limits.py examples/fs/.alloy/build-tree/nucleo_g0b1re/out/fs.elf
python3 scripts/static_limits.py <elf> --json | jq .stack
```

`static_limits.py --help` prints the method and its limits in full; they are
also at the top of the file.
