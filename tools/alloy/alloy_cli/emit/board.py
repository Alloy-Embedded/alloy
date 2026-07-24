"""Emit the board role layer: alloy/board.hpp + board.cpp.

Roles are resolved against the chip data — the emitter carries ZERO
addresses, AF numbers or clock constants of its own (the old ecosystem's
emit_board.py died of exactly that). Unknown pins/peripherals/profiles fail
generation with a message naming the board.json entry.
"""

from __future__ import annotations

from typing import Any

from .common import BANNER, EmitError, field_lookup, register_by_name


def _require(cond: bool, msg: str) -> None:
    if not cond:
        raise EmitError(msg)


def _dma_controller(chip: dict[str, Any], registers: dict[str, dict[str, Any]]) -> str | None:
    """First peripheral whose IP is class 'dma' (alphabetical for determinism)."""
    for name in sorted(chip["peripherals"]):
        ip = chip["peripherals"][name].get("ip")
        if ip and registers.get(ip, {}).get("class") == "dma":
            return name
    return None


def _require_curated(board_id: str, chip: dict[str, Any], periph: str, role: str) -> None:
    if chip["peripherals"].get(periph, {}).get("uncurated"):
        raise EmitError(
            f"board {board_id}: role {role} targets UNCURATED peripheral '{periph}' — "
            "curate its IP register file first"
        )


def _polarity(active: str) -> str:
    return "alloy::gpio::active_high_t" if active == "high" else "alloy::gpio::active_low_t"


