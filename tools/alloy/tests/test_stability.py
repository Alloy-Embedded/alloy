"""The public surface, recorded so it cannot be removed by accident.

docs/reference/stability.md tells companies which parts of alloy they may
build a product on and how much notice they get before those parts move. A
promise nothing enforces is marketing, so the surface is recorded HERE and
this file fails when a piece of it disappears.

Deliberately one-sided: every recorded name must still exist, but adding new
verbs, schemas or headers needs no edit. Removing or renaming one is supposed
to be a deliberate act — it fails here, and the fix is to follow the
deprecation window in the docs page (or, if the removal really is intended,
to delete the entry in the same commit that announces it in CHANGELOG.md).
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parents[3]
CLI = REPO / "tools" / "alloy" / "alloy_cli"
SRC = REPO / "src" / "alloy"

# --- Tier 1: the CLI verbs a build system may call. -------------------------
PUBLIC_VERBS = {
    "new", "boards", "board-info", "board-clone", "ci-init", "matrix", "svd",
    "board-validate", "product-validate", "size", "crash", "chips", "devices",
    "chip-info", "chip-status", "clock", "clean", "set-board", "setup",
    "debug-info",
    "emulate", "test", "frame-audit", "sbom", "lib", "image", "keygen",
    "secure", "provision", "ports", "update", "gen", "build", "flash",
    "monitor", "run",
}

# --- Tier 1: the JSON envelopes an IDE or CI job may parse. -----------------
# The ".vN" suffix IS the compatibility contract: a breaking change to any of
# these bumps the number and both are emitted for the deprecation window.
PUBLIC_SCHEMAS = {
    "alloy.board.v1", "alloy.board_info.v1", "alloy.board_validate.v1",
    "alloy.boards.v1", "alloy.build.v1", "alloy.chip_info.v1",
    "alloy.chip_status.v1", "alloy.chips.v1", "alloy.clock.v1",
    "alloy.clock_graph.v1",
    "alloy.crash.v1", "alloy.debug_info.v1", "alloy.devices.v1",
    "alloy.family.v1", "alloy.libs.v1", "alloy.matrix.v1", "alloy.ports.v1",
    "alloy.product.v1", "alloy.product_validate.v1", "alloy.registry.v1",
    "alloy.sbom.v1", "alloy.setup.v1", "alloy.size.v1",
}

# --- Tier 1: the headers application code includes. -------------------------
# Every one of these is included by at least one example in this repo; that is
# how the list was built, and the matrix build is what keeps them compiling.
PUBLIC_HEADERS = {
    "time.hpp", "delay.hpp", "gpio.hpp", "uart.hpp", "i2c.hpp",
    "spi.hpp", "adc.hpp", "dac.hpp", "pwm.hpp", "can.hpp", "rtc.hpp",
    "dma.hpp", "irq.hpp", "wdt.hpp", "flash.hpp", "log.hpp", "sched.hpp",
    "fault.hpp", "secure.hpp", "ota.hpp", "provision.hpp", "concepts.hpp",
    "fastcode.hpp",
    "async/task.hpp", "async/executor.hpp", "async/delay.hpp",
    "async/i2c.hpp", "async/spi.hpp", "async/dma.hpp",
    "net/socket.hpp", "net/http.hpp", "net/lwip.hpp",
    "ota/signed.hpp", "ota/uart_transport.hpp",
    "drivers/at24.hpp", "drivers/spi_flash.hpp",
    "dsp/biquad.hpp", "util/moving_average.hpp",
}

# --- Tier 1: the alloy.toml tables a project may write. ---------------------
PUBLIC_TOML_TABLES = {"project", "board", "alloy", "devices", "product",
                      "libs", "ota", "roles", "clock"}

# GENERATED headers: the ones an app includes that have no file in src/alloy
# at all — codegen writes them per board into .alloy/generated. Their NAMES are
# public (an app writes `#include <alloy/board.hpp>`); their CONTENT is the
# chip database's promise, which is what [devices] pins.
GENERATED_HEADERS = {"board.hpp", "device.hpp", "product.hpp",
                     "product_nvm.hpp", "slots.hpp", "ota_key.hpp"}

# Headers that are NOT public even though they are reachable: the backend a
# portable app is not supposed to name, the peripheral drivers the generated
# board.hpp instantiates, and vendored third-party trees.
INTERNAL_PREFIXES = ("arch/", "hal/", "vendor/")


def _verbs() -> set[str]:
    out = subprocess.run([sys.executable, "-m", "alloy_cli.cli", "--help"],
                         cwd=REPO / "tools" / "alloy",
                         capture_output=True, text=True, check=True).stdout
    start = out.index("{")
    return set(out[start + 1:out.index("}", start)].split(","))


def test_every_public_verb_still_exists() -> None:
    missing = PUBLIC_VERBS - _verbs()
    assert not missing, (
        f"public CLI verb(s) removed without a deprecation window: {sorted(missing)}"
    )


def test_new_verbs_do_not_need_a_test_edit() -> None:
    """The guard is one-sided on purpose — this documents that."""
    assert PUBLIC_VERBS <= _verbs()


def test_every_public_schema_id_is_still_emitted() -> None:
    text = "\n".join(p.read_text() for p in CLI.rglob("*.py"))
    missing = {s for s in PUBLIC_SCHEMAS if f'"{s}"' not in text}
    assert not missing, f"JSON schema id(s) no longer emitted: {sorted(missing)}"


def test_no_public_schema_lost_its_version_suffix() -> None:
    for schema in PUBLIC_SCHEMAS:
        assert schema.rsplit(".", 1)[1].startswith("v"), schema


def test_every_public_header_still_exists() -> None:
    missing = {h for h in PUBLIC_HEADERS if not (SRC / h).exists()}
    assert not missing, (
        f"public header(s) removed or moved: {sorted(missing)} — "
        "see docs/reference/stability.md before deleting one"
    )


def test_generated_headers_have_no_file_in_the_source_tree() -> None:
    """They are a separate tier because codegen owns them, not the repo."""
    wrong = {h for h in GENERATED_HEADERS if (SRC / h).exists()}
    assert not wrong, (
        f"{sorted(wrong)} exist in src/alloy — either they are no longer "
        "generated, or a stale copy shadows the generated one"
    )


def test_public_headers_are_not_internal_ones() -> None:
    """The public list must never quietly grow into the backend."""
    leaked = {h for h in PUBLIC_HEADERS if h.startswith(INTERNAL_PREFIXES)}
    assert not leaked, f"internal headers listed as public: {sorted(leaked)}"


def test_the_backend_namespace_stays_in_the_backend() -> None:
    """`alloy::arch::` is internal; portable code must not have to name it."""
    offenders = [
        p.relative_to(SRC).as_posix()
        for p in SRC.rglob("*.hpp")
        if not p.relative_to(SRC).as_posix().startswith(INTERNAL_PREFIXES)
        and "namespace alloy::arch" in p.read_text()
    ]
    assert not offenders, f"alloy::arch declared outside arch/: {offenders}"


@pytest.mark.parametrize("table", sorted(PUBLIC_TOML_TABLES))
def test_every_documented_alloy_toml_table_is_read_somewhere(table: str) -> None:
    """A table the docs promise but no code reads is a lie in the schema."""
    text = "\n".join(p.read_text() for p in CLI.rglob("*.py"))
    assert f'"{table}"' in text, f"alloy.toml [{table}] is read by no code"


def test_the_changelog_records_a_compatibility_policy() -> None:
    """The policy has to be findable from the file a user actually opens."""
    changelog = (REPO / "CHANGELOG.md").read_text()
    assert "reference/stability.md" in changelog or "stability" in changelog.lower()
