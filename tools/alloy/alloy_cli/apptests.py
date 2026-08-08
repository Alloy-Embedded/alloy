"""Host testing for APPLICATION code — the scaffold, the runner and the coverage
report behind `alloy new --with-tests` and `alloy test`.

alloy already had everything needed to test firmware logic on a laptop and no way
to discover it: `tests/doubles.hpp` (fake pin, fake UART, settable clock) and
`libs/testkit/mock_bus.hpp` (scriptable I2C/SPI) satisfy the same concepts the
silicon drivers do, and `tests/alloy_test.hpp` is a heapless, exception-free
harness that compiles under the firmware's own `-fno-exceptions -fno-rtti`. What
was missing was a project that arrives with them already wired up, and an
`alloy test` that runs the PROJECT's suite rather than only the framework's.

The scaffold teaches one thing, deliberately: put the logic in a header
templated on alloy's concepts, keep `main.cpp` to board wiring, and the logic is
testable off-target for free. That is the whole technique.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

# --- the scaffold ----------------------------------------------------------

APP_HPP = """\
// The part of a firmware worth testing lives HERE, not in main.cpp, and it names
// no chip: it is templated on alloy's concepts (I2cBus, OutputPin), so the same
// source compiles against the generated silicon driver on the target and against
// a fake on your laptop. tests/test_app.cpp does the second.
#pragma once

#include <cstdint>
#include <span>

#include <alloy/concepts.hpp>

namespace app {

// A thermostat with hysteresis: heat below `on_below_c`, stop above
// `off_above_c`. The gap between them is what stops the output chattering, and
// it is exactly the kind of rule that is miserable to verify on a bench and
// trivial to verify in a unit test.
struct thermostat_config {
    std::int16_t on_below_c = 18;
    std::int16_t off_above_c = 21;
};

template <class Bus, class Pin>
    requires alloy::I2cBus<Bus> && alloy::OutputPin<Pin>
class thermostat {
public:
    // The sensor: 7-bit address, one register, big-endian degrees in the high
    // byte. Made up for the template — replace with your real device (or better,
    // with a driver from `alloy lib`).
    static constexpr std::uint8_t kAddr = 0x48;
    static constexpr std::uint8_t kTempReg = 0x00;

    thermostat(const Bus& bus, const Pin& heater, thermostat_config cfg)
        : bus_(bus), heater_(heater), cfg_(cfg) {}

    // One control step. Returns false when the sensor did not answer — and
    // crucially does NOT change the output in that case, which is the bug this
    // being a unit test makes cheap to pin down.
    bool poll() {
        const std::uint8_t reg = kTempReg;
        std::uint8_t raw[2]{};
        if (!bus_.write_read(kAddr, std::span<const std::uint8_t>{&reg, 1},
                             std::span<std::uint8_t>{raw})) {
            ++faults_;
            return false;
        }
        last_c_ = static_cast<std::int16_t>(static_cast<std::int8_t>(raw[0]));
        if (!heating_ && last_c_ < cfg_.on_below_c) {
            heating_ = true;
            heater_.set_high();
        } else if (heating_ && last_c_ > cfg_.off_above_c) {
            heating_ = false;
            heater_.set_low();
        }
        return true;
    }

    [[nodiscard]] bool heating() const { return heating_; }
    [[nodiscard]] std::int16_t last_c() const { return last_c_; }
    [[nodiscard]] std::uint32_t faults() const { return faults_; }

private:
    const Bus& bus_;
    const Pin& heater_;
    thermostat_config cfg_;
    bool heating_ = false;
    std::int16_t last_c_ = 0;
    std::uint32_t faults_ = 0;
};

}  // namespace app
"""

TEST_APP_CPP = """\
// Host unit tests for the application logic. No hardware, no target, no heap —
// this runs on your laptop in milliseconds, in the SAME dialect the firmware is
// compiled in (-fno-exceptions -fno-rtti).
//
// The two ingredients:
//   alloy::testkit::mock_i2c  — a scriptable I2C bus (libs/testkit/mock_bus.hpp)
//   alloy::test::fake_pin     — a recording output pin (tests/doubles.hpp)
// Both satisfy the same concepts the real drivers do, which is why app::thermostat
// takes them without knowing.
#include "alloy_test.hpp"
#include "doubles.hpp"
#include "testkit/mock_bus.hpp"

#include "app.hpp"

namespace {

// Build a thermostat wired to fakes, with the sensor primed to report `deg`.
struct rig {
    alloy::testkit::mock_i2c bus;
    alloy::test::fake_pin heater;

    void sensor_says(int deg) {
        bus.reset();
        const std::uint8_t frame[2] = {static_cast<std::uint8_t>(deg), 0};
        bus.queue_read(frame);
    }
};

}  // namespace

