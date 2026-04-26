#!/usr/bin/env python3
from __future__ import annotations

import argparse
import difflib
import glob
import hashlib
import json
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


@dataclass(frozen=True)
class ConnectorInfo:
    alias: str
    peripheral: str
    bindings: tuple[str, ...]
    note: str = ""


@dataclass(frozen=True)
class BoardConfig:
    board: str
    build_dir: Path
    bundle_target: str
    firmware_targets: tuple[str, ...]
    openocd_args: tuple[str, ...]
    uart_globs: tuple[str, ...]
    uart_baud: int
    stm32_programmer_supported: bool = False
    esptool_chip: str = ""        # non-empty => use esptool instead of openocd
    esptool_app_offset: str = "0x0"  # flash offset for the app binary
    esptool_partition_table: str = ""  # path to partition table bin (relative to repo root)
    toolchain_file: str = "cmake/toolchains/arm-none-eabi.cmake"


@dataclass(frozen=True)
class BoardInsight:
    display_name: str
    clock_summary: str
    debug_uart_summary: str
    connectors: tuple[ConnectorInfo, ...]


BOARDS: dict[str, BoardConfig] = {
    "same70_xplained": BoardConfig(
        "same70_xplained",
        ROOT / "build" / "hw" / "same70",
        "same70_hardware_validation_bundle",
        ("blink", "time_probe", "uart_logger", "watchdog_probe", "analog_probe", "rtc_probe",
         "timer_pwm_probe", "i2c_scan", "spi_probe", "dma_probe", "can_probe"),
        ("openocd", "-f", "board/atmel_same70_xplained.cfg"),
        ("/dev/cu.usbmodem*", "/dev/ttyACM*", "/dev/cu.usbserial*"),
        115200,
    ),
    "same70_xpld": BoardConfig(
        "same70_xplained",
        ROOT / "build" / "hw" / "same70",
        "same70_hardware_validation_bundle",
        ("blink", "time_probe", "uart_logger", "watchdog_probe", "analog_probe", "rtc_probe",
         "timer_pwm_probe", "i2c_scan", "spi_probe", "dma_probe", "can_probe"),
        ("openocd", "-f", "board/atmel_same70_xplained.cfg"),
        ("/dev/cu.usbmodem*", "/dev/ttyACM*", "/dev/cu.usbserial*"),
        115200,
    ),
    "nucleo_g071rb": BoardConfig(
        "nucleo_g071rb",
        ROOT / "build" / "hw" / "g071",
        "stm32g0_hardware_validation_bundle",
        ("blink", "time_probe", "uart_logger", "watchdog_probe", "rtc_probe", "timer_pwm_probe",
         "analog_probe"),
        ("openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32g0x.cfg"),
        ("/dev/cu.usbmodem*", "/dev/ttyACM*", "/dev/cu.usbserial*"),
        115200,
        True,
    ),
    "nucleo_f401re": BoardConfig(
        "nucleo_f401re",
        ROOT / "build" / "hw" / "f401",
        "stm32f4_hardware_validation_bundle",
        ("blink", "time_probe", "uart_logger", "watchdog_probe", "rtc_probe", "timer_pwm_probe",
         "analog_probe", "dma_probe"),
        ("openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg"),
        ("/dev/cu.usbmodem*", "/dev/ttyACM*", "/dev/cu.usbserial*"),
        115200,
        True,
    ),
    "esp32c3_devkitm": BoardConfig(
        "esp32c3_devkitm",
        ROOT / "build" / "hw" / "esp32c3",
        "hello_esp32c3",
        ("hello_esp32c3",),
        (),
        ("/dev/cu.usbserial*", "/dev/cu.usbmodem*", "/dev/ttyUSB*", "/dev/ttyACM*"),
        115200,
        esptool_chip="esp32c3",
        toolchain_file="cmake/toolchains/riscv32-esp-elf.cmake",
    ),
    "esp32_devkit": BoardConfig(
        "esp32_devkit",
        ROOT / "build" / "hw" / "esp32",
        "hello_esp32",
        ("hello_esp32",),
        (),
        ("/dev/cu.usbserial*", "/dev/cu.usbmodem*", "/dev/ttyUSB*", "/dev/ttyACM*"),
        115200,
        esptool_chip="esp32",
        esptool_app_offset="0x10000",
        esptool_partition_table="boards/esp32_devkit/partitions.csv",
        toolchain_file="cmake/toolchains/xtensa-esp32-elf.cmake",
    ),
}

BOARD_INSIGHTS: dict[str, BoardInsight] = {
    "esp32_devkit": BoardInsight(
        display_name="ESP32-DevKit",
        clock_summary="80 MHz APB (set by ESP-IDF bootloader); app can raise to 240 MHz",
        debug_uart_summary="UART0 @ 115200 8N1 on GPIO1(TX)/GPIO3(RX) via onboard CP2102",
        connectors=(
            ConnectorInfo("debug-uart", "UART0", ("GPIO1 -> TX", "GPIO3 -> RX"),
                          "onboard CP2102 USB-serial — same port as flashing"),
        ),
    ),
    "esp32c3_devkitm": BoardInsight(
        display_name="ESP32-C3-DevKitM-1",
        clock_summary="ROM default (40 MHz crystal); no PLL configured in bare-metal bring-up",
        debug_uart_summary="UART0 @ 115200 8N1 on GPIO21(TX)/GPIO20(RX) — USB-serial adapter required",
        connectors=(
            ConnectorInfo("debug-uart", "UART0", ("GPIO21 -> TX", "GPIO20 -> RX"),
                          "USB-serial adapter; ROM pre-configures at 115200"),
        ),
    ),
    "same70_xplained": BoardInsight(
        display_name="SAME70 Xplained Ultra",
        clock_summary="clock_config::plla_150mhz (external 12 MHz crystal -> 150 MHz SYSCLK/HCLK/PCLK)",
        debug_uart_summary="USART1 @ 115200 8N1 on PB4(TX)/PA21(RX)",
        connectors=(
            ConnectorInfo("debug-uart", "USART1", ("PB4 -> TXD1", "PA21 -> RXD1"),
                          "official EDBG virtual COM path"),
            ConnectorInfo("i2c", "TWIHS0", ("PA4 -> TWCK0", "PA3 -> TWD0")),
            ConnectorInfo("spi", "SPI0", ("PD22 -> SPCK", "PD20 -> MISO", "PD21 -> MOSI")),
        ),
    ),
    "nucleo_g071rb": BoardInsight(
        display_name="Nucleo-G071RB",
        clock_summary="system_clock::default_pll_64mhz (HSI + PLL -> 64 MHz SYSCLK/PCLK)",
        debug_uart_summary="USART2 @ 115200 8N1 on PA2(TX)/PA3(RX)",
        connectors=(
            ConnectorInfo("debug-uart", "USART2", ("PA2 -> TX", "PA3 -> RX"),
                          "official ST-LINK VCP path"),
        ),
    ),
    "nucleo_f401re": BoardInsight(
        display_name="Nucleo-F401RE",
        clock_summary="system_clock::default_hse_pll_84mhz (external 8 MHz HSE + PLL -> 84 MHz SYSCLK)",
        debug_uart_summary="USART2 @ 115200 8N1 on PA2(TX)/PA3(RX)",
        connectors=(
            ConnectorInfo("debug-uart", "USART2", ("PA2 -> TX", "PA3 -> RX"),
                          "official ST-LINK VCP path"),
        ),
    ),
}

MANIFEST_PATH = ROOT / "docs" / "RELEASE_MANIFEST.json"


def die(msg: str) -> int:
    print(msg, file=sys.stderr)
    return 1


def resolve_board(explicit: str | None) -> str:
    """Return a board name. Fall back to the unique configured build dir, then
    to a pinned `.alloyctl-board` file at the repo root. Raise if ambiguous."""
    if explicit:
        return explicit
    pinned = ROOT / ".alloyctl-board"
    if pinned.exists():
        name = pinned.read_text(encoding="utf-8").strip()
        if name:
            return name
    configured = [
        name for name, cfg in BOARDS.items()
        if (cfg.build_dir / "CMakeCache.txt").exists()
    ]
    # Deduplicate entries that share the same build_dir (e.g. same70_xplained + same70_xpld).
    unique = sorted({BOARDS[n].build_dir: n for n in configured}.values())
    if len(unique) == 1:
        return unique[0]
    if len(unique) == 0:
        raise SystemExit(die(
            "no board configured — run with --board <name>, or "
            "`alloyctl configure --board <name>` first. "
            f"known boards: {', '.join(sorted(BOARDS))}"
        ))
    raise SystemExit(die(
        f"multiple boards configured ({', '.join(unique)}); pass --board explicitly "
        f"or pin one with: echo <board> > .alloyctl-board"
    ))


def load_manifest() -> dict:
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def suggest(value: str, options: list[str]) -> str:
    match = difflib.get_close_matches(value, options, n=1)
    return f"; did you mean '{match[0]}'?" if match else ""


def board_config(name: str) -> BoardConfig:
    cfg = BOARDS.get(name)
    if cfg is None:
        options = sorted(BOARDS)
        raise SystemExit(
            die(f"unsupported board: {name}; supported boards: {', '.join(options)}{suggest(name, options)}")
        )
    return cfg


def require_tool(name: str) -> None:
    if shutil.which(name) is None:
        raise SystemExit(die(f"missing tool: {name}"))


