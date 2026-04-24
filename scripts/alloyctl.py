#!/usr/bin/env python3
from __future__ import annotations

import argparse
import difflib
import glob
import json
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
}

BOARD_INSIGHTS: dict[str, BoardInsight] = {
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


def main() -> int:
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

    args = p.parse_args()
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
