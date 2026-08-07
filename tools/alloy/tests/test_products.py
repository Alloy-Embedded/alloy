"""Unit tests for the product system's loader, overlay semantics and emitters.

The resolution contract under test (products.py docstring):
    1. family defaults
    2. product overlay — an explicit product value always wins
    3. [[rules]] in file order, filling what the product did not set
    4. [constraints] checked against the final values

Emission goldens live here too: product.hpp (constexpr + strategy aliases),
product_nvm.hpp (keys + defaults, NEVER constexpr) and the plain-C twin.
"""

from __future__ import annotations

import copy
import tomllib
from pathlib import Path
from typing import Any

import pytest

from alloy_cli.emit.common import EmitError
from alloy_cli.emit.product import (
    emit_product_c_header,
    emit_product_header,
    emit_product_nvm_header,
)
from alloy_cli.products import known_products, resolve

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"
PRODUCTS_DIR = ALLOY_ROOT / "examples" / "product_line" / "products"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _family() -> dict[str, Any]:
    return tomllib.loads((PRODUCTS_DIR / "family.toml").read_text())


def _product(name: str) -> dict[str, Any]:
    return tomllib.loads((PRODUCTS_DIR / f"{name}.toml").read_text())


KNOWN = known_products(PRODUCTS_DIR)


# ------------------------------------------------------------ overlay + rules

def test_family_defaults_flow_through_when_the_product_is_silent() -> None:
    model = resolve(_family(), _product("chiller_s"), KNOWN)
    assert model["params"]["pwm"]["value"] == 20000
    assert model["params"]["compressor"]["value"] == "small"
    assert model["caps"] == {"has_display": False, "has_encoder": False}
    assert model["strategies"] == {"control": "six_step"}


def test_a_rule_fills_what_the_product_did_not_say() -> None:
    """chiller_l states compressor = large and NOTHING about pwm/current —
    the family's rules derive both. That is the point: stated once, in data."""
    model = resolve(_family(), _product("chiller_l"), KNOWN)
    assert model["params"]["pwm"]["value"] == 16000
    assert model["params"]["max_phase_current_ma"]["value"] == 12000


def test_an_explicit_product_value_wins_over_a_rule() -> None:
    """The product file is the single reviewable truth of what it chose."""
    product = _product("chiller_l")
    product["params"]["pwm"] = 18000
    model = resolve(_family(), product, KNOWN)
    assert model["params"]["pwm"]["value"] == 18000
    # the OTHER rule still applies — yielding is per-field, not per-product
    assert model["params"]["max_phase_current_ma"]["value"] == 12000


def test_rules_chain_in_file_order() -> None:
    family = _family()
    family["rules"].append(
        {"when": {"max_phase_current_ma": 12000}, "set": "has_display",
         "value": True})
    product = _product("chiller_l")
    del product["caps"]["has_display"]
    model = resolve(family, product, KNOWN)
    # rule 2 set the current; rule 3 saw it and set the cap
    assert model["caps"]["has_display"] is True


def test_a_constraint_violation_refuses_to_emit_naming_the_constraint() -> None:
    product = _product("chiller_s")
    product["strategies"] = {"control": "foc"}  # no encoder on chiller_s
    with pytest.raises(EmitError, match="foc_needs_encoder"):
        resolve(_family(), product, KNOWN)


def test_deeper_nesting_is_rejected_not_supported() -> None:
    product = _product("chiller_l")
    product["family"] = "chiller_s"  # names another PRODUCT
    with pytest.raises(EmitError, match="one level deep"):
        resolve(_family(), product, KNOWN)
    product = _product("chiller_l")
    product["extends"] = "chiller_s"
    with pytest.raises(EmitError, match="one level deep"):
        resolve(_family(), product, KNOWN)
    family = _family()
    family["family"] = "megafamily"
    with pytest.raises(EmitError, match="derives from nothing"):
        resolve(family, _product("chiller_s"), KNOWN)


def test_mutating_a_loaded_product_does_not_leak_between_tests() -> None:
    a = _product("chiller_s")
    a["params"]["modbus_baud"] = 1
    b = _product("chiller_s")
    assert b["params"]["modbus_baud"] == 9600
    assert copy.deepcopy(b) == b


