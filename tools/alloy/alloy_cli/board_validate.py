"""Board validation — every problem at once, located, with a way out.

`board-info` reports what the EMITTER would reject, which is exactly one problem
per run (it raises on the first bad role) and arrives as a sentence with no
structure. That is fine for a build log and useless for a form: a UI needs to
know which field is wrong and what to put there instead.

So the rules live here, expressed independently of emission. The headline one is
the ROUTE check. `alloy::i2c::bind` already refuses a pin that has no route to
its peripheral — but only as a `static_assert`, only when the app instantiates
the bus, and only at build time. A board can carry a wrong pin for months if
nothing opens that bus. This moves the same question to the moment of choosing,
and answers it with the pins that WOULD work.

Two rule sets describing one contract is a drift risk, so `test_board_validate`
pins them together: a board this module calls clean must generate, and a board
the emitter rejects must be reported here.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .devices import load_chip, load_registers
from .emit.board import dma_assignment_problems
from .emit.common import EmitError, uses_external_clock
from .roles import ROLES, ip_classes, role_pin_fields, routes_by_peripheral

MAX_SUGGESTIONS = 6


def _issue(level: str, message: str, *, role: str | None = None,
           field: str | None = None, pin: str | None = None,
           suggestions: list[str] | None = None) -> dict[str, Any]:
    return {
        "level": level, "role": role, "field": field, "pin": pin,
        "message": message, "suggestions": suggestions or [],
    }


def _check_peripheral(role: str, cfg: dict[str, Any], spec, chip: dict[str, Any],
                      classes: dict[str, tuple[str, ...]]) -> tuple[str | None, list[dict[str, Any]]]:
    """Resolve the role's peripheral, reporting why it can't be used."""
    peripherals = chip.get("peripherals") or {}
    name = cfg.get("peripheral")
    # MEMBERSHIP, NOT EQUALITY: a block can be driven as more than one class
    # (a timer is a PWM generator or an encoder counter), so `classes` maps an
    # instance to every class it can fill. See roles.ip_classes.
    alternatives = sorted(p for p, c in classes.items() if spec.ip_class in c
                          and not peripherals[p].get("uncurated", False))
    if not isinstance(name, str):
        return None, [_issue("error", f"{role}: no peripheral chosen",
                             role=role, field="peripheral",
                             suggestions=alternatives[:MAX_SUGGESTIONS])]
    if name not in peripherals:
        return None, [_issue("error",
                             f"{role}: peripheral '{name}' is not on this chip",
                             role=role, field="peripheral",
                             suggestions=alternatives[:MAX_SUGGESTIONS])]
    if peripherals[name].get("uncurated", False):
        return None, [_issue(
            "error",
            f"{role}: '{name}' has no curated register file, so no driver can "
            f"be selected for it",
            role=role, field="peripheral",
            suggestions=alternatives[:MAX_SUGGESTIONS])]
    if spec.ip_class not in classes.get(name, ()):
        return name, [_issue(
            "warning",
            f"{role}: '{name}' is a "
            f"{' / '.join(classes.get(name) or ['classless'])} peripheral, not "
            f"'{spec.ip_class}' — the driver may not match",
            role=role, field="peripheral",
            suggestions=alternatives[:MAX_SUGGESTIONS])]
    return name, []


def _check_external_clock(board: dict[str, Any],
                          program: list[Any]) -> list[dict[str, Any]]:
    """A board whose clock program starts a crystal MUST declare that crystal.

    Not pedantry: without the declaration nothing records that this firmware
    needs a component the PCB may not have. The failure it prevents is quiet —
    the HSERDY poll times out, `board::init()` falls back to the boot source, and
    the product runs at the wrong speed with every baud rate silently off.
    """
    hse = board.get("hse")
    uses = uses_external_clock(program)
    if uses and not isinstance(hse, dict):
        return [_issue(
            "error",
            "this clock runs from an external oscillator but the board declares no "
            "'hse' — add e.g. \"hse\": {\"hz\": 8000000, \"bypass\": false}, or "
            "re-solve without --hse",
            field="hse")]
    if isinstance(hse, dict):
        out: list[dict[str, Any]] = []
        if not isinstance(hse.get("hz"), int) or hse["hz"] <= 0:
            out.append(_issue("error", "hse.hz must be the oscillator frequency in Hz",
                              field="hse.hz"))
        if not uses:
            out.append(_issue(
                "warning",
                "the board declares an external oscillator but its clock does not use "
                "it — the crystal is unused (solve with --hse to run the PLL from it)",
                field="hse"))
        return out
    return []


def _check_overrides(settings: dict[str, Any],
                     board: dict[str, Any]) -> list[dict[str, Any]]:
    """alloy.toml may choose, not redesign.

    A project field is one the same hardware supports many values of — a baud
    rate, a watchdog timeout. Everything else describes what the board IS, and
    silently ignoring an attempt to change it would be the worst outcome: the
    user writes `pin = "pb7"`, nothing happens, and the LED stays dark with no
    explanation.
    """
    out: list[dict[str, Any]] = []
    enabled = board.get("roles") or {}
    for role, overrides in (settings.get("roles") or {}).items():
        spec = ROLES.get(role)
        if spec is None:
            out.append(_issue("error",
                              f"alloy.toml: '{role}' is not a board role",
                              role=role, suggestions=sorted(ROLES)[:MAX_SUGGESTIONS]))
            continue
        if role not in enabled:
            # The override is real and well-formed, and does nothing: there is
            # no such role on this board to change. Saying so beats leaving the
            # user to wonder why their timeout had no effect.
            out.append(_issue(
                "warning",
                f"alloy.toml: [roles.{role}] has no effect — "
                f"{board.get('id') or 'this board'} has no {role} role",
                role=role, suggestions=sorted(enabled)[:MAX_SUGGESTIONS]))
            continue
        for key in (overrides or {}):
            if key in spec.project_fields:
                continue
            out.append(_issue(
                "error",
                f"alloy.toml: {role}.{key} is a property of the BOARD, not of this "
                f"project — a project may choose "
                f"{', '.join(spec.project_fields) or 'nothing'} here. To change "
                f"anything else, make the board yours: alloy board-clone",
                role=role, field=key,
                suggestions=list(spec.project_fields)[:MAX_SUGGESTIONS]))
    return out


def _check_image_config(board: dict[str, Any], chip: dict[str, Any]) -> list[dict[str, Any]]:
    """Configuration the CPU reads at reset, before any code runs.

    A chip declares the region; the board fills it, because the values are the
    board's decisions — the watchdog interval, the brown-out level, and on RL78
    the HOCO frequency the whole clock tree derives from. Leaving it blank is
    legal and almost never intended: the part boots on the erased value, which
    generally means no watchdog and no brown-out detection. So it is a WARNING
    that names what was left to chance, not an error.

    Wrong-length or over-wide values ARE errors: those write past the region or
    silently truncate, and the CPU reads them before anything can complain.
    """
    out: list[dict[str, Any]] = []
    supplied = board.get("image_config") or {}
    for region in chip.get("image_config") or []:
        name, size = region["name"], int(region["size"])
        value = supplied.get(name)
        if value is None:
            out.append(_issue(
                "warning",
                f"{name} is not set — the CPU reads this at reset and will get "
                f"{region.get('erased_value', 'the erased value')}: "
                f"{region['description'].splitlines()[0].strip()}",
                field=name))
            continue
        if not isinstance(value, list) or len(value) != size:
            out.append(_issue(
                "error",
                f"{name} must be exactly {size} byte(s); got "
                f"{len(value) if isinstance(value, list) else type(value).__name__}",
                field=name))
            continue
        for i, byte in enumerate(value):
            raw = int(byte, 16) if isinstance(byte, str) else int(byte)
            if not 0 <= raw <= 0xFF:
                out.append(_issue("error",
                                  f"{name}[{i}] = {byte} does not fit a byte",
                                  field=name))
    return out


def validate_board(board: dict[str, Any], chip: dict[str, Any],
                   registers: dict[str, dict[str, Any]],
                   settings: dict[str, Any] | None = None) -> list[dict[str, Any]]:
    """Every problem in a board, as located, fixable issues."""
    issues: list[dict[str, Any]] = _check_overrides(settings or {}, board)
    issues += _check_image_config(board, chip)
    roles = board.get("roles") or {}
    pins = chip.get("pins") or {}
    classes = ip_classes(chip, registers)
    routes = routes_by_peripheral(chip)

    # --- clock -----------------------------------------------------------
    # Resolve the program whichever way the board names its clock, so the
    # external-oscillator rule applies to a curated chip profile too.
    program: list[Any] = []
    if isinstance(board.get("clock"), dict):
        for key in ("program", "sysclk_hz"):
            if key not in board["clock"]:
                issues.append(_issue("error",
                                     f"inline clock is missing '{key}'", field="clock"))
        program = board["clock"].get("program") or []
    else:
        profile = board.get("clock_profile")
        profiles = (chip.get("clock") or {}).get("profiles") or {}
        known = list(profiles)
        if profile not in known:
            issues.append(_issue(
                "error", f"clock profile '{profile}' is not one this chip offers",
                field="clock_profile", suggestions=known[:MAX_SUGGESTIONS]))
        else:
            program = (profiles[profile] or {}).get("program") or []
    issues.extend(_check_external_clock(board, program))

    # --- dma channel assignments -----------------------------------------
    # SAME rules the emitter refuses on (one function, no drift), reported
    # here in full with suggestions instead of stopping at the first.
    for problem in dma_assignment_problems(board, chip, registers):
        issues.append(_issue(
            "error", f"dma '{problem['key']}': {problem['message']}",
            field=f"dma.{problem['key']}",
            suggestions=problem["suggestions"][:MAX_SUGGESTIONS]))

    # --- roles -----------------------------------------------------------
    for role, cfg in sorted(roles.items()):
        spec = ROLES.get(role)
        if spec is None:
            issues.append(_issue("error", f"'{role}' is not a board role the "
                                          f"framework knows", role=role,
                                 suggestions=sorted(ROLES)[:MAX_SUGGESTIONS]))
            continue
        if not isinstance(cfg, dict):
            issues.append(_issue("error", f"{role}: expected a table of settings",
                                 role=role))
            continue

        peripheral: str | None = None
        if spec.kind == "peripheral":
            peripheral, found = _check_peripheral(role, cfg, spec, chip, classes)
            issues.extend(found)

        if spec.requires_role and spec.requires_role not in roles:
            issues.append(_issue(
                "error",
                f"{role}: needs the {spec.requires_role} role on the same board",
                role=role, suggestions=[spec.requires_role]))

        # Some settings switch a role into a shape with fewer requirements — a
        # boot-ROM UART has no pins to name at all.
        waived: set[str] = set()
        for field_name, value, fields in spec.waivers:
            if cfg.get(field_name) == value:
                waived.update(fields)

        for name in spec.required:
            if name not in cfg and name not in waived:
                issues.append(_issue("error", f"{role}: missing '{name}'",
                                     role=role, field=name))

        # Routed signal pins — the static_assert, moved forward.
        for signal in spec.signals:
            pin = cfg.get(signal)
            if pin is None or signal in waived:
                continue
            if pin not in pins:
                issues.append(_issue(
                    "error", f"{role}.{signal}: '{pin}' is not a pin on this chip",
                    role=role, field=signal, pin=pin,
                    suggestions=(routes.get(peripheral or "", {}).get(signal, [])
                                 )[:MAX_SUGGESTIONS]))
                continue
            if peripheral is None:
                continue
            routable = routes.get(peripheral, {}).get(signal, [])
            if routable and pin not in routable:
                issues.append(_issue(
                    "error",
                    f"{role}.{signal}: {pin} has no route to {peripheral} {signal} "
                    f"— this fails to compile with a static_assert when the app "
                    f"opens it",
                    role=role, field=signal, pin=pin,
                    suggestions=routable[:MAX_SUGGESTIONS]))
            elif not routable and signal not in spec.optional_signals:
                issues.append(_issue(
                    "warning",
                    f"{role}.{signal}: this chip's data has no {peripheral} "
                    f"{signal} route to check {pin} against",
                    role=role, field=signal, pin=pin))

        # Plain GPIO fields — existence is the whole contract.
        for name in spec.pin_fields:
            pin = cfg.get(name)
            if pin is not None and pin not in pins:
                issues.append(_issue(
                    "error", f"{role}.{name}: '{pin}' is not a pin on this chip",
                    role=role, field=name, pin=pin,
                    suggestions=sorted(pins)[:MAX_SUGGESTIONS]))

        if spec.pin_list_field:
            listed = cfg.get(spec.pin_list_field)
            if listed is not None and (not isinstance(listed, list) or not listed):
                issues.append(_issue(
                    "error", f"{role}: '{spec.pin_list_field}' must be a non-empty list",
                    role=role, field=spec.pin_list_field))
            for pin in listed or []:
                if pin not in pins:
                    issues.append(_issue(
                        "error", f"{role}: '{pin}' is not a pin on this chip",
                        role=role, field=spec.pin_list_field, pin=pin,
                        suggestions=sorted(pins)[:MAX_SUGGESTIONS]))

        # A PWM role names a channel; the pin must be one that channel drives.
        if role == "led_pwm" and peripheral and "channel" in cfg:
            signal = f"ch{cfg['channel']}"
            drivable = routes.get(peripheral, {}).get(signal, [])
            channels = sorted(s for s in routes.get(peripheral, {}) if s.startswith("ch"))
            pin = cfg.get("pin")
            if drivable and pin not in drivable:
                issues.append(_issue(
                    "error",
                    f"{role}: {pin} is not an output of {peripheral} channel "
                    f"{cfg['channel']}",
                    role=role, field="pin", pin=pin,
                    suggestions=drivable[:MAX_SUGGESTIONS]))
            elif not drivable:
                # The pin still has to route to THIS channel's signal at compile
                # time, and the data cannot say whether it does.
                issues.append(_issue(
                    "warning",
                    f"{role}: this chip's data has no {peripheral} {signal} route, "
                    f"so {pin} cannot be checked against channel {cfg['channel']}",
                    role=role, field="channel",
                    suggestions=[c.removeprefix("ch") for c in channels][:MAX_SUGGESTIONS]))

    # --- named pins (board.json "pins") -----------------------------------
    for pin, assign in sorted((board.get("pins") or {}).items()):
        if pin not in pins:
            issues.append(_issue("error", f"named pin '{pin}' is not on this chip",
                                 field="pins", pin=pin))
            continue
        function = (assign or {}).get("function", "")
        if function in ("gpio_out", "gpio_in"):
            continue
        if ":" not in function:
            issues.append(_issue("error",
                                 f"pin {pin}: unknown function '{function}'",
                                 field="pins", pin=pin,
                                 suggestions=["gpio_out", "gpio_in"]))
            continue
        periph, _, signal = function.partition(":")
        if pin not in routes.get(periph, {}).get(signal, []):
            available = [f"{p}:{s}" for p, sigs in routes.items()
                         for s, ps in sigs.items() if pin in ps]
            issues.append(_issue(
                "error", f"pin {pin} has no route to {periph} {signal}",
                field="pins", pin=pin,
                suggestions=(available or ["gpio_out", "gpio_in"])[:MAX_SUGGESTIONS]))

    # --- pins claimed twice ----------------------------------------------
    # NOT an error: a board may deliberately expose one LED as both a GPIO and a
    # PWM output (every shipped Nucleo does). Worth saying out loud, because the
    # app must then use one or the other.
    owners: dict[str, list[str]] = {}
    for role, cfg in roles.items():
        spec = ROLES.get(role)
        if spec is None or not isinstance(cfg, dict):
            continue
        claimed = [cfg.get(f) for f in role_pin_fields(spec)]
        claimed += list(cfg.get(spec.pin_list_field) or []) if spec.pin_list_field else []
        for pin in claimed:
            if isinstance(pin, str):
                owners.setdefault(pin, []).append(role)
    for pin, sharing in sorted(owners.items()):
        if len(sharing) > 1:
            issues.append(_issue(
                "warning",
                f"{pin} is used by {' and '.join(sorted(sharing))} — only one of "
                f"them can drive it at a time",
                pin=pin))

    order = {"error": 0, "warning": 1}
    issues.sort(key=lambda i: (order.get(i["level"], 2), i["role"] or "", i["field"] or ""))
    return issues


def _project_board_id(project_root: Path | None) -> str | None:
    from .project import load_project  # noqa: PLC0415

    try:
        return load_project(project_root).board_id if project_root else None
    except Exception:  # noqa: BLE001 - a broken alloy.toml is not this verb's news
        return None


def validate_board_file(devices_root: Path, path: Path,
                        project_root: Path | None = None) -> dict[str, Any]:
    """Validate a board.json on disk (or '-' for stdin, so an editor can check a
    candidate before writing it).

    `project_root` brings alloy.toml into it. Without it this verb checked the
    board and ignored the overrides applied to it — so the CI gate passed on a
    project whose `[roles.led] pin` was quietly doing nothing, which is the one
    outcome the override rules exist to prevent.
    """
    import sys  # noqa: PLC0415

    from .project import read_project_settings  # noqa: PLC0415

    text = sys.stdin.read() if str(path) == "-" else path.read_text()
    try:
        board = json.loads(text)
    except json.JSONDecodeError as exc:
        raise EmitError(f"{path}: not valid JSON — {exc}") from exc
    if not isinstance(board, dict) or "chip" not in board:
        raise EmitError(f"{path}: not a board.json (no 'chip')")
    chip = load_chip(devices_root, board["chip"])
    # Only the project's OWN board gets the project's choices: validating some
    # other board with them would report overrides as inert that are not.
    settings = read_project_settings(project_root) if project_root else {}
    if settings and board.get("id") != _project_board_id(project_root):
        settings = {}
    issues = validate_board(board, chip, load_registers(devices_root), settings)
    return {
        "schema": "alloy.board_validate.v1",
        "id": board.get("id"),
        "chip": board["chip"],
        "ok": not any(i["level"] == "error" for i in issues),
        "issues": issues,
    }