def find_stm32_programmer_cli() -> str | None:
    candidates = (
        shutil.which("STM32_Programmer_CLI"),
        "/Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/Resources/bin/STM32_Programmer_CLI",
    )
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return candidate
    return None


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd))
    try:
        subprocess.run(cmd, cwd=ROOT, check=True)
    except KeyboardInterrupt:
        raise SystemExit(130)


def run_soft(cmd: list[str]) -> int:
    print("+", " ".join(cmd))
    try:
        completed = subprocess.run(cmd, cwd=ROOT, check=False)
    except KeyboardInterrupt:
        raise SystemExit(130)
    return completed.returncode


def select_flash_backend(cfg: BoardConfig, requested_backend: str, recover: bool) -> str:
    if requested_backend != "auto":
        return requested_backend
    if cfg.esptool_chip:
        return "esptool"
    if recover and cfg.stm32_programmer_supported and find_stm32_programmer_cli():
        return "stm32cube"
    return "openocd"


def _find_gen_esp32part() -> str | None:
    """Locate gen_esp32part.py from ESP-IDF or Arduino ESP32 install."""
    import os
    candidates = [
        os.path.join(os.environ.get("IDF_PATH", ""), "components/partition_table/gen_esp32part.py"),
        str(Path.home() / "esp/esp-idf/components/partition_table/gen_esp32part.py"),
    ]
    for c in candidates:
        if c and Path(c).exists():
            return c
    return None


def _generate_partition_table(csv_path: Path, out_bin: Path) -> None:
    """Pure-Python minimal partition table generator (no IDF needed)."""
    entries = b""
    for line in csv_path.read_text().splitlines():
        line = line.split("#")[0].strip()
        if not line:
            continue
        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 5:
            continue
        name, type_s, subtype_s, offset_s, size_s = parts[:5]
        type_map = {"app": 0x00, "data": 0x01}
        sub_map = {"factory": 0x00, "ota_0": 0x10, "ota_1": 0x11,
                   "nvs": 0x02, "phy": 0x01, "otadata": 0x00}
        t = type_map.get(type_s, int(type_s, 0))
        s = sub_map.get(subtype_s, int(subtype_s, 0))
        offset = int(offset_s, 0)
        size = int(size_s, 0)
        label = name.encode("ascii").ljust(16, b"\x00")[:16]
        entry = struct.pack("<BBII", t, s, offset, size) + label + b"\x00\x00\x00\x00"
        entries += b"\xaa\x50" + entry
    md5_marker = b"\xeb\xeb" + b"\xff" * 14 + hashlib.md5(entries).digest()
    table = (entries + md5_marker).ljust(0xC00, b"\xff")
    out_bin.write_bytes(table)


def flash_plan_lines(cfg: BoardConfig, target: str, backend: str, recover: bool) -> list[str]:
    action = "recover-and-flash" if recover else "flash"
    lines = [
        f"board: {cfg.board}",
        f"target: {target}",
        f"action: {action}",
        f"selected-backend: {backend}",
    ]

    if backend == "esptool":
        if cfg.esptool_app_offset == "0x0":
            # ESP32-C3 direct-boot: raw binary at 0x0
            lines.extend([
                "supported-flow: esptool direct-boot write-flash 0x0",
                "planned-steps:",
                "  - convert ELF to raw binary (objcopy -O binary)",
                f"  - esptool --chip {cfg.esptool_chip} write-flash 0x0 <target>.bin",
            ])
        else:
            # ESP32 ESP-IDF bootloader: elf2image + optional partition table
            steps = ["planned-steps:"]
            if cfg.esptool_partition_table:
                steps.append(f"  - esptool --chip {cfg.esptool_chip} write-flash 0x8000 {cfg.esptool_partition_table}")
            steps.append(f"  - esptool --chip {cfg.esptool_chip} write-flash {cfg.esptool_app_offset} <target>.bin")
            lines.extend([
                f"supported-flow: esptool ESP-IDF-bootloader write-flash {cfg.esptool_app_offset}",
                *steps,
            ])
        if recover:
            lines.append("  - --before default_reset --after hard_reset")
        return lines

    if backend == "stm32cube":
        lines.extend(
            [
                "supported-flow: STM32CubeProgrammer connect-under-reset path",
                "planned-steps:",
                "  - connect over SWD with hardware reset",
            ]
        )
        if recover:
            lines.append("  - mass erase before flashing")
        lines.extend(
            [
                "  - write image, verify, then reset",
                f"tool-detection: {'STM32_Programmer_CLI found' if find_stm32_programmer_cli() else 'STM32_Programmer_CLI not found'}",
            ]
        )
        return lines

    lines.extend(
        [
            "supported-flow: OpenOCD board/probe path",
            "planned-steps:",
        ]
    )
    if recover:
        lines.append("  - connect under reset with conservative adapter speed")
    else:
        lines.append("  - connect with the board's published OpenOCD config")
    lines.extend(
        [
            "  - program image, verify, then reset",
            f"openocd-config: {' '.join(cfg.openocd_args)}",
        ]
    )
    return lines


VALIDATION_PRESETS: dict[str, dict[str, str]] = {
    "host": {
        "runtime": "host-mmio-validation",
    },
    "same70_xplained": {
        "runtime": "same70-runtime-validation",
        "smoke": "same70-renode-smoke",
        "zero-overhead": "same70-zero-overhead",
    },
    "same70_xpld": {
        "runtime": "same70-runtime-validation",
        "smoke": "same70-renode-smoke",
        "zero-overhead": "same70-zero-overhead",
    },
    "nucleo_g071rb": {
        "runtime": "stm32g0-runtime-validation",
        "smoke": "stm32g0-renode-smoke",
    },
    "nucleo_f401re": {
        "runtime": "stm32f4-runtime-validation",
        "smoke": "stm32f4-renode-smoke",
    },
}


def is_configured(cfg: BoardConfig) -> bool:
    return (cfg.build_dir / "CMakeCache.txt").exists()


def cache_value(cfg: BoardConfig, key: str) -> str | None:
    cache = cfg.build_dir / "CMakeCache.txt"
    if not cache.exists():
        return None
    prefix = f"{key}:"
    for line in cache.read_text().splitlines():
        if line.startswith(prefix):
            _, value = line.split("=", 1)
            return value.strip()
    return None


def ensure_configured(cfg: BoardConfig, build_type: str, build_tests: bool) -> None:
    expected_build_tests = "ON" if build_tests else "OFF"
    if (not is_configured(cfg) or cache_value(cfg, "ALLOY_BUILD_TESTS") != expected_build_tests or
            cache_value(cfg, "ALLOY_BOARD") != cfg.board or
            cache_value(cfg, "CMAKE_BUILD_TYPE") != build_type or
            cache_value(cfg, "CMAKE_TOOLCHAIN_FILE") != str(ROOT / cfg.toolchain_file)):
        configure(cfg, build_type, build_tests=build_tests)


def configure(cfg: BoardConfig, build_type: str, build_tests: bool) -> None:
    run(
        [
            "cmake",
            "-S",
            str(ROOT),
            "-B",
            str(cfg.build_dir),
            f"-DALLOY_BOARD={cfg.board}",
            f"-DALLOY_BUILD_TESTS={'ON' if build_tests else 'OFF'}",
            f"-DCMAKE_TOOLCHAIN_FILE={cfg.toolchain_file}",
            f"-DCMAKE_BUILD_TYPE={build_type}",
        ]
    )


def build(cfg: BoardConfig, target: str, jobs: int) -> None:
    run(["cmake", "--build", str(cfg.build_dir), "--target", target, f"-j{jobs}"])


def build_many(cfg: BoardConfig, targets: tuple[str, ...], jobs: int) -> None:
    run(["cmake", "--build", str(cfg.build_dir), "--target", *targets, f"-j{jobs}"])


def artifact(cfg: BoardConfig, target: str) -> Path:
    candidates = (
        cfg.build_dir / "examples" / target / f"{target}.elf",
        cfg.build_dir / "examples" / target / target,
    )
    for path in candidates:
        if path.exists():
            return path
    raise SystemExit(die(f"artifact not found: {candidates[0]} or {candidates[1]}"))


def stm32_program_image(cfg: BoardConfig, target: str) -> Path:
    candidates = (
        cfg.build_dir / "examples" / target / f"{target}.hex",
        cfg.build_dir / "examples" / target / f"{target}.bin",
    )
    for path in candidates:
        if path.exists():
            return path
    raise SystemExit(die(f"stm32 programmer image not found: {candidates[0]} or {candidates[1]}"))


def auto_port(cfg: BoardConfig) -> str:
    ports: list[str] = []
    for pattern in cfg.uart_globs:
        ports.extend(sorted(glob.glob(pattern)))
    if not ports:
        raise SystemExit(die("no serial port found; use --port"))
    return ports[0]


def board_insight(cfg: BoardConfig) -> BoardInsight:
    insight = BOARD_INSIGHTS.get(cfg.board)
    if insight is None:
        raise SystemExit(die(f"missing board insight for: {cfg.board}"))
    return insight


def board_release_entry(manifest: dict, cfg: BoardConfig) -> dict:
    entry = manifest["boards"].get(cfg.board)
    if entry is None:
        raise SystemExit(die(f"missing release manifest entry for board: {cfg.board}"))
    return entry


def known_examples(cfg: BoardConfig) -> tuple[str, ...]:
    return cfg.firmware_targets


def format_lines(lines: list[str]) -> None:
    for line in lines:
        print(line)