# -------------------------------------------------------------------- the nvm

def test_nvm_fields_are_defaults_not_constexpr() -> None:
    """The design decision, pinned: a runtime-configured field must never ALSO
    exist as a compile-time constant, or flash and constexpr disagree."""
    model = resolve(_family(), _product("chiller_s"), KNOWN)
    assert "modbus_baud" not in model["params"]
    assert model["nvm"]["modbus_baud"] == {"key": 0x10, "default": 9600}
    hpp = emit_product_header(model)
    assert "modbus_baud" not in hpp
    nvm = emit_product_nvm_header(model)
    assert "inline constexpr std::uint32_t k_modbus_baud = 0x00000010u;" in nvm
    assert "inline constexpr std::uint32_t modbus_baud_default = 9600u;" in nvm


def test_the_erased_slot_marker_is_not_a_key() -> None:
    family = _family()
    family["params"]["modbus_baud"]["key"] = 0xFFFFFFFF
    with pytest.raises(EmitError, match="erased-slot"):
        resolve(family, _product("chiller_s"), KNOWN)


def test_duplicate_nvm_keys_are_refused() -> None:
    family = _family()
    family["params"]["display_contrast"] = {
        "type": "int", "nvm": True, "key": 0x10, "default": 5}
    with pytest.raises(EmitError, match="duplicate nvm key"):
        resolve(family, _product("chiller_s"), KNOWN)


# ------------------------------------------------------------ emission golden

_MINI_FAMILY: dict[str, Any] = {
    "schema": "alloy.family.v1", "name": "mini",
    "params": {
        "count": {"type": "int", "default": 3},
        "gain": {"type": "float", "default": 1.5},
        "fast": {"type": "bool", "default": False},
        "pwm": {"type": "frequency", "default": 1000},
        "mode": {"type": "enum", "values": {"a": 1, "b": 2}, "default": "a"},
        "stored": {"type": "int", "nvm": True, "key": 7, "default": 42},
    },
    "caps": {"lit": True},
    "strategies": {"control": {"options": ["foo", "bar"], "default": "foo"}},
}
_MINI_PRODUCT: dict[str, Any] = {
    "schema": "alloy.product.v1", "name": "p1", "family": "mini",
    "params": {"mode": "b"}, "strategies": {"control": "bar"},
}

_GOLDEN_HPP = """\
// GENERATED by alloy — DO NOT EDIT (fix the data or the emitter and regenerate)
#pragma once

#include <cstdint>

#include "alloy/core/units.hpp"

namespace product {

inline constexpr char name[] = "p1";
inline constexpr char family[] = "mini";

enum class mode : std::uint32_t { a = 1, b = 2 };

namespace params {
inline constexpr std::int32_t count = 3;
inline constexpr float gain = 1.5f;
inline constexpr bool fast = false;
inline constexpr alloy::frequency pwm = alloy::frequency{1000u};
inline constexpr ::product::mode mode = ::product::mode::b;
}  // namespace params

namespace caps {
inline constexpr bool lit = true;
}  // namespace caps

// Strategy choices. The app provides these types: declare (or
// include) them in namespace ::product_strategy BEFORE this
// header — a missing one fails the compile at its alias below.
using control = ::product_strategy::bar;

}  // namespace product
"""


def test_product_header_golden() -> None:
    model = resolve(_MINI_FAMILY, _MINI_PRODUCT)
    assert emit_product_header(model) == _GOLDEN_HPP


def test_c_header_is_the_same_data_as_defines() -> None:
    model = resolve(_MINI_FAMILY, _MINI_PRODUCT)
    text = emit_product_c_header(model)
    assert '#define PRODUCT_NAME "p1"' in text
    assert "#define PRODUCT_MODE_A 1" in text
    assert "#define PRODUCT_MODE 2" in text, "enum param resolves to its wire value"
    assert "#define PRODUCT_CAP_LIT 1" in text
    assert '#define PRODUCT_STRATEGY_CONTROL "bar"' in text
    assert "#define PRODUCT_NVM_STORED_KEY 0x00000007u" in text
    assert "#define PRODUCT_NVM_STORED_DEFAULT 42u" in text
    assert "#if" not in text.replace("#pragma", "")


