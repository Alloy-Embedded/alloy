"""Product definitions: one firmware codebase shipping as many PRODUCTS.

The second instance of the board pattern. A board.json states what the
HARDWARE is; a product TOML states what this SKU of the firmware is — the
parameters, capabilities and strategy choices that today get smeared across a
codebase as #ifdef spaghetti. Same pipeline shape: data file -> validated ->
generated constexpr header -> `if constexpr` in app code.

Layout (inside the project):

    products/family.toml     the parameter space every product shares:
                             [params] typed declarations with defaults,
                             [caps] boolean capabilities, [strategies.X]
                             options, [[rules]] declarative derivations,
                             [constraints.NAME] forbidden/required combos
    products/<name>.toml     ONE product = ONE file = one clean git diff;
                             overlays the family, exactly one level deep

Resolution order (the documented contract, shared by the validator and the
emitter through THIS module so they cannot drift):

    1. family defaults
    2. the product's own overlay — an explicit product value always wins
    3. [[rules]] in file order, each filling values the product did NOT set
       explicitly (later rules see what earlier rules derived)
    4. [constraints] checked against the final values; any violation refuses
       emission and is reported by `alloy product-validate` under the
       constraint's own name

Design decisions carried from the audit of three industrial C codebases:
enum/wire values are EXPLICIT in the TOML (never auto-assigned); a field may
be `nvm = true` and is then emitted as an nvm_kv key + default instead of a
constexpr (some products configure it at runtime, so it must never ALSO exist
as a compile-time constant); data holds physical quantities (Hz as plain
integers -> alloy::frequency), unit scaling stays in C++; overlay is one
level deep BY DESIGN — deeper nesting is rejected, not supported.
"""

from __future__ import annotations

import re
import tomllib
from pathlib import Path
from typing import Any

from .emit.common import EmitError

FAMILY_SCHEMA = "alloy.family.v1"
PRODUCT_SCHEMA = "alloy.product.v1"

PARAM_TYPES = ("int", "float", "bool", "enum", "frequency")

# nvm_kv's erased-slot marker (src/alloy/util/nvm_kv.hpp empty_key): a key of
# all-ones reads as "free slot", so no product field may use it.
NVM_FREE_SLOT_KEY = 0xFFFFFFFF
U32_MAX = 0xFFFFFFFF
I32_MIN, I32_MAX = -(2**31), 2**31 - 1

_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")

# Identifiers already taken at `namespace product` scope by the emitted header
# (enum types and strategy aliases land there).
RESERVED_NAMES = frozenset({"name", "family", "params", "caps", "nvm"})

_FAMILY_KEYS = frozenset({"schema", "name", "params", "caps", "strategies",
                          "rules", "constraints"})
_PRODUCT_KEYS = frozenset({"schema", "name", "family", "params", "caps",
                           "strategies"})


def products_dir(project_root: Path) -> Path:
    return project_root / "products"


def known_products(pdir: Path) -> list[str]:
    """Every product this project defines (family.toml is the space, not a
    product)."""
    if not pdir.is_dir():
        return []
    return sorted(p.stem for p in pdir.glob("*.toml") if p.name != "family.toml")


def _load_toml(path: Path) -> dict[str, Any]:
    if not path.exists():
        raise EmitError(f"{path}: no such file")
    try:
        return tomllib.loads(path.read_text())
    except tomllib.TOMLDecodeError as exc:
        raise EmitError(f"{path}: not valid TOML — {exc}") from exc


def load_family(pdir: Path) -> dict[str, Any]:
    path = pdir / "family.toml"
    if not path.exists():
        raise EmitError(
            f"{pdir}: no family.toml — a products/ dir starts with the family "
            "(shared params/caps/strategies), then one <name>.toml per product")
    return _load_toml(path)


def load_product(pdir: Path, name: str) -> dict[str, Any]:
    path = pdir / f"{name}.toml"
    if not path.exists():
        known = known_products(pdir)
        raise EmitError(f"unknown product '{name}' — known products: "
                        f"{', '.join(known) or '(none)'}")
    return _load_toml(path)


