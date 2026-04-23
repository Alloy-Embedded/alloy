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
    require_tool("openocd")
    if args.build_first:
        ensure_configured(cfg, args.build_type, build_tests=False)
        build(cfg, args.target, args.jobs)
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
            raise SystemExit(
                die(
                    f"unsupported connector alias '{args.connector}' for {cfg.board}; "
                    f"supported connectors: {', '.join(options)}{suggest(args.connector, options)}"
                )
            )
        lines = [
            f"board: {cfg.board}",
            f"connector: {connector.alias}",
            f"peripheral: {connector.peripheral}",
            "bindings:",
            *[f"  - {binding}" for binding in connector.bindings],
        ]
        if connector.note:
            lines.append(f"note: {connector.note}")
        format_lines(lines)
        return

    if args.clock:
        format_lines(
            [
                f"board: {cfg.board}",
                f"clock: {insight.clock_summary}",
                f"debug-uart: {insight.debug_uart_summary}",
                f"required gates: {', '.join(release['required_gates'])}",
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
            f"clock: {insight.clock_summary}",
            f"debug-uart: {insight.debug_uart_summary}",
            f"required examples: {', '.join(release['required_examples'])}",
            f"required gates: {', '.join(release['required_gates'])}",
            f"known connector aliases: {', '.join(item.alias for item in insight.connectors)}",
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

    format_lines(
        [
            f"from: {left.board}",
            f"to: {right.board}",
            f"tier: {left_release['tier']} -> {right_release['tier']}",
            f"clock: {left_insight.clock_summary}",
            f"clock(target): {right_insight.clock_summary}",
            f"debug-uart: {left_insight.debug_uart_summary}",
            f"debug-uart(target): {right_insight.debug_uart_summary}",
            f"required examples only in {left.board}: {', '.join(sorted(left_examples - right_examples)) or 'none'}",
            f"required examples only in {right.board}: {', '.join(sorted(right_examples - left_examples)) or 'none'}",
            f"required gates only in {left.board}: {', '.join(sorted(left_gates - right_gates)) or 'none'}",
            f"required gates only in {right.board}: {', '.join(sorted(right_gates - left_gates)) or 'none'}",
            f"connector aliases only in {left.board}: {', '.join(sorted(left_connectors - right_connectors)) or 'none'}",
            f"connector aliases only in {right.board}: {', '.join(sorted(right_connectors - left_connectors)) or 'none'}",
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
    flash_p.add_argument("-j", "--jobs", type=int, default=8)
    flash_p.set_defaults(func=cmd_flash)

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