def connector_summary(connector: ConnectorInfo) -> str:
    bindings = ", ".join(connector.bindings)
    summary = f"{connector.alias}: {connector.peripheral} via {bindings}"
    if connector.note:
        summary += f" ({connector.note})"
    return summary


def connector_alternative_lines(connectors: dict[str, ConnectorInfo]) -> list[str]:
    return ["valid alternatives:", *[f"  - {connector_summary(connectors[name])}" for name in sorted(connectors)]]


def board_clock_guidance(cfg: BoardConfig, insight: BoardInsight) -> str:
    return (
        f"Use the published {cfg.board} clock profile as-is for bring-up and migration work; "
        "do not carry raw clock assumptions across boards."
    )


def board_debug_guidance(cfg: BoardConfig, insight: BoardInsight) -> str:
    return (
        f"Use the documented {cfg.board} debug UART path first when validating logs, flash recovery, "
        "or monitor output."
    )


def migration_watchpoints(left: BoardConfig, right: BoardConfig, left_insight: BoardInsight,
                          right_insight: BoardInsight, left_release: dict, right_release: dict) -> list[str]:
    notes = [
        f"retune clock assumptions from '{left_insight.clock_summary}' to '{right_insight.clock_summary}'",
        f"move debug and monitor expectations from '{left_insight.debug_uart_summary}' to '{right_insight.debug_uart_summary}'",
    ]

    left_connectors = {item.alias for item in left_insight.connectors}
    right_connectors = {item.alias for item in right_insight.connectors}
    dropped_connectors = sorted(left_connectors - right_connectors)
    added_connectors = sorted(right_connectors - left_connectors)
    if dropped_connectors:
        notes.append(f"replace source-only connector aliases: {', '.join(dropped_connectors)}")
    if added_connectors:
        notes.append(f"consider target-only connector aliases: {', '.join(added_connectors)}")

    left_gates = set(left_release["required_gates"])
    right_gates = set(right_release["required_gates"])
    if left_gates != right_gates:
        notes.append(
            f"re-run target release gates: {', '.join(sorted(right_gates)) or 'none'}"
        )

    left_examples = set(left_release["required_examples"])
    right_examples = set(right_release["required_examples"])
    if left_examples != right_examples:
        notes.append(
            f"recheck required example coverage on target: {', '.join(sorted(right_examples)) or 'none'}"
        )
    return notes


def route_family_name(peripheral: str) -> str:
    if peripheral.startswith(("USART", "UART")):
        return "uart"
    if peripheral.startswith("SPI"):
        return "spi"
    if peripheral.startswith(("TWI", "I2C")):
        return "i2c"
    if peripheral.startswith("CAN"):
        return "can"
    return "connection"


def split_binding(binding: str) -> tuple[str, str]:
    pin, signal = (part.strip() for part in binding.split("->", 1))
    return pin, signal


def signal_role_name(signal: str) -> str:
    signal_lower = signal.lower()
    for prefix in ("tx", "rx", "cts", "rts", "sck", "miso", "mosi", "scl", "sda"):
        if signal_lower.startswith(prefix):
            return prefix
    return signal_lower


def ergonomic_binding(binding: str) -> str:
    pin, signal = split_binding(binding)
    role = signal_role_name(signal)
    pin_text = f"alloy::dev::pin::{pin}"
    signal_lower = signal.lower()
    if signal_lower == role:
        return f"alloy::hal::{role}<{pin_text}>"
    return f"alloy::hal::{role}<{pin_text}, alloy::dev::sig::signal_{signal_lower}>"


def canonical_binding(binding: str) -> str:
    pin, signal = split_binding(binding)
    role = signal_role_name(signal)
    return (
        f"alloy::hal::{role}<alloy::device::PinId::{pin}, "
        f"alloy::device::SignalId::signal_{signal.lower()}>"
    )


def ergonomic_route(connector: ConnectorInfo) -> str:
    family = route_family_name(connector.peripheral)
    joined = ",\n    ".join(ergonomic_binding(binding) for binding in connector.bindings)
    return (
        f"alloy::hal::{family}::route<\n"
        f"    alloy::dev::periph::{connector.peripheral},\n"
        f"    {joined}\n"
        f">"
    )


def canonical_route(connector: ConnectorInfo) -> str:
    family = route_family_name(connector.peripheral)
    joined = ",\n    ".join(canonical_binding(binding) for binding in connector.bindings)
    return (
        f"alloy::hal::{family}::route<\n"
        f"    alloy::device::PeripheralId::{connector.peripheral},\n"
        f"    {joined}\n"
        f">"
    )


def cmd_build(args: argparse.Namespace) -> None:
    cfg = board_config(args.board)
    ensure_configured(cfg, args.build_type, build_tests=False)
    build(cfg, args.target, args.jobs)


def cmd_configure(args: argparse.Namespace) -> None:
    cfg = board_config(args.board)
    ensure_configured(cfg, args.build_type, build_tests=args.build_tests)


def cmd_bundle(args: argparse.Namespace) -> None:
    cfg = board_config(args.board)
    ensure_configured(cfg, args.build_type, build_tests=True)
    build(cfg, cfg.bundle_target, args.jobs)


def cmd_flash(args: argparse.Namespace) -> None:
    cfg = board_config(args.board)
    if args.build_first and not args.dry_run:
        ensure_configured(cfg, args.build_type, build_tests=False)
        build(cfg, args.target, args.jobs)
    backend = select_flash_backend(cfg, args.flash_backend, args.recover)

    if args.dry_run:
        format_lines(flash_plan_lines(cfg, args.target, backend, args.recover))
        return

    if backend == "esptool":
        esptool = shutil.which("esptool") or shutil.which("esptool.py") or "esptool"
        elf = artifact(cfg, args.target)
        bin_path = elf.with_suffix(".bin")
        port = getattr(args, "port", None) or auto_port(cfg)
        base_cmd = [esptool, "--chip", cfg.esptool_chip, "--port", port, "--baud", "460800"]

        if cfg.esptool_app_offset == "0x0":
            # ESP32-C3 direct-boot: raw binary via objcopy
            arch_prefix = "riscv32-esp-elf" if "c3" in cfg.esptool_chip else "xtensa-esp32-elf"
            objcopy = shutil.which(f"{arch_prefix}-objcopy") or f"{arch_prefix}-objcopy"
            run([objcopy, "-O", "binary", str(elf), str(bin_path)])
            run(base_cmd + ["write-flash", "0x0", str(bin_path)])
        else:
            # ESP32 ESP-IDF bootloader: .bin already produced by elf2image post-build
            if not bin_path.exists():
                die(f"binary not found: {bin_path}\nRun build first or check post-build elf2image step.")
            # Flash partition table: generate from CSV using gen_esp32part.py
            if cfg.esptool_partition_table:
                pt_csv = ROOT / cfg.esptool_partition_table
                if pt_csv.exists():
                    pt_bin = Path(tempfile.mkstemp(suffix=".bin", prefix="alloy_pt_")[1])
                    _gen = _find_gen_esp32part()
                    if _gen:
                        run(["python3", _gen, str(pt_csv), str(pt_bin)])
                    else:
                        _generate_partition_table(pt_csv, pt_bin)
                    run(base_cmd + ["write-flash", "0x8000", str(pt_bin)])
            run(base_cmd + ["write-flash", cfg.esptool_app_offset, str(bin_path)])
        return

    if backend == "stm32cube":
        if not cfg.stm32_programmer_supported:
            raise SystemExit(die(f"flash backend 'stm32cube' is not supported for board: {cfg.board}"))
        cli = find_stm32_programmer_cli()
        if cli is None:
            raise SystemExit(die("STM32_Programmer_CLI not found"))
        image = stm32_program_image(cfg, args.target)
        if args.recover:
            run([cli, "-c", "port=SWD", "mode=UR", "reset=HWrst", "freq=100", "-e", "all"])
        run([cli, "-c", "port=SWD", "mode=UR", "reset=HWrst", "freq=100", "-w", str(image), "-v", "-rst"])
        return

    require_tool("openocd")
    elf = artifact(cfg, args.target)
    cmd = list(cfg.openocd_args)
    if args.recover:
        cmd.extend(
            [
                "-c",
                "adapter speed 500",
                "-c",
                "reset_config srst_only srst_nogate connect_assert_srst",
                "-c",
                "init",
            ]
        )
    cmd.extend(["-c", f'program "{elf}" verify reset exit'])
    run(cmd)


def cmd_monitor(args: argparse.Namespace) -> None:
    cfg = board_config(args.board)
    port = args.port or auto_port(cfg)
    baud = args.baud if args.baud is not None else cfg.uart_baud
    run([sys.executable, str(ROOT / "scripts" / "uart_monitor.py"), "--port", port, "--baud", str(baud)])


def cmd_recover(args: argparse.Namespace) -> None:
    args.recover = True
    args.build_first = True
    cmd_flash(args)