# --------------------------------------------------------------- declarations

def declared(family: dict[str, Any]) -> dict[str, tuple[str, Any]]:
    """name -> (kind, declaration) over params, caps and strategies.

    ONE namespace on purpose: rules and constraints reference plain names, so
    a param and a cap sharing a name would make `when = {x = ...}` ambiguous.
    Raises EmitError on structural problems the resolver cannot survive; the
    validator reports the same conditions as located issues (the anti-drift
    tests pin the two together).
    """
    out: dict[str, tuple[str, Any]] = {}

    def _claim(name: str, kind: str, decl: Any) -> None:
        if name in out:
            raise EmitError(f"family: '{name}' declared twice "
                            f"({out[name][0]} and {kind}) — params, caps and "
                            "strategies share one namespace")
        out[name] = (kind, decl)

    params = family.get("params") or {}
    if not isinstance(params, dict):
        raise EmitError("family: [params] must be a table of declarations")
    for name, decl in params.items():
        if not isinstance(decl, dict):
            raise EmitError(f"family: params.{name} must be a table "
                            "(type = ..., default = ...)")
        ptype = decl.get("type")
        if ptype not in PARAM_TYPES:
            raise EmitError(f"family: params.{name}: unknown type '{ptype}' "
                            f"(one of: {', '.join(PARAM_TYPES)})")
        if ptype == "enum" and not isinstance(decl.get("values"), dict):
            # The design decision, enforced: wire values are explicit data.
            raise EmitError(f"family: params.{name}: an enum must carry "
                            "explicit wire values — values = { name = int, … } "
                            "(never auto-assigned)")
        if "default" not in decl:
            raise EmitError(f"family: params.{name}: missing 'default'")
        _claim(name, "param", decl)

    caps = family.get("caps") or {}
    if not isinstance(caps, dict):
        raise EmitError("family: [caps] must be a table of booleans")
    for name, default in caps.items():
        _claim(name, "cap", default)

    strategies = family.get("strategies") or {}
    if not isinstance(strategies, dict):
        raise EmitError("family: [strategies] must be a table")
    for name, decl in strategies.items():
        if not isinstance(decl, dict) or not isinstance(decl.get("options"), list):
            raise EmitError(f"family: strategies.{name}: needs "
                            "options = [\"a\", \"b\", …]")
        if "default" not in decl:
            raise EmitError(f"family: strategies.{name}: missing 'default'")
        _claim(name, "strategy", decl)
    return out


def is_nvm(kind: str, decl: Any) -> bool:
    return kind == "param" and isinstance(decl, dict) and bool(decl.get("nvm"))


# ------------------------------------------------------------- value checking

def check_value(kind: str, decl: Any, value: Any) -> str | None:
    """Why `value` is not a valid setting for this declaration, or None.

    The ONE type/range oracle: family defaults, product overlays, rule values
    and constraint literals all go through it, so "valid" can never mean
    different things in different places.
    """
    if kind == "cap":
        return None if isinstance(value, bool) else "a capability is a boolean"
    if kind == "strategy":
        options = decl.get("options") or []
        if value not in options:
            return f"not an option — one of: {', '.join(map(str, options))}"
        return None

    ptype = decl.get("type")
    if ptype == "bool":
        return None if isinstance(value, bool) else "expected a boolean"
    if ptype == "int":
        if not isinstance(value, int) or isinstance(value, bool):
            return "expected an integer"
        if is_nvm("param", decl):
            if not 0 <= value <= U32_MAX:
                return "an nvm field is stored as a u32 — value must be in [0, 2^32)"
        elif not I32_MIN <= value <= I32_MAX:
            return "out of int32 range (the emitted constexpr is std::int32_t)"
    elif ptype == "float":
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            return "expected a number"
    elif ptype == "frequency":
        if not isinstance(value, int) or isinstance(value, bool):
            return "a frequency is a plain integer in Hz (scaling stays in C++)"
        if not 0 <= value <= U32_MAX:
            return "out of range for alloy::frequency (u32 Hz)"
    elif ptype == "enum":
        members = decl.get("values") or {}
        if value not in members:
            return f"not a member — one of: {', '.join(members)}"
        return None
    else:
        return f"unknown type '{ptype}'"

    lo, hi = decl.get("min"), decl.get("max")
    if lo is not None and value < lo:
        return f"below min ({lo})"
    if hi is not None and value > hi:
        return f"above max ({hi})"
    return None


