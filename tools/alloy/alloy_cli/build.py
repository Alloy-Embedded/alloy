"""Render the CMake build tree and drive cmake + ninja.

Users never see CMake: the tree lives in .alloy/build-tree (gitignored,
regenerated every run). `alloy export cmake` will later emit a standalone
copy for people who want to own their build.
"""

from __future__ import annotations

import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .emit.common import EmitError
from .project import Project

# Core name -> GCC -mcpu flag; fallback per architecture.
_CPU_BY_CORE = {
    "cm0": "cortex-m0",
    "cm0plus": "cortex-m0plus",
    "cm3": "cortex-m3",
    "cm4": "cortex-m4",
    "cm7": "cortex-m7",
    "cm33": "cortex-m33",
}
_CPU_BY_ARCH = {
    "armv6m": "cortex-m0plus",
    "armv7m": "cortex-m3",
    "armv7em": "cortex-m4",
    "armv8m_base": "cortex-m23",
    "armv8m_main": "cortex-m33",
}


def _arch_ns(chip: dict[str, Any]) -> str:
    arch = chip["cores"][0]["arch"]
    if arch.startswith("armv"):
        return "cortex_m"
    if arch.startswith("xtensa"):
        return "xtensa"
    if arch.startswith("rl78"):
        return "rl78"
    raise EmitError(f"unsupported architecture {arch}")


def _xtensa_prefix() -> str:
    if found := shutil.which("xtensa-esp-elf-gcc"):
        return str(Path(found).with_name("xtensa-esp-elf-"))
    candidate = Path.home() / ".alloy/tools/xtensa-esp-elf/bin/xtensa-esp-elf-gcc"
    if candidate.exists():
        return str(candidate.with_name("xtensa-esp-elf-"))
    raise EmitError(
        "xtensa-esp-elf toolchain not found — looked on PATH and ~/.alloy/tools/xtensa-esp-elf"
    )


def _rl78_prefix() -> str:
    """Where rl78-elf-gcc is.

    Unlike arm-gnu-toolchain and xtensa-esp-elf there is no vendor binary
    release alloy can fetch: Renesas' own GNU distribution lags, and mainline
    GCC's rl78 backend has to be built. So this looks and then says exactly
    what to do, rather than offering a download that does not exist.

    NOTE the C++ requirement: alloy needs a HOSTED libstdc++, not the
    freestanding one — 13 headers include <chrono>, which is outside the C++23
    freestanding subset. A toolchain configured --disable-hosted-libstdcxx
    compiles the arch backend and then fails on the framework.
    """
    if found := shutil.which("rl78-elf-gcc"):
        return str(Path(found).with_name("rl78-elf-"))
    candidate = Path.home() / ".alloy/tools/rl78-elf/bin/rl78-elf-gcc"
    if candidate.exists():
        return str(candidate.with_name("rl78-elf-"))
    raise EmitError(
        "rl78-elf toolchain not found — looked on PATH and ~/.alloy/tools/rl78-elf.\n"
        "There is no binary release to fetch; build one from mainline GCC:\n"
        "  binutils --target=rl78-elf, then GCC --target=rl78-elf --with-newlib\n"
        "  --enable-languages=c,c++  (do NOT pass --disable-hosted-libstdcxx:\n"
        "  alloy needs <chrono>, which freestanding libstdc++ does not ship)"
    )


def _cpu_flags(chip: dict[str, Any]) -> str:
    if _arch_ns(chip) == "rl78":
        # -mmul=g13 is the hardware multiplier/divider this family carries;
        # without it GCC calls into libgcc for every multiply. The core is an
        # S3, which is the default, so it is not spelled again here.
        #
        # --param=min-pagesize=0 is not an optimisation knob, it is what makes
        # MMIO legal here. GCC assumes the first page is unmapped, so a
        # peripheral pointer built from a small constant trips -Warray-bounds
        # ("subscript 0 is outside array bounds"). On ARM the peripheral window
        # sits high in the address space and it never comes up; RL78 reaches its
        # SFRs through small 16-bit near addresses, so every access does.
        return "-mmul=g13 --param=min-pagesize=0"
    if _arch_ns(chip) == "xtensa":
        # The unified xtensa-esp-elf toolchain is multi-core: -mdynconfig
        # selects the concrete core (ESP32 = LX6 little-endian); without it
        # GCC emits generic BIG-endian objects that esptool rejects. The flag
        # must stay RELATIVE to match the multilib table (esp32/libgcc.a);
        # macOS hardened dlopen then needs XTENSA_GNU_CONFIG (absolute) set
        # in the environment — see _build_env().
        return f"-mdynconfig=xtensa_{chip['family']}.so -mlongcalls"
    core = chip["cores"][0]
    cpu = _CPU_BY_CORE.get(core["name"]) or _CPU_BY_ARCH.get(core["arch"])
    if cpu is None:
        raise EmitError(f"no CPU mapping for core {core['name']} / arch {core['arch']}")
    # Soft-float everywhere for now, even on FPU parts: hard-float needs a
    # CPACR enable in the arch startup before the first FP instruction.
    # TODO(arch): enable CP10/CP11 on armv7em, then honor core.fpu here.
    return f"-mcpu={cpu} -mthumb -mfloat-abi=soft"