def emit_board_header(board: dict[str, Any], chip: dict[str, Any],
                      registers: dict[str, dict[str, Any]]) -> str:
    roles = board.get("roles", {})
    profile_name = board["clock_profile"]
    profile = chip["clock"]["profiles"].get(profile_name)
    _require(profile is not None, f"board {board['id']}: clock_profile '{profile_name}' not in chip data")

    caps: dict[str, bool] = {"led": False, "button": False, "debug_uart": False,
                             "led_pwm": False, "adc": False, "i2c": False,
                             "spi": False, "eeprom": False,
                             # Interrupt layer: needs a generated vector table
                             # (chips without an interrupts list — ESP32 v1 —
                             # have no dispatch to attach to).
                             "irq": bool(chip.get("interrupts")),
                             # DMA: chip declares a controller whose IP is
                             # class "dma" (st dma1, microchip xdmac, ...).
                             "dma": _dma_controller(chip, registers) is not None,
                             # Heavy connectivity roles: unlike the small
                             # peripherals these do NOT get a no-op stub — an
                             # absent one emits a POISONED type (static_assert
                             # on use) so portable if-constexpr(caps::net) code
                             # compiles everywhere but unconditional use of a
                             # missing subsystem is a readable compile error.
                             "ethernet": False, "wifi": False}
    decls: list[str] = []

    extra_includes: list[str] = []
    led = roles.get("led")
    if led:
        _require(led["pin"] in chip.get("pins", {}),
                 f"board {board['id']}: led pin '{led['pin']}' not in chip data")
        caps["led"] = True
        led_kind = led.get("kind", "gpio")
        if led_kind == "ws2812":
            extra_includes.append("alloy/drivers/ws2812.hpp")
            decls.append(
                f"inline constexpr alloy::drivers::ws2812<alloy::dev::{led['pin']}_t, "
                f"clock_profile> led{{}};"
            )
        elif led_kind == "gpio":
            decls.append(
                f"inline constexpr alloy::gpio::output<alloy::dev::{led['pin']}_t, "
                f"{_polarity(led.get('active', 'high'))}> led{{}};"
            )
        else:
            raise EmitError(f"board {board['id']}: unknown led kind '{led_kind}'")

    button = roles.get("button")
    if button:
        _require(button["pin"] in chip.get("pins", {}),
                 f"board {board['id']}: button pin '{button['pin']}' not in chip data")
        caps["button"] = True
        decls.append(
            f"inline constexpr alloy::gpio::input<alloy::dev::{button['pin']}_t, "
            f"{_polarity(button.get('active', 'high'))}> button{{}};"
        )

    uart = roles.get("debug_uart")
    if uart:
        _require("peripheral" in uart, f"board {board['id']}: debug_uart missing 'peripheral'")
        _require(uart["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: debug_uart peripheral '{uart['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, uart["peripheral"], "debug_uart")
        caps["debug_uart"] = True
        if uart.get("mode") == "rom":
            # Boot-ROM-configured UART (classic ESP32 UART0): no pin routing.
            decls.append(
                f"using debug_uart = alloy::uart::rom_bind<alloy::dev::{uart['peripheral']}_t>;\n"
                f"inline constexpr std::uint32_t debug_uart_baud = {uart.get('baud', 115200)}u;"
            )
        else:
            for key in ("tx", "rx"):
                _require(key in uart, f"board {board['id']}: debug_uart missing '{key}'")
            decls.append(
                f"using debug_uart = alloy::uart::bind<alloy::dev::{uart['peripheral']}_t,\n"
                f"                                     alloy::uart::tx<alloy::dev::{uart['tx']}_t>,\n"
                f"                                     alloy::uart::rx<alloy::dev::{uart['rx']}_t>,\n"
                f"                                     clock_profile>;\n"
                f"inline constexpr std::uint32_t debug_uart_baud = {uart.get('baud', 115200)}u;"
            )
    else:
        decls.append(
            "// This board declares no debug UART; the stub keeps\n"
            "// `if constexpr (board::caps::debug_uart)` code compiling everywhere.\n"
            "struct debug_uart {\n"
            "    struct null_handle {\n"
            "        void write(std::uint8_t) const {}\n"
            "        void write(const char*) const {}\n"
            "        bool read(std::uint8_t&) const { return false; }\n"
            "        void flush() const {}\n"
            "    };\n"
            "    static null_handle open(alloy::uart::config) { return {}; }\n"
            "};\n"
            "inline constexpr std::uint32_t debug_uart_baud = 0u;"
        )

    led_pwm = roles.get("led_pwm")
    if led_pwm:
        for key in ("peripheral", "channel", "pin"):
            _require(key in led_pwm, f"board {board['id']}: led_pwm missing '{key}'")
        _require(led_pwm["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: led_pwm peripheral '{led_pwm['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, led_pwm["peripheral"], "led_pwm")
        caps["led_pwm"] = True
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

    i2c_role = roles.get("i2c")
    if i2c_role:
        for key in ("peripheral", "scl", "sda"):
            _require(key in i2c_role, f"board {board['id']}: i2c role missing '{key}'")
        _require(i2c_role["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: i2c peripheral '{i2c_role['peripheral']}' not in chip data")
        _require_curated(board["id"], chip, i2c_role["peripheral"], "i2c")
        caps["i2c"] = True
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
        caps["spi"] = True
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
        caps["eeprom"] = True
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
        caps["adc"] = True
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
            "struct adc {\n"
            "    struct null_handle {\n"
            "        std::uint16_t read(std::uint8_t) const { return 0u; }\n"
            "    };\n"
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
        caps["ethernet"] = True
        decls.append(
            f"// Ethernet: GMAC + {phy['kind']} PHY (addr {phy.get('addr', 0)}, "
            f"{phy.get('mode', 'rmii')}). The NetDevice binding arrives in M1;\n"
            f"// M0 records the facts and the capability.\n"
            f"using eth_mac = alloy::dev::{eth['peripheral']}_t;\n"
            f"inline constexpr std::uint8_t eth_phy_addr = {phy.get('addr', 0)}u;"
        )
    else:
        decls.append(
            "// No Ethernet. Scalar facts get honest-zero stubs (like eeprom_addr)\n"
            "// so non-dependent references in discarded if-constexpr branches\n"
            "// still name-resolve; only the heavy TYPE is poisoned — using\n"
            "// board::eth unguarded is a readable compile error, never a no-op.\n"
            "inline constexpr std::uint8_t eth_phy_addr = 0u;\n"
            "template <class T = void>\n"
            "struct eth_absent {\n"
            "    static_assert(sizeof(T) == 0,\n"
            "        \"this board has no Ethernet — guard with board::caps::ethernet\");\n"
            "};\n"
            "using eth = eth_absent<>;"
        )
    decls.append(
        "// Umbrella capability: any owned-MAC or vendor-blob link.\n"
        "namespace caps { inline constexpr bool net = ethernet || wifi; }"
    )

    caps_body = "\n".join(
        f"inline constexpr bool {name} = {'true' if value else 'false'};"
        for name, value in sorted(caps.items())
    )
    decl_body = "\n\n".join(decls)
    extra_include_block = "".join(f'\n#include "{inc}"' for inc in sorted(extra_includes))

    return f"""{BANNER}// Board: {board['id']} ({board.get('name', '')})
#pragma once

#include <cstdint>

#include "alloy/adc.hpp"
#include "alloy/device.hpp"
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
    profile = chip["clock"]["profiles"][board["clock_profile"]]
    boot_hz = chip["clock"]["sources"][chip["clock"]["boot_source"]]["hz"]

    steps = "\n".join(_resolve_step(chip, registers, op) for op in profile["program"])

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

// Clock profile '{board['clock_profile']}' resolved from chip data.
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