# ------------------------------------------------------ one-level overlay law

def structure_errors(family: dict[str, Any], product: dict[str, Any],
                     known: list[str] | tuple[str, ...] = ()) -> list[str]:
    """The one-level-overlay contract, checked positively.

    A product overlays THE family — never another product, and a family never
    overlays anything. Deeper nesting is rejected by design (an acceptance
    criterion, not a missing feature): three real industrial codebases showed
    that overlay chains are where per-product configuration stops being
    reviewable.
    """
    errs: list[str] = []
    if family.get("schema") != FAMILY_SCHEMA:
        errs.append(f"family: schema must be \"{FAMILY_SCHEMA}\" "
                    f"(got {family.get('schema')!r})")
    if not isinstance(family.get("name"), str):
        errs.append("family: missing name = \"<family name>\"")
    for key in ("family", "extends"):
        if key in family:
            errs.append(f"family: '{key}' is not allowed — a family is the top "
                        "of the (single) overlay level, it derives from nothing")
    if unknown := sorted(set(family) - _FAMILY_KEYS):
        errs.append(f"family: unknown key(s): {', '.join(unknown)}")

    if product.get("schema") != PRODUCT_SCHEMA:
        errs.append(f"product: schema must be \"{PRODUCT_SCHEMA}\" "
                    f"(got {product.get('schema')!r})")
    if "extends" in product:
        errs.append("product: 'extends' is not a thing — a product overlays the "
                    "family, exactly one level deep; put shared values in "
                    "family.toml")
    if unknown := sorted(set(product) - _PRODUCT_KEYS):
        errs.append(f"product: unknown key(s): {', '.join(unknown)}")
    if not isinstance(product.get("name"), str):
        errs.append("product: missing name = \"<product>\"")

    fam_ref = product.get("family")
    if not isinstance(fam_ref, str):
        errs.append("product: missing family = \"<family name>\"")
    elif fam_ref != family.get("name"):
        if fam_ref in known:
            errs.append(f"product: family = \"{fam_ref}\" names another PRODUCT "
                        "— overlay chains are rejected by design; a product "
                        "overlays the family, one level deep")
        else:
            errs.append(f"product: family = \"{fam_ref}\" but this family is "
                        f"\"{family.get('name')}\"")
    return errs


# ------------------------------------------------------- rules & constraints

def _fmt_cond(cond: dict[str, Any]) -> str:
    return ", ".join(f"{k} = {v!r}" for k, v in cond.items())


def _matches(cond: dict[str, Any], values: dict[str, Any]) -> bool:
    return all(values.get(k) == v for k, v in cond.items())


def _check_refs(cond: Any, decls: dict[str, tuple[str, Any]],
                where: str) -> None:
    if not isinstance(cond, dict):
        raise EmitError(f"family: {where}: expected a table of name = value")
    for name, value in cond.items():
        if name not in decls:
            raise EmitError(f"family: {where}: '{name}' is not a declared "
                            "param/cap/strategy")
        kind, decl = decls[name]
        if is_nvm(kind, decl):
            raise EmitError(f"family: {where}: '{name}' is nvm-configured at "
                            "RUNTIME — compile-time rules/constraints cannot "
                            "see its value")
        if (why := check_value(kind, decl, value)) is not None:
            raise EmitError(f"family: {where}: {name}: {why}")