def cmd_run(args: argparse.Namespace) -> None:
    """Build -> program -> open monitor -> reset (so early probe output is captured).

    One-shot helper for hardware probes. Works for any board registered in
    BOARDS and any cmake target that produces
    `examples/<target>/<target>.elf` (or .hex/.bin for STM32 programmer).

    Flow is ordered so the serial port is already open when the target comes
    out of reset — probes that print once and then enter a blink loop still
    get their output captured.
    """
    args.board = resolve_board(getattr(args, "board", None))
    cfg = board_config(args.board)
    ensure_configured(cfg, args.build_type, build_tests=False)
    build(cfg, args.target, args.jobs)

    backend = select_flash_backend(cfg, args.flash_backend, False)
    port = args.port or auto_port(cfg)
    baud = args.baud if args.baud is not None else cfg.uart_baud

    # Step 1: program the target (no reset yet). openocd needs the chip halted
    # to flash anyway, so there's no boot output to miss during this step.
    if backend == "stm32cube":
        if not cfg.stm32_programmer_supported:
            raise SystemExit(die(f"flash backend 'stm32cube' is not supported for board: {cfg.board}"))
        cli = find_stm32_programmer_cli()
        if cli is None:
            raise SystemExit(die("STM32_Programmer_CLI not found"))
        image = stm32_program_image(cfg, args.target)
        run([cli, "-c", "port=SWD", "mode=UR", "reset=HWrst", "freq=100",
             "-w", str(image), "-v"])
    elif backend == "esptool":
        # ESP32/C3/S3: esptool handles both flash and reset in one pass.
        # We use --after hard_reset so the chip boots immediately after write-flash.
        # The serial port is opened after this step, so very early boot output may
        # be missed; this is acceptable for probe/demo targets.
        esptool = shutil.which("esptool") or shutil.which("esptool.py") or "esptool"
        elf = artifact(cfg, args.target)
        bin_path = elf.with_suffix(".bin")
        resolved_port = port or auto_port(cfg)
        base_cmd = [esptool, "--chip", cfg.esptool_chip,
                    "--port", resolved_port, "--baud", "460800",
                    "--after", "hard-reset"]
        if cfg.esptool_app_offset == "0x0":
            arch_prefix = "riscv32-esp-elf" if "c3" in cfg.esptool_chip else "xtensa-esp32-elf"
            objcopy = shutil.which(f"{arch_prefix}-objcopy") or f"{arch_prefix}-objcopy"
            run([objcopy, "-O", "binary", str(elf), str(bin_path)])
            run(base_cmd + ["write-flash", "0x0", str(bin_path)])
        else:
            if not bin_path.exists():
                raise SystemExit(die(
                    f"binary not found: {bin_path}\n"
                    "Run build first or check the post-build elf2image step."))
            if cfg.esptool_partition_table:
                pt_csv = ROOT / cfg.esptool_partition_table
                if pt_csv.exists():
                    pt_bin = Path(tempfile.mkstemp(suffix=".bin", prefix="alloy_pt_")[1])
                    _gen = _find_gen_esp32part()
                    if _gen:
                        run(["python3", _gen, str(pt_csv), str(pt_bin)])
                    else:
                        _generate_partition_table(pt_csv, pt_bin)
                    run([esptool, "--chip", cfg.esptool_chip,
                         "--port", resolved_port, "--baud", "460800",
                         "write-flash", "0x8000", str(pt_bin)])
            run(base_cmd + ["write-flash", cfg.esptool_app_offset, str(bin_path)])
    else:
        require_tool("openocd")
        elf = artifact(cfg, args.target)
        # Program without resetting here; `run_soft` tolerates non-zero rc
        # from openocd-on-shutdown (its "exit" can return 1 on some versions)
        # so long as `program ... verify` itself prints "Verified OK".
        rc = run_soft(list(cfg.openocd_args)
                      + ["-c", f'program "{elf}" verify exit'])
        if rc not in (0, 1):  # treat 1 as "verify ok, exit noise" — common
            raise SystemExit(die(f"openocd program failed (rc={rc})"))

    # Step 2: open the serial port IN-PROCESS (not via subprocess). A Python
    # subprocess takes ~1 s to cold-start + import pyserial + open the port,
    # which means probes that print once right after reset lose their output.
    # By opening the port here, before issuing the reset, we guarantee the
    # FIFO is drained live.
    try:
        import serial  # type: ignore
    except ModuleNotFoundError:
        raise SystemExit(die(
            "pyserial not installed. install with: python3 -m pip install pyserial"
        ))
    print(f"+ opening {port} @ {baud} 8N1")
    try:
        ser = serial.Serial(port=port, baudrate=baud, bytesize=8,
                            parity="N", stopbits=1, timeout=0.05)
    except serial.SerialException as exc:
        raise SystemExit(die(f"could not open {port}: {exc}"))
    ser.reset_input_buffer()

    # Step 3: trigger a clean reset now that the port is open.
    # For esptool boards the chip was already reset by write-flash --after hard_reset.
    print(f"+ resetting target via {backend} ...")
    if backend == "stm32cube":
        cli = find_stm32_programmer_cli()
        if cli is not None:
            run_soft([cli, "-c", "port=SWD", "mode=UR",
                      "reset=HWrst", "freq=100", "-rst"])
    elif backend == "esptool":
        pass  # reset already issued by write-flash --after hard_reset
    else:
        run_soft(list(cfg.openocd_args)
                 + ["-c", "init", "-c", "reset run", "-c", "exit"])

    # Step 4: monitor loop — print bytes as they arrive. Ctrl+C exits.
    print("-" * 60)
    print(f"monitor: {port} @ {baud} 8N1 — Ctrl+C to exit")
    print("-" * 60)
    try:
        while True:
            data = ser.read(4096)
            if data:
                sys.stdout.write(data.decode("utf-8", errors="replace"))
                sys.stdout.flush()
    except KeyboardInterrupt:
        print("\n-- monitor closed --")
    finally:
        try:
            ser.close()
        except Exception:
            pass


def monitor_for(cfg: BoardConfig, port: str | None, baud: int, seconds: float) -> int:
    resolved_port = port or auto_port(cfg)
    cmd = [
        sys.executable,
        str(ROOT / "scripts" / "uart_monitor.py"),
        "--port",
        resolved_port,
        "--baud",
        str(baud),
    ]
    print("+", " ".join(cmd), f"(for {seconds:.1f}s)")
    process = subprocess.Popen(cmd, cwd=ROOT)
    try:
        process.wait(timeout=seconds)
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=2.0)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        return 0
    return process.returncode


def flash_with_retry(cfg: BoardConfig, target: str, attempts: int, delay_seconds: float) -> int:
    cmd = list(cfg.openocd_args) + ["-c", f'program "{artifact(cfg, target)}" verify reset exit']
    last_rc = 1
    for attempt in range(1, attempts + 1):
        if attempt > 1:
            print(f"retrying flash for {target} ({attempt}/{attempts}) after {delay_seconds:.1f}s")
            time.sleep(delay_seconds)
        last_rc = run_soft(cmd)
        if last_rc == 0:
            return 0
    return last_rc


def cmd_sweep(args: argparse.Namespace) -> None:
    cfg = board_config(args.board)
    require_tool("openocd")

    targets = tuple(args.targets) if args.targets else cfg.firmware_targets
    if not targets:
        raise SystemExit(die(f"no firmware targets configured for board: {cfg.board}"))

    ensure_configured(cfg, args.build_type, build_tests=False)

    build_many(cfg, targets, args.jobs)

    summary: list[tuple[str, str]] = []
    baud = args.baud if args.baud is not None else cfg.uart_baud
    port = args.port or auto_port(cfg)

    for target in targets:
        print(f"\n=== {target} ===")
        flash_rc = flash_with_retry(cfg, target, args.flash_retries, args.retry_delay_seconds)
        if flash_rc != 0:
            summary.append((target, f"flash_failed({flash_rc})"))
            continue

        time.sleep(args.settle_seconds)
        monitor_rc = monitor_for(cfg, port, baud, args.monitor_seconds)
        if monitor_rc == 0:
            summary.append((target, "ok"))
        else:
            summary.append((target, f"monitor_failed({monitor_rc})"))

    print("\n=== sweep summary ===")
    for target, status in summary:
        print(f"{target}: {status}")


def cmd_gdbserver(args: argparse.Namespace) -> None:
    cfg = board_config(args.board)
    require_tool("openocd")
    run(list(cfg.openocd_args) + ["-c", f"gdb_port {args.gdb_port}", "-c", "init", "-c", "reset halt"])


def cmd_validate(args: argparse.Namespace) -> None:
    presets = VALIDATION_PRESETS.get(args.board)
    if presets is None:
        raise SystemExit(die(f"unsupported board: {args.board}"))

    preset = presets.get(args.kind)
    if preset is None:
        supported_kinds = ", ".join(sorted(presets))
        raise SystemExit(die(f"validation kind '{args.kind}' is not supported for {args.board}; supported kinds: {supported_kinds}"))

    run(["cmake", "--workflow", "--preset", preset])


