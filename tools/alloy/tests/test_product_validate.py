"""Unit tests for `alloy product-validate`.

`product_validate` is a SECOND expression of the product emitter's contract,
written to report every problem at once with a location and a fix. Two
descriptions of one contract can drift, so most of this file is the property
that keeps them tied — the same pair test_board_validate pins for boards:

    P1  validate finds no errors  =>  resolve + emission succeed
    P2  resolve/emission fails    =>  validate found an error

The converse of P2 is deliberately NOT required: validate may be STRICTER
than emission (e.g. a param name that is not a C++ identifier emits fine as
text and then fails the compile — validation names it at config time).
"""

from __future__ import annotations

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
from alloy_cli.product_validate import validate_product, validate_product_file
from alloy_cli.products import known_products, resolve

ALLOY_ROOT = Path(__file__).resolve().parents[3]
PRODUCTS_DIR = ALLOY_ROOT / "examples" / "product_line" / "products"
PRODUCT_IDS = known_products(PRODUCTS_DIR)


def _family() -> dict[str, Any]:
    return tomllib.loads((PRODUCTS_DIR / "family.toml").read_text())


def _product(name: str) -> dict[str, Any]:
    return tomllib.loads((PRODUCTS_DIR / f"{name}.toml").read_text())


def _emits(family: dict[str, Any], product: dict[str, Any]) -> bool:
    """Would `alloy gen` emit this product? (the resolve + emitter path)"""
    try:
        model = resolve(family, product, PRODUCT_IDS)
        emit_product_header(model)
        emit_product_nvm_header(model)
        emit_product_c_header(model)
    except (EmitError, KeyError, TypeError, ValueError):
        return False
    return True


def _errors(family: dict[str, Any], product: dict[str, Any]) -> list[dict[str, Any]]:
    return [i for i in validate_product(family, product, PRODUCT_IDS)
            if i["level"] == "error"]


# ------------------------------------------------------ the shipped products

@pytest.mark.parametrize("product_id", PRODUCT_IDS)
def test_every_shipped_product_is_clean(product_id: str) -> None:
    """The example's products build in CI, so validation calling them broken
    would make the verb useless noise on day one."""
    assert validate_product(_family(), _product(product_id), PRODUCT_IDS) == []


# ---------------------------------------------------------------- the rules

def test_every_problem_is_reported_not_just_the_first() -> None:
    """The reason this is a separate rule set: a form needs all of them."""
    family = _family()
    product = _product("chiller_s")
    product["params"]["bus_voltage_mv"] = "high"     # wrong type
    product["params"]["nope"] = 1                    # undeclared
    product["strategies"] = {"control": "sensorless"}  # not an option
    errors = _errors(family, product)
    fields = {e["field"] for e in errors}
    assert {"params.bus_voltage_mv", "params.nope",
            "strategies.control"} <= fields


def test_an_unknown_param_offers_the_declared_ones() -> None:
    product = _product("chiller_s")
    product["params"]["pwm_khz"] = 16
    issue = next(i for i in _errors(_family(), product)
                 if i["field"] == "params.pwm_khz")
    assert "pwm" in issue["suggestions"]


def test_a_bad_strategy_choice_offers_the_options() -> None:
    product = _product("chiller_s")
    product["strategies"] = {"control": "sensorless"}
    issue = next(i for i in _errors(_family(), product)
                 if i["field"] == "strategies.control")
    assert issue["suggestions"] == ["six_step", "foc"]


def test_constraint_violations_carry_the_constraint_name() -> None:
    product = _product("chiller_s")
    product["strategies"] = {"control": "foc"}       # and no encoder
    issue = next(i for i in _errors(_family(), product)
                 if "constraint" in i["message"])
    assert "foc_needs_encoder" in i_msg(issue)
    assert issue["field"] == "constraints.foc_needs_encoder"


def i_msg(issue: dict[str, Any]) -> str:
    return issue["message"]


def test_an_enum_without_explicit_values_is_refused() -> None:
    """The audit's design decision, enforced: wire values are data, never
    auto-assigned."""
    family = _family()
    del family["params"]["compressor"]["values"]
    issues = _errors(family, _product("chiller_s"))
    assert any("never auto-assigned" in i["message"] for i in issues)