ALLOY_TEST(thermostat_heats_when_cold) {
    rig r;
    app::thermostat t{r.bus, r.heater, {}};
    r.sensor_says(10);
    ALLOY_CHECK(t.poll());
    ALLOY_CHECK_EQ(t.last_c(), 10);
    ALLOY_CHECK(t.heating());
    ALLOY_CHECK(r.heater.level);
}

ALLOY_TEST(thermostat_idles_when_warm) {
    rig r;
    app::thermostat t{r.bus, r.heater, {}};
    r.sensor_says(25);
    ALLOY_CHECK(t.poll());
    ALLOY_CHECK(!t.heating());
    ALLOY_CHECK(!r.heater.level);
}

// The whole point of hysteresis: between the two thresholds, nothing changes.
ALLOY_TEST(thermostat_does_not_chatter_in_the_band) {
    rig r;
    app::thermostat t{r.bus, r.heater, {}};
    r.sensor_says(10);
    ALLOY_CHECK(t.poll());
    ALLOY_CHECK(t.heating());
    for (int deg = 19; deg <= 21; ++deg) {
        r.sensor_says(deg);
        ALLOY_CHECK(t.poll());
        ALLOY_CHECK(t.heating());  // still on: not yet ABOVE off_above_c
    }
    r.sensor_says(22);
    ALLOY_CHECK(t.poll());
    ALLOY_CHECK(!t.heating());
}

// A sensor that NACKs must not move the output. Reproducing this on a bench
// means unplugging a wire at the right moment; here it is one flag.
ALLOY_TEST(thermostat_holds_output_when_the_sensor_fails) {
    rig r;
    app::thermostat t{r.bus, r.heater, {}};
    r.sensor_says(10);
    ALLOY_CHECK(t.poll());
    ALLOY_CHECK(t.heating());

    r.bus.fail = true;
    ALLOY_CHECK(!t.poll());
    ALLOY_CHECK_EQ(t.faults(), 1u);
    ALLOY_CHECK(t.heating());  // unchanged
    ALLOY_CHECK(r.heater.level);
}

// The bus double records every transaction, so "did it address the right device
// and ask for the right register?" is an assertion, not an oscilloscope session.
ALLOY_TEST(thermostat_addresses_the_sensor_correctly) {
    rig r;
    // Name the instantiation: a bare `app::thermostat<A, B>::kAddr` inside
    // ALLOY_CHECK_EQ would hand the preprocessor three arguments, because the
    // macro cannot see that the comma is inside template brackets.
    using thermo = app::thermostat<alloy::testkit::mock_i2c, alloy::test::fake_pin>;
    thermo t{r.bus, r.heater, {}};
    r.sensor_says(20);
    ALLOY_CHECK(t.poll());
    ALLOY_CHECK_EQ(r.bus.last_addr, thermo::kAddr);
    ALLOY_CHECK_EQ(r.bus.last_write_len, 1u);
    ALLOY_CHECK_EQ(r.bus.last_write[0], 0x00u);
}
"""

MAIN_CPP = """\
// Board wiring only. Every decision the firmware makes lives in src/app.hpp,
// which is templated on alloy's concepts and therefore runs unchanged on your
// laptop under `alloy test` — see tests/test_app.cpp.
#include <alloy/board.hpp>

#include <cstdint>

#include "app.hpp"

using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("@NAME@ up\\r\\n");

    // Swap these two for the real peripherals once the board has an i2c role and
    // a heater pin: `auto bus = board::i2c::open({});` and a board pin. The LED
    // stands in so a fresh scaffold builds and runs on every board.
    if constexpr (board::caps::led) {
        while (true) {
            board::led.toggle();
            alloy::sleep_for(500ms);
        }
    }
    while (true) {
        alloy::sleep_for(1s);
    }
}
"""

TESTS_CMAKE = """\
# Host unit tests for THIS project's application logic. Native toolchain, no
# board data, no linker script: it compiles src/*.hpp against fakes.
#
# ALLOY_ROOT is handed in by `alloy test`, which knows where the framework is
# from alloy.toml. Run the suite that way rather than by bare cmake.
cmake_minimum_required(VERSION 3.25)
project(@NAME@_tests CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED ALLOY_ROOT)
    message(FATAL_ERROR
        "ALLOY_ROOT not set — run these tests with `alloy test`, which resolves "
        "the framework from alloy.toml and passes it in.")
endif()

# The firmware's own dialect, so code that passes here cross-compiles unchanged.
add_compile_options(-fno-exceptions -fno-rtti -Wall -Wextra -Werror -g -O1)

# ASan's at-exit pass hangs on macOS/arm64, so default sanitizers on elsewhere.
if(APPLE)
    set(_san_default OFF)
else()
    set(_san_default ON)