def evaluate_rules(family: dict[str, Any], decls: dict[str, tuple[str, Any]],
                   values: dict[str, Any], explicit: set[str]) -> None:
    """Apply [[rules]] in file order, mutating `values`.

    A rule fills a value the product left to the family; an EXPLICIT product
    value wins over any rule (the product file stays the single reviewable
    truth of what that product chose). Later rules see earlier derivations.
    """
    rules = family.get("rules") or []
    if not isinstance(rules, list):
        raise EmitError("family: [[rules]] must be an array of tables")
    for i, rule in enumerate(rules):
        where = f"rules[{i}]"
        if not isinstance(rule, dict) or "set" not in rule or "value" not in rule:
            raise EmitError(f"family: {where}: a rule is "
                            "{{ when = {{…}}, set = \"name\", value = … }}")
        _check_refs(rule.get("when") or {}, decls, f"{where}.when")
        target = rule["set"]
        if target not in decls:
            raise EmitError(f"family: {where}: set = '{target}' is not a "
                            "declared param/cap/strategy")
        kind, decl = decls[target]
        if is_nvm(kind, decl):
            raise EmitError(f"family: {where}: set = '{target}' is an nvm "
                            "field — runtime-configured, a rule cannot derive it")
        if (why := check_value(kind, decl, rule["value"])) is not None:
            raise EmitError(f"family: {where}: value for '{target}': {why}")
        if target in explicit:
            continue  # the product said so explicitly; the rule yields
        if _matches(rule.get("when") or {}, values):
            values[target] = rule["value"]


def constraint_violations(family: dict[str, Any],
                          decls: dict[str, tuple[str, Any]],
                          values: dict[str, Any]) -> list[dict[str, str]]:
    """[constraints] against final values: [{name, message}], all of them.

    Two forms, both named (the name is the review handle AND the error):
      [constraints.X] forbid = {a = 1, b = true}      all matching = violation
      [constraints.X] when = {…} require = {…}        when matches, require must
    """
    out: list[dict[str, str]] = []
    constraints = family.get("constraints") or {}
    if not isinstance(constraints, dict):
        raise EmitError("family: [constraints] must be a table of named entries")
    for name, spec in constraints.items():
        if not isinstance(spec, dict):
            raise EmitError(f"family: constraints.{name}: expected a table")
        has_forbid, has_when = "forbid" in spec, "when" in spec
        if has_forbid == has_when or (has_when and "require" not in spec):
            raise EmitError(
                f"family: constraints.{name}: use forbid = {{…}} OR "
                "when = {…} + require = {…}")
        if unknown := sorted(set(spec) - {"forbid", "when", "require"}):
            raise EmitError(f"family: constraints.{name}: unknown key(s): "
                            f"{', '.join(unknown)}")
        if has_forbid:
            _check_refs(spec["forbid"], decls, f"constraints.{name}.forbid")
            if _matches(spec["forbid"], values):
                out.append({"name": name, "message":
                            f"constraint '{name}': the combination "
                            f"{_fmt_cond(spec['forbid'])} is forbidden"})
        else:
            _check_refs(spec["when"], decls, f"constraints.{name}.when")
            _check_refs(spec["require"], decls, f"constraints.{name}.require")
            if _matches(spec["when"], values) and not _matches(spec["require"], values):
                have = {k: values.get(k) for k in spec["require"]}
                out.append({"name": name, "message":
                            f"constraint '{name}': with {_fmt_cond(spec['when'])}, "
                            f"this product must have {_fmt_cond(spec['require'])} "
                            f"(it has {_fmt_cond(have)})"})
    return out


# ------------------------------------------------------------------ resolve

