"""Emit the board role layer: alloy/board.hpp + board.cpp.

Roles are resolved against the chip data — the emitter carries ZERO
addresses, AF numbers or clock constants of its own (the old ecosystem's
emit_board.py died of exactly that). Unknown pins/peripherals/profiles fail
generation with a message naming the board.json entry.
"""

from __future__ import annotations

import re
from typing import Any

from .common import (
    BANNER,
    EmitError,
    field_lookup,
    register_by_name,
    uses_external_clock,
)


def _require(cond: bool, msg: str) -> None:
    if not cond:
        raise EmitError(msg)


def _region_bytes_ok(size: int, erase_size: int) -> bool:
    """A flash-backed region (nvm/fs) must be a positive whole number of erase
    pages — otherwise it erases across page boundaries into the app/neighbor and
    gives littlefs a fractional/zero block count. erase_size == 0 means the chip
    data doesn't declare the page size yet, so the check is skipped."""
    return erase_size == 0 or (size > 0 and size % erase_size == 0)


# Default sizes of the flash regions carved for the nvm/fs roles (bytes). Single
# source of truth: used both by the carve in emit_board_header and by
# flash_reserved_bytes (which the linker uses to fence the app off the regions).
_NVM_DEFAULT_BYTES = 2048
_FS_DEFAULT_BYTES = 32768


def flash_reserved_bytes(board: dict[str, Any]) -> int:
    """Total flash a board's roles reserve at the top (nvm + fs). The linker
    subtracts this from FLASH LENGTH so an app that would grow into a reserved
    region fails the LINK instead of overlapping it and being erased at runtime
    (the flash twin of the RAM overflow guard — NORTH_STAR goal #4)."""
    roles = board.get("roles", {})
    total = 0
    if "nvm" in roles:
        total += int(roles["nvm"].get("bytes", _NVM_DEFAULT_BYTES))
    if "fs" in roles:
        total += int(roles["fs"].get("bytes", _FS_DEFAULT_BYTES))
    return total


def _dma_controller(chip: dict[str, Any], registers: dict[str, dict[str, Any]]) -> str | None:
    """First peripheral whose IP is class 'dma' (alphabetical for determinism)."""
    for name in sorted(chip["peripherals"]):
        ip = chip["peripherals"][name].get("ip")
        if ip and registers.get(ip, {}).get("class") == "dma":
            return name
    return None


def _chip_singleton(chip: dict[str, Any], registers: dict[str, dict[str, Any]],
                    ip_class: str) -> str | None:
    """First CURATED peripheral of `ip_class` (alphabetical for determinism), or
    None when the chip has none.

    For blocks a board never has to wire up, so there is no role and no
    board.json entry: a CRC accelerator, a factory-programmed device ID. The
    presence of the block is a DIE fact, exactly like `caps["dma"]`, and putting
    it in `roles.py` would ask every board on a chip to repeat something none of
    them can disagree about. Uncurated peripherals are invisible here because
    they carry no `ip` — question 0 row 1, arriving at the emitter.
    """
    for name in sorted(chip["peripherals"]):
        ip = chip["peripherals"][name].get("ip")
        if ip and registers.get(ip, {}).get("class") == ip_class:
            return name
    return None


def _eth_mac_names(ip_key: str) -> tuple[str, str]:
    """(HAL include stem, HAL type name) for a MAC IP version, so a MAC is picked
    by data like every other peripheral: microchip/gmac_v1 ->
    ('microchip_gmac_v1', 'gmac'); st/eth_v1 -> ('st_eth_v1', 'eth')."""
    vendor, ip = ip_key.split("/", 1)
    return f"{vendor}_{ip}", ip.rsplit("_v", 1)[0]


def _require_curated(board_id: str, chip: dict[str, Any], periph: str, role: str) -> None:
    if chip["peripherals"].get(periph, {}).get("uncurated"):
        raise EmitError(
            f"board {board_id}: role {role} targets UNCURATED peripheral '{periph}' — "
            "curate its IP register file first"
        )


#: Which PERSONALITY each peripheral-bearing role puts a block into. A
#: personality is a mutually exclusive whole-block mode — a timer is a PWM
#: generator or an encoder counter, never both — so two roles that name one
#: peripheral under DIFFERENT personalities describe a board that cannot exist.
#:
#: `nvm` and `fs` share `flash` deliberately: two memory regions carved out of
#: one flash controller are not two personalities, they are one driver serving
#: two roles, and the nucleo_g0b1re board declares exactly that today.
#:
#: This is the BUILD-TIME half of the instance-ownership rule
#: (docs/reference/peripheral-surface.md). The runtime half is
#: alloy::claim::exclusive/shared in src/alloy/core/claim.hpp, which catches
#: the hand-written binds a board.json never sees. Neither subsumes the other:
#: this one runs before a compiler does and names the board file; that one sees
#: across translation units, where a compiler cannot.
ROLE_PERSONALITY = {
    "debug_uart": "uart",
    #: Same personality as debug_uart, which makes this table SILENT about the
    #: one board error it looks like it would catch: a board that names one
    #: peripheral for both UART roles agrees on the personality, so the check
    #: below passes it. That is the same allowance nvm/fs get above, and here it
    #: is not benign — two binds on one block are two owners. What catches it is
    #: the runtime half, alloy::claim::exclusive<Inst, personality::uart>, which
    #: traps on the second open. Stated rather than left to be discovered.
    "low_power_uart": "uart",
    "i2c": "i2c",
    "spi": "spi",
    "adc": "adc",
    "dac": "dac",
    "can": "can",
    "led_pwm": "pwm",
    "encoder": "encoder",
    "bridge": "bridge",
    "watchdog": "wdt",
    "window_watchdog": "wwdt",
    "tick": "tick",
    "rtc": "rtc",
    "nvm": "flash",
    "fs": "flash",
    "ethernet": "ethernet",
}


def _check_role_personalities(board: dict[str, Any]) -> None:
    """Refuse a board that gives one peripheral to two mutually exclusive modes."""
    claimed: dict[str, tuple[str, str]] = {}
    for role in sorted(ROLE_PERSONALITY):
        spec = (board.get("roles") or {}).get(role)
        if not isinstance(spec, dict) or "peripheral" not in spec:
            continue
        periph = spec["peripheral"]
        personality = ROLE_PERSONALITY[role]
        prev = claimed.get(periph)
        if prev is not None and prev[1] != personality:
            raise EmitError(
                f"board {board['id']}: peripheral '{periph}' is claimed by role "
                f"'{prev[0]}' as a {prev[1]} and by role '{role}' as a "
                f"{personality} — a block runs in one personality at a time, so "
                "one of the two roles has to name a different peripheral"
            )
        if prev is None:
            claimed[periph] = (role, personality)


# ── DMA channel assignments (board.json "dma") ──────────────────────────────
#
# The board states which controller+channel serves which role signal; the
# request id stays the CHIP's fact and never appears in board.json
# (docs/design/dma-streams.md §1). These helpers are the ONE expression of the
# legality rules: the emitter refuses the first problem (like
# _check_role_personalities), board_validate reports all of them with
# suggestions — both call dma_assignment_problems, so the two verbs cannot
# drift.

_DMA_IDENT = re.compile(r"[a-z_][a-z0-9_]*\Z")


def _dma_class_controllers(chip: dict[str, Any],
                           registers: dict[str, dict[str, Any]]) -> list[str]:
    return sorted(
        name for name, p in chip["peripherals"].items()
        if p.get("ip") and registers.get(p["ip"], {}).get("class") == "dma")


def dma_signal_candidates(board: dict[str, Any], chip: dict[str, Any]) -> list[str]:
    """Every 'role.signal' this board could legally assign: a declared role whose
    bound peripheral advertises the signal under `dma_requests` (G0-shape) or
    `dma_routes` (stream-engine shape, F4/F7)."""
    out: list[str] = []
    for role, cfg in (board.get("roles") or {}).items():
        if not isinstance(cfg, dict) or not isinstance(cfg.get("peripheral"), str):
            continue
        periph = chip["peripherals"].get(cfg["peripheral"]) or {}
        signals = set(periph.get("dma_requests") or {})
        signals.update(periph.get("dma_routes") or {})
        out.extend(f"{role}.{signal}" for signal in signals)
    return sorted(out)


def _stream_triples(chip: dict[str, Any], periph: str,
                    signal: str) -> list[dict[str, Any]] | None:
    """The signal's `dma_routes` triples when the bound peripheral is the
    STREAM-ENGINE shape (F4/F7 dma_v2): each triple is {controller, stream,
    request}, `stream` is 0-BASED per ST stream numbering (dma_v1's `channel`
    is 1-based — the phase-1 rename keeps the two nouns distinct on purpose),
    and the matched triple supplies the request (CHSEL) exactly as
    `dma_requests` does on G0. None on the G0 shape; `dma_requests` wins if
    data ever stated both, preserving the phase-1 path."""
    p = chip["peripherals"].get(periph) or {}
    if signal in (p.get("dma_requests") or {}):
        return None
    return (p.get("dma_routes") or {}).get(signal)


def _triple_name(triple: dict[str, Any]) -> str:
    """One legal alternative, printed the way design §1 promises:
    "{controller, stream N}"."""
    return f"{{{triple['controller']}, stream {int(triple['stream'])}}}"


