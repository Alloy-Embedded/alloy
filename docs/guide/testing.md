# Testing your firmware on a laptop

Most firmware bugs are not electrical. They are an off-by-one in a state machine, a threshold that
chatters, a retry that never retries, an error path nobody took. None of those need a board to find
— but they usually get one anyway, because the logic is tangled up with the peripherals.

alloy's answer is not a mocking framework. It is a shape:

!!! tip "The whole technique"
    Put the decisions in a header **templated on alloy's concepts**. Keep `main.cpp` to board
    wiring. The same source then compiles against the generated silicon driver on the target and
    against a fake on your laptop, because both satisfy the same concept.

## Start from the scaffold

```console
$ alloy new thermo --board nucleo_g071rb --with-tests
created thermo/ (board: nucleo_g071rb, framework: /path/to/alloy)
  + src/app.hpp
  + src/main.cpp
  + tests/test_app.cpp
  + tests/CMakeLists.txt
next:  cd thermo && alloy test     # runs on your laptop, no board
       cd thermo && alloy run      # and on the target
```

```console
$ cd thermo && alloy test
...
1/1 Test #1: thermo_tests .....................   Passed    0.31 sec
100% tests passed, 0 tests failed out of 1
```

The runner's own output, if you run the binary directly:

```console
$ ./.alloy/host-tests/thermo_tests
[ PASS ] thermostat_heats_when_cold
[ PASS ] thermostat_idles_when_warm
[ PASS ] thermostat_does_not_chatter_in_the_band
[ PASS ] thermostat_holds_output_when_the_sensor_fails
[ PASS ] thermostat_addresses_the_sensor_correctly
PASSED 5/5
```

Five tests, no hardware, a third of a second. Two of them — the hysteresis band and the sensor that
NACKs mid-operation — are genuinely awkward to reproduce on a bench.

## What the scaffold actually does

`src/app.hpp` holds the logic and names no chip:

```cpp title="illustrative: the scaffold's app.hpp, elided — `alloy new --with-tests` writes the whole of it"
template <class Bus, class Pin>
    requires alloy::I2cBus<Bus> && alloy::OutputPin<Pin>
class thermostat {
    bool poll();          // read the sensor, decide, drive the output
    ...
};
```

`tests/test_app.cpp` instantiates it against fakes:

```cpp title="illustrative: a host test, built by `alloy test` and not by the firmware toolchain"
#include "alloy_test.hpp"
#include "doubles.hpp"
#include "testkit/mock_bus.hpp"
#include "app.hpp"

ALLOY_TEST(thermostat_holds_output_when_the_sensor_fails) {
    alloy::testkit::mock_i2c bus;      // scriptable I²C
    alloy::test::fake_pin heater;      // recording output pin
    app::thermostat t{bus, heater, {}};

    bus.queue_read(cold_reading);
    ALLOY_CHECK(t.poll());
    ALLOY_CHECK(t.heating());

    bus.fail = true;                   // the sensor stops answering
    ALLOY_CHECK(!t.poll());
    ALLOY_CHECK(t.heating());          // and the output must NOT move
}
```

`main.cpp` is the only file that mentions a board, and it stays short.

## The fakes you get for free

They are not test-framework mocks; they are ordinary structs that satisfy the same concepts the
silicon drivers do, each with a `static_assert` against the real concept so a concept change breaks
the double instead of silently diverging.

| Double | Satisfies | Header | Gives you |
|---|---|---|---|
| `alloy::test::fake_pin` | `OutputPin` | `doubles.hpp` | current `level`, `toggles` count |
| `alloy::test::fake_uart<Cap>` | `ByteStream` | `doubles.hpp` | captured TX, replayed RX, `tx_contains()` |
| `alloy::test::fake_clock` | callable clock | `doubles.hpp` | `advance(ms)` — drive timeouts without waiting |
| `alloy::testkit::mock_i2c` | `I2cBus` | `testkit/mock_bus.hpp` | `queue_read()`, `last_addr`, full `writes[]` history, `fail` |
| `alloy::testkit::mock_spi` | `SpiBus` | `testkit/mock_bus.hpp` | the same, for SPI |
| `alloy::testkit::mock_delay` | `DelayNs` | `testkit/mock_bus.hpp` | records requested delays instead of sleeping |

