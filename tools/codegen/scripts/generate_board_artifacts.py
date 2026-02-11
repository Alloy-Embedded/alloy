#!/usr/bin/env python3
"""
Generate board-derived artifacts from canonical boards/*/board.yaml metadata.

Artifacts:
  - boards/<board>/board_config.hpp
  - boards/generated/board_catalog.json
  - boards/generated/<board>/board_metadata.hpp
  - cmake/generated/board_metadata.cmake
  - cmake/generated/boards/<board>.cmake
"""

from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from pathlib import Path
from typing import Any, Dict, Tuple

import yaml


PROJECT_ROOT = Path(__file__).resolve().parents[3]
CODEGEN_ROOT = PROJECT_ROOT / "tools" / "codegen"
CONTRACT_PATH = CODEGEN_ROOT / "contracts" / "board_artifacts_contract.json"

if str(CODEGEN_ROOT) not in sys.path:
    sys.path.insert(0, str(CODEGEN_ROOT))

from cli.generators.board_generator import BoardGenerator  # noqa: E402
from cli.loaders.board_yaml_loader import BoardYAMLLoader  # noqa: E402
from core.version import VERSION as CODEGEN_VERSION  # noqa: E402


def _arch_display(arch: str) -> str:
    mapping = {
        "cortex-m0": "Cortex-M0",
        "cortex-m0+": "Cortex-M0+",
        "cortex-m3": "Cortex-M3",
        "cortex-m4": "Cortex-M4",
        "cortex-m7": "Cortex-M7",
        "cortex-m33": "Cortex-M33",
    }
    return mapping.get(arch, arch)


def _relative(path: Path) -> str:
    return str(path.relative_to(PROJECT_ROOT))


def _read_yaml(path: Path) -> Dict:
    with path.open("r", encoding="utf-8") as handle:
        data = yaml.safe_load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"Invalid YAML object in {path}")
    return data


def _read_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"Invalid JSON object in {path}")
    return data


def _parse_framework_version() -> tuple[str, int, int, int]:
    cmake_lists = PROJECT_ROOT / "CMakeLists.txt"
    content = cmake_lists.read_text(encoding="utf-8")
    match = re.search(r"project\(\s*microcore[\s\S]*?VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)", content)
    if not match:
        raise RuntimeError("Unable to locate framework version in CMakeLists.txt")

    version = match.group(1)
    major, minor, patch = (int(part) for part in version.split("."))
    return version, major, minor, patch


def _load_codegen_contract() -> Dict[str, Any]:
    contract = _read_json(CONTRACT_PATH)
    generator = contract.get("generator", {})
    expected_codegen_version = generator.get("version")
    if expected_codegen_version != CODEGEN_VERSION:
        raise RuntimeError(
            "Codegen contract mismatch: "
            f"contract expects generator version '{expected_codegen_version}' "
            f"but runtime codegen version is '{CODEGEN_VERSION}'"
        )
    return contract


def _extract_board_schema_version(boards: Dict[str, Dict]) -> str:
    schema_versions = sorted({str(raw.get("schema_version", "")).strip() for raw in boards.values()})
    if not schema_versions or schema_versions == [""]:
        raise RuntimeError("Board metadata is missing required 'schema_version'")
    if len(schema_versions) != 1:
        raise RuntimeError(f"Mixed board schema versions detected: {schema_versions}")
    return schema_versions[0]


def _load_board_metadata() -> Dict[str, Dict]:
    loader = BoardYAMLLoader()
    boards_dir = PROJECT_ROOT / "boards"
    boards: Dict[str, Dict] = {}

    for yaml_path in sorted(boards_dir.glob("*/board.yaml")):
        # Schema validation + type checks
        loader.load_board(yaml_path)
        raw = _read_yaml(yaml_path)

        board_id = raw["board"]["id"]
        dir_id = yaml_path.parent.name
        if board_id != dir_id:
            raise ValueError(
                f"Board ID mismatch in {_relative(yaml_path)}: "
                f"board.id='{board_id}' but directory is '{dir_id}'"
            )

        boards[board_id] = raw

    if not boards:
        raise RuntimeError("No board.yaml files found under boards/*/")

    return boards