def dma_assignment_problems(board: dict[str, Any], chip: dict[str, Any],
                            registers: dict[str, dict[str, Any]]) -> list[dict[str, Any]]:
    """Every problem in the board's `dma:` map, as
    {key, message, suggestions} — deterministic order, all found in one pass.

    TWO silicon shapes, decided per signal at the resolution step below:

    - DMAMUX/free-router (G0/G4, `dma_requests`): the request id is chip-wide
      so ANY channel of ANY dma-class controller may serve it, and the only
      inter-assignment rule is collision. Channels are 1-BASED (dma_v1).
    - Stream engine (F4/F7, `dma_routes` — the phase-3 extension this point
      was marked for): one signal reaches ONLY its triples, so the stated
      (controller, stream) must match one, the matched triple supplies the
      request (CHSEL), and a non-match is refused listing every legal
      "{controller, stream}" (design §1). Streams are 0-BASED per ST stream
      numbering; the errors say which base they mean, both ways.
    """
    problems: list[dict[str, Any]] = []
    assignments = board.get("dma") or {}
    roles = board.get("roles") or {}
    controllers = _dma_class_controllers(chip, registers)
    candidates = dma_signal_candidates(board, chip)
    # (controller, channel) -> first key that claimed it, for the collision rule.
    taken: dict[tuple[str, int], str] = {}

    def bad(key: str, message: str, suggestions: list[str]) -> None:
        problems.append({"key": str(key), "message": message,
                         "suggestions": suggestions})

    for key in sorted(assignments, key=str):
        role_name, dot, signal = str(key).partition(".")
        if not dot or not _DMA_IDENT.match(role_name) or not _DMA_IDENT.match(signal):
            bad(key, "the key must be 'role.signal' (e.g. \"adc.conv\")", candidates)
            continue
        cfg = roles.get(role_name)
        if not isinstance(cfg, dict) or not isinstance(cfg.get("peripheral"), str):
            bad(key, f"'{role_name}' is not a peripheral-bearing role this board "
                     f"declares", candidates)
            continue
        periph = cfg["peripheral"]
        requests = (chip["peripherals"].get(periph) or {}).get("dma_requests") or {}
        triples = _stream_triples(chip, periph, signal)
        if signal not in requests and triples is None:
            role_signals = sorted(c for c in candidates if c.startswith(f"{role_name}."))
            bad(key, f"the chip states no DMA request for {periph} '{signal}'"
                     + ("" if role_signals else
                        f" — {periph} advertises no DMA requests at all"),
                role_signals or candidates)
            continue

        assign = assignments[key]
        if not isinstance(assign, dict):
            want = "stream" if triples is not None else "channel"
            bad(key, f'the assignment must be {{"controller": ..., "{want}": ...}}', [])
            continue
        controller = assign.get("controller")
        if controller not in controllers:
            bad(key, f"'{controller}' is not a DMA controller on this chip",
                controllers)
            continue

        if triples is not None:
            # Stream-engine shape: the board's (controller, stream) must match
            # one of the chip's triples — refusal prints every legal one.
            legal = [_triple_name(t) for t in triples]
            if "channel" in assign and "stream" not in assign:
                bad(key, f"{periph} '{signal}' rides a stream engine: the key "
                         f"is 'stream' (0-based, ST stream numbering), not "
                         f"'channel' (dma_v1's 1-based) — legal: "
                         + ", ".join(legal), legal)
                continue
            stream = assign.get("stream")
            if isinstance(stream, bool) or not isinstance(stream, int):
                bad(key, "'stream' must be an integer (0-based, per ST stream "
                         "numbering)", [])
                continue
            if not any(t["controller"] == controller and int(t["stream"]) == stream
                       for t in triples):
                bad(key, f"{{{controller}, stream {stream}}} does not reach "
                         f"{periph} '{signal}' — the chip's dma_routes allow "
                         f"only: " + ", ".join(legal), legal)
                continue
            prev = taken.setdefault((controller, stream), str(key))
            if prev != str(key):
                free = [_triple_name(t) for t in triples
                        if (t["controller"], int(t["stream"])) not in taken]
                bad(key, f"{controller} stream {stream} already serves "
                         f"'{prev}' — one stream moves one signal", free)
            continue

        # G0 shape (dma_requests): any channel of any dma-class controller.
        if "stream" in assign and "channel" not in assign:
            bad(key, f"{periph} '{signal}' routes through a free router "
                     f"(DMAMUX): the key is 'channel' (1-based, dma_v1 "
                     f"numbering), not 'stream' (the stream engines' 0-based "
                     f"key)", [])
            continue
        channel = assign.get("channel")
        if isinstance(channel, bool) or not isinstance(channel, int):
            bad(key, "'channel' must be an integer", [])
            continue
        # dma_v1 channels are 1-based; `channels.count` is the chip's geometry.
        # A dma-class controller without a count (no geometry curated yet) gets
        # no range check rather than a made-up one.
        count = ((chip["peripherals"][controller].get("channels") or {}).get("count"))
        if isinstance(count, int) and not 1 <= channel <= count:
            bad(key, f"{controller} has channels 1..{count}, not {channel}",
                [str(c) for c in range(1, count + 1)
                 if (controller, c) not in taken])
            continue
        prev = taken.setdefault((controller, channel), str(key))
        if prev != str(key):
            free = [str(c) for c in range(1, (count or channel) + 1)
                    if (controller, c) not in taken]
            bad(key, f"{controller} channel {channel} already serves '{prev}' — "
                     f"one channel moves one stream", free)
    return problems


def _dma_route_type(board: dict[str, Any], chip: dict[str, Any],
                    key: str) -> str | None:
    """The C++ type of one assignment's route constant, or None when the board
    does not assign `key`. ONE spelling for the two places a route type is
    emitted — the `board::dma` constant and the role binder's route parameter —
    so the binder and the constant cannot drift. Assumes the assignment already
    passed dma_assignment_problems (emit_board_header refuses before here)."""
    assign = (board.get("dma") or {}).get(key)
    if not assign:
        return None
    role_name, _, signal = key.partition(".")
    periph = (board.get("roles") or {})[role_name]["peripheral"]
    triples = _stream_triples(chip, periph, signal)
    if triples is None:
        # G0 shape: chip-wide request id, board-chosen channel (1-based).
        number = int(assign["channel"])
        request = int(chip["peripherals"][periph]["dma_requests"][signal])
    else:
        # Stream engine: the route's number is the 0-BASED STREAM and the
        # request (CHSEL) comes from the matched dma_routes triple — the
        # chip's fact, exactly like dma_requests, never the board's.
        number = int(assign["stream"])
        request = next(int(t["request"]) for t in triples
                       if t["controller"] == assign["controller"]
                       and int(t["stream"]) == number)
    return (f"alloy::dma::route<alloy::dev::{assign['controller']}_t, "
            f"{number}, /*request=*/{request}>")


def _dma_binder_tags(board: dict[str, Any], chip: dict[str, Any], role: str,
                     facade: str, indent: int) -> str:
    """The role's `<role>.rx` / `<role>.tx` assignments as binder tag arguments,
    ready to concatenate after `clock_profile`.

    Design §1: the generator emits the constant AND attaches it to the role's
    binder. The attachment is what makes the anchor spellings real —
    `uart.rx_ring(buf)`, `spi.transfer_dma(tx, rx)` — because the handle then
    knows its route without the user naming one, and it is what a PORTABLE
    program probes: `requires { ... }` folds on the binder's dependent alias
    (`Role::rx_route`) and cannot fold on the namespace-scope `board::dma`
    constant (the phase-2 lesson, paid for once).

    ONE helper for every role that carries routes, because phase 4 adds a
    third and fourth facade that does this and a per-role copy is how the
    spellings drift. Route types still come from `_dma_route_type`, so a
    binder tag and its `board::dma` twin are the same text by construction.
    No assignment -> no tag -> the facade's route parameter defaults to void
    and the DMA method is constrained away with a named error instead of a
    missing symbol.
    """
    tags = ""
    for signal in ("rx", "tx"):
        route = _dma_route_type(board, chip, f"{role}.{signal}")
        if route:
            tags += (f",\n{' ' * indent}"
                     f"alloy::{facade}::{signal}_dma<{route}>")
    return tags


def _polarity(active: str) -> str:
    return "alloy::gpio::active_high_t" if active == "high" else "alloy::gpio::active_low_t"


# Every role whose capability is simply "the board declares it". The order is
# the emission order below; `alloy board-info` reports the SAME set, so the IDE
# can never disagree with the generated board::caps.
PRESENCE_ROLES = (
    "led", "button", "debug_uart", "led_pwm", "encoder", "bridge", "adc", "i2c", "spi",
    "eeprom", "watchdog", "window_watchdog", "nvm", "fs", "rtc", "dac", "can",
    "gpio_bus", "ethernet", "tick", "low_power_uart",
)


def role_caps(board: dict[str, Any], chip: dict[str, Any],
              registers: dict[str, dict[str, Any]]) -> dict[str, bool]:
    """board::caps as a plain dict — the one place that decides what a board can
    do. Presence of a role is the capability; the emitter still VALIDATES each
    declared role and fails generation on a bad one, so a cap reported here can
    only be wrong in a board that would not build at all.

    `net` is deliberately absent: it is emitted as a derived constant
    (`ethernet || wifi`) inside the caps namespace, and duplicating it here
    would redefine it in the generated header.
    """
    roles = board.get("roles", {})
    caps = {name: name in roles for name in PRESENCE_ROLES}
    # Interrupt layer: needs a generated vector table (chips without an
    # interrupts list — ESP32 v1 — have no dispatch to attach to).
    caps["irq"] = bool(chip.get("interrupts"))
    # DMA: the chip declares a controller whose IP is class "dma" (st dma1,
    # microchip xdmac, ...).
    caps["dma"] = _dma_controller(chip, registers) is not None
    # Blocks with no wiring and therefore no role: the die either has one or it
    # does not, and no board on that die can disagree. `crc` says the checksum
    # is computed in silicon rather than by alloy::ota::crc::crc32 — the VALUE
    # is the same either way, so this is a speed fact, not a correctness one.
    # `uid` says the part carries a factory identifier.
    caps["crc"] = _chip_singleton(chip, registers, "crc") is not None
    caps["uid"] = _chip_singleton(chip, registers, "uid") is not None
    # Can the low-power UART actually be woken by traffic while the core sleeps?
    # A SECOND cap and not a refinement of the first, because the two are
    # genuinely independent: the role says a board has a spare serial link, this
    # says the block behind it keeps its receiver clocked in Stop mode. Decided
    # by the backing IP, never by the peripheral's NAME — `lpuart1` on an
    # STM32L4 is a usart:v3 block and cannot do this, and its name would have
    # said it could.
    #
    # A note the honest reader is owed: TRUE here means the SILICON has the
    # feature and the driver programs CR1.UESM + CR3.WUS for it. It does not
    # mean alloy can put the core into Stop — there is no low-power entry API
    # yet, so the WFI that would prove any of this is the application's.
    lp = roles.get("low_power_uart")
    lp_ip = ""
    if lp and lp.get("peripheral") in chip.get("peripherals", {}):
        lp_ip = str(chip["peripherals"][lp["peripheral"]].get("ip", ""))
    caps["low_power_uart_wake"] = lp_ip.rsplit("/", 1)[-1].startswith("lpuart_")
    # Vendor-blob radio: no supported family yet, but portable
    # `if constexpr (caps::wifi)` code must still compile.
    caps["wifi"] = False
    return caps