def cmd_explain(args: argparse.Namespace) -> None:
    manifest = load_manifest()
    cfg = board_config(args.board)
    insight = board_insight(cfg)
    release = board_release_entry(manifest, cfg)
    connectors = {item.alias: item for item in insight.connectors}
    gates = manifest["release_gates"]
    peripheral_classes = manifest["peripheral_classes"]

    selectors = [bool(args.connector), bool(args.clock), bool(args.peripheral), bool(args.gate), bool(args.example)]
    if sum(selectors) > 1:
        raise SystemExit(die("use only one of --connector, --clock, --peripheral, --gate, or --example"))

    if args.connector:
        connector = connectors.get(args.connector)
        options = sorted(connectors)
        if connector is None:
            message = (
                f"unsupported connector alias '{args.connector}' for {cfg.board} ({insight.display_name}); "
                f"supported connectors: {', '.join(options)}{suggest(args.connector, options)}"
            )
            raise SystemExit(
                die("\n".join([message, *connector_alternative_lines(connectors)]))
            )
        lines = [
            f"board: {cfg.board}",
            f"board-name: {insight.display_name}",
            f"connector: {connector.alias}",
            f"ergonomic-route: {ergonomic_route(connector)}",
            f"canonical-route: {canonical_route(connector)}",
            f"peripheral: {connector.peripheral}",
            "bindings:",
            *[f"  - {binding}" for binding in connector.bindings],
        ]
        if connector.note:
            lines.append(f"note: {connector.note}")
        lines.append(f"migration-guidance: prefer this published connector alias on {cfg.board} before spelling a raw route")
        format_lines(lines)
        return

    if args.clock:
        format_lines(
            [
                f"board: {cfg.board}",
                f"board-name: {insight.display_name}",
                f"board-clock-profile: {insight.clock_summary}",
                f"board-debug-uart: {insight.debug_uart_summary}",
                f"required gates: {', '.join(release['required_gates'])}",
                f"clock-guidance: {board_clock_guidance(cfg, insight)}",
                f"debug-guidance: {board_debug_guidance(cfg, insight)}",
            ]
        )
        return

    if args.peripheral:
        entry = peripheral_classes.get(args.peripheral)
        options = sorted(peripheral_classes)
        if entry is None:
            raise SystemExit(
                die(
                    f"unsupported peripheral class '{args.peripheral}'; "
                    f"supported classes: {', '.join(options)}{suggest(args.peripheral, options)}"
                )
            )
        format_lines(
            [
                f"peripheral-class: {args.peripheral}",
                f"tier: {entry['tier']}",
                f"required gates: {', '.join(entry['required_gates'])}",
            ]
        )
        return

    if args.gate:
        entry = gates.get(args.gate)
        options = sorted(gates)
        if entry is None:
            raise SystemExit(
                die(
                    f"unsupported release gate '{args.gate}'; "
                    f"supported gates: {', '.join(options)}{suggest(args.gate, options)}"
                )
            )
        lines = [
            f"gate: {args.gate}",
            f"kind: {entry['kind']}",
        ]
        for key in ("command", "target", "label", "preset", "presets", "notes"):
            if key in entry:
                value = entry[key]
                if isinstance(value, list):
                    value = ", ".join(value)
                lines.append(f"{key}: {value}")
        format_lines(lines)
        return

    if args.example:
        examples = known_examples(cfg)
        if args.example not in examples:
            options = sorted(examples)
            raise SystemExit(
                die(
                    f"unsupported example '{args.example}' for {cfg.board}; "
                    f"supported examples: {', '.join(options)}{suggest(args.example, options)}"
                )
            )
        required = "yes" if args.example in release["required_examples"] else "no"
        format_lines(
            [
                f"board: {cfg.board}",
                f"example: {args.example}",
                f"in-board-bundle: yes",
                f"required-for-release: {required}",
            ]
        )
        return

    format_lines(
        [
            f"board: {cfg.board}",
            f"display-name: {insight.display_name}",
            f"tier: {release['tier']}",
            f"board-clock-profile: {insight.clock_summary}",
            f"board-debug-uart: {insight.debug_uart_summary}",
            f"required examples: {', '.join(release['required_examples'])}",
            f"required gates: {', '.join(release['required_gates'])}",
            f"known connector aliases: {', '.join(item.alias for item in insight.connectors)}",
            "connector summary:",
            *[f"  - {connector_summary(item)}" for item in insight.connectors],
            f"sample ergonomic route: {ergonomic_route(insight.connectors[0])}",
            f"clock-guidance: {board_clock_guidance(cfg, insight)}",
            f"debug-guidance: {board_debug_guidance(cfg, insight)}",
            f"notes: {release['notes']}",
        ]
    )


def _read_project_version() -> str:
    cml = ROOT / "CMakeLists.txt"
    try:
        text = cml.read_text(encoding="utf-8")
    except OSError:
        return "unknown"
    match = re.search(r"project\(alloy\s*\n\s*VERSION\s+([0-9][0-9A-Za-z.\-+]*)", text)
    return match.group(1) if match else "unknown"


def _read_release_manifest() -> dict:
    path = ROOT / "docs" / "RELEASE_MANIFEST.json"
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}


def _detect_tool_version(executable: str, args: tuple[str, ...] = ("--version",)) -> dict:
    path = shutil.which(executable)
    if path is None:
        return {"found": False, "path": None, "version": None}
    try:
        completed = subprocess.run([path, *args], capture_output=True, text=True, timeout=10)
    except (OSError, subprocess.TimeoutExpired) as err:
        return {"found": True, "path": path, "version": None, "error": str(err)}
    raw = (completed.stdout or completed.stderr or "").strip().splitlines()
    first = raw[0] if raw else ""
    return {"found": True, "path": path, "version": first}


def _detect_python_package(name: str) -> dict:
    try:
        import importlib.metadata as md
    except ImportError:
        return {"found": False, "name": name, "version": None}
    try:
        return {"found": True, "name": name, "version": md.version(name)}
    except md.PackageNotFoundError:
        return {"found": False, "name": name, "version": None}


def _current_git_sha() -> str | None:
    try:
        completed = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True, timeout=5
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    sha = completed.stdout.strip()
    return sha or None


def _detected_alloy_devices_ref() -> str | None:
    candidate = ROOT.parent / "alloy-devices"
    if not (candidate / ".git").exists():
        return None
    try:
        completed = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=candidate, capture_output=True, text=True, timeout=5
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    sha = completed.stdout.strip()
    return sha or None


def _board_summary_for_info() -> list[dict]:
    manifest = _read_release_manifest()
    boards = manifest.get("boards", {}) if isinstance(manifest, dict) else {}
    out: list[dict] = []
    for name in sorted(boards):
        entry = boards[name]
        out.append(
            {
                "board": name,
                "tier": entry.get("tier"),
                "required_gates": list(entry.get("required_gates", []) or []),
                "required_examples": list(entry.get("required_examples", []) or []),
                "configured": name in BOARDS,
            }
        )
    return out


def cmd_compile_commands(args: argparse.Namespace) -> None:
    cfg = board_config(args.board)
    src = cfg.build_dir / "compile_commands.json"
    if not src.exists():
        if args.configure:
            ensure_configured(cfg, args.build_type, build_tests=False)
        else:
            print(
                f"error: {src} not found. Configure first or re-run with --configure.",
                file=sys.stderr,
            )
            raise SystemExit(2)
    if not src.exists():
        print(f"error: {src} still missing after configure", file=sys.stderr)
        raise SystemExit(2)
    dest = ROOT / "compile_commands.json"
    if dest.is_symlink() or dest.exists():
        dest.unlink()
    if args.copy:
        shutil.copy2(src, dest)
        print(f"copied {src} -> {dest}")
    else:
        dest.symlink_to(src)
        print(f"linked {dest} -> {src}")


def _read_driver_manifest() -> list[dict]:
    """Load drivers/MANIFEST.json and return the drivers list."""
    path = ROOT / "drivers" / "MANIFEST.json"
    if not path.exists():
        return []
    with path.open() as f:
        data = json.load(f)
    return data.get("drivers", [])


def cmd_info(args: argparse.Namespace) -> None:
    if getattr(args, "drivers", False):
        drivers = _read_driver_manifest()
        if not drivers:
            print("drivers/MANIFEST.json not found or empty", file=sys.stderr)
            sys.exit(1)
        # Group by category for readable output.
        by_cat: dict[str, list[dict]] = {}
        for d in drivers:
            by_cat.setdefault(d["category"], []).append(d)
        col_w = {"name": 16, "chips": 28, "iface": 8, "status": 20}
        for cat in sorted(by_cat):
            print(f"\n{cat.upper()}")
            header = (
                f"  {'name':<{col_w['name']}}  {'chips':<{col_w['chips']}}"
                f"  {'interface':<{col_w['iface']}}  status"
            )
            print(header)
            print("  " + "-" * (len(header) - 2))
            for d in by_cat[cat]:
                chips = ", ".join(d.get("chips", []))
                print(
                    f"  {d['name']:<{col_w['name']}}  {chips:<{col_w['chips']}}"
                    f"  {d['interface']:<{col_w['iface']}}  {d['status']}"
                )
        total = len(drivers)
        validated = sum(1 for d in drivers if d["status"] == "hardware-validated")
        print(f"\n{total} drivers total — {validated} hardware-validated, "
              f"{total - validated} compile-review")
        return
    manifest = _read_release_manifest()
    manifest_ref = None
    if isinstance(manifest, dict):
        manifest_ref = (manifest.get("alloy_devices") or {}).get("ref")
    detected_ref = _detected_alloy_devices_ref()
    info = {
        "alloy_version": _read_project_version(),
        "git_sha": _current_git_sha(),
        "alloy_devices": {
            "manifest_ref": manifest_ref,
            "detected_ref": detected_ref,
            "aligned": (manifest_ref is not None and manifest_ref == detected_ref),
        },
        "boards": _board_summary_for_info(),
        "release_gates": sorted((manifest.get("release_gates") or {}).keys())
            if isinstance(manifest, dict) else [],
        "tools": {
            "cmake": _detect_tool_version("cmake"),
            "ninja": _detect_tool_version("ninja"),
            "arm_none_eabi_gcc": _detect_tool_version("arm-none-eabi-gcc"),
            "openocd": _detect_tool_version("openocd"),
            "python": {
                "found": True,
                "path": sys.executable,
                "version": sys.version.split()[0],
            },
        },
        "python_packages": {
            "pyserial": _detect_python_package("pyserial"),
        },
    }
    print(json.dumps(info, indent=2, sort_keys=True))