def test_a_rule_referencing_an_undeclared_field_is_an_error() -> None:
    family = _family()
    family["rules"].append({"when": {"phase_count": 3}, "set": "pwm",
                            "value": 1000})
    issues = _errors(family, _product("chiller_s"))
    assert any("phase_count" in i["message"] for i in issues)


def test_overlaying_a_field_the_family_never_declared_is_an_error() -> None:
    product = _product("chiller_s")
    product["caps"] = {"has_wings": True}
    issue = next(i for i in _errors(_family(), product)
                 if i["field"] == "caps.has_wings")
    assert "only set what the family declares" in issue["message"]


def test_nesting_is_positively_rejected_in_both_directions() -> None:
    # a product naming another product as its family
    product = _product("chiller_l")
    product["family"] = "chiller_s"
    assert any("one level deep" in i["message"]
               for i in _errors(_family(), product))
    # extends is not a thing
    product = _product("chiller_l")
    product["extends"] = "chiller_s"
    assert any("one level deep" in i["message"]
               for i in _errors(_family(), product))
    # a family deriving from anything
    family = _family()
    family["family"] = "megafamily"
    assert any("derives from nothing" in i["message"]
               for i in _errors(family, _product("chiller_s")))


def test_errors_sort_before_warnings() -> None:
    family = _family()
    product = _product("chiller_s")
    product["params"]["nope"] = 1
    issues = validate_product(family, product, PRODUCT_IDS)
    levels = [i["level"] for i in issues]
    assert levels == sorted(levels, key=lambda l: {"error": 0}.get(l, 1))


def test_selecting_a_product_that_does_not_exist_names_the_ones_that_do() -> None:
    report = validate_product_file(ALLOY_ROOT / "examples" / "product_line",
                                   name="chiller_xxl")
    assert report["schema"] == "alloy.product_validate.v1"
    assert not report["ok"]
    assert report["issues"][0]["suggestions"] == PRODUCT_IDS


def test_a_file_whose_name_disagrees_with_its_content_is_reported(tmp_path) -> None:
    import shutil

    root = tmp_path / "proj"
    shutil.copytree(ALLOY_ROOT / "examples" / "product_line", root)
    renamed = root / "products" / "chiller_m.toml"
    (root / "products" / "chiller_s.toml").rename(renamed)
    report = validate_product_file(root, name="chiller_m")
    assert not report["ok"]
    assert any("one product, one file, one name" in i["message"]
               for i in report["issues"])


# ------------------------------------------------- the anti-drift property

