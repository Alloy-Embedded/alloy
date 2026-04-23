#!/usr/bin/env python3
from __future__ import annotations

import argparse
import glob
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


@dataclass(frozen=True)
class BoardConfig:
    board: str
    build_dir: Path
    bundle_target: str
    firmware_targets: tuple[str, ...]
    openocd_args: tuple[str, ...]
    uart_globs: tuple[str, ...]
    uart_baud: int


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


def die(msg: str) -> int:
    print(msg, file=sys.stderr)
    return 1


def board_config(name: str) -> BoardConfig:
    cfg = BOARDS.get(name)
    if cfg is None:
        raise SystemExit(die(f"unsupported board: {name}"))
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


def main() -> int:
    p = argparse.ArgumentParser(prog="alloyctl", description="Board-aware Alloy helper")
    sub = p.add_subparsers(dest="cmd", required=True)

    def add_build_args(sp: argparse.ArgumentParser) -> None:
        sp.add_argument("--board", required=True, choices=sorted(BOARDS))
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
    mon_p.add_argument("--board", required=True, choices=sorted(BOARDS))
    mon_p.add_argument("--port")
    mon_p.add_argument("--baud", type=int)
    mon_p.set_defaults(func=cmd_monitor)

    gdb_p = sub.add_parser("gdbserver")
    gdb_p.add_argument("--board", required=True, choices=sorted(BOARDS))
    gdb_p.add_argument("--gdb-port", type=int, default=3333)
    gdb_p.set_defaults(func=cmd_gdbserver)

    validate_p = sub.add_parser("validate")
    validate_p.add_argument(
        "--board",
        required=True,
        choices=("host", *sorted(BOARDS)),
    )
    validate_p.add_argument(
        "--kind",
        default="runtime",
        choices=("runtime", "smoke", "zero-overhead"),
    )
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

    args = p.parse_args()
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