def cmd_doctor(args: argparse.Namespace) -> None:
    del args  # signature required by argparse dispatch
    failures: list[str] = []

    def check(label: str, ok: bool, hint: str) -> None:
        status = "ok" if ok else "FAIL"
        print(f"[{status}] {label}")
        if not ok:
            print(f"       hint: {hint}")
            failures.append(label)

    cmake = _detect_tool_version("cmake")
    check("cmake >= 3.25 on PATH", cmake["found"],
          "install cmake 3.25 or newer; see docs/QUICKSTART.md")

    gcc = _detect_tool_version("arm-none-eabi-gcc")
    check("arm-none-eabi-gcc on PATH", gcc["found"],
          "install the ARM bare-metal toolchain via scripts/install-xpack-toolchain.sh")

    openocd = _detect_tool_version("openocd")
    check("openocd on PATH", openocd["found"],
          "install OpenOCD for flash/debug flows")

    pyserial = _detect_python_package("pyserial")
    check("python pyserial", pyserial["found"],
          "pip install pyserial (needed for alloyctl monitor)")

    manifest = _read_release_manifest()
    manifest_ref = (manifest.get("alloy_devices") or {}).get("ref") if isinstance(manifest, dict) else None
    detected_ref = _detected_alloy_devices_ref()
    aligned = manifest_ref is not None and manifest_ref == detected_ref
    label = "alloy-devices ref aligned with RELEASE_MANIFEST.json"
    if detected_ref is None:
        check(label, False,
              "../alloy-devices checkout not found; clone it next to the alloy repo")
    else:
        check(label, aligned,
              f"check out ref {manifest_ref} in ../alloy-devices (current: {detected_ref})")

    if failures:
        print(f"\n{len(failures)} check(s) failed.", file=sys.stderr)
        raise SystemExit(1)
    print("\nAll preflight checks passed.")


def cmd_new(args: argparse.Namespace) -> None:
    if args.board not in BOARDS:
        available = ", ".join(sorted(BOARDS))
        print(f"error: board '{args.board}' not available. Choose from: {available}", file=sys.stderr)
        raise SystemExit(2)
    dest = Path(args.path).resolve()
    if dest.exists() and any(dest.iterdir()):
        print(f"error: destination {dest} exists and is not empty", file=sys.stderr)
        raise SystemExit(2)
    dest.mkdir(parents=True, exist_ok=True)
    project_name = args.name or dest.name.replace("-", "_")

    (dest / "CMakeLists.txt").write_text(
        _starter_cmakelists(project_name, args.board), encoding="utf-8"
    )
    (dest / "src").mkdir(exist_ok=True)
    (dest / "src" / "main.cpp").write_text(_starter_main_cpp(), encoding="utf-8")
    (dest / "README.md").write_text(_starter_readme(project_name, args.board), encoding="utf-8")
    (dest / ".gitignore").write_text("build/\ncompile_commands.json\n", encoding="utf-8")
    print(f"scaffolded {project_name} (board={args.board}) at {dest}")
    print("next steps:")
    print(f"  cd {dest}")
    print("  cmake -S . -B build -DALLOY_ROOT=/path/to/alloy")
    print("  cmake --build build -j8")


def _starter_cmakelists(name: str, board: str) -> str:
    return f"""cmake_minimum_required(VERSION 3.25)

# Starter firmware generated by `alloyctl new`.
# See docs/CMAKE_CONSUMPTION.md in the alloy repo for the supported integration path.

set(ALLOY_BOARD {board} CACHE STRING "Target alloy board")
set(ALLOY_ROOT "" CACHE PATH "Path to the alloy repo checkout")

if(NOT ALLOY_ROOT)
    message(FATAL_ERROR "Set -DALLOY_ROOT=/path/to/alloy when configuring this project.")
endif()

project({name} LANGUAGES C CXX ASM)

add_subdirectory(${{ALLOY_ROOT}} ${{CMAKE_BINARY_DIR}}/_alloy)

add_executable({name} src/main.cpp)
target_link_libraries({name} PRIVATE alloy::alloy)
"""


def _starter_main_cpp() -> str:
    return """// Starter firmware — mirrors examples/blink/main.cpp from the alloy repo.
// Replace with your application. See docs/COOKBOOK.md for canonical patterns.

#include "board/board.hpp"

int main() {
    board::init();
    auto led = board::make_led();
    while (true) {
        led.toggle();
        for (volatile int i = 0; i < 1'000'000; ++i) { }
    }
}
"""


def _starter_readme(name: str, board: str) -> str:
    return f"""# {name}

Starter firmware generated by `alloyctl new` for `{board}`.

## Build

```bash
cmake -S . -B build -DALLOY_ROOT=/path/to/alloy
cmake --build build -j8
```

See [docs/CMAKE_CONSUMPTION.md](https://github.com/) in the alloy repo for the supported
CMake consumption path and [docs/COOKBOOK.md](https://github.com/) for common runtime
patterns.
"""


_CATEGORY_DEFAULTS: dict[str, str] = {
    "i2c": "sensor",
    "spi": "memory",
    "uart": "net",
    "1wire": "sensor",
}


def cmd_new_driver(args: argparse.Namespace) -> None:
    name: str = args.name.lower().replace("-", "_")
    interface: str = args.interface
    category: str = args.category or _CATEGORY_DEFAULTS[interface]
    board: str = args.board

    driver_dir = ROOT / "drivers" / category / name
    test_path = ROOT / "tests" / "compile_tests" / f"test_driver_{name}.cpp"
    example_dir = ROOT / "examples" / f"driver_{name}_probe"

    for path in (driver_dir, example_dir):
        if path.exists() and any(path.iterdir()):
            print(f"error: {path} exists and is not empty", file=sys.stderr)
            raise SystemExit(2)
        path.mkdir(parents=True, exist_ok=True)

    header_path = driver_dir / f"{name}.hpp"
    header_path.write_text(_driver_header(name, category, interface), encoding="utf-8")
    test_path.write_text(_driver_compile_test(name, category, interface), encoding="utf-8")
    (example_dir / "main.cpp").write_text(_driver_probe_main(name, category, interface), encoding="utf-8")
    (example_dir / "CMakeLists.txt").write_text(_driver_probe_cmake(name), encoding="utf-8")

    print(f"scaffolded driver '{name}' (interface={interface}, category={category}, board={board})")
    print(f"  header:       {header_path.relative_to(ROOT)}")
    print(f"  compile test: {test_path.relative_to(ROOT)}")
    print(f"  probe example:{(example_dir / 'main.cpp').relative_to(ROOT)}")
    print("next steps:")
    print(f"  1. Fill in register addresses and operations in {header_path.relative_to(ROOT)}")
    print(f"  2. Update the Mock bus in {test_path.relative_to(ROOT)} if needed")
    print(f"  3. Wire the probe in {(example_dir / 'main.cpp').relative_to(ROOT)}")
    print("  4. Run: alloyctl build --board <board> --target driver_{name}_probe")


def _driver_namespace(category: str, name: str) -> str:
    return f"alloy::drivers::{category}::{name}"


def _mock_bus_i2c() -> str:
    return """\
struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t, std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
    [[nodiscard]] auto read(std::uint16_t, std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0; return alloy::core::Ok();
    }
    [[nodiscard]] auto write_read(std::uint16_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0; return alloy::core::Ok();
    }
};"""


def _mock_bus_spi() -> str:
    return """\
struct MockSpiBus {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0; return alloy::core::Ok();
    }
};"""


def _mock_bus_uart() -> str:
    return """\
struct MockUartBus {
    [[nodiscard]] auto transmit(std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
    [[nodiscard]] auto receive(std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0xFF; return alloy::core::Ok();
    }
};"""


def _mock_bus(interface: str) -> str:
    return {"i2c": _mock_bus_i2c, "spi": _mock_bus_spi,
            "uart": _mock_bus_uart, "1wire": _mock_bus_uart}[interface]()


def _bus_template(interface: str) -> str:
    if interface == "spi":
        return "template <typename BusHandle, typename CsPolicy = NoOpCsPolicy>"
    return "template <typename BusHandle>"


def _bus_ctor_params(interface: str) -> str:
    if interface == "spi":
        return "BusHandle& bus, CsPolicy cs = {}, Config cfg = {}"
    return "BusHandle& bus, Config cfg = {}"


def _bus_ctor_init(interface: str) -> str:
    if interface == "spi":
        return "bus_{&bus}, cs_{cs}, cfg_{cfg}"
    return "bus_{&bus}, cfg_{cfg}"


def _bus_private_members(interface: str) -> str:
    if interface == "spi":
        return "    BusHandle* bus_;\n    CsPolicy   cs_;\n    Config     cfg_;"
    return "    BusHandle* bus_;\n    Config     cfg_;"


def _board_bus_header(interface: str) -> str:
    return {"i2c": "BOARD_I2C_HEADER", "spi": "BOARD_SPI_HEADER",
            "uart": "BOARD_UART_HEADER", "1wire": "BOARD_GPIO_HEADER"}.get(interface, "BOARD_I2C_HEADER")


def _board_make_bus(interface: str) -> str:
    return {"i2c": "board::make_i2c()", "spi": "board::make_spi()",
            "uart": "board::make_debug_uart()", "1wire": "/* configure GPIO pin */"}.get(interface, "board::make_i2c()")