_MUTATIONS: list[tuple[str, Any]] = [
    ("unknown overlay param",
     lambda f, p: p["params"].__setitem__("nope", 1)),
    ("wrong value type",
     lambda f, p: p["params"].__setitem__("bus_voltage_mv", "high")),
    ("value below min",
     lambda f, p: p["params"].__setitem__("bus_voltage_mv", -5)),
    ("enum non-member",
     lambda f, p: p["params"].__setitem__("compressor", "huge")),
    ("strategy not an option",
     lambda f, p: p.__setitem__("strategies", {"control": "sensorless"})),
    ("cap that is not a bool",
     lambda f, p: p.__setitem__("caps", {"has_display": 1})),
    ("undeclared cap",
     lambda f, p: p.__setitem__("caps", {"has_wings": True})),
    ("product extends product",
     lambda f, p: p.__setitem__("extends", "chiller_s")),
    ("product family names a product",
     lambda f, p: p.__setitem__("family", "chiller_s")),
    ("product family names nothing known",
     lambda f, p: p.__setitem__("family", "washing_machines")),
    ("missing product name",
     lambda f, p: p.pop("name")),
    ("wrong product schema",
     lambda f, p: p.__setitem__("schema", "alloy.product.v2")),
    ("family with extends",
     lambda f, p: f.__setitem__("extends", "base")),
    ("family with unknown key",
     lambda f, p: f.__setitem__("flavour", "spicy")),
    ("unknown param type",
     lambda f, p: f["params"].__setitem__("shape", {"type": "str",
                                                    "default": "round"})),
    ("enum without explicit values",
     lambda f, p: f["params"]["compressor"].pop("values")),
    ("enum with duplicate wire values",
     lambda f, p: f["params"]["compressor"].__setitem__(
         "values", {"none": 0, "small": 1, "large": 1})),
    ("enum member without an integer value",
     lambda f, p: f["params"]["compressor"].__setitem__(
         "values", {"none": 0, "small": "one", "large": 2})),
    ("param without a default",
     lambda f, p: f["params"]["bus_voltage_mv"].pop("default")),
    ("default out of its own range",
     lambda f, p: f["params"]["bus_voltage_mv"].__setitem__("default", -1)),
    ("strategy default not an option",
     lambda f, p: f["strategies"]["control"].__setitem__("default", "psychic")),
    ("rule referencing an undeclared field",
     lambda f, p: f["rules"].append({"when": {"phase_count": 3},
                                     "set": "pwm", "value": 1})),
    ("rule setting an undeclared field",
     lambda f, p: f["rules"].append({"set": "phase_count", "value": 3})),
    ("rule deriving an nvm field",
     lambda f, p: f["rules"].append({"set": "modbus_baud", "value": 9600})),
    ("rule with an invalid value for its target",
     lambda f, p: f["rules"].append({"set": "pwm", "value": "fast"})),
    ("constraint referencing an undeclared field",
     lambda f, p: f["constraints"].__setitem__(
         "ghost", {"forbid": {"phase_count": 3}})),
    ("constraint with both forbid and when",
     lambda f, p: f["constraints"].__setitem__(
         "confused", {"forbid": {"has_display": True},
                      "when": {"has_display": True},
                      "require": {"has_encoder": True}})),
    ("constraint violated by the product",
     lambda f, p: p.__setitem__("strategies", {"control": "foc"})),
    ("nvm field without a key",
     lambda f, p: f["params"]["modbus_baud"].pop("key")),
    ("nvm key is the erased-slot marker",
     lambda f, p: f["params"]["modbus_baud"].__setitem__("key", 0xFFFFFFFF)),
    ("duplicate nvm keys",
     lambda f, p: f["params"].__setitem__(
         "contrast", {"type": "int", "nvm": True, "key": 0x10, "default": 1})),
    ("nvm on a non-int type",
     lambda f, p: f["params"]["modbus_baud"].__setitem__("type", "frequency")),
    ("name declared twice across params and caps",
     lambda f, p: f["caps"].__setitem__("pwm", False)),
]


@pytest.mark.parametrize("name,mutate", _MUTATIONS, ids=[m[0] for m in _MUTATIONS])
def test_emission_failure_is_always_reported(name: str, mutate) -> None:
    """P2: whatever makes resolve/emission fail must show up as an error here —
    so the UI never says "looks fine" about a product that cannot build."""
    family, product = _family(), _product("chiller_s")
    mutate(family, product)
    if _emits(family, product):
        pytest.skip(f"{name} is accepted by the emitter (validate is stricter here)")
    assert _errors(family, product), \
        f"{name}: emission fails but validation reported nothing"


@pytest.mark.parametrize("name,mutate", _MUTATIONS, ids=[m[0] for m in _MUTATIONS])
def test_every_mutation_is_actually_broken(name: str, mutate) -> None:
    """Negative control for the suite itself: each mutation must trip AT LEAST
    one side. A mutation both sides accept is testing nothing."""
    family, product = _family(), _product("chiller_s")
    mutate(family, product)
    assert not _emits(family, product) or _errors(family, product), \
        f"{name}: neither the emitter nor the validator objects — dead mutation"


@pytest.mark.parametrize("product_id", PRODUCT_IDS)
def test_clean_products_emit(product_id: str) -> None:
    """P1: if validation is silent, emission must work — otherwise 'ok' lies."""
    family, product = _family(), _product(product_id)
    assert not _errors(family, product)
    assert _emits(family, product)


def test_a_clean_mutation_still_emits() -> None:
    """P1 with something actually changed: choosing another enum member and
    flipping a cap must stay clean AND still emit."""
    family, product = _family(), _product("chiller_s")
    product["params"]["compressor"] = "none"
    product["caps"] = {"has_display": True}
    assert not _errors(family, product)
    assert _emits(family, product)
