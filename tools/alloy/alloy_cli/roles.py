"""The board-role catalogue — one description of what every role needs.

Three consumers ask different questions of the same facts: `chip-info` asks
"which peripherals of this chip could fill this role", `board-info` asks "which
pins has this board already spoken for", and `board-validate` asks "is what the
user wrote actually buildable". Before this module each kept its own table, and
a third copy was one commit away from disagreeing with the emitter.

The field names here mirror ``emit/board.py`` exactly. That emitter is what
rejects a bad board, so anything described here must be what it asks for.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass(frozen=True)
class RoleSpec:
    #: How the role is chosen: a peripheral instance, a bare pin, a list of
    #: pins, or a part on a bus that the chip knows nothing about.
    kind: str  # "peripheral" | "pin" | "pins" | "external"
    #: IP class (as declared in registers/<vendor>/<ip>.yaml) whose instances
    #: can fill this role. Matching on class, not on instance names, is what
    #: finds `adc`, `flash` and `iwdg` — none of which are named after a role.
    ip_class: str | None = None
    #: Fields naming a pin that must ROUTE to the peripheral. Each is both a
    #: form control and a static_assert waiting to happen at compile time.
    signals: tuple[str, ...] = ()
    #: Fields naming a plain GPIO — no routing involved.
    pin_fields: tuple[str, ...] = ()
    #: Field holding a list of pins (the GPIO bus).
    pin_list_field: str | None = None
    required: tuple[str, ...] = ()
    optional: tuple[str, ...] = ()
    #: Another role this one depends on (EEPROM needs a bus to sit on).
    requires_role: str | None = None
    #: Signal fields the emitter does NOT demand — a CAN board may declare only
    #: the controller, with the transceiver pins fixed by the board layout.
    optional_signals: tuple[str, ...] = field(default=())
    #: Fields that describe what THIS PROJECT chose, not what the board IS.
    #:
    #: The line: a board fact is fixed by the PCB (which pin the LED is on, that
    #: it is active-low, which peripheral the header exposes). A project field
    #: is one the same hardware supports many values of, and the application
    #: picks one — a baud rate, a watchdog timeout, how much flash to reserve.
    #:
    #: Only these may be overridden from alloy.toml. Everything else needs a
    #: board of your own, because changing it means the hardware is different.
    project_fields: tuple[str, ...] = field(default=())
    #: (field, value, waived fields): when the role sets `field` to `value`, the
    #: listed fields stop being required. The one case is a boot-ROM-configured
    #: UART (classic ESP32 UART0), which has no pin routing at all — the emitter
    #: skips tx/rx for it, so validation must too.
    waivers: tuple[tuple[str, Any, tuple[str, ...]], ...] = field(default=())


ROLES: dict[str, RoleSpec] = {
    "led": RoleSpec(
        kind="pin", pin_fields=("pin",),
        required=("pin",), optional=("active", "kind", "label")),
    "button": RoleSpec(
        kind="pin", pin_fields=("pin",),
        required=("pin",), optional=("active", "pull", "label")),
    "gpio_bus": RoleSpec(
        kind="pins", pin_list_field="pins",
        required=("pins",), optional=("label",)),
    "debug_uart": RoleSpec(
        kind="peripheral", ip_class="uart", signals=("tx", "rx"),
        required=("peripheral", "tx", "rx"), optional=("baud", "mode", "label"),
        project_fields=("baud",),
        waivers=(("mode", "rom", ("tx", "rx")),)),
    "i2c": RoleSpec(
        kind="peripheral", ip_class="i2c", signals=("scl", "sda"),
        required=("peripheral", "scl", "sda"), optional=("label",)),
    "spi": RoleSpec(
        kind="peripheral", ip_class="spi", signals=("sck", "miso", "mosi"),
        pin_fields=("cs",),
        required=("peripheral", "sck", "miso", "mosi"), optional=("cs", "label")),
    "led_pwm": RoleSpec(
        kind="peripheral", ip_class="pwm", pin_fields=("pin",),
        required=("peripheral", "channel", "pin"), optional=("label",)),
    "adc": RoleSpec(
        kind="peripheral", ip_class="adc",
        required=("peripheral",), optional=("label",)),
    "dac": RoleSpec(
        kind="peripheral", ip_class="dac",
        required=("peripheral",), optional=("label",)),
    "can": RoleSpec(
        kind="peripheral", ip_class="can", signals=("tx", "rx"),
        optional_signals=("tx", "rx"),
        required=("peripheral",), optional=("tx", "rx", "label")),
    "rtc": RoleSpec(
        kind="peripheral", ip_class="rtc",
        required=("peripheral",), optional=("label",)),
    "watchdog": RoleSpec(
        kind="peripheral", ip_class="watchdog",
        required=("peripheral",), optional=("timeout_ms", "label"),
        project_fields=("timeout_ms",)),
    "nvm": RoleSpec(
        kind="peripheral", ip_class="flash",
        required=("peripheral",), optional=("bytes",),
        project_fields=("bytes",)),
    "fs": RoleSpec(
        kind="peripheral", ip_class="flash",
        required=("peripheral",), optional=("bytes",),
        project_fields=("bytes",)),
    "ethernet": RoleSpec(
        kind="peripheral", ip_class="eth", pin_fields=("reset_pin",),
        required=("peripheral", "reset_pin", "phy"), optional=("label",)),
    "eeprom": RoleSpec(
        kind="external", pin_fields=("wp",), requires_role="i2c",
        required=("addr",),
        optional=("bus", "id_addr", "page_size", "bytes", "wp", "label")),
}


def routes_by_peripheral(chip: dict[str, Any]) -> dict[str, dict[str, list[str]]]:
    """{peripheral: {signal: [pins…]}} — ALL pins a signal can reach, not just
    the first. A picker that offers one pin per signal is why users ended up
    editing board.json by hand."""
    out: dict[str, dict[str, list[str]]] = {}
    for route in chip.get("routes") or []:
        if not all(k in route for k in ("pin", "peripheral", "signal")):
            continue
        pins = out.setdefault(route["peripheral"], {}).setdefault(route["signal"], [])
        if route["pin"] not in pins:
            pins.append(route["pin"])
    return out


def ip_classes(chip: dict[str, Any],
               registers: dict[str, dict[str, Any]]) -> dict[str, str | None]:
    """{peripheral instance: IP class}, resolved through the register files."""
    return {
        name: registers.get(spec.get("ip") or "", {}).get("class")
        for name, spec in (chip.get("peripherals") or {}).items()
    }


def role_pin_fields(spec: RoleSpec) -> tuple[str, ...]:
    """Every field of this role that names a single pin — routed signals first,
    then plain GPIOs. Used to answer "which pins are taken"."""
    return (*spec.signals, *spec.pin_fields)