def resolve(family: dict[str, Any], product: dict[str, Any],
            known: list[str] | tuple[str, ...] = ()) -> dict[str, Any]:
    """family + product -> the resolved model the emitters consume.

    Raises EmitError on anything malformed or any constraint violation — bad
    data fails generation, never the compile. `alloy product-validate` reports
    the same contract as located issues, every problem at once; the
    test_product_validate P1/P2 properties pin the two expressions together.
    """
    if errs := structure_errors(family, product, known):
        raise EmitError("product definition rejected:\n"
                        + "\n".join(f"  {e}" for e in errs))
    decls = declared(family)

    values: dict[str, Any] = {}      # compile-time values (rules see these)
    nvm_values: dict[str, Any] = {}  # runtime-configured defaults, kept apart
    for name, (kind, decl) in decls.items():
        default = decl if kind == "cap" else decl["default"]
        if (why := check_value(kind, decl, default)) is not None:
            raise EmitError(f"family: default for '{name}': {why}")
        (nvm_values if is_nvm(kind, decl) else values)[name] = default

    explicit: set[str] = set()
    for section, want_kind in (("params", "param"), ("caps", "cap"),
                               ("strategies", "strategy")):
        table = product.get(section) or {}
        if not isinstance(table, dict):
            raise EmitError(f"product {product.get('name')}: [{section}] must "
                            "be a table")
        for name, value in table.items():
            kind, decl = decls.get(name, (None, None))
            if kind != want_kind:
                raise EmitError(
                    f"product {product.get('name')}: [{section}] {name}: the "
                    f"family declares no such {want_kind} — a product may only "
                    "set what the family declares")
            if (why := check_value(kind, decl, value)) is not None:
                raise EmitError(f"product {product.get('name')}: "
                                f"{section}.{name}: {why}")
            if is_nvm(kind, decl):
                nvm_values[name] = value
            else:
                values[name] = value
                explicit.add(name)

    evaluate_rules(family, decls, values, explicit)
    if violations := constraint_violations(family, decls, values):
        raise EmitError(f"product '{product.get('name')}' violates the family's "
                        "constraints:\n"
                        + "\n".join(f"  {v['message']}" for v in violations))

    # ---- the model ------------------------------------------------------
    enums: dict[str, dict[str, int]] = {}
    params: dict[str, dict[str, Any]] = {}
    nvm: dict[str, dict[str, int]] = {}
    for name, (kind, decl) in decls.items():
        if kind != "param":
            continue
        if is_nvm(kind, decl):
            key = decl.get("key")
            if not isinstance(key, int) or isinstance(key, bool) \
                    or not 0 <= key <= U32_MAX:
                raise EmitError(f"family: params.{name}: nvm = true needs an "
                                "explicit u32 'key' (design decision: keys are "
                                "never auto-assigned)")
            if key == NVM_FREE_SLOT_KEY:
                raise EmitError(f"family: params.{name}: key {key:#x} is "
                                "nvm_kv's erased-slot marker — pick another")
            nvm[name] = {"key": key, "default": nvm_values[name]}
            continue
        if decl.get("type") == "enum":
            members = decl["values"]
            for member, wire in members.items():
                if not isinstance(wire, int) or isinstance(wire, bool):
                    raise EmitError(f"family: params.{name}: enum value "
                                    f"'{member}' must be an explicit integer")
            if len(set(members.values())) != len(members):
                raise EmitError(f"family: params.{name}: duplicate wire values "
                                "in the enum")
            enums[name] = dict(members)
        params[name] = {"type": decl["type"], "value": values[name]}
    dup_keys = [k for k in {v["key"] for v in nvm.values()}
                if sum(1 for v in nvm.values() if v["key"] == k) > 1]
    if dup_keys:
        raise EmitError("family: duplicate nvm key(s): "
                        + ", ".join(f"{k:#x}" for k in sorted(dup_keys)))

    strategies = {name: values[name] for name, (kind, _) in decls.items()
                  if kind == "strategy"}
    caps = {name: values[name] for name, (kind, _) in decls.items()
            if kind == "cap"}
    return {
        "name": product["name"],
        "family": family["name"],
        "enums": enums,
        "params": params,
        "caps": caps,
        "strategies": strategies,
        "nvm": nvm,
        "values": values,
    }