def _toolchain_cmake(chip: dict[str, Any], cpu_flags: str) -> str:
    if _arch_ns(chip) == "xtensa":
        prefix = _xtensa_prefix()
        processor = "xtensa"
        cc, cxx = f"{prefix}gcc", f"{prefix}g++"
        objcopy, size = f"{prefix}objcopy", f"{prefix}size"
    elif _arch_ns(chip) == "rl78":
        prefix = _rl78_prefix()
        processor = "rl78"
        cc, cxx = f"{prefix}gcc", f"{prefix}g++"
        objcopy, size = f"{prefix}objcopy", f"{prefix}size"
    else:
        processor = "arm"
        cc, cxx = "arm-none-eabi-gcc", "arm-none-eabi-g++"
        objcopy, size = "arm-none-eabi-objcopy", "arm-none-eabi-size"
    return f"""# GENERATED by alloy — DO NOT EDIT
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR {processor})
set(CMAKE_C_COMPILER "{cc}")
set(CMAKE_CXX_COMPILER "{cxx}")
set(CMAKE_ASM_COMPILER "{cc}")
set(CMAKE_OBJCOPY "{objcopy}" CACHE FILEPATH "objcopy for post-build bin/hex")
set(CMAKE_SIZE "{size}" CACHE FILEPATH "size for the build summary")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_FLAGS_INIT "{cpu_flags}")
set(CMAKE_CXX_FLAGS_INIT "{cpu_flags}")
set(CMAKE_ASM_FLAGS_INIT "{cpu_flags}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "{cpu_flags}")
"""


def generated_sources(project: Project) -> list[Path]:
    """The codegen output this configuration compiles.

    Named once, here, because two consumers need the same answer: the CMake
    writer, and `alloy sbom` — which attributes these files to the device
    database they were emitted from (and notices the RP2040 boot2 blob among
    them).
    """
    gen = project.gen_dir
    out = [gen / "board.cpp"]
    for optional in ("vector_table.c", "boot2.c", "irq_data.c"):
        if (gen / optional).exists():
            out.append(gen / optional)
    return out


def _toolchain_root(chip: dict[str, Any]) -> Path | None:
    """Where the cross toolchain is installed, or None if it cannot be found.

    Only used to build a -ffile-prefix-map entry: GCC records its OWN include
    directories in .debug_line, so two machines that keep arm-none-eabi-gcc in
    different places produced different ELFs from identical source. Not being
    able to locate it is not an error — the map entry is simply omitted (the
    build then still works, it is just host-dependent in its debug info).
    """
    ns = _arch_ns(chip)
    try:
        if ns == "xtensa":
            prefix = _xtensa_prefix()
        elif ns == "rl78":
            prefix = _rl78_prefix()
        else:
            found = shutil.which("arm-none-eabi-gcc")
            if found is None:
                return None
            prefix = str(Path(found).with_name("arm-none-eabi-"))
    except EmitError:
        return None
    # <root>/bin/<tuple>-gcc -> <root>. Resolve symlinks: GCC computes its own
    # include paths from the REAL location of the binary, which is what lands
    # in the debug info (xPack ships bin/ entries that are symlinks).
    return Path(prefix).parent.resolve().parent


