"""Flash runners.

The board declares its probe (board.json "probe"); this module picks the
first available runner on the host: probe-rs (preferred), openocd, st-flash,
or — for probe-less boards like the Raspberry Pi Pico — a built-in UF2
encoder that copies to the BOOTSEL mass-storage volume. Runner choice is
host tooling BEHAVIOR — the only board/chip FACTS used are the declared
probe kind, chip ids/family and the flash base address.
"""

from __future__ import annotations

import shutil
import struct
import subprocess
import time
from pathlib import Path
from typing import Any

from .emit.common import EmitError

# OpenOCD target script per chip family (tool-integration map, not silicon).
_OPENOCD_TARGET = {
    "stm32g0": "stm32g0x",
    "stm32f4": "stm32f4x",
    "stm32f7": "stm32f7x",
    "stm32g4": "stm32g4x",
    "same70": "atsamv",
}

_OPENOCD_INTERFACE = {
    "stlink": "stlink",
    "cmsis-dap": "cmsis-dap",
}


def flash(board: dict[str, Any], chip: dict[str, Any], elf: Path) -> str:
    probe = board.get("probe")
    if probe is None:
        raise EmitError(f"board {board['id']} declares no probe — cannot flash")

    if shutil.which("probe-rs") and "chip_id" in probe:
        subprocess.run(
            ["probe-rs", "download", "--chip", probe["chip_id"], str(elf)], check=True
        )
        subprocess.run(["probe-rs", "reset", "--chip", probe["chip_id"]], check=True)
        return "probe-rs"

    if shutil.which("openocd"):
        interface = _OPENOCD_INTERFACE.get(probe.get("kind", ""))
        target = _OPENOCD_TARGET.get(chip["family"])
        if interface and target:
            subprocess.run(
                ["openocd",
                 "-f", f"interface/{interface}.cfg",
                 "-f", f"target/{target}.cfg",
                 "-c", f"program {{{elf}}} verify reset exit"],
                check=True,
            )
            return "openocd"

    if probe.get("kind") == "bootsel":
        return _flash_uf2(chip, elf, probe)

    if probe.get("kind") == "esptool":
        return _flash_esptool(elf, probe)

    if shutil.which("st-flash") and probe.get("kind") == "stlink":
        flash_base = next(m["base"] for m in chip["memories"] if m["kind"] == "flash")
        bin_path = elf.with_suffix(".bin")
        objcopy = shutil.which("arm-none-eabi-objcopy")
        if objcopy is None:
            raise EmitError("st-flash fallback needs arm-none-eabi-objcopy on PATH")
        subprocess.run([objcopy, "-O", "binary", str(elf), str(bin_path)], check=True)
        subprocess.run(["st-flash", "--reset", "write", str(bin_path), flash_base], check=True)
        return "st-flash"

    raise EmitError(
        "no usable flash runner found — install probe-rs, openocd or stlink tools"
    )


def find_serial_port(probe: dict[str, Any]) -> str:
    """Locate the board's serial port (flash + monitor).

    usbserial* covers USB-UART bridges (FTDI/CP210x/CH34x); usbmodem*
    covers CDC probes (ST-Link VCP, EDBG). BOOTSEL boards have no serial.
    """
    if port := probe.get("port"):
        return str(port)
    kind = probe.get("kind", "")
    if kind == "bootsel":
        raise EmitError(
            "this board flashes over BOOTSEL mass-storage and exposes no "
            "serial port — wire a USB-serial adapter to use the UART"
        )
    pattern = "cu.usbmodem*" if kind in ("stlink", "cmsis-dap") else "cu.usbserial*"
    candidates = sorted(Path("/dev").glob(pattern))
    if len(candidates) == 1:
        return str(candidates[0])
    if len(candidates) == 2 and candidates[0].name[:-1] == candidates[1].name[:-1]:
        # Dual-channel FTDI (WROVER-KIT FT2232H): channel A is JTAG,
        # channel B — the higher suffix — is the UART.
        return str(candidates[1])
    names = ", ".join(str(c) for c in candidates) or "(none)"
    raise EmitError(
        f"could not auto-pick a serial port (found: {names}) — "
        "set probe.port in board.json or plug exactly one board"
    )


# ── UF2 (BOOTSEL mass-storage) ──────────────────────────────────────────────

_UF2_MAGIC_START0 = 0x0A324655  # contract-ok: UF2 file-format magic, not a silicon fact
_UF2_MAGIC_START1 = 0x9E5D5157  # contract-ok: UF2 file-format magic
_UF2_MAGIC_END = 0x0AB16F30  # contract-ok: UF2 file-format magic
_UF2_FLAG_FAMILY_ID = 0x00002000  # contract-ok: UF2 header flag