endif()
option(ALLOY_TEST_SANITIZE "Build with AddressSanitizer + UBSan" ${_san_default})
if(ALLOY_TEST_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

option(ALLOY_TEST_COVERAGE "Instrument for gcov/llvm-cov line coverage" OFF)
if(ALLOY_TEST_COVERAGE)
    add_compile_options(--coverage -O0)
    add_link_options(--coverage)
endif()

include_directories(
    ${ALLOY_ROOT}/src        # <alloy/...>
    ${ALLOY_ROOT}/tests      # alloy_test.hpp, doubles.hpp
    ${ALLOY_ROOT}/libs       # testkit/mock_bus.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)

enable_testing()

# Glob so a new test_*.cpp joins the suite with no edit here.
file(GLOB APP_TESTS ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

add_executable(@NAME@_tests
    ${ALLOY_ROOT}/tests/main.cpp          # the runner (stdout sink, exit code)
    ${ALLOY_ROOT}/tests/host_support.cpp  # host impls of the arch/timebase seams
    ${APP_TESTS})

add_test(NAME @NAME@_tests COMMAND @NAME@_tests)
"""


def scaffold(target: Path, name: str, *, with_main: bool = True) -> list[Path]:
    """Write the test-wired extras into an already-created project. Returns the
    files written, so `alloy new` can tell the user what it made.

    with_main=False for the `--chip` route: that scaffold makes a board with NO
    roles yet, so a main.cpp opening board::debug_uart would not compile. The
    logic and its tests are board-independent and go in regardless."""
    (target / "tests").mkdir(parents=True, exist_ok=True)
    files: list[tuple[Path, str]] = [
        (Path("src") / "app.hpp", APP_HPP),
        (Path("tests") / "test_app.cpp", TEST_APP_CPP),
        (Path("tests") / "CMakeLists.txt", TESTS_CMAKE.replace("@NAME@", name)),
    ]
    if with_main:
        files.insert(1, (Path("src") / "main.cpp", MAIN_CPP.replace("@NAME@", name)))
    written = []
    for rel, text in files:
        (target / rel).write_text(text)
        written.append(target / rel)
    return written


# --- the runner ------------------------------------------------------------


@dataclass(frozen=True)
class Suite:
    """Which test suite `alloy test` is about to run."""

    kind: str  # "project" | "framework"
    source: Path  # dir holding CMakeLists.txt
    build: Path
    name: str


def project_suite(project_root: Path) -> Path | None:
    """The project's own tests/, if it has one. A project is anything with an
    alloy.toml; without one we are standing in the framework and there is no
    project suite to run."""
    root = project_root.resolve()
    if not (root / "alloy.toml").exists():
        return None
    tests = root / "tests"
    return tests if (tests / "CMakeLists.txt").exists() else None


def _gcov_tool(build_dir: Path) -> list[str] | None:
    """gcovr needs to be told how to read the profile when the compiler is
    Clang: its .gcda are llvm's, and plain `gcov` cannot parse them. Read the
    compiler CMake actually used rather than guessing from the platform."""
    cache = build_dir / "CMakeCache.txt"
    compiler = ""
    if cache.exists():
        for line in cache.read_text(errors="ignore").splitlines():
            if line.startswith("CMAKE_CXX_COMPILER:"):
                compiler = line.split("=", 1)[-1]
                break
    if "clang" in compiler.lower() or (not compiler and sys.platform == "darwin"):
        llvm_cov = shutil.which("llvm-cov")
        if llvm_cov is None:
            return None
        return [f"{llvm_cov} gcov"]
    return []


def coverage_report(build_dir: Path, source_dir: Path) -> int:
    """Summarise line coverage over `source_dir` from the profile the run just
    left in `build_dir`. gcovr is the only supported reporter — it is one pip
    install, reads both GCC and Clang profiles, and shipping a hand-rolled gcov
    parser to avoid a dependency would be a worse trade."""
    gcovr = shutil.which("gcovr")
    if gcovr is None:
        print(
            "error: --coverage needs gcovr on PATH (pip install gcovr, or "
            "pipx install gcovr). The build WAS instrumented; the profile is in "
            f"{build_dir}.",
            file=sys.stderr,
        )
        return 1
    gcov_flag = _gcov_tool(build_dir)
    if gcov_flag is None:
        print(
            "error: --coverage under Clang needs llvm-cov on PATH (gcovr cannot "
            "read llvm .gcda with plain gcov).",
            file=sys.stderr,
        )
        return 1
    # No --txt: on gcovr 8 it takes an OPTIONAL filename and swallows the
    # following positional (the search path), which then fails as "Is a
    # directory". Plain text to stdout is the default anyway.
    # --root at the project's src/ keeps the report to the application's own
    # files — the framework's runner and host_support.cpp are not the user's
    # coverage.
    cmd = [gcovr, "--root", str(source_dir), "--exclude-unreachable-branches",
           "--print-summary"]
    for flag in gcov_flag:
        cmd += ["--gcov-executable", flag]
    cmd += [str(build_dir)]
    return subprocess.run(cmd, check=False).returncode