def prefix_maps(project: Project, chip: dict[str, Any]) -> list[tuple[Path, str]]:
    """(real prefix, virtual prefix) pairs baked in as -ffile-prefix-map.

    Absolute paths were the ONLY thing keeping two identical checkouts at
    different paths from producing identical ELFs — the .bin already matched
    byte for byte, because code never carried a path; DWARF, DW_AT_comp_dir
    and __FILE__ did. Mapping each real root onto a fixed virtual one makes the
    whole artefact a function of the source, not of where the source lives.

    Nested roots are dropped (an in-repo example lives UNDER the framework
    root), so the emitted prefixes are pairwise disjoint and the order GCC
    applies them in cannot change the answer.

    The cost is honest and small: a debugger no longer finds sources by itself,
    and `alloy crash` reports /alloy/framework/src/… instead of a host path.
    `alloy debug-info --json` carries the inverse map so an IDE can undo it.
    """
    candidates: list[tuple[Path, str]] = [
        (project.root.resolve(), "/alloy/project"),
        (project.alloy_root.resolve(), "/alloy/framework"),
    ]
    if (toolchain := _toolchain_root(chip)) is not None:
        candidates.append((toolchain, "/alloy/toolchain"))
    kept: list[tuple[Path, str]] = []
    for real, virtual in candidates:
        if any(real != other and real.is_relative_to(other) for other, _ in candidates):
            continue  # covered by an ancestor entry
        if any(real == other for other, _ in kept):
            continue  # project == alloy_root (framework built as its own project)
        kept.append((real, virtual))
    return kept


def _cmakelists(project: Project, chip: dict[str, Any], sources: list[Path],
                runtime_sources: list[Path], vendor_sources: list[Path],
                lwip: dict[str, list[Path]] | None = None,
                lfs_sources: list[Path] | None = None) -> str:
    gen = project.gen_dir
    lwip = lwip or {"c": [], "glue": [], "inc": []}
    gen_sources = generated_sources(project)
    src_list = "\n    ".join(
        f'"{p}"' for p in [*sources, *gen_sources, *runtime_sources, *vendor_sources,
                           *lwip["c"], *lwip["glue"]]
    )
    # Include dirs of any ecosystem libraries the project vendored (alloy lib add),
    # plus the lwIP package's port + vendored headers when a net example pulls it in.
    _lib_incs = [*project.lib_includes(), *lwip["inc"]]
    lib_includes = (
        "\n    " + "\n    ".join(f'"{p}"' for p in _lib_incs) if _lib_incs else ""
    )
    # Vendored C packages (littlefs, monocypher, ...) are framework-owned, not
    # header-only: silence their warnings, scoped to the vendored TUs only.
    # Package-specific config is set PER PACKAGE — the LFS_* block below
    # applies to littlefs alone (the facade hands it all buffers statically,
    # so malloc/asserts/logging are stripped on firmware). It used to ride on
    # every vendored TU, silently defining littlefs macros into monocypher —
    # harmless by luck, and exactly the cross-package leak this split ends.
    lfs_sources = lfs_sources or []
    lfs_set = set(lfs_sources)
    plain_vendor = [p for p in vendor_sources if p not in lfs_set]
    vendor_props = ""
    if plain_vendor:
        vendor_list = "\n    ".join(f'"{p}"' for p in plain_vendor)
        vendor_props += f"""

set_source_files_properties(
    {vendor_list}
    PROPERTIES
    COMPILE_OPTIONS "-w"
)"""
    if lfs_sources:
        lfs_list = "\n    ".join(f'"{p}"' for p in lfs_sources)
        vendor_props += f"""

set_source_files_properties(
    {lfs_list}
    PROPERTIES
    COMPILE_OPTIONS "-w"
    COMPILE_DEFINITIONS "LFS_NO_MALLOC;LFS_NO_ASSERT;LFS_NO_DEBUG;LFS_NO_WARN;LFS_NO_ERROR"
)"""
    # Vendored lwIP: silence its warnings (its own config drives it via lwipopts.h).
    lwip_props = ""
    if lwip["c"]:
        lwip_list = "\n    ".join(f'"{p}"' for p in lwip["c"])
        lwip_props = f"""

set_source_files_properties(
    {lwip_list}
    PROPERTIES
    COMPILE_OPTIONS "-w"
)"""
    # newlib nano/nosys specs are an ARM-newlib convention; the xtensa
    # toolchain links its own newlib without them.
    specs = "" if _arch_ns(chip) == "xtensa" else """
    --specs=nano.specs
    --specs=nosys.specs"""
    # Post-build .bin/.hex are meaningful only for the arm/RP2040 targets: the
    # xtensa image is a multi-segment ELF (DROM..IROM) that objcopy -O binary
    # would flatten into a ~13 MB junk file, and esp32 gets its real image from
    # esptool elf2image at flash time anyway — so skip it there.
    post_build = "" if _arch_ns(chip) == "xtensa" else f"""

# Post-build: raw .bin (st-flash/dfu) + Intel .hex (many flashers) beside the
# ELF + .map the link already emits.
add_custom_command(TARGET {project.name}.elf POST_BUILD
    COMMAND ${{CMAKE_OBJCOPY}} -O binary $<TARGET_FILE:{project.name}.elf> {project.name}.bin
    COMMAND ${{CMAKE_OBJCOPY}} -O ihex   $<TARGET_FILE:{project.name}.elf> {project.name}.hex
    VERBATIM
)"""
    # Reproducibility: no absolute host path may reach the artefact. See
    # prefix_maps() — one -ffile-prefix-map per disjoint root, in sorted order
    # so the flag list itself is stable across runs.
    repro = "".join(
        f'\n    "-ffile-prefix-map={real}={virtual}"'
        for real, virtual in sorted(prefix_maps(project, chip))
    )
    return f"""# GENERATED by alloy — DO NOT EDIT
cmake_minimum_required(VERSION 3.25)
project({project.name} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable({project.name}.elf
    {src_list}
)

target_include_directories({project.name}.elf PRIVATE
    "{project.alloy_root / 'src'}"
    "{gen}"
    "{project.alloy_root / 'third_party' / 'monocypher'}"{lib_includes}
)

target_compile_options({project.name}.elf PRIVATE
    -Os -g3
    -ffunction-sections -fdata-sections
    # Every compiled function writes its own frame size to a .su file beside
    # its object. Costs nothing in the image (GCC emits a side file; the code
    # is byte-identical, and scripts/check_reproducible.sh keeps that honest)
    # and it is the raw material for a whole-program stack bound —
    # scripts/check_static_limits.sh walks the call graph over these.
    -fstack-usage
    -Wall -Wextra{repro}
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    # Freestanding: main never returns, so static destructors never run. Register
    # them normally and __cxa_atexit pulls in __dso_handle (absent here). This
    # drops both — required for any namespace-scope object with a destructor.
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
)

target_link_options({project.name}.elf PRIVATE
    -T "{gen / 'linker.ld'}"
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map={project.name}.map{specs}
){post_build}{vendor_props}{lwip_props}
"""