def _driver_header(name: str, category: str, interface: str) -> str:
    ns = _driver_namespace(category, name)
    tmpl = _bus_template(interface)
    ctor_params = _bus_ctor_params(interface)
    ctor_init = _bus_ctor_init(interface)
    private_members = _bus_private_members(interface)
    spi_cs_include = '\n#include "drivers/memory/w25q/w25q.hpp"  // for NoOpCsPolicy / GpioCsPolicy' if interface == "spi" else ""
    return f"""\
#pragma once

// drivers/{category}/{name}/{name}.hpp
//
// Driver for <Vendor> <PartNumber> <brief description> over {interface.upper()}.
// Written against datasheet revision <rev> (<date>).
// Seed driver: chip-ID probe + <primary operation>. See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"{spi_cs_include}

namespace {ns} {{

// ── Constants ─────────────────────────────────────────────────────────────────

// TODO: fill in device-specific addresses / chip-ID constants.
// inline constexpr std::uint16_t kDefaultAddress = 0x00;
// inline constexpr std::uint8_t  kExpectedChipId = 0x00;

// ── Types ─────────────────────────────────────────────────────────────────────

struct Config {{
    // TODO: add device-specific configuration fields with defaults.
    std::uint16_t address = 0x00;  // I2C address (remove for SPI drivers)
}};

struct Measurement {{
    // TODO: replace with the actual physical quantities this device outputs.
    float value = 0.0f;
}};

// ── Device ────────────────────────────────────────────────────────────────────

{tmpl}
class Device {{
public:
    explicit Device({ctor_params})
        : {ctor_init} {{}}

    // Verifies device presence (chip-ID or equivalent). Must be called before
    // any other method.
    [[nodiscard]] auto init() -> alloy::core::Result<void, alloy::core::ErrorCode> {{
        // TODO: implement chip-ID check or init sequence.
        (void)bus_;
        return alloy::core::Err(alloy::core::ErrorCode::NotSupported);
    }}

    // Reads one measurement from the device.
    [[nodiscard]] auto read() -> alloy::core::Result<Measurement, alloy::core::ErrorCode> {{
        // TODO: implement register read + physical-unit conversion.
        return alloy::core::Err(alloy::core::ErrorCode::NotSupported);
    }}

private:
{private_members}
}};

}}  // namespace {ns}

// Concept gate — fails at include time if the Device no longer compiles against
// the documented bus surface.
namespace {{
struct _Mock{name.capitalize()}BusGate {{
    // TODO: replace with the real bus surface for this interface.
    [[nodiscard]] auto write(std::uint16_t, std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {{ return alloy::core::Ok(); }}
    [[nodiscard]] auto read(std::uint16_t, std::span<std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {{ return alloy::core::Ok(); }}
    [[nodiscard]] auto write_read(std::uint16_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {{ return alloy::core::Ok(); }}
}};
static_assert(
    sizeof({ns}::Device<_Mock{name.capitalize()}BusGate>) > 0,
    "{name} Device must compile against the documented bus surface");
}}  // namespace
"""


def _driver_compile_test(name: str, category: str, interface: str) -> str:
    ns = _driver_namespace(category, name)
    mock = _mock_bus(interface)
    mock_type = {"i2c": "MockI2cBus", "spi": "MockSpiBus",
                 "uart": "MockUartBus", "1wire": "MockUartBus"}[interface]
    return f"""\
// Compile test: {name} seed driver instantiates against the documented public
// {interface.upper()} HAL surface. Exercises init()/read() so that any drift in
// the bus handle's signature fails the build.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/{category}/{name}/{name}.hpp"

namespace {{

{mock}

[[maybe_unused]] void compile_{name}_against_public_{interface}_handle() {{
    {mock_type} bus;
    {ns}::Device sensor{{bus}};
    (void)sensor.init();
    auto m = sensor.read();
    (void)m.is_ok();
}}

}}  // namespace
"""


def _driver_probe_main(name: str, category: str, interface: str) -> str:
    bus_header = _board_bus_header(interface)
    make_bus = _board_make_bus(interface)
    return f"""\
// examples/driver_{name}_probe/main.cpp
//
// SAME70 Xplained Ultra — {name} driver probe.
//
// TODO: document wiring (connector pins, I2C address / SPI CS, VCC/GND).
//
// Expected UART output:
//   [{name}] booting
//   [{name}] init ok
//   [{name}] value=<reading>
//   [{name}] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_{name}_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef {bus_header}
#    error "driver_{name}_probe requires {bus_header} for the selected board"
#endif

#include BOARD_UART_HEADER
#include {bus_header}

#include "drivers/{category}/{name}/{name}.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::{category}::{name};

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {{
    while (true) {{
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }}
}}

int main() {{
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) {{ halt_blink(100); }}
    uart::write_line(debug, "[{name}] booting");

    // TODO: configure bus (I2C address, SPI mode, baud rate, etc.)
    auto bus = {make_bus};
    if (bus.configure().is_err()) {{
        uart::write_line(debug, "[{name}] bus configure failed");
        halt_blink(100);
    }}

    drv::Device sensor{{bus}};
    if (sensor.init().is_err()) {{
        uart::write_line(debug, "[{name}] init failed");
        halt_blink(100);
    }}
    uart::write_line(debug, "[{name}] init ok");

    auto m = sensor.read();
    if (m.is_err()) {{
        uart::write_line(debug, "[{name}] read failed");
        halt_blink(100);
    }}

    // TODO: print measurement fields.
    uart::write_line(debug, "[{name}] read ok");
    uart::write_line(debug, "[{name}] PROBE PASS");
    halt_blink(500);
}}
"""


def _driver_probe_cmake(name: str) -> str:
    proj = f"driver_{name}_probe"
    return f"""\
cmake_minimum_required(VERSION 3.25)

project({proj}
    VERSION 1.0.0
    DESCRIPTION "{name} driver probe"
    LANGUAGES CXX C ASM
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES main.cpp)

if(DEFINED ALLOY_BOARD_SOURCE_DIR)
    set(BOARD_SOURCE_DIR "${{ALLOY_BOARD_SOURCE_DIR}}")
else()
    set(BOARD_SOURCE_DIR "${{CMAKE_SOURCE_DIR}}/boards/${{ALLOY_BOARD}}")
endif()

if(EXISTS "${{BOARD_SOURCE_DIR}}/board.cpp")
    list(APPEND SOURCES "${{BOARD_SOURCE_DIR}}/board.cpp")
endif()

if(CMAKE_CROSSCOMPILING AND EXISTS "${{BOARD_SOURCE_DIR}}/syscalls.cpp")
    list(APPEND SOURCES "${{BOARD_SOURCE_DIR}}/syscalls.cpp")
endif()

if(CMAKE_CROSSCOMPILING AND DEFINED STARTUP_SOURCE AND NOT DEFINED ALLOY_HAL_LIBRARY)
    list(APPEND SOURCES ${{STARTUP_SOURCE}})
endif()

add_executable(${{PROJECT_NAME}} ${{SOURCES}})

target_include_directories(${{PROJECT_NAME}} PRIVATE
    ${{CMAKE_SOURCE_DIR}}
    ${{CMAKE_SOURCE_DIR}}/src
    ${{CMAKE_SOURCE_DIR}}/boards
)

target_link_libraries(${{PROJECT_NAME}} PRIVATE ${{ALLOY_HAL_LIBRARY}})

target_compile_options(${{PROJECT_NAME}} PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O2 -g>
    $<$<CONFIG:MinSizeRel>:-Os -g>
)

if(CMAKE_CROSSCOMPILING)
    target_compile_options(${{PROJECT_NAME}} PRIVATE
        -ffunction-sections -fdata-sections
        -fno-exceptions -fno-rtti -fno-threadsafe-statics
    )
    target_link_options(${{PROJECT_NAME}} PRIVATE
        -Wl,--gc-sections -Wl,--print-memory-usage
        --specs=nano.specs --specs=nosys.specs
    )
    if(DEFINED LINKER_SCRIPT AND EXISTS "${{LINKER_SCRIPT}}")
        target_link_options(${{PROJECT_NAME}} PRIVATE -T${{LINKER_SCRIPT}})
    endif()
    add_custom_command(TARGET ${{PROJECT_NAME}} POST_BUILD
        COMMAND ${{CMAKE_OBJCOPY}} -O ihex $<TARGET_FILE:${{PROJECT_NAME}}> ${{PROJECT_NAME}}.hex
        COMMAND ${{CMAKE_OBJCOPY}} -O binary $<TARGET_FILE:${{PROJECT_NAME}}> ${{PROJECT_NAME}}.bin
        COMMAND ${{CMAKE_SIZE}} $<TARGET_FILE:${{PROJECT_NAME}}>
        WORKING_DIRECTORY ${{CMAKE_CURRENT_BINARY_DIR}}
        COMMENT "Generating HEX and BIN files"
    )
endif()
"""