`fake_clock` and `mock_delay` matter more than they look: a timeout test that really waits is a
test nobody runs twice.

## The harness

`alloy_test.hpp` is deliberately not Catch2 or doctest. Those need exceptions, RTTI and a heap, so
they could never cross-compile onto a `-fno-exceptions` target — and then "the tests pass" would be a
statement about a *different* build of your code than the one you ship. alloy's harness is
freestanding and heapless, and your project's tests are compiled with the firmware's own
`-fno-exceptions -fno-rtti`, so passing here means the same source compiles for the target.

Two macros:

```cpp title="illustrative: the two macros, not a program"
ALLOY_TEST(name) { ... }        // self-registering test
ALLOY_CHECK(expr);              // record a condition
ALLOY_CHECK_EQ(a, b);           // ... and report both sides on failure
```

!!! warning "Commas inside template arguments"
    `ALLOY_CHECK_EQ` is a macro, so `ALLOY_CHECK_EQ(x, foo<A, B>::k)` looks like three arguments to
    the preprocessor. Name the instantiation first: `using foo_t = foo<A, B>;`.

Sanitizers (ASan + UBSan) are on by default except on macOS, where ASan's at-exit pass hangs on
arm64. `alloy test --no-sanitize` turns them off.

## Adding your own tests

Drop another `tests/test_*.cpp` in. The scaffold's CMake globs them, so there is nothing to edit.

## Coverage

```console
$ alloy test --coverage
...
File                                       Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
app.hpp                                       25       25   100%
------------------------------------------------------------------------------
lines: 100.0% (25 out of 25)
functions: 100.0% (5 out of 5)
branches: 100.0% (12 out of 12)
```

The report is scoped to your project's `src/` — the framework's runner and host seams are not your
coverage. It needs **gcovr** on `PATH` (`pip install gcovr`), and under Clang it also needs
`llvm-cov`, because Clang's profile data is not plain gcov's and `alloy test` tells gcovr so. If
either is missing you get a message naming what to install, and the profile is left in place rather
than thrown away.

## `alloy test`, precisely

| Where you run it | What runs |
|---|---|
| in a project with `tests/CMakeLists.txt` | that project's suite |
| in a project without one | the framework's own suite |
| in the alloy repo | the framework's own suite |
| anywhere, with `--framework` | the framework's own suite |

The framework is resolved from your `alloy.toml` (`[alloy] root`, and the `[devices]` pin if you
have one), not from whatever checkout happens to sit above your working directory — so a project's
tests compile against the framework the project *declares*.

## Say which tree you measured

A build result is a fact about a working tree at an instant, not about a
commit — and it is easy to forget that, because the two usually agree.

They stop agreeing whenever something edits the tree while you build:
another person, another session, a script mid-rebase. The CLI makes this
sharper than it looks. `uv run --project <tree>/tools/alloy` and an editable
install both resolve to the *live* tree, so codegen runs with whatever is on
disk at that moment — including changes nobody has committed. Building an
old commit in a worktree does not escape it either; the CLI still emits from
the tree the install points at.

So when a result is going to be quoted, stamp it:

```bash
git status --porcelain | wc -l   # 0, or the number is part of your claim
git rev-parse --short HEAD
```

This matters most for **red** results, which is the counter-intuitive half.
A false green wastes a review; a false red triggers a fix — for a defect
that may live in somebody's uncommitted edits and vanish on its own. Two
sessions in this project once reported, independently and in good faith,
that every example failed to build on three boards at a given commit. The
commit was fine. The emitter change they had both compiled against existed
only in a working tree, transiently, and one of them nearly landed a
"hotfix" for a repository that was never broken.

If a result is worth acting on, regenerate from scratch (`rm -rf .alloy`)
on a tree you have just confirmed clean, and quote the commit beside it.

## What this does not give you

Being direct, because it is easy to oversell:

- **This is not hardware validation.** It tests logic against fakes. A driver that writes the wrong
  register still passes, because the fake accepts whatever it is given. Register-level behaviour is
  what the [emulation legs](emulation.md) are for, and neither substitutes for silicon.