@dataclass(frozen=True)
class BuildInputs:
    """Every translation unit this configuration actually compiles.

    Extracted from build() so a second consumer — `alloy sbom` — can answer
    "what is IN this firmware" from the same seam that decides what gets
    compiled, instead of from a hand-kept inventory that drifts the first time
    somebody vendors a new tree. If the build stops compiling littlefs, the
    SBOM stops listing it, with no second edit.
    """

    app: list[Path]                 # the project's own src/
    runtime: list[Path]             # src/alloy/arch/<ns>
    vendor: list[Path]              # every vendored C package pulled in
    lfs: list[Path]                 # the littlefs subset of `vendor`
    lwip: dict[str, list[Path]]     # c / glue / inc

    def compiled(self) -> list[Path]:
        """All sources on the compiler's command line, generated files aside."""
        return [*self.app, *self.runtime, *self.vendor,
                *self.lwip["c"], *self.lwip["glue"]]


def build_inputs(project: Project, chip: dict[str, Any]) -> BuildInputs:
    """Resolve the build seam: which optional packages this configuration pulls in.

    The signals are the GENERATED headers, not the board JSON — codegen has
    already decided by the time this runs, so the answer is what the compiler
    will really see. Reading them requires `alloy gen` (or a full build) to have
    happened; a project that has never been generated resolves to the lean set.
    """
    sources = sorted((project.root / "src").rglob("*.cpp")) + \
        sorted((project.root / "src").rglob("*.c"))
    if not sources:
        raise EmitError(f"no sources under {project.root / 'src'}")

    arch_dir = project.alloy_root / "src" / "alloy" / "arch" / _arch_ns(chip)
    runtime_sources = sorted(arch_dir.glob("*.cpp")) + sorted(arch_dir.glob("*.S"))

    # Vendored C packages are framework-owned and compiled ONLY when the board
    # pulls the package in — the generated board.hpp is the signal (its caps
    # block emits `bool fs = true;` for an fs role). This keeps every other
    # example's build lean (no needless littlefs TUs) — the real "build seam".
    vendor_sources: list[Path] = []
    # Ed25519 image verification: compiled ONLY when the project configures an
    # [ota] public_key (generated ota_key.hpp is the signal, same "build seam"
    # rule as fs/lwIP). Its headers are always on the include path so
    # alloy/ota/signed.hpp parses in every build.
    key_hpp = project.gen_dir / "alloy" / "ota_key.hpp"
    if key_hpp.exists() and "configured = true" in key_hpp.read_text():
        mono = project.alloy_root / "third_party" / "monocypher"
        vendor_sources += [mono / "monocypher.c", mono / "monocypher-ed25519.c"]

    board_hpp = project.gen_dir / "alloy" / "board.hpp"
    board_text = board_hpp.read_text() if board_hpp.exists() else ""
    lfs_sources: list[Path] = []
    if "bool fs = true;" in board_text:
        fs_vendor = project.alloy_root / "src" / "alloy" / "fs" / "vendor"
        lfs_sources = sorted(fs_vendor.glob("*.c"))
        vendor_sources += lfs_sources

    # lwIP (net stack) is a heavier opt-in package: compiled only when an example
    # actually pulls in the facade (`#include <alloy/net/lwip...>`) AND the board
    # has an ethernet MAC. Its port headers (lwipopts.h / arch/cc.h) + the
    # vendored lwIP headers go on the include path for the whole target.
    lwip: dict[str, list[Path]] = {"c": [], "glue": [], "inc": []}
    src_text = "\n".join(p.read_text(errors="ignore") for p in sources)
    if "alloy/net/lwip" in src_text and "bool ethernet = true;" in board_text:
        net = project.alloy_root / "src" / "alloy" / "net"
        vlwip = net / "vendor" / "lwip" / "src"
        lwip["c"] = (sorted(vlwip.glob("core/*.c")) + sorted(vlwip.glob("core/ipv4/*.c"))
                     + [vlwip / "netif" / "ethernet.c"])
        lwip["glue"] = [net / "lwip" / "port.cpp"]
        lwip["inc"] = [net / "lwip", vlwip / "include"]

    return BuildInputs(app=sources, runtime=runtime_sources, vendor=vendor_sources,
                       lfs=lfs_sources, lwip=lwip)