def cmd_diff(args: argparse.Namespace) -> None:
    manifest = load_manifest()
    left = board_config(args.from_board)
    right = board_config(args.to_board)
    left_release = board_release_entry(manifest, left)
    right_release = board_release_entry(manifest, right)
    left_insight = board_insight(left)
    right_insight = board_insight(right)

    left_examples = set(left_release["required_examples"])
    right_examples = set(right_release["required_examples"])
    left_gates = set(left_release["required_gates"])
    right_gates = set(right_release["required_gates"])
    left_connectors = {item.alias for item in left_insight.connectors}
    right_connectors = {item.alias for item in right_insight.connectors}
    shared_connectors = sorted(left_connectors & right_connectors)
    watchpoints = migration_watchpoints(left, right, left_insight, right_insight, left_release, right_release)

    format_lines(
        [
            f"from: {left.board}",
            f"to: {right.board}",
            f"tier: {left_release['tier']} -> {right_release['tier']}",
            f"clock(source): {left_insight.clock_summary}",
            f"clock(target): {right_insight.clock_summary}",
            f"debug-uart(source): {left_insight.debug_uart_summary}",
            f"debug-uart(target): {right_insight.debug_uart_summary}",
            f"required examples only in {left.board}: {', '.join(sorted(left_examples - right_examples)) or 'none'}",
            f"required examples only in {right.board}: {', '.join(sorted(right_examples - left_examples)) or 'none'}",
            f"required gates only in {left.board}: {', '.join(sorted(left_gates - right_gates)) or 'none'}",
            f"required gates only in {right.board}: {', '.join(sorted(right_gates - left_gates)) or 'none'}",
            f"connector aliases only in {left.board}: {', '.join(sorted(left_connectors - right_connectors)) or 'none'}",
            f"connector aliases only in {right.board}: {', '.join(sorted(right_connectors - left_connectors)) or 'none'}",
            f"connector aliases shared by both boards: {', '.join(shared_connectors) or 'none'}",
            f"sample ergonomic route ({left.board}): {ergonomic_route(left_insight.connectors[0])}",
            f"sample ergonomic route ({right.board}): {ergonomic_route(right_insight.connectors[0])}",
            "migration watchpoints:",
            *[f"  - {note}" for note in watchpoints],
        ]
    )


def _maybe_warn_deprecated_invocation() -> None:
    """Warn once when invoked directly as `python scripts/alloyctl.py ...`.

    `alloy-cli` re-uses this module by importing it and setting ``sys.argv[0] = "alloy"``
    before calling :func:`main`; in that case we skip the warning so the user only sees it
    when they reach for the legacy script themselves.
    """
    invoked = Path(sys.argv[0]).name if sys.argv else ""
    if invoked == "alloy":
        return
    sys.stderr.write(
        "alloyctl.py is a deprecated alias for the `alloy` CLI; "
        "prefer `pipx install alloy-cli` and `alloy <command> ...` (see docs/CLI.md).\n"
    )


def main() -> int:
    _maybe_warn_deprecated_invocation()
    p = argparse.ArgumentParser(prog="alloyctl", description="Board-aware Alloy helper")
    sub = p.add_subparsers(dest="cmd", required=True)

    def add_build_args(sp: argparse.ArgumentParser) -> None:
        sp.add_argument("--board", required=True)
        sp.add_argument("--build-type", default="Debug", choices=("Debug", "Release", "MinSizeRel", "RelWithDebInfo"))

    cfg_p = sub.add_parser("configure")
    add_build_args(cfg_p)
    cfg_p.add_argument("--build-tests", action="store_true")
    cfg_p.set_defaults(func=cmd_configure)

    build_p = sub.add_parser("build")
    add_build_args(build_p)
    build_p.add_argument("--target", required=True)
    build_p.add_argument("-j", "--jobs", type=int, default=8)
    build_p.set_defaults(func=cmd_build)

    bundle_p = sub.add_parser("bundle")
    add_build_args(bundle_p)
    bundle_p.add_argument("-j", "--jobs", type=int, default=8)
    bundle_p.set_defaults(func=cmd_bundle)

    flash_p = sub.add_parser("flash")
    add_build_args(flash_p)
    flash_p.add_argument("--target", required=True)
    flash_p.add_argument("--build-first", action="store_true")
    flash_p.add_argument("--recover", action="store_true")
    flash_p.add_argument("--dry-run", action="store_true")
    flash_p.add_argument("--flash-backend", default="auto", choices=("auto", "openocd", "stm32cube"))
    flash_p.add_argument("-j", "--jobs", type=int, default=8)
    flash_p.set_defaults(func=cmd_flash)

    recover_p = sub.add_parser("recover")
    add_build_args(recover_p)
    recover_p.add_argument("--target", required=True)
    recover_p.add_argument("--dry-run", action="store_true")
    recover_p.add_argument("--flash-backend", default="auto", choices=("auto", "openocd", "stm32cube"))
    recover_p.add_argument("-j", "--jobs", type=int, default=8)
    recover_p.set_defaults(func=cmd_recover)

    mon_p = sub.add_parser("monitor")
    mon_p.add_argument("--board", required=True)
    mon_p.add_argument("--port")
    mon_p.add_argument("--baud", type=int)
    mon_p.set_defaults(func=cmd_monitor)

    gdb_p = sub.add_parser("gdbserver")
    gdb_p.add_argument("--board", required=True)
    gdb_p.add_argument("--gdb-port", type=int, default=3333)
    gdb_p.set_defaults(func=cmd_gdbserver)

    validate_p = sub.add_parser("validate")
    validate_p.add_argument("--board", required=True)
    validate_p.add_argument("--kind", default="runtime")
    validate_p.set_defaults(func=cmd_validate)

    sweep_p = sub.add_parser("sweep")
    add_build_args(sweep_p)
    sweep_p.add_argument("--port")
    sweep_p.add_argument("--baud", type=int)
    sweep_p.add_argument("--monitor-seconds", type=float, default=5.0)
    sweep_p.add_argument("--settle-seconds", type=float, default=1.0)
    sweep_p.add_argument("--flash-retries", type=int, default=2)
    sweep_p.add_argument("--retry-delay-seconds", type=float, default=1.0)
    sweep_p.add_argument("--target", dest="targets", action="append")
    sweep_p.add_argument("-j", "--jobs", type=int, default=8)
    sweep_p.set_defaults(func=cmd_sweep)

    explain_p = sub.add_parser("explain")
    explain_p.add_argument("--board", required=True)
    explain_p.add_argument("--connector")
    explain_p.add_argument("--clock", action="store_true")
    explain_p.add_argument("--peripheral")
    explain_p.add_argument("--gate")
    explain_p.add_argument("--example")
    explain_p.set_defaults(func=cmd_explain)

    diff_p = sub.add_parser("diff")
    diff_p.add_argument("--from", dest="from_board", required=True)
    diff_p.add_argument("--to", dest="to_board", required=True)
    diff_p.set_defaults(func=cmd_diff)

    cc_p = sub.add_parser(
        "compile-commands",
        help="expose compile_commands.json at the repo root for clangd/LSP",
    )
    add_build_args(cc_p)
    cc_p.add_argument("--configure", action="store_true",
                      help="configure the board build dir first if compile_commands.json is missing")
    cc_p.add_argument("--copy", action="store_true",
                      help="copy the file instead of symlinking")
    cc_p.set_defaults(func=cmd_compile_commands)

    info_p = sub.add_parser("info", help="print machine-readable environment report (JSON)")
    info_p.add_argument("--drivers", action="store_true",
                        help="list available drivers and status from drivers/MANIFEST.json")
    info_p.set_defaults(func=cmd_info)

    doctor_p = sub.add_parser("doctor", help="preflight check: toolchain, probe, python deps, ref")
    doctor_p.set_defaults(func=cmd_doctor)

    run_p = sub.add_parser(
        "run",
        help="build + flash + open serial monitor for an example target (openocd-only, no bossac)",
    )
    run_p.add_argument(
        "--board",
        help="board name (auto-detected from the single configured build dir, or from .alloyctl-board)",
    )
    run_p.add_argument("--build-type", default="Debug",
                       choices=("Debug", "Release", "MinSizeRel", "RelWithDebInfo"))
    run_p.add_argument("--target", required=True,
                       help="cmake target name (e.g. driver_at24mac402_probe)")
    run_p.add_argument("--flash-backend", default="auto",
                       choices=("auto", "openocd", "stm32cube"))
    run_p.add_argument("-j", "--jobs", type=int, default=8)
    run_p.add_argument("--port", help="override serial port (default: auto-detect)")
    run_p.add_argument("--baud", type=int, help="override serial baud (default: board setting)")
    run_p.add_argument("--settle-seconds", type=float, default=0.5,
                       help="delay between flash reset and monitor open")
    run_p.set_defaults(func=cmd_run)

    new_p = sub.add_parser("new", help="scaffold a downstream firmware starter for a board")
    new_p.add_argument("--board", required=True, help="foundational board to target")
    new_p.add_argument("--path", required=True, help="destination directory for the new project")
    new_p.add_argument("--name", help="override project name (defaults to the destination basename)")
    new_p.set_defaults(func=cmd_new)

    nd_p = sub.add_parser("new-driver", help="scaffold a new seed driver (header + compile test + probe example)")
    nd_p.add_argument("--name", required=True, help="driver name in snake_case (e.g. sht40)")
    nd_p.add_argument("--interface", required=True, choices=["i2c", "spi", "uart", "1wire"],
                      help="bus interface the driver uses")
    nd_p.add_argument("--category", default="",
                      help="driver category: sensor, display, memory, power, net (auto-inferred if omitted)")
    nd_p.add_argument("--board", default="same70_xplained",
                      help="target board for the probe example (default: same70_xplained)")
    nd_p.set_defaults(func=cmd_new_driver)

    args = p.parse_args()
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
