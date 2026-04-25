#!/usr/bin/env python3
from __future__ import annotations

import argparse
import difflib
import glob
import json
import re
import shutil
import subprocess
import sys
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
    esptool_chip: str = ""   # non-empty => use esptool instead of openocd


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


def flash_plan_lines(cfg: BoardConfig, target: str, backend: str, recover: bool) -> list[str]:
    action = "recover-and-flash" if recover else "flash"
    lines = [
        f"board: {cfg.board}",
        f"target: {target}",
        f"action: {action}",
        f"selected-backend: {backend}",
    ]

    if backend == "esptool":
        lines.extend([
            "supported-flow: esptool.py direct-boot write_flash 0x0",
            "planned-steps:",
            "  - convert ELF to raw binary (objcopy -O binary)",
            "  - esptool.py --chip esp32c3 write_flash 0x0 <target>.bin",
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
            cache_value(cfg, "CMAKE_BUILD_TYPE") != build_type):
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
            "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake",
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
        require_tool("esptool.py")
        elf = artifact(cfg, args.target)
        bin_path = elf.with_suffix(".bin")
        objcopy = shutil.which("riscv32-esp-elf-objcopy") or "riscv32-esp-elf-objcopy"
        run([objcopy, "-O", "binary", str(elf), str(bin_path)])
        port = getattr(args, "port", None) or auto_port(cfg)
        cmd = [
            "esptool.py",
            "--chip", cfg.esptool_chip,
            "--port", port,
            "--baud", "460800",
            "write_flash", "0x0", str(bin_path),
        ]
        run(cmd)
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
    print(f"+ resetting target via {backend} ...")
    if backend == "stm32cube":
        cli = find_stm32_programmer_cli()
        if cli is not None:
            run_soft([cli, "-c", "port=SWD", "mode=UR",
                      "reset=HWrst", "freq=100", "-rst"])
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


def cmd_info(args: argparse.Namespace) -> None:
    del args  # signature required by argparse dispatch
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

    args = p.parse_args()
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
