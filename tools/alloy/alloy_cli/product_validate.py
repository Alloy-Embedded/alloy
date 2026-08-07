"""Product validation — every problem at once, located, with a way out.

The product twin of board_validate. The emitter (products.resolve + emit/
product.py) rejects exactly one problem per run and says so as a build-log
sentence; a form — or a CI gate — needs ALL of them, each with the field that
is wrong and what to put there instead. So the rules live here a second time,
expressed as located issues.

Two expressions of one contract is a drift risk, so test_product_validate
pins them together the same way test_board_validate pins the board pair:

    P1  validate finds no errors  =>  resolve + emit succeed
    P2  resolve/emit fails        =>  validate found an error

The matching/derivation SEMANTICS are not duplicated: check_value,
evaluate_rules and constraint_violations are the single implementations in
products.py, called from both sides — only the reporting shape differs.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .emit.common import EmitError
from .products import (
    FAMILY_SCHEMA,
    NVM_FREE_SLOT_KEY,
    PARAM_TYPES,
    PRODUCT_SCHEMA,
    RESERVED_NAMES,
    U32_MAX,
    _IDENT,
    check_value,
    constraint_violations,
    evaluate_rules,
    is_nvm,
    known_products,
    load_family,
    load_product,
    structure_errors,
)

MAX_SUGGESTIONS = 6


def _issue(level: str, message: str, *, field: str | None = None,
           suggestions: list[str] | None = None) -> dict[str, Any]:
    return {"level": level, "field": field, "message": message,
            "suggestions": (suggestions or [])[:MAX_SUGGESTIONS]}


def _try_declared(family: dict[str, Any]) -> dict[str, tuple[str, Any]] | None:
    """declared(), or None when the family is too broken to build the
    namespace — the granular checks below already reported why."""
    from .products import declared  # noqa: PLC0415

    try:
        return declared(family)
    except EmitError:
        return None


def _check_family_params(family: dict[str, Any]) -> list[dict[str, Any]]:
    issues: list[dict[str, Any]] = []
    params = family.get("params") or {}
    if not isinstance(params, dict):
        return [_issue("error", "[params] must be a table of declarations",
                       field="params")]
    seen_nvm_keys: dict[int, str] = {}
    for name, decl in params.items():
        where = f"params.{name}"
        if not _IDENT.match(name):
            issues.append(_issue("error", f"'{name}' is not a valid C++ "
                                          "identifier", field=where))
        if not isinstance(decl, dict):
            issues.append(_issue("error", "a param declaration is a table "
                                          "(type = ..., default = ...)",
                          field=where))
            continue
        ptype = decl.get("type")
        if ptype not in PARAM_TYPES:
            issues.append(_issue("error", f"unknown type '{ptype}'",
                                 field=f"{where}.type",
                                 suggestions=list(PARAM_TYPES)))
            continue
        if ptype == "enum":
            members = decl.get("values")
            if not isinstance(members, dict) or not members:
                issues.append(_issue(
                    "error",
                    "an enum must carry explicit wire values — "
                    "values = { name = <int>, … }, never auto-assigned",
                    field=f"{where}.values"))
                continue
            for member, wire in members.items():
                if not _IDENT.match(member):
                    issues.append(_issue("error", f"member '{member}' is not a "
                                                  "valid C++ identifier",
                                         field=f"{where}.values"))
                if not isinstance(wire, int) or isinstance(wire, bool):
                    issues.append(_issue(
                        "error", f"member '{member}' needs an explicit integer "
                                 f"wire value (got {wire!r})",
                        field=f"{where}.values"))
            wires = [w for w in members.values()
                     if isinstance(w, int) and not isinstance(w, bool)]
            if len(set(wires)) != len(wires):
                issues.append(_issue("error", "duplicate wire values in the "
                                              "enum", field=f"{where}.values"))
            if name in RESERVED_NAMES:
                issues.append(_issue("error", f"'{name}' collides with a name "
                                              "the generated header already "
                                              "uses at product:: scope",
                                     field=where))
        if "default" not in decl:
            issues.append(_issue("error", "missing 'default' — the family is "
                                          "the defaults", field=where))
        elif (why := check_value("param", decl, decl["default"])) is not None:
            issues.append(_issue("error", f"default: {why}",
                                 field=f"{where}.default"))
        lo, hi = decl.get("min"), decl.get("max")
        if lo is not None and hi is not None and lo > hi:
            issues.append(_issue("error", f"min ({lo}) > max ({hi})",
                                 field=where))
        if decl.get("nvm"):
            if ptype != "int":
                issues.append(_issue(
                    "error", "nvm = true stores the value in nvm_kv, which "
                             "holds u32 — the type must be 'int'",
                    field=f"{where}.type", suggestions=["int"]))
            key = decl.get("key")
            if not isinstance(key, int) or isinstance(key, bool) \
                    or not 0 <= key <= U32_MAX:
                issues.append(_issue(
                    "error", "nvm = true needs an explicit u32 'key' — keys "
                             "are never auto-assigned",
                    field=f"{where}.key"))
            elif key == NVM_FREE_SLOT_KEY:
                issues.append(_issue(
                    "error", f"key {key:#x} is nvm_kv's erased-slot marker "
                             "(an all-ones slot reads as free) — pick another",
                    field=f"{where}.key"))
            elif key in seen_nvm_keys:
                issues.append(_issue(
                    "error", f"key {key:#x} is already used by "
                             f"'{seen_nvm_keys[key]}' — nvm keys must be "
                             "unique across the family",
                    field=f"{where}.key"))
            else:
                seen_nvm_keys[key] = name
    return issues


def _check_family(family: dict[str, Any]) -> list[dict[str, Any]]:
    """Every problem in the family itself. A family problem is every
    product's problem, so it surfaces in each product's report."""
    issues: list[dict[str, Any]] = []
    if family.get("schema") != FAMILY_SCHEMA:
        issues.append(_issue("error", f"schema must be \"{FAMILY_SCHEMA}\"",
                             field="schema", suggestions=[FAMILY_SCHEMA]))
    if not isinstance(family.get("name"), str):
        issues.append(_issue("error", "missing name = \"<family name>\"",
                             field="name"))
    for key in ("family", "extends"):
        if key in family:
            issues.append(_issue(
                "error", f"'{key}' is not allowed in a family — the overlay "
                         "is one level deep by design; a family derives from "
                         "nothing", field=key))
    known_keys = {"schema", "name", "params", "caps", "strategies", "rules",
                  "constraints"}
    for key in sorted(set(family) - known_keys - {"family", "extends"}):
        issues.append(_issue("error", f"unknown key '{key}'", field=key,
                             suggestions=sorted(known_keys)))

    issues += _check_family_params(family)

    caps = family.get("caps") or {}
    if not isinstance(caps, dict):
        issues.append(_issue("error", "[caps] must be a table of booleans",
                             field="caps"))
        caps = {}
    for name, default in caps.items():
        if not _IDENT.match(name):
            issues.append(_issue("error", f"'{name}' is not a valid C++ "
                                          "identifier", field=f"caps.{name}"))
        if not isinstance(default, bool):
            issues.append(_issue("error", "a capability is a boolean",
                                 field=f"caps.{name}",
                                 suggestions=["true", "false"]))

    strategies = family.get("strategies") or {}
    if not isinstance(strategies, dict):
        issues.append(_issue("error", "[strategies] must be a table",
                             field="strategies"))
        strategies = {}
    for name, decl in strategies.items():
        where = f"strategies.{name}"
        if not _IDENT.match(name) or name in RESERVED_NAMES:
            issues.append(_issue("error", f"'{name}' is not usable as the "
                                          "alias name the header emits",
                                 field=where))
        if not isinstance(decl, dict):
            issues.append(_issue("error", "a strategy is a table "
                                          "(options = […], default = \"…\")",
                                 field=where))
            continue
        options = decl.get("options")
        if not isinstance(options, list) or not options \
                or not all(isinstance(o, str) and _IDENT.match(o) for o in options):
            issues.append(_issue(
                "error", "options must be a non-empty list of type names "
                         "(each becomes ::product_strategy::<option>)",
                field=f"{where}.options"))
            continue
        if len(set(options)) != len(options):
            issues.append(_issue("error", "duplicate options",
                                 field=f"{where}.options"))
        if "default" not in decl:
            issues.append(_issue("error", "missing 'default'",
                                 field=f"{where}.default", suggestions=options))
        elif decl["default"] not in options:
            issues.append(_issue("error",
                                 f"default '{decl['default']}' is not an option",
                                 field=f"{where}.default", suggestions=options))

    # one namespace across params/caps/strategies
    names = [*(family.get("params") or {}), *caps, *strategies] \
        if isinstance(family.get("params"), dict) else [*caps, *strategies]
    dups = sorted({n for n in names if names.count(n) > 1})
    for name in dups:
        issues.append(_issue(
            "error", f"'{name}' is declared more than once — params, caps and "
                     "strategies share one namespace (rules and constraints "
                     "reference plain names)", field=name))

    # rules + constraints reference checks ride the shared evaluators: run
    # them against the family DEFAULTS so a reference/shape problem is
    # reported even before any product exists.
    decls = _try_declared(family)
    if decls is not None and not dups:
        defaults = {n: (d if k == "cap" else d.get("default"))
                    for n, (k, d) in decls.items() if not is_nvm(k, d)}
        try:
            evaluate_rules(family, decls, dict(defaults), set())
        except EmitError as exc:
            issues.append(_issue("error", str(exc), field="rules"))
        try:
            constraint_violations(family, decls, defaults)
        except EmitError as exc:
            issues.append(_issue("error", str(exc), field="constraints"))
    return issues


def validate_product(family: dict[str, Any], product: dict[str, Any],
                     known: list[str] | tuple[str, ...] = ()) -> list[dict[str, Any]]:
    """Every problem in family + product, as located, fixable issues."""
    issues = []
    for err in structure_errors(family, product, known):
        scope, _, rest = err.partition(": ")
        issues.append(_issue("error", rest or err,
                             field=scope if scope in ("family", "product") else None))
    family_issues = _check_family(family)
    issues += [dict(i, field=f"family.{i['field']}" if i["field"] else "family")
               for i in family_issues]

    decls = _try_declared(family)
    overlay_ok = True
    if decls is not None:
        for section, want_kind in (("params", "param"), ("caps", "cap"),
                                   ("strategies", "strategy")):
            table = product.get(section) or {}
            if not isinstance(table, dict):
                issues.append(_issue("error", f"[{section}] must be a table",
                                     field=section))
                overlay_ok = False
                continue
            declared_here = sorted(n for n, (k, _) in decls.items()
                                   if k == want_kind)
            for name, value in table.items():
                kind, decl = decls.get(name, (None, None))
                if kind != want_kind:
                    issues.append(_issue(
                        "error", f"the family declares no {want_kind} "
                                 f"'{name}' — a product may only set what "
                                 "the family declares",
                        field=f"{section}.{name}", suggestions=declared_here))
                    overlay_ok = False
                elif (why := check_value(kind, decl, value)) is not None:
                    sugg = (decl.get("options") if want_kind == "strategy"
                            else list((decl.get("values") or {}))
                            if isinstance(decl, dict) else [])
                    issues.append(_issue("error", why,
                                         field=f"{section}.{name}",
                                         suggestions=sugg or []))
                    overlay_ok = False

    # Constraints only mean something on a resolvable product: with the
    # structure clean, run the SAME evaluation the emitter runs.
    if decls is not None and overlay_ok \
            and not any(i["level"] == "error" for i in issues):
        values: dict[str, Any] = {}
        explicit: set[str] = set()
        for name, (kind, decl) in decls.items():
            if is_nvm(kind, decl):
                continue
            values[name] = decl if kind == "cap" else decl["default"]
        for section in ("params", "caps", "strategies"):
            for name, value in (product.get(section) or {}).items():
                kind, decl = decls[name]
                if is_nvm(kind, decl):
                    continue
                values[name] = value
                explicit.add(name)
        try:
            evaluate_rules(family, decls, values, explicit)
            for v in constraint_violations(family, decls, values):
                issues.append(_issue("error", v["message"],
                                     field=f"constraints.{v['name']}"))
        except EmitError as exc:  # family rule/constraint shape problems
            issues.append(_issue("error", str(exc), field="family"))

    order = {"error": 0, "warning": 1}
    issues.sort(key=lambda i: (order.get(i["level"], 2), i["field"] or ""))
    return issues


def validate_product_file(project_root: Path | None, name: str | None = None,
                          file: Path | None = None) -> dict[str, Any]:
    """Validate one product of a project (or a candidate file / '-' = stdin,
    so an editor can check before writing). Returns the
    alloy.product_validate.v1 envelope; raises EmitError only when there is
    nothing coherent to report about (no project, no family)."""
    import sys  # noqa: PLC0415
    import tomllib  # noqa: PLC0415

    if project_root is None:
        raise EmitError("product-validate needs a project (products/ lives in "
                        "the project root)")
    pdir = project_root / "products"
    known = known_products(pdir)
    family = load_family(pdir)

    if file is not None:
        text = sys.stdin.read() if str(file) == "-" else Path(file).read_text()
        try:
            product = tomllib.loads(text)
        except tomllib.TOMLDecodeError as exc:
            raise EmitError(f"{file}: not valid TOML — {exc}") from exc
        expected_name = None if str(file) == "-" else Path(file).stem
    else:
        if name is None:
            from .project import load_project  # noqa: PLC0415

            name = load_project(project_root).product_id
            if name is None:
                raise EmitError(
                    "no product selected — pass a name, or set "
                    "[product] name = \"…\" in alloy.toml "
                    f"(known products: {', '.join(known) or '(none)'})")
        if name not in known:
            # The alloy.toml (or CLI) names a product that does not exist —
            # report it as the located issue it is, listing the ones that do.
            return {
                "schema": "alloy.product_validate.v1",
                "id": name, "family": family.get("name"), "ok": False,
                "issues": [_issue(
                    "error", f"no products/{name}.toml in this project",
                    field="product", suggestions=known)],
            }
        product = load_product(pdir, name)
        expected_name = name

    issues = validate_product(family, product, known)
    if expected_name is not None and isinstance(product.get("name"), str) \
            and product["name"] != expected_name:
        issues.insert(0, _issue(
            "error", f"name '{product['name']}' != file name "
                     f"'{expected_name}' — one product, one file, one name",
            field="name", suggestions=[expected_name]))
    return {
        "schema": "alloy.product_validate.v1",
        "id": product.get("name") or expected_name,
        "family": family.get("name"),
        "ok": not any(i["level"] == "error" for i in issues),
        "issues": issues,
    }
