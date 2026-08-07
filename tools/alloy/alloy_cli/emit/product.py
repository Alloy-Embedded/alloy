"""Emit the product layer: alloy/product.hpp + alloy/product_nvm.hpp (+ a
plain-C twin for legacy codebases).

Pure functions over the resolved model from products.resolve() — the emitter
carries no policy of its own: what a value MEANS (types, ranges, rules,
constraints) was already decided when the model was built, so a bad product
fails generation there, never the compile.

The strategy contract: the family names options, the APP provides the types.
product.hpp emits `using X = ::product_strategy::<option>;`, so the app must
declare (or include) its strategy types in namespace `product_strategy`
BEFORE including <alloy/product.hpp>. A missing type fails the compile at the
alias line. Witnessed on arm-none-eabi-g++ 14.2.1: the diagnostic is
"'<option>' in namespace 'product_strategy' does not name a type", pointing
at the `using` line for that alias in product.hpp.
"""

from __future__ import annotations

from typing import Any

from .common import BANNER, EmitError


def _require(cond: bool, msg: str) -> None:
    if not cond:
        raise EmitError(msg)


def _cpp_value(name: str, ptype: str, value: Any) -> tuple[str, str]:
    """(C++ type, C++ initializer) for one param."""
    if ptype == "int":
        return "std::int32_t", str(value)
    if ptype == "bool":
        return "bool", "true" if value else "false"
    if ptype == "float":
        return "float", f"{value!r}f" if isinstance(value, float) else f"{value}.0f"
    if ptype == "frequency":
        return "alloy::frequency", f"alloy::frequency{{{value}u}}"
    if ptype == "enum":
        # The enum type lives at product:: scope, the constant of the same name
        # at product::params:: — fully qualified so the shadowing is harmless.
        return f"::product::{name}", f"::product::{name}::{value}"
    raise EmitError(f"product param '{name}': unknown type '{ptype}'")


def emit_product_header(model: dict[str, Any]) -> str:
    """gen/alloy/product.hpp — constexpr params, caps, strategy aliases."""
    _require(bool(model.get("name")), "product model carries no name")
    out: list[str] = [BANNER, "#pragma once\n\n"]
    out.append("#include <cstdint>\n\n")
    if any(p["type"] == "frequency" for p in model["params"].values()):
        out.append('#include "alloy/core/units.hpp"\n\n')
    out.append("namespace product {\n\n")
    out.append(f'inline constexpr char name[] = "{model["name"]}";\n')
    out.append(f'inline constexpr char family[] = "{model["family"]}";\n\n')

    for ename, members in model["enums"].items():
        _require(len(set(members.values())) == len(members),
                 f"enum '{ename}': duplicate wire values")
        body = ", ".join(f"{m} = {v}" for m, v in members.items())
        out.append(f"enum class {ename} : std::uint32_t {{ {body} }};\n")
    if model["enums"]:
        out.append("\n")

    out.append("namespace params {\n")
    for pname, p in model["params"].items():
        ctype, init = _cpp_value(pname, p["type"], p["value"])
        out.append(f"inline constexpr {ctype} {pname} = {init};\n")
    out.append("}  // namespace params\n\n")

    out.append("namespace caps {\n")
    for cname, value in model["caps"].items():
        out.append(f"inline constexpr bool {cname} = "
                   f"{'true' if value else 'false'};\n")
    out.append("}  // namespace caps\n")

    if model["strategies"]:
        out.append(
            "\n// Strategy choices. The app provides these types: declare (or"
            "\n// include) them in namespace ::product_strategy BEFORE this"
            "\n// header — a missing one fails the compile at its alias below.\n")
        for sname, option in model["strategies"].items():
            out.append(f"using {sname} = ::product_strategy::{option};\n")

    out.append("\n}  // namespace product\n")
    return "".join(out)


def emit_product_nvm_header(model: dict[str, Any]) -> str:
    """gen/alloy/product_nvm.hpp — nvm_kv keys + per-product defaults.

    Deliberately a SEPARATE header from product.hpp: an nvm field is
    runtime-configured, so it must never also exist as a constexpr param (the
    compile-time constant and the flash value would disagree). Consume as:
        board::nvm.get(product::nvm::k_<field>, product::nvm::<field>_default)
    """
    out: list[str] = [BANNER, "#pragma once\n\n#include <cstdint>\n\n",
                      "namespace product::nvm {\n"]
    for fname, entry in model["nvm"].items():
        key, default = entry["key"], entry["default"]
        _require(0 <= key < 0xFFFFFFFF,
                 f"nvm field '{fname}': key must be a u32 other than the "
                 "erased-slot marker (all-ones)")
        out.append(f"inline constexpr std::uint32_t k_{fname} = 0x{key:08X}u;\n")
        out.append(f"inline constexpr std::uint32_t {fname}_default = {default}u;\n")
    out.append("}  // namespace product::nvm\n")
    return "".join(out)


def emit_product_c_header(model: dict[str, Any]) -> str:
    """The plain-C twin: same data as #define lines, for migrating legacy C
    codebases off their #ifdef spaghetti one file at a time. SECONDARY output
    — one function, no acceptance weight."""
    up = str.upper
    lines = [f"/* {BANNER[3:].rstrip()} */", "#pragma once",
             f'#define PRODUCT_NAME "{model["name"]}"',
             f'#define PRODUCT_FAMILY "{model["family"]}"']
    for ename, members in model["enums"].items():
        lines += [f"#define PRODUCT_{up(ename)}_{up(m)} {v}"
                  for m, v in members.items()]
    for pname, p in model["params"].items():
        v = p["value"]
        if p["type"] == "enum":
            v = model["enums"][pname][v]
        elif p["type"] == "bool":
            v = int(v)
        elif p["type"] == "float" and isinstance(v, float):
            v = f"{v!r}f"
        lines.append(f"#define PRODUCT_{up(pname)} {v}")
    lines += [f"#define PRODUCT_CAP_{up(c)} {int(v)}"
              for c, v in model["caps"].items()]
    lines += [f'#define PRODUCT_STRATEGY_{up(s)} "{opt}"'
              for s, opt in model["strategies"].items()]
    lines += [f"#define PRODUCT_NVM_{up(f)}_KEY 0x{e['key']:08X}u\n"
              f"#define PRODUCT_NVM_{up(f)}_DEFAULT {e['default']}u"
              for f, e in model["nvm"].items()]
    return "\n".join(lines) + "\n"