def _elf_to_uf2(elf: Path, flash_base: int, family_id: int) -> bytes:
    """Flatten the ELF to a binary and encode it as UF2 (256B payload/block)."""
    objcopy = shutil.which("arm-none-eabi-objcopy")
    if objcopy is None:
        raise EmitError("UF2 conversion needs arm-none-eabi-objcopy on PATH")
    bin_path = elf.with_suffix(".bin")
    subprocess.run([objcopy, "-O", "binary", str(elf), str(bin_path)], check=True)
    payload = bin_path.read_bytes()

    num_blocks = (len(payload) + 255) // 256
    out = bytearray()
    for block in range(num_blocks):
        chunk = payload[block * 256:(block + 1) * 256].ljust(256, b"\x00")
        header = struct.pack(
            "<8I",
            _UF2_MAGIC_START0,
            _UF2_MAGIC_START1,
            _UF2_FLAG_FAMILY_ID,
            flash_base + block * 256,
            256,
            block,
            num_blocks,
            family_id,
        )
        out += header + chunk + b"\x00" * (476 - 256) + struct.pack("<I", _UF2_MAGIC_END)
    return bytes(out)


def _flash_uf2(chip: dict[str, Any], elf: Path, probe: dict[str, Any]) -> str:
    flash_base = int(
        next(m["base"] for m in chip["memories"] if m["kind"] == "flash"), 16
    )
    family_id = int(probe["family_id"], 16)
    uf2 = _elf_to_uf2(elf, flash_base, family_id)
    uf2_path = elf.with_suffix(".uf2")
    uf2_path.write_bytes(uf2)

    volume = Path(probe.get("volume", "/Volumes/RPI-RP2"))
    if not volume.exists():
        print(f"waiting for {volume} — hold BOOTSEL and (re)plug the board's USB…")
        deadline = time.time() + 120
        while not volume.exists():
            if time.time() > deadline:
                raise EmitError(f"{volume} never appeared — is the board in BOOTSEL mode?")
            time.sleep(0.5)
        time.sleep(1.0)  # let the mount settle: an immediate open can ENXIO

    # The device reboots the instant the last block lands, so ENXIO during
    # write/close with the volume gone is SUCCESS, and an open() during mount
    # settling deserves a retry.
    import os  # noqa: PLC0415

    dst = volume / uf2_path.name
    for attempt in range(5):
        try:
            fd = os.open(dst, os.O_WRONLY | os.O_CREAT | os.O_TRUNC)
            try:
                os.write(fd, uf2)
                os.fsync(fd)
            finally:
                try:
                    os.close(fd)
                except OSError:
                    pass
            break
        except OSError as exc:
            if exc.errno == 6 and not volume.exists():
                break  # rebooted mid-write: all blocks were streamed
            if attempt == 4:
                raise EmitError(f"could not write {dst}: {exc}") from exc
            time.sleep(1.0)

    deadline = time.time() + 15
    while volume.exists() and time.time() < deadline:
        time.sleep(0.5)
    if volume.exists():
        raise EmitError(
            "RPI-RP2 is still mounted — the bootrom did not accept the image "
            "(bad boot2 checksum or malformed UF2)"
        )
    print(f"flashed {uf2_path.name} ({len(uf2) // 1024} KiB) — bootrom accepted the image and rebooted")
    return "uf2-bootsel"


# ── esptool (classic ESP32 under resident IDF bootloader) ───────────────────


def _generate_partition_table(partitions: list[list[Any]], out_bin: Path) -> None:
    """Minimal ESP-IDF partition table (ported verbatim from the old repo's
    hardware-validated alloyctl fallback: 0xAA50 entries + MD5 marker,
    padded to 0xC00)."""
    import hashlib  # noqa: PLC0415

    type_map = {"app": 0x00, "data": 0x01}
    sub_map = {"factory": 0x00, "ota_0": 0x10, "ota_1": 0x11,
               "nvs": 0x02, "phy": 0x01, "otadata": 0x00}
    entries = b""
    for name, type_s, subtype_s, offset_s, size_s in partitions:
        t = type_map.get(type_s, None)
        s = sub_map.get(subtype_s, None)
        if t is None or s is None:
            raise EmitError(f"partition {name}: unknown type/subtype {type_s}/{subtype_s}")
        entry = struct.pack("<BBII", t, s, int(offset_s, 16), int(size_s, 16))
        entry += name.encode("ascii").ljust(16, b"\x00")[:16] + b"\x00\x00\x00\x00"
        entries += b"\xaa\x50" + entry
    md5_marker = b"\xeb\xeb" + b"\xff" * 14 + hashlib.md5(entries).digest()
    out_bin.write_bytes((entries + md5_marker).ljust(0xC00, b"\xff"))


def _flash_esptool(elf: Path, probe: dict[str, Any]) -> str:
    esptool = shutil.which("esptool") or shutil.which("esptool.py")
    if esptool is None:
        raise EmitError("esptool not found on PATH (pip install esptool)")
    chip_id = probe["chip_id"]

    app_bin = elf.with_suffix(".bin")
    subprocess.run(
        [esptool, "--chip", chip_id, "elf2image", "--dont-append-digest",
         "--output", str(app_bin), str(elf)],
        check=True,
    )

    port = find_serial_port(probe)

    write_args = []
    if "partitions" in probe:
        pt_bin = elf.with_name("partition_table.bin")
        _generate_partition_table(probe["partitions"], pt_bin)
        write_args += [probe.get("pt_offset", "0x8000"), str(pt_bin)]
    write_args += [probe.get("app_offset", "0x10000"), str(app_bin)]

    subprocess.run(
        [esptool, "--chip", chip_id, "--port", port, "--baud", "460800",
         "write-flash", *write_args],
        check=True,
    )
    # The factory bootloader at 0x1000 is deliberately untouched.
    return "esptool"