def test_units_include_only_when_a_frequency_param_exists() -> None:
    family = copy.deepcopy(_MINI_FAMILY)
    del family["params"]["pwm"]
    model = resolve(family, _MINI_PRODUCT)
    assert "units.hpp" not in emit_product_header(model)


# ------------------------------------------------- project/tree-key coverage

@skip_no_devices
def test_build_trees_are_keyed_by_board_and_product() -> None:
    """Product A's product.hpp and objects must never serve product B — and a
    project with NO product keeps today's bare-board key, so existing trees
    stay valid."""
    from alloy_cli.project import load_project

    root = ALLOY_ROOT / "examples" / "product_line"
    p = load_project(root)  # alloy.toml selects chiller_s
    assert p.product_id == "chiller_s"
    assert p.gen_dir.name == "nucleo_g071rb+chiller_s"
    assert p.build_dir.name == "nucleo_g071rb+chiller_s"

    over = load_project(root, product_override="chiller_l")
    assert over.gen_dir.name == "nucleo_g071rb+chiller_l"

    plain = load_project(ALLOY_ROOT / "examples" / "blink")
    assert plain.product_id is None
    assert plain.gen_dir.name == plain.board_id


@skip_no_devices
def test_generate_emits_product_artifacts_only_when_selected(tmp_path) -> None:
    """The orchestration seam end-to-end: same project, with and without a
    product — and the no-product path emits no product files (zero
    regression)."""
    import shutil

    from alloy_cli.project import load_project

    root = tmp_path / "proj"
    shutil.copytree(ALLOY_ROOT / "examples" / "product_line", root)
    (root / "alloy.toml").write_text(
        (root / "alloy.toml").read_text() + f'\n[alloy]\nroot = "{ALLOY_ROOT}"\n')
    from alloy_cli.emit import generate

    written = generate(load_project(root))
    names = {p.name for p in written}
    assert {"product.hpp", "product_nvm.hpp", "product.h"} <= names
    hpp = (root / ".alloy" / "generated" / "nucleo_g071rb+chiller_s" /
           "alloy" / "product.hpp").read_text()
    assert 'inline constexpr char name[] = "chiller_s";' in hpp

    # strip the product -> no product artifacts, bare-board tree
    toml = (root / "alloy.toml").read_text()
    (root / "alloy.toml").write_text(
        toml.replace('[product]\nname = "chiller_s"\n', ""))
    written = generate(load_project(root))
    assert not any(p.name.startswith("product") for p in written)


@skip_no_devices
def test_a_bad_product_fails_generation_never_the_compile(tmp_path) -> None:
    """Validate-before-emit: generate() refuses with the validator's own list,
    so no partial product.hpp ever reaches the compiler."""
    import shutil

    from alloy_cli.emit import generate
    from alloy_cli.project import load_project

    root = tmp_path / "proj"
    shutil.copytree(ALLOY_ROOT / "examples" / "product_line", root)
    (root / "alloy.toml").write_text(
        (root / "alloy.toml").read_text() + f'\n[alloy]\nroot = "{ALLOY_ROOT}"\n')
    bad = root / "products" / "chiller_s.toml"
    bad.write_text(bad.read_text() + '\n[strategies]\ncontrol = "foc"\n')

    with pytest.raises(EmitError, match="foc_needs_encoder"):
        generate(load_project(root))
    gen = root / ".alloy" / "generated" / "nucleo_g071rb+chiller_s"
    assert not (gen / "alloy" / "product.hpp").exists()


def test_matrix_table_grows_a_product_column() -> None:
    from alloy_cli.matrix import format_table

    report = {
        "schema": "alloy.matrix.v1",
        "boards": [
            {"board": "b1", "product": "p1", "ok": False, "error": "boom",
             "flash": None, "ram": None, "seconds": 0.1, "chip": None},
            {"board": "b1", "product": "p2", "ok": False, "error": "boom",
             "flash": None, "ram": None, "seconds": 0.1, "chip": None},
        ],
        "built": 0, "failed": 2, "ok": False,
    }
    table = format_table(report)
    assert "b1+p1" in table and "b1+p2" in table
    assert "configurations" in table