- **It does not test your `main.cpp`.** Board wiring is exactly the part with no double. Keep it
  thin so there is little there to be wrong.
- **Concept satisfaction is not behavioural equivalence.** `mock_i2c` satisfies `I2cBus`; it does not
  reproduce clock stretching, arbitration loss, or a device that NACKs on the third byte.
- **There is no test for the scaffold's own sensor.** The thermostat's device is invented for the
  template. Replace it with your real part — or with a driver from `alloy lib`, which arrives with
  its own testkit-based tests.
- **Coverage is line and branch coverage of host runs.** It says nothing about the target build,
  and 100% of a header is not 100% of a product.

## In CI

The `scaffold` job does this end to end on every push: scaffolds `--with-tests` out-of-repo, builds
the firmware, runs the suite, reports coverage, and then **inverts one expectation and demands a red
run** — a test scaffold that cannot fail is a decoration.

## If you are changing alloy itself

Everything above is about testing *your* firmware. This last section is for contributors: the
gates that guard the framework, all of them runnable on a laptop, in the order they get cheaper
to be wrong about.

| Run this | Costs | Fails when |
|---|---|---|
| `./scripts/check_contract.sh` | < 1 s | a silicon address is hand-written in `src/`, `libs/` or the generator; a 32-bit mask is built from `1u <<`; a `struct feat` is hand-written; an example uses a preprocessor conditional |
| `python3 scripts/check_doc_facts.py -v` | ~1 s | a page names a board that is not in `boards/`, an `alloy <verb>` the CLI does not define, or a link/anchor that resolves to nothing |
| `python3 scripts/check_doc_facts.py --self-test` | ~5 s | that checker has stopped catching a wrong board, a wrong verb, a wrong subcommand or a dangling anchor |
| `alloy test --framework` | ~30 s | a host unit test regresses (640 assertions today, ASan + UBSan, built `-fno-exceptions -fno-rtti` like the firmware) |
| `uv run --project tools/alloy python -m pytest tools/alloy/tests -q` | ~10 s | the CLI, the emitters or the board/product validators regress |
| `mkdocs build --strict` | ~1 s | a docs page has a broken reference mkdocs can see |
| `python3 scripts/check_doc_snippets.py --audit` | < 1 s | a fence opts out of the compile gate without saying why |
| `python3 scripts/check_doc_snippets.py` | ~20 min | a `cpp` sample in `docs/` does not compile |
| `uv run --project tools/alloy python scripts/check_compile_errors.py` | ~2 min | wrong code stops failing to compile, or stops saying *why* |
| `bash scripts/check_static_limits.sh blink nucleo_g071rb` | ~1 min | an image grows a heap, an exception table, recursion, or an unbounded stack |

The last four need a cross toolchain (`arm-none-eabi-g++` on `PATH`, `alloy setup`) and skip
themselves with a message if there is none, rather than passing quietly.

### The docs are gated too, and how to opt a snippet out

`check_doc_snippets.py` pulls every ` ```cpp ` fence out of **every page under `docs/`**, drops it
into a throwaway project and builds it. The default is that a page is checked, so a new page
arrives checked; pages that claim to build everywhere (`index.md`, `getting-started.md`,
`portable-code.md`) are built for all nine boards.

Not every fence is a program. When one genuinely is not, say so **in the page**:

| Marker | Means |
|---|---|
| ` ```cpp title="illustrative: <why>" ` | not built. The reason is required, and mkdocs renders it as a caption — so the reader is told this block is not a program, which is the point of putting it there rather than in a skip-list nobody reads |
| `<!-- docgate: setup` … `-->` | file-scope lines the gate injects before the next fence, for a name the page uses but never shows being created |
| `<!-- docgate: setup-local` … `-->` | the same, as statements inside `main()` — needed when a later fence captures the name in a lambda |
| `<!-- docgate: boards <id> … -->` | the next fence is a claim about other silicon; hold it to that silicon instead of the page's default board |
| `<!-- docgate: ungated — <why> -->` | the whole page is out. Two design records use it; every run prints the list with its reason |

`--audit` prints, per page, how many fences are built, how many opted out and how many lean on
hidden setup. That number is the honest measure of how checked the docs are, so it is worth
watching rather than growing.
