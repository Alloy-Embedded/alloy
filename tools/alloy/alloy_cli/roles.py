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
    # A UART whose RECEIVER STAYS CLOCKED WHILE THE CORE SLEEPS. A second uart
    # role and not `debug_uart` pointed at a different peripheral, because the
    # two say different things about the BOARD: `debug_uart` is where a human
    # reads the console (on a Nucleo, the ST-LINK's virtual COM port), and this
    # one is the link a product may be woken by. A board can have both, one, or
    # neither, and which peripheral fills each is a board fact.
    #
    # `ip_class` is "uart" and NOT an "lpuart" of its own, deliberately. The
    # class is what an instance can be DRIVEN as, and an LPUART is driven as a
    # UART — alloy::uart::bind, alloy::uart::config, the same handle. What makes
    # the block special is its divisor and its Stop-mode receiver, and neither
    # of those is a different driver interface. A board whose only spare UART is
    # a plain USART may fill this role with one; it simply cannot then be woken,
    # which is a property of the silicon the board chose and not of this table.
    #
    # `de` IS A SIGNAL AND IS OPTIONAL. RS-485 driver-enable needs a routed pin
    # (see alloy::uart::de<> — a pin-less `de_enable` bool is the exact lie
    # routes::routable<> exists to kill), and half the boards that want a
    # low-power link are point-to-point with no transceiver to turn around.
    # Validation checks it against the chip's `de` route; the binder's own
    # static_assert then checks the same pin against `rts`, which is the output
    # the silicon repurposes. Both are true of a real DE pin and neither alone
    # would catch a pin that is only one of them.
    #
    # `baud` is a project field for the same reason debug_uart's is — the same
    # PCB runs 9600 or 115200 and the application picks. It is ALSO the field
    # most likely to be rejected at compile time on an LPUART, whose divisor has
    # a window; see src/alloy/hal/uart/st_lpuart_baud.hpp.
    "low_power_uart": RoleSpec(
        kind="peripheral", ip_class="uart", signals=("tx", "rx", "de"),
        optional_signals=("de",),
        required=("peripheral", "tx", "rx"),
        optional=("de", "baud", "label"),
        project_fields=("baud",)),
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
    # The timer's OTHER personality. `a`/`b` are PIN FIELDS and not `signals`
    # for the same reason led_pwm's `pin` is: the route they must satisfy is
    # not a signal of their own name. A quadrature counter is wired to the
    # timer's channels 1 and 2 or it is not one, so `a` must route on ch1 and
    # `b` on ch2 — a constraint with no spelling in this table, and one that
    # `encoder::bind`'s static_assert states precisely, by pin and by signal,
    # at compile time. `alloy chip-info` already reports each timer's ch1/ch2
    # pins under `channels`, which is where a picker gets the candidates.
    #
    # `period` and `reverse` are PROJECT fields: the same board with the same
    # wiring serves a 400-count and a 4096-count encoder, and which one is
    # plugged in is the application's fact, not the PCB's.
    "encoder": RoleSpec(
        kind="peripheral", ip_class="encoder", pin_fields=("a", "b"),
        required=("peripheral", "a", "b"), optional=("period", "reverse", "label"),
        project_fields=("period", "reverse")),
    # A THIRD personality of a timer, and the one where getting the board file
    # wrong destroys hardware. Six pin fields, in high/low pairs, because a
    # bridge phase is a PAIR — `ah`/`al` is phase 1's high and low side. Only
    # the first pair is required, so a half-bridge board declares two pins and
    # a three-phase inverter declares six.
    #
    # PIN FIELDS AND NOT `signals`, for the same reason as `encoder`'s a/b:
    # the route each must satisfy is not a signal of its own name. `ah` has to
    # route on CH1 and `al` on CH1N, and that constraint has no spelling in
    # this table — `bridge::bind`'s static_asserts state it precisely, by pin
    # and by signal, at compile time.
    #
    # `brk` is OPTIONAL and its absence is generated as
    # `alloy::bridge::unprotected`, which is a loud thing to read in a bind.
    #
    # `dead_time_ns` IS REQUIRED AND IS NOT A PROJECT FIELD, and the first
    # draft of this entry had it as both — which the project/board split
    # refuted in one line: a project field is one the same HARDWARE supports
    # many values of. A dead time is not. It is the gate driver's turn-off
    # delay plus the transistors' fall time, and those are soldered to the
    # PCB. Making it overridable from alloy.toml would let an application
    # LOWER it, which is the exact edit that destroys a bridge, and it would
    # let it do so without touching a board file anybody reviews. A product
    # that needs a different dead time has a different power stage.
    #
    # `freq_hz` IS a project field: the same PCB switches at 16 or 20 kHz and
    # which one is an application trade (audible noise against switching
    # loss). `break_active` is a board fact — how the fault comparator is
    # wired — so it is neither required nor overridable, just optional with a
    # safe default of active-low.
    "bridge": RoleSpec(
        kind="peripheral", ip_class="bridge",
        pin_fields=("ah", "al", "bh", "bl", "ch", "cl", "brk"),
        required=("peripheral", "ah", "al", "dead_time_ns"),
        optional=("bh", "bl", "ch", "cl", "brk", "break_active",
                  "freq_hz", "label"),
        project_fields=("freq_hz",)),
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
    # The OTHER watchdog, and a SECOND role rather than a mode of the first.
    # `ip_class` is the substitutability gate — it is what lets a board name
    # any peripheral of that class for a role — and the two watchdogs are not
    # substitutable in either direction. An IWDG has no early bound to program;
    # a WWDG resets on a feed that arrives too soon, which is a promise the
    # `watchdog` role's facade explicitly does not make. So st/wwdg_v2's class
    # is `window_watchdog`, and naming an `iwdg` here (or a `wwdg` there) is a
    # validation error that says which class it found. A board may declare
    # BOTH: two blocks, two clocks, two independent safety properties.
    #
    # The window is PROJECT data, like the encoder's period: the same PCB
    # serves a 1 ms loop and a 10 ms one, and which is plugged in is the
    # application's fact. Unlike `watchdog`'s `timeout_ms`, these two reach
    # generated code — they are the defaults `start()` programs.
    "window_watchdog": RoleSpec(
        kind="peripheral", ip_class="window_watchdog",
        required=("peripheral",),
        optional=("deadline_us", "earliest_us", "label"),
        project_fields=("deadline_us", "earliest_us")),
    # A timer used as a bare periodic time base. NO PINS AT ALL, which is what
    # makes it a role worth having rather than a `dev::tim6_t` written into the
    # application: nothing about the board constrains it except WHICH block is
    # free, and that is exactly a board fact. `hz` is a project field — the
    # same PCB serves a 10 Hz housekeeping tick and a 20 kHz control loop, and
    # which one is running is the application's business.
    "tick": RoleSpec(
        kind="peripheral", ip_class="tick",
        required=("peripheral",), optional=("hz", "label"),
        project_fields=("hz",)),
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
               registers: dict[str, dict[str, Any]]) -> dict[str, tuple[str, ...]]:
    """{peripheral instance: the IP classes it can be driven as}, resolved
    through the register files.

    A TUPLE, NOT A STRING, and that change is the whole of this function's
    history. `class` is single-valued in the register data and every consumer
    used to compare it for equality, which quietly encoded "a block has one
    job". A general-purpose timer has two: it is a PWM generator or a
    quadrature encoder counter — same registers, same instance, mutually
    exclusive — and the second one was unreachable from any board role while
    the answer here was one string. The IP data now says so
    (`personalities:`), and asking "is this class among its classes" is the
    only question that has an answer.

    The PRIMARY class stays first, so a caller that wants "what is this
    block called" can still take element 0. Empty for an uncurated
    peripheral, which has no register file to ask.
    """
    out: dict[str, tuple[str, ...]] = {}
    for name, spec in (chip.get("peripherals") or {}).items():
        doc = registers.get(spec.get("ip") or "", {})
        primary = doc.get("class")
        out[name] = () if primary is None else (
            primary, *(doc.get("personalities") or []))
    return out


def role_pin_fields(spec: RoleSpec) -> tuple[str, ...]:
    """Every field of this role that names a single pin — routed signals first,
    then plain GPIOs. Used to answer "which pins are taken"."""
    return (*spec.signals, *spec.pin_fields)
