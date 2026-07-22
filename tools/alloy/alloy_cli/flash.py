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