def build(project: Project, chip: dict[str, Any]) -> Path:
    inputs = build_inputs(project, chip)
    sources, runtime_sources = inputs.app, inputs.runtime
    vendor_sources, lfs_sources, lwip = inputs.vendor, inputs.lfs, inputs.lwip

    tree = project.build_dir
    tree.mkdir(parents=True, exist_ok=True)
    (tree / "toolchain.cmake").write_text(_toolchain_cmake(chip, _cpu_flags(chip)))
    (tree / "CMakeLists.txt").write_text(
        _cmakelists(project, chip, sources, runtime_sources, vendor_sources, lwip,
                    lfs_sources=lfs_sources))

    env = None
    if _arch_ns(chip) == "xtensa":
        import os  # noqa: PLC0415

        dynconfig = (Path(_xtensa_prefix()).parent.parent / "lib" /
                     f"xtensa_{chip['family']}.so")
        if not dynconfig.exists():
            raise EmitError(f"toolchain dynconfig missing: {dynconfig}")
        env = os.environ | {"XTENSA_GNU_CONFIG": str(dynconfig)}

    out = tree / "out"
    subprocess.run(
        ["cmake", "-G", "Ninja", "-S", str(tree), "-B", str(out),
         f"-DCMAKE_TOOLCHAIN_FILE={tree / 'toolchain.cmake'}",
         "-DCMAKE_BUILD_TYPE=MinSizeRel"],
        check=True, env=env,
    )
    subprocess.run(["cmake", "--build", str(out)], check=True, env=env)

    elf = out / f"{project.name}.elf"
    if not elf.exists():
        raise EmitError(f"build finished but {elf} is missing")

    size_tool = (
        f"{_xtensa_prefix()}size" if _arch_ns(chip) == "xtensa"
        else shutil.which("arm-none-eabi-size")
    )
    if size_tool:
        subprocess.run([size_tool, str(elf)], check=False)

    # clangd support out of the box.
    cc_json = out / "compile_commands.json"
    link = project.root / "compile_commands.json"
    if cc_json.exists():
        link.unlink(missing_ok=True)
        link.symlink_to(cc_json)

    return elf