def emit_board_header(board: dict[str, Any], chip: dict[str, Any],
                      registers: dict[str, dict[str, Any]]) -> str:
    roles = board.get("roles", {})
    # Before anything is emitted: no peripheral may be handed to two mutually
    # exclusive personalities. Generation is the earliest place that can see it.
    _check_role_personalities(board)
    # Same doctrine for the DMA map: a channel assignment the chip's routing
    # data contradicts (or two assignments on one channel) never generates.
    # board_validate reports the SAME rules, all at once, with suggestions.
    dma_problems = dma_assignment_problems(board, chip, registers)
    if dma_problems:
        first = dma_problems[0]
        hint = f" (try: {', '.join(first['suggestions'][:6])})" if first["suggestions"] else ""
        raise EmitError(
            f"board {board['id']}: dma '{first['key']}': {first['message']}{hint}")
    # A board may carry an INLINE clock profile (a custom PLL from `alloy clock`
    # / the visual editor) instead of naming one of the chip's curated profiles.
    if isinstance(board.get("clock"), dict):
        profile = board["clock"]
        _require("program" in profile and "sysclk_hz" in profile,
                 f"board {board['id']}: inline clock needs 'program' and 'sysclk_hz'")
    else:
        profile_name = board["clock_profile"]
        profile = chip["clock"]["profiles"].get(profile_name)
        _require(profile is not None,
                 f"board {board['id']}: clock_profile '{profile_name}' not in chip data")
    # Same rule as board_validate._check_external_clock, enforced here too so a
    # board can never GENERATE while depending on a crystal it never declared.
    _require(not uses_external_clock(profile.get("program") or [])
             or isinstance(board.get("hse"), dict),
             f"board {board['id']}: the clock program starts an external "
             "oscillator but the board declares no 'hse' — add "
             '"hse": {"hz": <frequency>, "bypass": <true|false>}')

    # Capabilities come from role_caps() so `alloy board-info` and the generated
    # header can never drift apart. The heavy connectivity roles (ethernet/wifi)
    # differ from the small peripherals in what an ABSENT one emits — a poisoned
    # type rather than a no-op stub — but the capability itself is still just
    # "declared or not".
    caps = role_caps(board, chip, registers)
    decls: list[str] = []

    extra_includes: list[str] = []
    led = roles.get("led")
    if led:
        _require(led["pin"] in chip.get("pins", {}),
                 f"board {board['id']}: led pin '{led['pin']}' not in chip data")
        led_kind = led.get("kind", "gpio")
        if led_kind == "ws2812":
            extra_includes.append("alloy/drivers/ws2812.hpp")
            decls.append(
                f"inline constexpr alloy::drivers::ws2812<alloy::dev::{led['pin']}_t, "
                f"clock_profile> led{{}};"
            )
            # An RGB pixel is not a simple on/off status LED — the uniform
            # accessor stays a no-op so status_led() compiles the same everywhere.
            decls.append(
                "[[nodiscard]] inline alloy::gpio::null_output status_led() { return {}; }"
            )
        elif led_kind == "gpio":
            decls.append(
                f"inline constexpr alloy::gpio::output<alloy::dev::{led['pin']}_t, "
                f"{_polarity(led.get('active', 'high'))}> led{{}};"
            )
            decls.append("[[nodiscard]] inline auto status_led() { return led; }")
        else:
            raise EmitError(f"board {board['id']}: unknown led kind '{led_kind}'")
    else:
        # No board LED — the named accessor still exists so portable code
        # (`board::status_led().on()`) compiles on every board (guard #6).
        decls.append(
            "[[nodiscard]] inline alloy::gpio::null_output status_led() { return {}; }"
        )

    button = roles.get("button")
    if button:
        _require(button["pin"] in chip.get("pins", {}),
                 f"board {board['id']}: button pin '{button['pin']}' not in chip data")
        decls.append(
            f"inline constexpr alloy::gpio::input<alloy::dev::{button['pin']}_t, "
            f"{_polarity(button.get('active', 'high'))}> button{{}};"
        )
        decls.append("[[nodiscard]] inline auto user_button() { return button; }")
    else:
        decls.append(
            "[[nodiscard]] inline alloy::gpio::null_input user_button() { return {}; }"
        )

    # Watchdog: a real handle when wired, a no-op stub otherwise, so
    # `board::watchdog.feed()` compiles on every board (guard #6). On the SAM
    # E70 the WDT is write-once and enabled at reset; the presence of this role
    # makes emit_board_source SKIP the bring-up disable (below) so start() can
    # program it.
    extra_includes.append("alloy/wdt.hpp")
    watchdog = roles.get("watchdog")
    if watchdog:
        _require("peripheral" in watchdog,
                 f"board {board['id']}: watchdog missing 'peripheral'")
        _require(watchdog["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: watchdog peripheral "
                 f"'{watchdog['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, watchdog["peripheral"], "watchdog")
        decls.append(
            f"inline constexpr alloy::wdt::watchdog<alloy::dev::{watchdog['peripheral']}_t> "
            f"watchdog{{}};"
        )
    else:
        decls.append("inline constexpr alloy::wdt::null_watchdog watchdog{};")

    # The WINDOW watchdog, and a role of its own — see src/alloy/wwdt.hpp for
    # the argument. A board may declare this AND `watchdog`: two blocks, two
    # clocks, two different safety properties, and neither implies the other.
    # The declared window reaches generated code as the template defaults, so
    # `board::window_watchdog.start()` programs what the board file says.
    extra_includes.append("alloy/wwdt.hpp")
    wwdg = roles.get("window_watchdog")
    if wwdg:
        _require("peripheral" in wwdg,
                 f"board {board['id']}: window_watchdog missing 'peripheral'")
        _require(wwdg["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: window_watchdog peripheral "
                 f"'{wwdg['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, wwdg["peripheral"], "window_watchdog")
        deadline_us = int(wwdg.get("deadline_us", 0))
        earliest_us = int(wwdg.get("earliest_us", 0))
        # The one relation the emitter can check without knowing the clock
        # tree. Everything else — is this deadline reachable at this PCLK? —
        # is the facade's admit check, which HAS the clock and says so.
        _require(earliest_us < deadline_us or deadline_us == 0,
                 f"board {board['id']}: window_watchdog earliest_us ({earliest_us}) "
                 f"must be below deadline_us ({deadline_us}) — a window that opens "
                 "after it closes admits no feed at all")
        decls.append(
            f"inline constexpr alloy::wwdt::window_watchdog<"
            f"alloy::dev::{wwdg['peripheral']}_t, clock_profile, "
            f"{deadline_us}u, {earliest_us}u> window_watchdog{{}};"
        )
    else:
        decls.append(
            "inline constexpr alloy::wwdt::null_window_watchdog window_watchdog{};")


    # A timer as a bare periodic time base (board::tick). NO PINS: the only
    # board fact is which block is free, and the rate is a project field. The
    # stub carries the SAME SHAPE as a real bind — including an `inst::feat`
    # block of zeroes — because `feat::trgo` is what portable code branches on,
    # and a board with no tick must still compile that branch.
    extra_includes.append("alloy/tick.hpp")
    tick = roles.get("tick")
    if tick:
        _require("peripheral" in tick, f"board {board['id']}: tick missing 'peripheral'")
        _require(tick["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: tick peripheral "
                 f"'{tick['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, tick["peripheral"], "tick")
        decls.append(
            f"using tick = alloy::tick::bind<alloy::dev::{tick['peripheral']}_t,\n"
            f"                               clock_profile>;")
        decls.append(
            "// What THIS PROJECT wants the tick to run at — an application\n"
            "// fact, overridable from alloy.toml, not a property of the PCB.\n"
            "inline constexpr alloy::tick::config tick_defaults{\n"
            f"    .hz = {int(tick.get('hz', 1000))}u,\n"
            "};")
    else:
        decls.append(
            "// No tick timer declared; stub keeps caps-guarded code compiling.\n"
            "struct tick {\n"
            "    struct null_handle {\n"
            "        [[nodiscard]] bool expired() const { return false; }\n"
            "        [[nodiscard]] std::uint16_t count() const { return 0u; }\n"
            "        void restart() const {}\n"
            "        void stop() const {}\n"
            "        void start() const {}\n"
            "        [[nodiscard]] std::uint32_t achieved_hz() const { return 0u; }\n"
            "        [[nodiscard]] std::uint32_t period_ticks() const { return 0u; }\n"
            "        void irq_on_update(bool = true) const {}\n"
            "        void dma_on_update(bool = true) const {}\n"
            "        // A no-op rather than an absent member, and the reason is\n"
            "        // GCC: `if constexpr` in a non-template function still\n"
            "        // CHECKS the discarded branch, and hiding the call in a\n"
            "        // generic lambda does not make it dependent enough to\n"
            "        // escape that. Portable code guarded by `feat::trgo`\n"
            "        // therefore has to compile against this stub even though\n"
            "        // `trgo` here is zero. On a REAL instance the guard is a\n"
            "        // requires-clause on the handle and is enforced.\n"
            "        void trigger_on_update() const {}\n"
            "    };\n"
            "    struct inst {\n"
            "        struct feat {\n"
            "            static constexpr std::uint32_t channels = 0u;\n"
            "            static constexpr std::uint32_t complementary = 0u;\n"
            "            static constexpr std::uint32_t trgo = 0u;\n"
            "            static constexpr std::uint32_t moe = 0u;\n"
            "        };\n"
            "    };\n"
            "    static null_handle open(alloy::tick::config = {}) { return {}; }\n"
            "};\n"
            "inline constexpr alloy::tick::config tick_defaults{};")

    # Flash-backed persistent regions (board::nvm, board::fs) are carved from the
    # TOP of flash so a small app never collides with them. A board may declare
    # both: they STACK — nvm takes the topmost page, the filesystem the region
    # just below it — tracked by a running reserve cursor so they never overlap.
    # The flash HAL turns each absolute base into the erase page/bank at run time.
    extra_includes.append("alloy/flash.hpp")
    extra_includes.append("alloy/util/nvm_kv.hpp")
    extra_includes.append("alloy/fs/littlefs.hpp")
    flash_mem = next((m for m in chip.get("memories", []) if m.get("kind") == "flash"), None)
    flash_base = int(str(flash_mem["base"]), 16) if flash_mem else 0
    flash_top = (flash_base + int(flash_mem["size"])) if flash_mem else 0
    flash_reserved = 0

    erase_size = int(flash_mem.get("erase_size", 0)) if flash_mem else 0

    def _carve(role_name: str, size: int) -> int:
        """Reserve `size` bytes below the running cursor and return the base.
        Fails generation (never the device) if the region is misaligned or runs
        off the bottom of flash — a bad board.json `bytes`."""
        nonlocal flash_reserved
        _require(_region_bytes_ok(size, erase_size),
                 f"board {board['id']}: {role_name} bytes ({size}) must be a positive multiple "
                 f"of the flash erase page ({erase_size} B) — a misaligned region erases across "
                 f"page boundaries and yields a fractional block count")
        base = flash_top - flash_reserved - size
        _require(base >= flash_base,
                 f"board {board['id']}: {role_name} region ({size} B) does not fit — it would "
                 f"start at 0x{base & 0xFFFFFFFF:08X}, below flash base 0x{flash_base:08X} "
                 f"(only {flash_top - flash_base - flash_reserved} B left after other reserved regions)")
        flash_reserved += size
        return base

    # Persistent key/value store over one reserved flash page (board::nvm).
    nvm = roles.get("nvm")
    if nvm:
        _require("peripheral" in nvm, f"board {board['id']}: nvm missing 'peripheral'")
        fp = nvm["peripheral"]
        _require(fp in chip["peripherals"],
                 f"board {board['id']}: nvm peripheral '{fp}' not in chip data")
        _require_curated(board["id"], chip, fp, "nvm")
        if flash_mem is None:
            raise EmitError(f"board {board['id']}: chip declares no flash memory for the nvm region")
        size = int(nvm.get("bytes", _NVM_DEFAULT_BYTES))
        base = _carve("nvm", size)
        decls.append(
            f"inline alloy::nvm::store<alloy::flash::controller<alloy::dev::{fp}_t>> "
            f"nvm{{0x{base:08X}u, {size}u}};"
        )
    else:
        decls.append("inline constexpr alloy::nvm::null_store nvm{};")

    # Filesystem (board::fs): a littlefs instance over a reserved flash region,
    # carved BELOW the nvm page. It needs several erase blocks, so `bytes`
    # defaults much larger than nvm's single page (32 KB).
    fs_role = roles.get("fs")
    if fs_role:
        _require("peripheral" in fs_role, f"board {board['id']}: fs missing 'peripheral'")
        fp = fs_role["peripheral"]
        _require(fp in chip["peripherals"],
                 f"board {board['id']}: fs peripheral '{fp}' not in chip data")
        _require_curated(board["id"], chip, fp, "fs")
        if flash_mem is None:
            raise EmitError(f"board {board['id']}: chip declares no flash memory for the fs region")
        size = int(fs_role.get("bytes", _FS_DEFAULT_BYTES))
        base = _carve("fs", size)
        decls.append(
            f"inline alloy::fs::flash_filesystem<alloy::flash::controller<alloy::dev::{fp}_t>> "
            f"fs{{{{0x{base:08X}u, {size}u}}}};"
        )
    else:
        decls.append("inline alloy::fs::null_filesystem fs{};")

    # Real-time clock (board::rtc). The backup-domain clock (LSI + RCC.BDCR) is
    # brought up by the clock program when this role is present (see
    # emit_board_source); the handle only sets/reads the calendar.
    extra_includes.append("alloy/rtc.hpp")
    rtc = roles.get("rtc")
    if rtc:
        _require("peripheral" in rtc, f"board {board['id']}: rtc missing 'peripheral'")
        _require(rtc["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: rtc peripheral '{rtc['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, rtc["peripheral"], "rtc")
        decls.append(
            f"inline constexpr alloy::rtc::rtc<alloy::dev::{rtc['peripheral']}_t> rtc{{}};")
    else:
        decls.append("inline constexpr alloy::rtc::null_rtc rtc{};")

    # DAC output channel (board::dac). The output pin is fixed by silicon and
    # resets to analog, so the role only names the peripheral.
    extra_includes.append("alloy/dac.hpp")
    dac = roles.get("dac")
    if dac:
        _require("peripheral" in dac, f"board {board['id']}: dac missing 'peripheral'")
        _require(dac["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: dac peripheral '{dac['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, dac["peripheral"], "dac")
        decls.append(
            f"inline constexpr alloy::dac::channel<alloy::dev::{dac['peripheral']}_t> dac{{}};")
    else:
        decls.append("inline constexpr alloy::dac::null_channel dac{};")

    # CAN controller (board::can). Defaults to internal loopback for self-test;
    # the driver reaches its message RAM through the peripheral's companion.
    extra_includes.append("alloy/can.hpp")
    can = roles.get("can")
    if can:
        _require("peripheral" in can, f"board {board['id']}: can missing 'peripheral'")
        _require(can["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: can peripheral '{can['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, can["peripheral"], "can")
        decls.append(
            f"inline constexpr alloy::can::controller<alloy::dev::{can['peripheral']}_t> can{{}};")
    else:
        decls.append("inline constexpr alloy::can::null_controller can{};")

    # Multi-pin GPIO bus (board::gpio_bus): several same-port pins driven/read
    # together in one atomic store. gpio.hpp is already pulled in by the LED role.
    gpio_bus = roles.get("gpio_bus")
    if gpio_bus:
        pins = gpio_bus.get("pins")
        _require(isinstance(pins, list) and len(pins) >= 1,
                 f"board {board['id']}: gpio_bus needs a non-empty 'pins' list")
        for p in pins:
            _require(p in chip.get("pins", {}),
                     f"board {board['id']}: gpio_bus pin '{p}' not in chip data")
        pin_types = ", ".join(f"alloy::dev::{p}_t" for p in pins)
        decls.append(f"inline constexpr alloy::gpio::bus<{pin_types}> gpio_bus{{}};")
    else:
        decls.append("inline constexpr alloy::gpio::null_bus gpio_bus{};")

    uart = roles.get("debug_uart")
    if uart:
        _require("peripheral" in uart, f"board {board['id']}: debug_uart missing 'peripheral'")
        _require(uart["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: debug_uart peripheral '{uart['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, uart["peripheral"], "debug_uart")
        if uart.get("mode") == "rom":
            # Boot-ROM-configured UART (classic ESP32 UART0): no pin routing.
            decls.append(
                f"using debug_uart = alloy::uart::rom_bind<alloy::dev::{uart['peripheral']}_t>;\n"
                f"inline constexpr std::uint32_t debug_uart_baud = {uart.get('baud', 115200)}u;"
            )
        else:
            for key in ("tx", "rx"):
                _require(key in uart, f"board {board['id']}: debug_uart missing '{key}'")
            # The board's `debug_uart.rx`/`.tx` assignments ride on the BINDER
            # as `alloy::uart::rx_dma<>`/`tx_dma<>` tags in the Extra... list
            # — what makes the anchor-2.2/2.3 spellings `uart.rx_ring(rxbuf)`
            # and `uart.write_dma(...)` real. `_dma_binder_tags` is the one
            # place any role does this (see its note).
            rx_tag = _dma_binder_tags(board, chip, "debug_uart", "uart", 37)
            decls.append(
                f"using debug_uart = alloy::uart::bind<alloy::dev::{uart['peripheral']}_t,\n"
                f"                                     alloy::uart::tx<alloy::dev::{uart['tx']}_t>,\n"
                f"                                     alloy::uart::rx<alloy::dev::{uart['rx']}_t>,\n"
                f"                                     clock_profile{rx_tag}>;\n"
                f"inline constexpr std::uint32_t debug_uart_baud = {uart.get('baud', 115200)}u;"
            )
    else:
        decls.append(
            "// This board declares no debug UART; the stub keeps\n"
            "// `if constexpr (board::caps::debug_uart)` code compiling everywhere.\n"
            "// It must carry the SAME SHAPE as a real bind — the Layer-2 `opts`\n"
            "// template parameter and an `inst::feat` block of zeroes — or\n"
            "// portable code that names either stops compiling on the boards\n"
            "// that have no UART, which is exactly the boards it was written to\n"
            "// survive.\n"
            "struct debug_uart {\n"
            "    struct null_handle {\n"
            "        void write(std::uint8_t) const {}\n"
            "        void write(const char*) const {}\n"
            "        bool read(std::uint8_t&) const { return false; }\n"
            "        void flush() const {}\n"
            "    };\n"
            "    struct opts {};\n"
            "    struct inst {\n"
            "        struct feat {\n"
            "            static constexpr std::uint32_t rx_fifo_depth = 0u;\n"
            "            static constexpr std::uint32_t tx_fifo_depth = 0u;\n"
            "        };\n"
            "    };\n"
            "    template <opts = {}>\n"
            "    static null_handle open(alloy::uart::config = {}) { return {}; }\n"
            "};\n"
            "inline constexpr std::uint32_t debug_uart_baud = 0u;"
        )

    # ── The UART that outlives the core's clock ──────────────────────────────
    #
    # Same bind, same handle, same alloy::uart::config as debug_uart: an LPUART
    # is DRIVEN as a UART and the role says nothing else. Three things make it
    # worth emitting separately.
    #
    #   * `de`, an OPTIONAL routed pin, appended as an `alloy::uart::de<>` tag
    #     AFTER clock_profile — the binder's parameter order — so RS-485
    #     driver-enable is one board fact rather than an application's bool.
    #
    #   * `low_power_uart_wake`, a vocabulary that ALWAYS EXISTS. Portable code
    #     names `board::low_power_uart_wake::start_bit` inside an `if constexpr`
    #     and that name is looked up even on boards where the branch is
    #     discarded, so a board with no LPUART still has to spell it. The real
    #     alias is emitted only when the backing IP actually carries the Stop-
    #     mode receiver; otherwise this is a same-shaped enum and the cap below
    #     is false, which is the honest pair — the name resolves, the capability
    #     does not claim to be there.
    #
    #   * The stub, which must carry the SAME SHAPE as a real bind for the same
    #     reason debug_uart's does, plus `open_checked` — the entry point whose
    #     whole purpose here is the compile-time divisor window.
    lp_uart = roles.get("low_power_uart")
    lp_wake_real = False
    if lp_uart:
        _require("peripheral" in lp_uart,
                 f"board {board['id']}: low_power_uart missing 'peripheral'")
        _require(lp_uart["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: low_power_uart peripheral "
                 f"'{lp_uart['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, lp_uart["peripheral"], "low_power_uart")
        for key in ("tx", "rx"):
            _require(key in lp_uart,
                     f"board {board['id']}: low_power_uart missing '{key}'")
        # WHICH IP, not which NAME. `lpuart2` on an STM32L4 is a usart:v3 block
        # and cannot wake anything; the name would have said it could.
        lp_ip = str(chip["peripherals"][lp_uart["peripheral"]].get("ip", ""))
        lp_wake_real = lp_ip.rsplit("/", 1)[-1].startswith("lpuart_")
        de_tag = ""
        if "de" in lp_uart:
            de_tag = (f",\n                                         "
                      f"alloy::uart::de<alloy::dev::{lp_uart['de']}_t>")
        # Same attachment as debug_uart's, keyed `low_power_uart.*`. No
        # shipped board assigns it today; the tags exist so the day one does,
        # the constant and the binder cannot disagree about the route.
        de_tag += _dma_binder_tags(board, chip, "low_power_uart", "uart", 41)
        decls.append(
            f"using low_power_uart = alloy::uart::bind<alloy::dev::{lp_uart['peripheral']}_t,\n"
            f"                                         alloy::uart::tx<alloy::dev::{lp_uart['tx']}_t>,\n"
            f"                                         alloy::uart::rx<alloy::dev::{lp_uart['rx']}_t>,\n"
            f"                                         clock_profile{de_tag}>;\n"
            f"inline constexpr std::uint32_t low_power_uart_baud = "
            f"{lp_uart.get('baud', 115200)}u;"
        )
    else:
        decls.append(
            "// This board declares no low-power UART; the stub keeps\n"
            "// `if constexpr (board::caps::low_power_uart)` code compiling.\n"
            "// It carries `open_checked` as well as `open`, because the whole\n"
            "// point of the role is a baud rate checked against a divisor.\n"
            "struct low_power_uart {\n"
            "    struct null_handle {\n"
            "        void write(std::uint8_t) const {}\n"
            "        void write(const char*) const {}\n"
            "        bool read(std::uint8_t&) const { return false; }\n"
            "        void flush() const {}\n"
            "    };\n"
            "    struct opts {\n"
            "        std::uint8_t data_bits = 8;\n"
            "        bool fifo_enable = false;\n"
            "        std::uint8_t de_assert_16ths = 8;\n"
            "        std::uint8_t de_deassert_16ths = 8;\n"
            "        low_power_uart_wake wake_from_stop = low_power_uart_wake::off;\n"
            "    };\n"
            "    struct inst {\n"
            "        struct feat {\n"
            "            static constexpr std::uint32_t rx_fifo_depth = 0u;\n"
            "            static constexpr std::uint32_t tx_fifo_depth = 0u;\n"
            "        };\n"
            "    };\n"
            "    static constexpr bool has_de = false;\n"
            "    static constexpr std::uint32_t kernel_hz() { return 0u; }\n"
            "    template <opts = {}>\n"
            "    static null_handle open(alloy::uart::config = {}) { return {}; }\n"
            "    template <alloy::frequency, std::uint32_t = 20, opts = {}>\n"
            "    static null_handle open_checked(alloy::uart::frame = {}) { return {}; }\n"
            "};\n"
            "inline constexpr std::uint32_t low_power_uart_baud = 0u;"
        )
    # Emitted BEFORE the block above in the header, so `opts` can name it.
    if lp_wake_real:
        decls.insert(
            len(decls) - 1,
            "// The backing IP is an LPUART, so the wake vocabulary is the\n"
            "// driver's own — three CR3.WUS encodings whose values live in the\n"
            "// curated data, not here.\n"
            "using low_power_uart_wake = alloy::uart::lpuart_wake;")
    else:
        decls.insert(
            len(decls) - 1,
            "// No Stop-mode receiver behind this role (or no role at all), so\n"
            "// this is the NAME without the capability: portable code still\n"
            "// compiles, and `board::caps::low_power_uart_wake` is false.\n"
            "enum class low_power_uart_wake : std::uint8_t {\n"
            "    off, address_match, start_bit, rx_not_empty };")

    led_pwm = roles.get("led_pwm")
    if led_pwm:
        for key in ("peripheral", "channel", "pin"):
            _require(key in led_pwm, f"board {board['id']}: led_pwm missing '{key}'")
        _require(led_pwm["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: led_pwm peripheral '{led_pwm['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, led_pwm["peripheral"], "led_pwm")
        ch = led_pwm["channel"]
        decls.append(
            f"using led_pwm = alloy::pwm::bind<alloy::dev::{led_pwm['peripheral']}_t, {ch}u,\n"
            f"                                 alloy::dev::{led_pwm['pin']}_t,\n"
            f"                                 alloy::signal::ch{ch}, clock_profile>;"
        )
    else:
        decls.append(
            "// No PWM-capable LED declared; stub keeps caps-guarded code compiling.\n"
            "struct led_pwm {\n"
            "    struct null_handle {\n"
            "        void set_duty(std::uint16_t) const {}\n"
            "        void off() const {}\n"
            "    };\n"
            "    static null_handle open(alloy::pwm::config = {}) { return {}; }\n"
            "};"
        )

    # THE SAME BLOCK'S OTHER PERSONALITY, and it is emitted right after
    # led_pwm on purpose: these two aliases are what _check_role_personalities
    # above refuses to emit together for one peripheral.
    encoder_role = roles.get("encoder")
    if encoder_role:
        for key in ("peripheral", "a", "b"):
            _require(key in encoder_role,
                     f"board {board['id']}: encoder role missing '{key}'")
        _require(encoder_role["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: encoder peripheral "
                 f"'{encoder_role['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, encoder_role["peripheral"], "encoder")
        # No clock_profile: an encoder divides no kernel clock, so its binder
        # does not take one (see src/alloy/encoder.hpp).
        decls.append(
            f"using encoder = alloy::encoder::bind<alloy::dev::{encoder_role['peripheral']}_t,\n"
            f"                                     alloy::encoder::a<alloy::dev::{encoder_role['a']}_t>,\n"
            f"                                     alloy::encoder::b<alloy::dev::{encoder_role['b']}_t>>;"
        )
        decls.append(
            "// What THIS PROJECT plugged into the connector — an application\n"
            "// fact, overridable from alloy.toml, not a property of the PCB.\n"
            "inline constexpr alloy::encoder::config encoder_defaults{\n"
            f"    .period = {int(encoder_role.get('period', 65536))}u,\n"
            f"    .reverse = {'true' if encoder_role.get('reverse') else 'false'},\n"
            "};"
        )
    else:
        decls.append(
            "// No encoder declared; stub keeps caps-guarded code compiling.\n"
            "struct encoder {\n"
            "    struct null_handle {\n"
            "        [[nodiscard]] std::uint32_t count() const { return 0u; }\n"
            "        [[nodiscard]] alloy::encoder::direction direction() const {\n"
            "            return alloy::encoder::direction::up;\n"
            "        }\n"
            "        [[nodiscard]] std::int32_t delta() { return 0; }\n"
            "        void reset() {}\n"
            "        [[nodiscard]] std::uint32_t period() const { return 0u; }\n"
            "    };\n"
            "    static null_handle open(alloy::encoder::config = {}) { return {}; }\n"
            "};\n"
            "inline constexpr alloy::encoder::config encoder_defaults{};"
        )

    # THE TIMER'S THIRD PERSONALITY, and the one whose board file can destroy
    # hardware. Emitted right after the other two on purpose: these three
    # aliases are what _check_role_personalities refuses to emit together for
    # one peripheral.
    bridge_role = roles.get("bridge")
    if bridge_role:
        for key in ("peripheral", "ah", "al", "dead_time_ns"):
            _require(key in bridge_role,
                     f"board {board['id']}: bridge role missing '{key}'")
        _require(bridge_role["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: bridge peripheral "
                 f"'{bridge_role['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, bridge_role["peripheral"], "bridge")
        # A PHASE IS A PAIR, AND HALF A PAIR IS THE DANGEROUS SHAPE. A board
        # that names `bh` without `bl` has declared a high side whose low side
        # is driven by something this program does not know about — and the
        # dead time it configures then protects nothing. Refused here, where
        # the message can name the board file, rather than at the C++ level
        # where it would be a missing template argument.
        phase_pins: list[tuple[str, str]] = []
        for ordinal, (hi, lo) in enumerate((("ah", "al"), ("bh", "bl"), ("ch", "cl")), 1):
            if hi not in bridge_role and lo not in bridge_role:
                break
            _require(hi in bridge_role and lo in bridge_role,
                     f"board {board['id']}: bridge phase {ordinal} names "
                     f"'{hi if hi in bridge_role else lo}' and not "
                     f"'{lo if hi in bridge_role else hi}' — a bridge phase is "
                     "a PAIR of pins, and half of one is a half-bridge whose "
                     "other transistor nothing here controls")
            phase_pins.append((bridge_role[hi], bridge_role[lo]))
        _require(bool(phase_pins),
                 f"board {board['id']}: bridge role declares no phase pins")
        _require(bridge_role.get("break_active", "low") in ("low", "high"),
                 f"board {board['id']}: bridge break_active must be 'low' or "
                 f"'high' — it is the level that MEANS fault")
        if "brk" in bridge_role:
            polarity = ("alloy::bridge::break_polarity::active_high"
                        if bridge_role.get("break_active", "low") == "high"
                        else "alloy::bridge::break_polarity::active_low")
            break_tag = (f"alloy::bridge::brk<alloy::dev::{bridge_role['brk']}_t,\n"
                         f"                    {polarity}>")
        else:
            # No fault line on this board. The tag is named `unprotected` and
            # it is generated in full, in the bind, because "this inverter has
            # no hardware protection" is a sentence somebody should have to
            # read.
            break_tag = "alloy::bridge::unprotected"
        phase_tags = ",\n".join(
            f"    alloy::bridge::phase<{i}, alloy::dev::{hi}_t, alloy::dev::{lo}_t>"
            for i, (hi, lo) in enumerate(phase_pins, 1))
        decls.append(
            f"using bridge = alloy::bridge::bind<\n"
            f"    alloy::dev::{bridge_role['peripheral']}_t, clock_profile,\n"
            f"    {break_tag},\n{phase_tags}>;"
        )
        decls.append(
            "// What THIS PROJECT's power stage needs — the switching rate and\n"
            "// the gate driver's dead time. The FREQUENCY is an application\n"
            "// fact and is overridable from alloy.toml. The DEAD TIME is not:\n"
            "// it is the gate driver's turn-off delay plus the transistors'\n"
            "// fall time, both soldered to this PCB, and letting a project\n"
            "// LOWER it from a toml file is the one edit that destroys a\n"
            "// bridge. A different dead time means a different power stage.\n"
            "//\n"
            "// THE DEAD TIME HAS NO DEFAULT ANYWHERE IN THIS CHAIN. board.json\n"
            "// must state it (roles.py lists it as required), and\n"
            "// alloy::bridge::config's own default is the `unstated` value,\n"
            "// which is a compile error at open(). A number invented by a\n"
            "// generator is exactly the number that would be wrong on the next\n"
            "// board.\n"
            "inline constexpr alloy::bridge::config bridge_defaults{\n"
            f"    .freq_hz = {int(bridge_role.get('freq_hz', 20000))}u,\n"
            f"    .dead_time = alloy::bridge::dead_time::ns({int(bridge_role['dead_time_ns'])}u),\n"
            "};"
        )
    else:
        decls.append(
            "// No bridge declared; stub keeps caps-guarded code compiling.\n"
            "//\n"
            "// THE STUB CARRIES EVERY ENTRY POINT A REAL BIND HAS, and\n"
            "// open_checked<> is the one that is easy to forget: an\n"
            "// `if constexpr (board::caps::bridge)` in a non-template main()\n"
            "// still PARSES its discarded branch, so a missing member\n"
            "// template turns `open_checked<cfg>()` into a less-than\n"
            "// comparison and the error a user sees is\n"
            "// \"expected primary-expression before ')'\" on a board that has\n"
            "// no bridge at all. `config` is aliased for the same reason: the\n"
            "// call site names the type.\n"
            "struct bridge {\n"
            "    using config = alloy::bridge::config;\n"
            "    struct null_handle {\n"
            "        static constexpr unsigned phases = 0;\n"
            "        template <unsigned N>\n"
            "        void set_duty(std::uint16_t) const {}\n"
            "        void enable_outputs() const {}\n"
            "        void disable_outputs() const {}\n"
            "        [[nodiscard]] bool outputs_enabled() const { return false; }\n"
            "        [[nodiscard]] bool faulted() const { return false; }\n"
            "        void acknowledge_fault() const {}\n"
            "        void emergency_stop() const {}\n"
            "        [[nodiscard]] std::uint32_t period_ticks() const { return 0u; }\n"
            "        [[nodiscard]] std::uint32_t dead_time_ns() const { return 0u; }\n"
            "    };\n"
            "    static null_handle open(alloy::bridge::config = {}) { return {}; }\n"
            "    template <alloy::bridge::config = {}>\n"
            "    static null_handle open_checked() { return {}; }\n"
            "};\n"
            "inline constexpr alloy::bridge::config bridge_defaults{};"
        )

    i2c_role = roles.get("i2c")
    if i2c_role:
        for key in ("peripheral", "scl", "sda"):
            _require(key in i2c_role, f"board {board['id']}: i2c role missing '{key}'")
        _require(i2c_role["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: i2c peripheral '{i2c_role['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, i2c_role["peripheral"], "i2c")
        decls.append(
            f"using i2c = alloy::i2c::bind<alloy::dev::{i2c_role['peripheral']}_t,\n"
            f"                             alloy::i2c::scl<alloy::dev::{i2c_role['scl']}_t>,\n"
            f"                             alloy::i2c::sda<alloy::dev::{i2c_role['sda']}_t>,\n"
            f"                             clock_profile>;"
        )
    else:
        decls.append(
            "// No I2C role declared; stub keeps caps-guarded code compiling.\n"
            "struct i2c {\n"
            "    struct null_handle {\n"
            "        bool write(std::uint8_t, std::span<const std::uint8_t>) const { return false; }\n"
            "        bool read(std::uint8_t, std::span<std::uint8_t>) const { return false; }\n"
            "        bool write_read(std::uint8_t, std::span<const std::uint8_t>,\n"
            "                        std::span<std::uint8_t>) const { return false; }\n"
            "        bool probe(std::uint8_t) const { return false; }\n"
            "    };\n"
            "    static null_handle open(alloy::i2c::config = {}) { return {}; }\n"
            "};"
        )

    spi_role = roles.get("spi")
    if spi_role:
        for key in ("peripheral", "sck", "miso", "mosi"):
            _require(key in spi_role, f"board {board['id']}: spi role missing '{key}'")
        _require(spi_role["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: spi peripheral '{spi_role['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, spi_role["peripheral"], "spi")
        decls.append(
            f"using spi = alloy::spi::bind<alloy::dev::{spi_role['peripheral']}_t,\n"
            f"                             alloy::spi::sck<alloy::dev::{spi_role['sck']}_t>,\n"
            f"                             alloy::spi::miso<alloy::dev::{spi_role['miso']}_t>,\n"
            f"                             alloy::spi::mosi<alloy::dev::{spi_role['mosi']}_t>,\n"
            f"                             clock_profile>;"
        )
        if "cs" in spi_role:
            _require(spi_role["cs"] in chip.get("pins", {}),
                     f"board {board['id']}: spi cs pin '{spi_role['cs']}' not in chip data")
            decls.append(
                "inline constexpr bool spi_has_cs = true;\n"
                "// Chip-select is active-low; drivers use set_high/set_low directly.\n"
                f"inline constexpr alloy::gpio::output<alloy::dev::{spi_role['cs']}_t, "
                f"alloy::gpio::active_low_t> spi_cs{{}};"
            )
        else:
            decls.append(
                "inline constexpr bool spi_has_cs = false;\n"
                "inline constexpr alloy::gpio::null_output spi_cs{};"
            )
    else:
        decls.append(
            "// No SPI role declared; stub keeps caps-guarded code compiling.\n"
            "struct spi {\n"
            "    struct null_handle {\n"
            "        std::uint8_t xfer(std::uint8_t) const { return 0u; }\n"
            "        void write(std::span<const std::uint8_t>) const {}\n"
            "        void transfer(std::span<std::uint8_t>) const {}\n"
            "    };\n"
            "    static null_handle open(alloy::spi::config = {}) { return {}; }\n"
            "};\n"
            "inline constexpr bool spi_has_cs = false;\n"
            "inline constexpr alloy::gpio::null_output spi_cs{};"
        )

    eeprom_role = roles.get("eeprom")
    if eeprom_role:
        _require("addr" in eeprom_role, f"board {board['id']}: eeprom role missing 'addr'")
        _require(eeprom_role.get("bus", "i2c") == "i2c",
                 f"board {board['id']}: eeprom role only supports bus 'i2c' for now")
        _require("i2c" in roles,
                 f"board {board['id']}: eeprom role needs the i2c role on the same board")
        decls.append(
            f"inline constexpr std::uint8_t eeprom_addr = {int(eeprom_role['addr'], 16)}u;\n"
            f"// 0 = the part has no separate identity/serial block.\n"
            f"inline constexpr std::uint8_t eeprom_id_addr = {int(eeprom_role.get('id_addr', '0x0'), 16)}u;\n"
            f"inline constexpr std::uint8_t eeprom_page_size = {eeprom_role.get('page_size', 16)}u;\n"
            f"inline constexpr std::uint16_t eeprom_bytes = {eeprom_role.get('bytes', 256)}u;"
        )
        if "wp" in eeprom_role:
            _require(eeprom_role["wp"] in chip.get("pins", {}),
                     f"board {board['id']}: eeprom wp pin '{eeprom_role['wp']}' not in chip data")
            decls.append(
                "// Drive LOW to enable EEPROM writes (WP is active-high protect).\n"
                "inline constexpr bool eeprom_has_wp = true;\n"
                f"inline constexpr alloy::gpio::output<alloy::dev::{eeprom_role['wp']}_t, "
                f"alloy::gpio::active_high_t> eeprom_wp{{}};"
            )
        else:
            decls.append(
                "inline constexpr bool eeprom_has_wp = false;\n"
                "inline constexpr alloy::gpio::null_output eeprom_wp{};"
            )
    else:
        decls.append(
            "// No EEPROM declared; zeros keep caps-guarded code compiling.\n"
            "inline constexpr std::uint8_t eeprom_addr = 0u;\n"
            "inline constexpr std::uint8_t eeprom_id_addr = 0u;\n"
            "inline constexpr std::uint8_t eeprom_page_size = 0u;\n"
            "inline constexpr std::uint16_t eeprom_bytes = 0u;\n"
            "inline constexpr bool eeprom_has_wp = false;\n"
            "inline constexpr alloy::gpio::null_output eeprom_wp{};"
        )

    adc_role = roles.get("adc")
    if adc_role:
        _require("peripheral" in adc_role, f"board {board['id']}: adc role missing 'peripheral'")
        periph_name = adc_role["peripheral"]
        _require(periph_name in chip["peripherals"],
                 f"board {board['id']}: adc peripheral '{periph_name}' not in chip data")
        # The board's `adc.conv` DMA assignment rides on the BINDER (design §1:
        # the generator "attaches it to the role's binder"), which is what makes
        # the anchor spelling `adc.ring(samples)` real — the handle knows its
        # route without the user naming one. No assignment -> the parameter
        # defaults to void and ring() is constrained away, a named compile
        # error at the facade's requires-gate.
        adc_conv_route = _dma_route_type(board, chip, "adc.conv")
        if adc_conv_route:
            decls.append(
                f"using adc = alloy::adc::bind<alloy::dev::{periph_name}_t, "
                f"clock_profile, {adc_conv_route}>;"
            )
        else:
            decls.append(
                f"using adc = alloy::adc::bind<alloy::dev::{periph_name}_t, clock_profile>;"
            )
        channels = chip["peripherals"][periph_name].get("channels", {})
        for chname in ("vref", "temp"):
            present = chname in channels
            decls.append(
                f"inline constexpr bool adc_has_{chname} = {'true' if present else 'false'};\n"
                f"inline constexpr std::uint8_t adc_{chname}_channel = {channels.get(chname, 0)}u;"
            )
    else:
        decls.append(
            "// No ADC role declared; stub keeps caps-guarded code compiling.\n"
            "// It carries the SAME SHAPE as a real bind, degree included: a\n"
            "// program that branches on `board::adc::watchdogs` must compile on\n"
            "// the boards that have no ADC at all, which is exactly the set that\n"
            "// branch was written to survive. Zero means absent, here as\n"
            "// everywhere else.\n"
            "struct adc {\n"
            "    struct null_handle {\n"
            "        std::uint16_t read(std::uint8_t) const { return 0u; }\n"
            "    };\n"
            "    static constexpr unsigned watchdogs = 0u;\n"
            "    static null_handle open(alloy::adc::config = {}) { return {}; }\n"
            "};\n"
            "inline constexpr bool adc_has_vref = false;\n"
            "inline constexpr bool adc_has_temp = false;\n"
            "inline constexpr std::uint8_t adc_vref_channel = 0u;\n"
            "inline constexpr std::uint8_t adc_temp_channel = 0u;"
        )

    dma_name = _dma_controller(chip, registers)
    if dma_name:
        decls.append(f"using dma_t = alloy::dev::{dma_name}_t;")
    else:
        decls.append(
            "// No DMA controller in this chip's data yet; the tag keeps\n"
            "// caps-guarded generic lambdas compiling (never instantiated).\n"
            "struct dma_t {};"
        )

    # DMA routes — the board's channel assignments (board.json "dma"), already
    # validated against the chip's request routing up top. One typed constant
    # per assignment; the REQUEST id comes from the chip's `dma_requests`,
    # never from the board file. The namespace is emitted on EVERY board so a
    # `requires { board::dma::adc_conv; }` probe is always well-formed: a board
    # that assigns nothing gets the empty namespace, and a facade's
    # requires-gated stream method reports the missing route by name at compile
    # time instead of misrouting silently.
    route_decls: list[str] = []
    for key in sorted(board.get("dma") or {}):
        role_name, _, signal = key.partition(".")
        periph = roles[role_name]["peripheral"]
        # On stream engines the route's number is the 0-BASED STREAM (ST
        # numbering) and the request is the matched triple's CHSEL — say so
        # at the constant, where a reader will compare it against the RM.
        shape = (" (0-based stream, CHSEL from the matched dma_routes triple)"
                 if _stream_triples(chip, periph, signal) is not None else "")
        route_decls.append(
            f"inline constexpr {_dma_route_type(board, chip, key)} "
            f"{role_name}_{signal}{{}};  // serves {periph} {signal}{shape}"
        )
    if route_decls:
        extra_includes.append("alloy/dma.hpp")
    decls.append(
        "// Board-assigned DMA routes (see board.json \"dma\"); empty when the\n"
        "// board assigns none.\n"
        "namespace dma {\n" + "".join(f"{d}\n" for d in route_decls) +
        "}  // namespace dma"
    )

    # --- CRC unit and factory device ID: no role, no pins, no board.json. ---
    #
    # Both are emitted for EVERY board, present or not, because both facades
    # answer with something real when the silicon has nothing. `board::crc` on a
    # chip without the block opens the SOFTWARE CRC-32 — provably the same
    # number (tests/test_crc.cpp), so the substitution is invisible and allowed
    # to be. `board::uid` on a chip without one reads an identifier of ZERO
    # words, which is the framework's "0 means absent" degree rule applied to a
    # whole value rather than to a count. Neither needs `if constexpr` at the
    # call site; `caps::crc` / `caps::uid` are there for a program that wants to
    # SAY which it got.
    extra_includes.append("alloy/crc.hpp")
    crc_name = _chip_singleton(chip, registers, "crc")
    if crc_name:
        decls.append(f"inline constexpr alloy::crc::engine<alloy::dev::{crc_name}_t> crc{{}};")
    else:
        decls.append(
            "// No CRC block on this chip; open() returns the software CRC-32,\n"
            "// which is the same function and the same value.\n"
            "inline constexpr alloy::crc::software_engine crc{};"
        )

    extra_includes.append("alloy/uid.hpp")
    uid_name = _chip_singleton(chip, registers, "uid")
    if uid_name:
        decls.append(f"inline constexpr alloy::uid::reader<alloy::dev::{uid_name}_t> uid{{}};")
    else:
        decls.append(
            "// No factory device ID on this chip; read() returns the empty\n"
            "// identifier (zero words), never a fabricated one.\n"
            "inline constexpr alloy::uid::null_reader uid{};"
        )

    # --- ethernet role (M0: records facts + sets caps; the NetDevice bind
    # is M1). Poisoned board::eth when absent keeps zero-ifdef code honest. ---
    eth = roles.get("ethernet")
    if eth:
        _require("peripheral" in eth, f"board {board['id']}: ethernet role missing 'peripheral'")
        _require(eth["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: ethernet peripheral '{eth['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, eth["peripheral"], "ethernet")
        phy = eth.get("phy", {})
        _require("kind" in phy, f"board {board['id']}: ethernet.phy missing 'kind'")
        _require("reset_pin" in eth, f"board {board['id']}: ethernet role missing 'reset_pin'")
        _require(eth["reset_pin"] in chip.get("pins", {}),
                 f"board {board['id']}: ethernet reset_pin '{eth['reset_pin']}' not in chip data")
        # RMII pins + AF are a chip fact (the MAC's fixed pin bank), not board
        # config — read them from chip data so no silicon AF lives in board.json.
        rmii_spec = chip["peripherals"][eth["peripheral"]].get("rmii")
        _require(rmii_spec is not None,
                 f"board {board['id']}: chip peripheral '{eth['peripheral']}' has no "
                 "'rmii' pin bank in chip data")
        rmii = rmii_spec["pins"]
        for pn in rmii:
            _require(pn in chip.get("pins", {}),
                     f"chip {chip['part']}: gmac rmii pin '{pn}' not in chip 'pins'")
        pin_cfg = "".join(
            f"    alloy::hal::pin_impl<alloy::dev::{pn}_t>::make_af({rmii_spec['af']});\n"
            for pn in rmii
        )
        # Some MACs select RMII in a register OUTSIDE their own block (STM32: in
        # SYSCFG). That address is a chip fact, carried as data and emitted here
        # into the generated eth_configure_pins() — so no silicon address lives in
        # the hand-written HAL. MACs whose select is in-block (GMAC's UR) omit it.
        sel = chip["peripherals"][eth["peripheral"]].get("rmii_select")
        sel_cfg = ""
        if sel:
            clk = sel["clock"]
            sel_cfg = (
                "    // Enable the select register's clock, then choose RMII (chip data).\n"
                f"    *reinterpret_cast<volatile std::uint32_t*>({clk['addr']}u) |= "
                f"(std::uint32_t{{1}} << {clk['bit']}u);\n"
                f"    *reinterpret_cast<volatile std::uint32_t*>({sel['addr']}u) |= "
                f"(std::uint32_t{{1}} << {sel['bit']}u);\n"
            )
        # The MAC HAL is selected by IP version, exactly like every other
        # peripheral (facts generated, behavior per-IP): the include is
        # alloy/hal/net/<vendor>_<ip>.hpp and the type is alloy::hal::<ip-stem>
        # (gmac_v1 -> alloy::hal::gmac, eth_v1 -> alloy::hal::eth). A second MAC
        # family plugs in with data + its HAL, no change here.
        eth_ip = chip["peripherals"][eth["peripheral"]]["ip"]  # e.g. "microchip/gmac_v1"
        eth_include, eth_hal = _eth_mac_names(eth_ip)
        extra_includes.append(f"alloy/hal/net/{eth_include}.hpp")
        extra_includes.append(f"alloy/drivers/net/{phy['kind']}.hpp")
        decls.append(
            f"// Ethernet: {eth_hal} MAC ({eth_ip}) + {phy['kind']} PHY (addr "
            f"{phy.get('addr', 0)}, {phy.get('mode', 'rmii')}).\n"
            f"using eth_mac_t = alloy::hal::{eth_hal}<alloy::dev::{eth['peripheral']}_t>;\n"
            f"inline eth_mac_t eth{{}};\n"
            f"using eth_reset_t = alloy::gpio::output<alloy::dev::{eth['reset_pin']}_t, "
            f"alloy::gpio::active_low_t>;\n"
            f"inline constexpr eth_reset_t eth_reset{{}};\n"
            f"inline constexpr std::uint8_t eth_phy_addr = {phy.get('addr', 0)}u;\n"
            f"inline alloy::drivers::{phy['kind']}<eth_mac_t, eth_reset_t> eth_phy{{\n"
            f"    eth, eth_reset, eth_phy_addr}};\n"
            f"// Route the RMII pads to the MAC (peripheral function) + select RMII —\n"
            f"// call once before eth.begin_mdio().\n"
            f"inline void eth_configure_pins() {{\n{sel_cfg}{pin_cfg}}}"
        )
    else:
        # No Ethernet: no-op stubs (same doctrine as the led_pwm/i2c/spi
        # stubs) so `if constexpr (caps::ethernet)`-guarded portable code
        # compiles everywhere. caps::ethernet=false is the honest signal;
        # the stub methods are never reached at runtime.
        decls.append(
            "inline constexpr std::uint8_t eth_phy_addr = 0u;\n"
            "struct eth_mac_stub {\n"
            "    void begin_mdio() {}\n"
            "    void start(const std::uint8_t*, bool, bool) {}\n"
            "    std::uint16_t read(std::uint8_t, std::uint8_t) const { return 0; }\n"
            "    void write(std::uint8_t, std::uint8_t, std::uint16_t) const {}\n"
            "    std::uint32_t receive(std::span<std::uint8_t>) { return 0; }\n"
            "    bool transmit(std::span<const std::uint8_t>) { return false; }\n"
            "    bool link_up() const { return false; }\n"
            "    std::uint32_t mtu() const { return 0; }\n"
            "};\n"
            "inline eth_mac_stub eth{};\n"
            "struct eth_phy_stub {\n"
            "    bool init() const { return false; }\n"
            "    bool link_up() const { return false; }\n"
            "    bool speed_100() const { return false; }\n"
            "    bool full_duplex() const { return false; }\n"
            "};\n"
            "inline constexpr eth_phy_stub eth_phy{};\n"
            "inline void eth_configure_pins() {}"
        )
    decls.append(
        "// Umbrella capability: any owned-MAC or vendor-blob link.\n"
        "namespace caps { inline constexpr bool net = ethernet || wifi; }"
    )

    # User-named pins from the visual configurator (board.json "pins"): each
    # entry becomes a code alias under board::pins so the label on the pin map IS
    # the identifier in firmware. gpio_out/gpio_in emit ready-to-use gpio
    # handles; an alternate-function assignment ("periph:signal") is validated
    # against the chip's route table (a function the pin can't do is a GENERATION
    # error naming the pin, same spirit as guard #7) and emits the pin-type alias
    # for use in explicit bind<>s.
    pin_decls: list[str] = []
    for pin, assign in sorted((board.get("pins") or {}).items()):
        _require(pin in chip.get("pins", {}),
                 f"board {board['id']}: named pin '{pin}' not in chip data")
        fn = assign.get("function", "")
        label = assign.get("label")
        if label is not None and not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", label):
            raise EmitError(f"board {board['id']}: pin '{pin}' label '{label}' is "
                            f"not a valid identifier")
        if fn in ("gpio_out", "gpio_in"):
            if label:
                tmpl = "output" if fn == "gpio_out" else "input"
                pin_decls.append(
                    f"inline constexpr alloy::gpio::{tmpl}<alloy::dev::{pin}_t, "
                    f"alloy::gpio::active_high_t> {label}{{}};  // {pin}")
        elif ":" in fn:
            periph, _, signal = fn.partition(":")
            routes = chip.get("routes") or []
            _require(any(r.get("pin") == pin and r.get("peripheral") == periph
                         and r.get("signal") == signal for r in routes),
                     f"board {board['id']}: pin '{pin}' has no route to "
                     f"{periph} {signal} on this chip")
            if label:
                pin_decls.append(
                    f"using {label} = alloy::dev::{pin}_t;  // {pin}: {periph} {signal}")
        else:
            raise EmitError(f"board {board['id']}: pin '{pin}' has unknown "
                            f"function '{fn}'")
    if pin_decls:
        pins_body = "\n".join(pin_decls)
        decls.append("// Named pins from the board's pin map (see board.json \"pins\").\n"
                     f"namespace pins {{\n{pins_body}\n}}  // namespace pins")

    caps_body = "\n".join(
        f"inline constexpr bool {name} = {'true' if value else 'false'};"
        for name, value in sorted(caps.items())
    )
    decl_body = "\n\n".join(decls)
    extra_include_block = "".join(f'\n#include "{inc}"' for inc in sorted(extra_includes))

    return f"""{BANNER}// Board: {board['id']} ({board.get('name', '')})
#pragma once

#include <cstdint>
#include <span>

#include "alloy/adc.hpp"
#include "alloy/bridge.hpp"
#include "alloy/device.hpp"
#include "alloy/encoder.hpp"
#include "alloy/gpio.hpp"
#include "alloy/i2c.hpp"
#include "alloy/pwm.hpp"
#include "alloy/routes_gen.hpp"
#include "alloy/spi.hpp"
#include "alloy/time.hpp"
#include "alloy/uart.hpp"{extra_include_block}

namespace board {{

struct clock_profile {{
    static constexpr std::uint32_t sysclk_hz = {profile['sysclk_hz']}u;
    static constexpr std::uint32_t ahb_hz = {profile['ahb_hz']}u;
    static constexpr std::uint32_t apb_hz = {profile['apb_hz']}u;
    // Mirrors apb_hz when the chip data declares no second APB bus.
    static constexpr std::uint32_t apb2_hz = {profile.get('apb2_hz', profile['apb_hz'])}u;
    // The board's external oscillator, 0 when it has none. Peripherals that can
    // be clocked straight off it (CAN bit timing, MCO, USB) read it from here
    // instead of assuming the PLL source.
    static constexpr std::uint32_t hse_hz = {int((board.get('hse') or {}).get('hz', 0))}u;
}};
inline constexpr std::uint32_t system_clock_hz = clock_profile::sysclk_hz;

namespace caps {{
{caps_body}
}}  // namespace caps

{decl_body}

// Clocks + timebase + role pins; returns false if the clock program timed
// out and the board is running on the boot clock instead.
bool init();

}}  // namespace board
"""


def _resolve_step(chip: dict[str, Any], registers: dict[str, dict[str, Any]],
                  op: dict[str, Any]) -> str:
    if op["op"] == "delay":
        return (f"    alloy::clock_step{{alloy::clock_step::op::delay, 0u, 0u, "
                f"{op['us']}u, 0u}},")

    periph = chip["peripherals"][op["peripheral"]]
    ip_doc = registers[periph["ip"]]
    reg = register_by_name(ip_doc, op["register"])
    addr = int(periph["base"], 16) + int(reg["offset"], 16)

    if op["op"] == "write":
        return (f"    alloy::clock_step{{alloy::clock_step::op::write, 0x{addr:08X}u, "
                f"0xFFFFFFFFu, {op['value']}u, 0u}},")
    if op["op"] == "rmw":
        mask = 0
        value = 0
        for fname, fval in op["fields"].items():
            bit, width = field_lookup(reg, fname)
            fmask = ((1 << width) - 1) << bit
            mask |= fmask
            value |= (fval << bit) & fmask
        return (f"    alloy::clock_step{{alloy::clock_step::op::rmw, 0x{addr:08X}u, "
                f"0x{mask:08X}u, 0x{value:08X}u, 0u}},")
    if op["op"] == "poll":
        bit, width = field_lookup(reg, op["field"])
        mask = ((1 << width) - 1) << bit
        value = (op["equals"] << bit) & mask
        return (f"    alloy::clock_step{{alloy::clock_step::op::poll, 0x{addr:08X}u, "
                f"0x{mask:08X}u, 0x{value:08X}u, {op['timeout_us']}u}},")
    raise EmitError(f"unknown clock op {op['op']}")


def emit_board_source(board: dict[str, Any], chip: dict[str, Any],
                      registers: dict[str, dict[str, Any]], arch_ns: str) -> str:
    roles = board.get("roles", {})
    # Inline custom clock (from `alloy clock` / the visual editor) or a named
    # profile from the chip data — same shape either way.
    profile = board["clock"] if isinstance(board.get("clock"), dict) \
        else chip["clock"]["profiles"][board["clock_profile"]]
    # A board carrying its own PLL (from `alloy clock` or the visual editor) has
    # no profile NAME to quote — reading one unconditionally used to make every
    # custom-clock board fail codegen with a KeyError.
    clock_origin = (
        f"Custom clock carried by the board: {profile.get('description', 'inline PLL')}"
        if isinstance(board.get("clock"), dict)
        else f"Clock profile '{board['clock_profile']}' resolved from chip data."
    )
    boot_hz = chip["clock"]["sources"][chip["clock"]["boot_source"]]["hz"]

    program = profile["program"]
    watchdog = roles.get("watchdog")
    if watchdog is not None:
        # The board uses its watchdog: leave the counter running for
        # board::watchdog.start() to program, so DROP the bring-up MR write that
        # would disable it (the SAM E70 WDT_MR is write-once — one shot only).
        wp = watchdog.get("peripheral")
        program = [op for op in program
                   if not (op.get("peripheral") == wp and op.get("register") == "MR")]
    if roles.get("rtc") is not None:
        # The RTC lives in the always-on backup domain, clocked separately from
        # the core: enable PWR, lift backup-domain write protection, start the
        # LSI, then select it + enable the RTC in RCC.BDCR. Appended to the clock
        # program (data-driven, resolved against curated rcc/pwr registers) so
        # the driver only ever touches RTC registers. Register/field names are
        # the same across STM32 families, so this one sequence generalizes.
        program = program + [
            {"op": "rmw", "peripheral": "rcc", "register": "APBENR1", "fields": {"PWREN": 1}},
            {"op": "rmw", "peripheral": "pwr", "register": "CR1", "fields": {"DBP": 1}},
            {"op": "rmw", "peripheral": "rcc", "register": "CSR", "fields": {"LSION": 1}},
            {"op": "poll", "peripheral": "rcc", "register": "CSR", "field": "LSIRDY",
             "equals": 1, "timeout_us": 2000},
            {"op": "rmw", "peripheral": "rcc", "register": "BDCR",
             "fields": {"RTCSEL": 2, "RTCEN": 1}},
        ]
    steps = "\n".join(_resolve_step(chip, registers, op) for op in program)

    role_init: list[str] = []
    if "led" in roles:
        role_init.append("    led.init();\n    led.off();")
    if "button" in roles:
        if roles["button"].get("pull") == "up":
            role_init.append("    button.init_pullup();")
        else:
            role_init.append("    button.init();")

    role_block = "\n".join(role_init)
    # Xtensa: take the vectors over from the ROM (VECBASE) before anything
    # can enable an interrupt line; the symbol lives in arch/xtensa/vectors.S.
    vectors_decl = ""
    vectors_call = ""
    if arch_ns == "xtensa" and chip.get("interrupts"):
        vectors_decl = 'extern "C" void alloy_vectors_install();\n\n'
        vectors_call = "    alloy_vectors_install();\n"
    return f"""{BANNER}// Board: {board['id']} — role + clock bring-up
#include "alloy/board.hpp"

#include "alloy/arch/{arch_ns}/systick.hpp"
#include "alloy/hal/clock_program.hpp"

{vectors_decl}namespace board {{
namespace {{

// {clock_origin}
constexpr alloy::clock_step kClockProgram[] = {{
{steps}
}};

}}  // namespace

bool init() {{
{vectors_call}    const bool clock_ok = alloy::hal::run_clock_program(kClockProgram);
    // SysTick counts the CPU clock (sysclk). On failure the chip is still on
    // its boot clock; keep the timebase honest.
    const std::uint32_t core_hz = clock_ok ? clock_profile::sysclk_hz : {boot_hz}u;
    alloy::arch::{arch_ns}::systick_init(core_hz);
{role_block}
    alloy::arch::{arch_ns}::enable_irq();
    return clock_ok;
}}

}}  // namespace board
"""