def _render_board_catalog(
    boards: Dict[str, Dict],
    contract: Dict[str, Any],
    framework_version: str,
    board_schema_version: str,
) -> str:
    payload = {
        "schema_version": "1.0",
        "artifact_contract": {
            "contract_id": contract["contract_id"],
            "contract_version": contract["contract_version"],
            "generator_name": contract["generator"]["name"],
            "generator_version": contract["generator"]["version"],
            "framework_version": framework_version,
            "board_schema_version": board_schema_version,
        },
        "boards": {},
    }

    for board_id in sorted(boards):
        raw = boards[board_id]
        clock_hz = int(raw["clock"]["system_clock_hz"])
        mcu = raw["mcu"]
        tooling = raw["tooling"]
        build = raw["build"]

        payload["boards"][board_id] = {
            "id": board_id,
            "name": raw["board"]["name"],
            "vendor": raw["board"]["vendor"],
            "description": raw["board"].get("description", ""),
            "url": raw["board"].get("url", ""),
            "platform": raw["platform"],
            "mcu": {
                "part_number": mcu["part_number"],
                "generated_namespace": mcu.get("generated_namespace", ""),
                "architecture": mcu["architecture"],
                "architecture_display": _arch_display(mcu["architecture"]),
                "flash_kb": mcu.get("flash_kb", 0),
                "ram_kb": mcu.get("ram_kb", 0),
                "frequency_mhz": mcu.get("frequency_mhz", clock_hz // 1_000_000),
            },
            "clock": {
                "system_clock_hz": clock_hz,
                "frequency_human": f"{clock_hz // 1_000_000} MHz",
            },
            "build": {
                "linker_script": build["linker_script"],
                "startup_source": build["startup_source"],
                "toolchain_file": build["toolchain_file"],
                "default_build_type": build.get("default_build_type", "Release"),
                "supports_rtos": bool(build.get("supports_rtos", False)),
                "supported_examples": build.get("supported_examples", []),
            },
            "tooling": tooling,
        }

    return json.dumps(payload, indent=2, sort_keys=True) + "\n"


def _render_board_metadata_header(board_id: str, raw: Dict) -> str:
    board = raw["board"]
    mcu = raw["mcu"]
    clock = raw["clock"]
    tooling = raw["tooling"]
    build = raw["build"]

    flash = tooling["flash"]
    debug = tooling["debug"]

    lines = [
        "#pragma once",
        "",
        "/**",
        f" * @file board_metadata.hpp",
        f" * @brief Generated board metadata fragment for {board_id}",
        " *",
        " * Auto-generated from boards/*/board.yaml.",
        " * DO NOT EDIT MANUALLY.",
        " */",
        "",
        "namespace microcore::generated::board_metadata::" + board_id + " {",
        "",
        f'inline constexpr char board_id[] = "{board_id}";',
        f'inline constexpr char board_name[] = "{board["name"]}";',
        f'inline constexpr char board_vendor[] = "{board["vendor"]}";',
        f'inline constexpr char platform[] = "{raw["platform"]}";',
        f'inline constexpr char mcu_part_number[] = "{mcu["part_number"]}";',
        f'inline constexpr char architecture[] = "{mcu["architecture"]}";',
        f"inline constexpr unsigned int system_clock_hz = {int(clock['system_clock_hz'])}u;",
        "",
        f'inline constexpr char linker_script[] = "{build["linker_script"]}";',
        f'inline constexpr char startup_source[] = "{build["startup_source"]}";',
        f'inline constexpr char toolchain_file[] = "{build["toolchain_file"]}";',
        "",
        f'inline constexpr char flash_command[] = "{flash["command"]}";',
        f'inline constexpr char flash_binary_format[] = "{flash.get("binary_format", "bin")}";',
        f'inline constexpr char flash_load_address[] = "{flash.get("load_address", "")}";',
        f'inline constexpr char openocd_target[] = "{flash.get("openocd_target", "")}";',
        f'inline constexpr char openocd_interface[] = "{flash.get("openocd_interface", "")}";',
        f'inline constexpr char debug_transport[] = "{debug["transport"]}";',
        f'inline constexpr char debug_probe[] = "{debug.get("probe", "")}";',
        "",
        f"inline constexpr unsigned int led_count = {len(raw.get('leds', []))}u;",
        f"inline constexpr unsigned int button_count = {len(raw.get('buttons', []))}u;",
        f"inline constexpr unsigned int uart_count = {len(raw.get('uart', []))}u;",
        f"inline constexpr unsigned int spi_count = {len(raw.get('spi', []))}u;",
        f"inline constexpr unsigned int i2c_count = {len(raw.get('i2c', []))}u;",
        "",
        "}  // namespace microcore::generated::board_metadata::" + board_id,
        "",
    ]
    return "\n".join(lines)


def _render_cmake_board_fragment(board_id: str, raw: Dict) -> str:
    board = raw["board"]
    mcu = raw["mcu"]
    build = raw["build"]
    flash = raw["tooling"]["flash"]
    supported_examples = build.get("supported_examples", [])
    supported_examples_list = ";".join(supported_examples)
    supports_rtos = "ON" if bool(build.get("supports_rtos", False)) else "OFF"

    lines = [
        "# Auto-generated from boards/*/board.yaml",
        f"# Board: {board_id}",
        "",
        f'set(MICROCORE_GENERATED_BOARD_PLATFORM_{board_id} "{raw["platform"]}")',
        f'set(MICROCORE_GENERATED_BOARD_NAME_{board_id} "{board["name"]}")',
        f'set(MICROCORE_GENERATED_BOARD_VENDOR_{board_id} "{board["vendor"]}")',
        f'set(MICROCORE_GENERATED_BOARD_MCU_{board_id} "{mcu["part_number"]}")',
        f'set(MICROCORE_GENERATED_BOARD_ARCH_{board_id} "{mcu["architecture"]}")',
        f'set(MICROCORE_GENERATED_BOARD_HEADER_{board_id} "boards/{board_id}/board.hpp")',
        f'set(MICROCORE_GENERATED_BOARD_LINKER_SCRIPT_{board_id} "{build["linker_script"]}")',
        f'set(MICROCORE_GENERATED_BOARD_STARTUP_SOURCE_{board_id} "{build["startup_source"]}")',
        f'set(MICROCORE_GENERATED_BOARD_TOOLCHAIN_{board_id} "{build["toolchain_file"]}")',
        f'set(MICROCORE_GENERATED_BOARD_SUPPORTED_EXAMPLES_{board_id} "{supported_examples_list}")',
        f'set(MICROCORE_GENERATED_BOARD_SUPPORTS_RTOS_{board_id} "{supports_rtos}")',
        f'set(MICROCORE_GENERATED_BOARD_FLASH_COMMAND_{board_id} "{flash["command"]}")',
        f'set(MICROCORE_GENERATED_BOARD_FLASH_ADDRESS_{board_id} "{flash.get("load_address", "")}")',
        f'set(MICROCORE_GENERATED_BOARD_OPENOCD_TARGET_{board_id} "{flash.get("openocd_target", "")}")',
        f'set(MICROCORE_GENERATED_BOARD_OPENOCD_INTERFACE_{board_id} "{flash.get("openocd_interface", "")}")',
        "",
    ]
    return "\n".join(lines)


def _render_cmake_catalog(
    boards: Dict[str, Dict],
    contract: Dict[str, Any],
    framework_version: str,
    framework_major: int,
    framework_minor: int,
    framework_patch: int,
    board_schema_version: str,
) -> str:
    compat = contract["framework_compatibility"]
    lines = [
        "# Auto-generated from boards/*/board.yaml",
        "# DO NOT EDIT MANUALLY.",
        "",
        f'set(MICROCORE_GENERATED_CODEGEN_CONTRACT_ID "{contract["contract_id"]}")',
        f'set(MICROCORE_GENERATED_CODEGEN_CONTRACT_VERSION "{contract["contract_version"]}")',
        f'set(MICROCORE_GENERATED_CODEGEN_VERSION "{contract["generator"]["version"]}")',
        f'set(MICROCORE_GENERATED_BOARD_SCHEMA_VERSION "{board_schema_version}")',
        f'set(MICROCORE_GENERATED_FRAMEWORK_VERSION "{framework_version}")',
        f'set(MICROCORE_GENERATED_FRAMEWORK_VERSION_MAJOR "{framework_major}")',
        f'set(MICROCORE_GENERATED_FRAMEWORK_VERSION_MINOR "{framework_minor}")',
        f'set(MICROCORE_GENERATED_FRAMEWORK_VERSION_PATCH "{framework_patch}")',
        f'set(MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MAJOR "{compat["compatible_major"]}")',
        f'set(MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MIN "{compat["compatible_minor_min"]}")',
        f'set(MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MAX "{compat["compatible_minor_max"]}")',
        "",
        "set(MICROCORE_GENERATED_SUPPORTED_BOARDS",
    ]
    for board_id in sorted(boards):
        lines.append(f'    "{board_id}"')
    lines.append(")")
    lines.append("")

    lines.append("set(MICROCORE_GENERATED_BOARD_PLATFORM_MAP")
    for board_id in sorted(boards):
        platform = boards[board_id]["platform"]
        lines.append(f'    "{board_id}:{platform}"')
    lines.append(")")
    lines.append("")

    for board_id in sorted(boards):
        lines.append(_render_cmake_board_fragment(board_id, boards[board_id]).rstrip())
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def _render_contract_manifest(
    boards: Dict[str, Dict],
    contract: Dict[str, Any],
    framework_version: str,
    framework_major: int,
    framework_minor: int,
    framework_patch: int,
    board_schema_version: str,
) -> str:
    payload = {
        "contract_id": contract["contract_id"],
        "contract_version": contract["contract_version"],
        "generator": {
            "name": contract["generator"]["name"],
            "version": contract["generator"]["version"],
        },
        "framework_version": {
            "value": framework_version,
            "major": framework_major,
            "minor": framework_minor,
            "patch": framework_patch,
        },
        "framework_compatibility": contract["framework_compatibility"],
        "board_schema_version": board_schema_version,
        "boards": sorted(boards.keys()),
        "runtime_artifacts": contract["outputs"]["runtime_artifacts"],
    }
    return json.dumps(payload, indent=2, sort_keys=True) + "\n"


def _write_or_check(path: Path, content: str, check: bool) -> Tuple[bool, str]:
    if check:
        if not path.exists():
            return False, f"Missing generated file: {_relative(path)}"

        current = path.read_text(encoding="utf-8")
        if current != content:
            diff = "".join(
                difflib.unified_diff(
                    current.splitlines(keepends=True),
                    content.splitlines(keepends=True),
                    fromfile=str(path),
                    tofile=f"{path} (expected)",
                    n=3,
                )
            )
            message = f"Out-of-date generated file: {_relative(path)}\n{diff}"
            return False, message

        return True, f"OK: {_relative(path)}"

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True, f"Wrote: {_relative(path)}"


def _generate_board_configs(boards: Dict[str, Dict], check: bool) -> Tuple[bool, list[str]]:
    generator = BoardGenerator()
    messages: list[str] = []
    ok = True

    for board_id in sorted(boards):
        yaml_path = PROJECT_ROOT / "boards" / board_id / "board.yaml"
        output_path = PROJECT_ROOT / "boards" / board_id / "board_config.hpp"

        board_config = generator.loader.load_board(yaml_path)
        rendered = generator._render_template(board_config)
        this_ok, msg = _write_or_check(output_path, rendered, check=check)
        messages.append(msg)
        ok = ok and this_ok

    return ok, messages


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate board-derived artifacts")
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check generated artifacts are up-to-date (no writes)",
    )
    args = parser.parse_args()

    try:
        contract = _load_codegen_contract()
        framework_version, framework_major, framework_minor, framework_patch = _parse_framework_version()
        boards = _load_board_metadata()
        board_schema_version = _extract_board_schema_version(boards)
    except Exception as exc:
        print(f"[error] Failed to load board metadata: {exc}")
        return 1

    results: list[Tuple[bool, str]] = []

    # board_config.hpp generation
    config_ok, config_messages = _generate_board_configs(boards, check=args.check)
    results.extend((config_ok, msg) for msg in config_messages)

    # CLI board catalog
    catalog_path = PROJECT_ROOT / "boards" / "generated" / "board_catalog.json"
    catalog_content = _render_board_catalog(
        boards=boards,
        contract=contract,
        framework_version=framework_version,
        board_schema_version=board_schema_version,
    )
    results.append(_write_or_check(catalog_path, catalog_content, check=args.check))

    # Contract manifest (framework/codegen compatibility pin)
    contract_manifest_path = PROJECT_ROOT / "boards" / "generated" / "codegen_contract_manifest.json"
    contract_manifest_content = _render_contract_manifest(
        boards=boards,
        contract=contract,
        framework_version=framework_version,
        framework_major=framework_major,
        framework_minor=framework_minor,
        framework_patch=framework_patch,
        board_schema_version=board_schema_version,
    )
    results.append(_write_or_check(contract_manifest_path, contract_manifest_content, check=args.check))

    # CMake catalog
    cmake_catalog_path = PROJECT_ROOT / "cmake" / "generated" / "board_metadata.cmake"
    cmake_catalog_content = _render_cmake_catalog(
        boards=boards,
        contract=contract,
        framework_version=framework_version,
        framework_major=framework_major,
        framework_minor=framework_minor,
        framework_patch=framework_patch,
        board_schema_version=board_schema_version,
    )
    results.append(_write_or_check(cmake_catalog_path, cmake_catalog_content, check=args.check))

    # Per-board fragments
    for board_id in sorted(boards):
        board_raw = boards[board_id]
        board_cmake_path = PROJECT_ROOT / "cmake" / "generated" / "boards" / f"{board_id}.cmake"
        board_cmake_content = _render_cmake_board_fragment(board_id, board_raw)
        results.append(_write_or_check(board_cmake_path, board_cmake_content, check=args.check))

        board_header_path = PROJECT_ROOT / "boards" / "generated" / board_id / "board_metadata.hpp"
        board_header_content = _render_board_metadata_header(board_id, board_raw)
        results.append(_write_or_check(board_header_path, board_header_content, check=args.check))

    all_ok = True
    for ok, message in results:
        if message:
            print(message)
        all_ok = all_ok and ok

    if args.check:
        if all_ok:
            print("Board metadata artifacts: up-to-date")
            return 0
        print("Board metadata artifacts: drift detected")
        return 1

    print("Board metadata artifacts: generated")
    return 0


if __name__ == "__main__":
    sys.exit(main())
