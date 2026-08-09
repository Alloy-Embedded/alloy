# Supply chain

Three questions a company asks before it bets a product on a framework, and
what alloy can actually show for each.

| Question | Answer | Gate that keeps it true |
|---|---|---|
| Does the same source produce the same bytes? | Yes, for `.elf`, `.bin` and `.hex` | `scripts/check_reproducible.sh`, run on two examples in CI |
| Can we build with no internet? | Yes, from a local mirror of the same pinned archives | host tests in `tools/alloy/tests/test_toolchains.py` |
| What is in the image, legally? | `alloy sbom` — SPDX, CycloneDX or a human NOTICE | CI asserts the report follows the build seam |

---

## Reproducible builds

The firmware is a function of the source, not of where the source lives.

```console
$ scripts/check_reproducible.sh blink nucleo_g071rb
ok   blink.elf  7f18f6d5d788d2efe9a1d08301ec6f3a04fa5349eb7130916f59359b9f768cb2
ok   blink.bin  2a9963ab936d760e26f951a33c003b663c62b1afea112e8bea3c8121aff4d8da
ok   blink.hex  ae6aeae495cb209a27f71a681be2cb976f61d10d6ec69de0eee15b2f0f31017e
ok   negative control: a one-byte change is detected
ok   no absolute host paths in blink.elf
reproducible: blink@nucleo_g071rb is byte-identical from two different paths
```

Those three digests are one build's, with arm-none-eabi-gcc 14.2.1: the claim is that **the two
paths agree**, not that the hex strings are constants. Touch a framework source or move to another
compiler release and they change together — the `.elf` line in particular moves whenever any linked
translation unit does, since DWARF records more than code.

The script copies the framework tree to two deliberately different absolute
paths, builds the example in each and compares digests. It also runs a negative
control (mutate one byte, the comparison must notice) and scans the ELF for any
remaining absolute host path — because two builds from the *same* leaked prefix
would otherwise compare equal and prove nothing.

### What was leaking, and what fixes it

Before this, the `.bin` was **already** byte-identical across paths — code never
carried a path. The `.elf` was not: DWARF, `DW_AT_comp_dir` and the compiler's
own include directories are recorded as absolute paths, 121 of them in a plain
`blink`.

The build now passes one `-ffile-prefix-map` per root:

| Real root | Recorded as |
|---|---|
| the project directory | `/alloy/project` |
| the framework checkout | `/alloy/framework` |
| the cross toolchain install | `/alloy/toolchain` |

Nested roots collapse (an in-repo example lives *under* the framework root), so
the maps are pairwise disjoint and their order cannot change the result. The
toolchain entry is why an image built on a laptop with the toolchain in
`~/Library/xPacks/...` matches one built on a server with it in `/opt` —
verified locally by building the same example against two installs of the same
compiler at different paths and getting the same ELF digest.

### What is *not* claimed

- **The `.map` file is not reproducible** and is not compared. CMake derives
  object-file names from the absolute path of any source outside its own tree,
  and the map prints those names. Nothing in the map reaches the device.
- **A different compiler build gives different bytes.** Reproducibility here
  means *same source + same toolchain version → same image*. Pin the toolchain
  (alloy's `toolchains.json` does, by sha256) if you need this across machines.
- **RL78 is unverified.** The byte-for-byte comparison has been run on Cortex-M
  (`blink`, `fs` with littlefs, `http_server` with lwIP) and on Xtensa
  (`blink` for `esp32_devkit`, ELF only — that target emits no flat `.bin`).
  The prefix maps are emitted for RL78 too, but nothing has measured it: there
  is no binary `rl78-elf` release to install, so no build was available. Both
  CI legs are Cortex-M; the Xtensa result above is a local measurement, not a
  gate — that job would have to install the Xtensa toolchain to run it.

### The debugger, honestly

Remapped paths mean a debugger no longer finds sources by itself, and `alloy
crash` prints `/alloy/framework/src/...` rather than a path on your disk. The
inverse map is published so tooling can undo it:

```console
$ alloy debug-info --json
  ...
  "source_map": {
    "/alloy/framework": "/home/you/src/alloy",
    "/alloy/toolchain": "/opt/arm-gnu-toolchain"
  }
```

Feed it to Cortex-Debug as `sourceFileMap`, or to gdb as
`set substitute-path /alloy/framework /home/you/src/alloy`.

---

## Offline / air-gapped builds

`alloy setup` downloads toolchains over HTTPS and verifies each archive against
a sha256 pinned in `toolchains.json`. A build server behind a firewall has no
route to `developer.arm.com` — but the digest, not the origin, is the security
property. So the same archives can come from a directory.

**On a machine with network**, populate a mirror:

```console
$ alloy setup --fetch /media/usb/alloy-tools --platform linux-x86_64
  arm-gnu-toolchain/linux-x86_64: mirrored -> arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz  sha256 6cd1bbc1…
  openocd/linux-x86_64: mirrored -> xpack-openocd-0.12.0-3-linux-x64.tar.gz  sha256 …
mirror at /media/usb/alloy-tools — use it with `alloy setup --from /media/usb/alloy-tools --offline`
```

`--platform` matters: the laptop populating the stick is usually not the
architecture of the build server. `--platform all` mirrors every platform the
manifest knows. Re-running skips archives already present with the right digest,
so a mirror can be topped up over a slow link.

**On the air-gapped host**, install from it:

```console
$ alloy setup --from /media/usb/alloy-tools --offline
  arm-gnu-toolchain: from mirror /media/usb/alloy-tools/arm-gnu-toolchain-13.2.rel1-…tar.xz
  arm-gnu-toolchain: sha256 39c44f8af42695b7b871df42e346c09fee670ea8dfc11f17083e296ea2b0d279
  arm-gnu-toolchain: installed -> ~/.alloy/tools/arm-gnu-toolchain

add these to PATH before `alloy build` — it looks for the cross compiler on PATH only:
  export PATH="~/.alloy/tools/arm-gnu-toolchain/bin:~/.alloy/tools/openocd/bin:$PATH"
```

**Do the export.** `alloy setup --check` will report the toolchain as
`ok (~/.alloy/tools)` whether or not it is on `PATH`, but `alloy build` searches
`PATH` and nowhere else — without the export the next command is CMake saying
*"The CXX compiler identification is unknown"*, with nothing to connect it back
to a mirror that worked perfectly. Verified end to end: mirror, `--offline`
install into a clean `HOME`, export, `alloy build` — a 2 528-byte `blink.elf`
with no network at any point.

`ALLOY_TOOLS_MIRROR=/media/usb/alloy-tools` does the same without the flag.
`--offline` is a promise, not a hint: with it set and no mirror, the installer
refuses rather than attempting a connection, and the refusal prints the exact
`--fetch` command that would have produced the mirror.

Check a mirror is complete *before* the build server needs it:

```console
$ alloy setup --check --from /media/usb/alloy-tools --family stm32g0
arm-gnu-toolchain    MISSING     — not in mirror /media/usb/alloy-tools (arm-gnu-toolchain-…tar.xz)
openocd              MISSING     — archive present in the mirror
```

### The one thing alloy cannot mirror

`cmake` and `ninja` are declared `kind: system` — they come from the OS image,
not from alloy. On an air-gapped host, bake them into the image; `alloy setup`
says so instead of printing a `brew install` you cannot run.

---

## `alloy sbom`

```console
$ alloy build
$ alloy sbom                      # human attribution text
$ alloy sbom --format spdx        # SPDX 2.3 JSON
$ alloy sbom --format cyclonedx   # CycloneDX 1.5 JSON
$ alloy sbom --format json        # alloy's own schema, for tooling
$ alloy sbom --format spdx --out sbom.spdx.json
```

It requires a completed build, because it reports what the build *did*, not what
it might do:

- **compiled** — from the same seam that drives the compiler. littlefs appears
  only when the board has an `fs` role, Monocypher only when the project
  configured an OTA key, lwIP only when the sources use the net facade *and* the
  board has a MAC. Stop compiling one and it stops being reported, with no
  second edit anywhere.
- **generated** — `board.cpp`, `vector_table.c`, `boot2.c`, attributed to the
  alloy-devices database they were emitted from. The RP2040 second-stage
  bootloader is a precompiled third-party blob and is reported as its own
  component, with the licence string the chip record carries.
- **linked** — parsed out of the linker's own map file. On a plain `blink` only
  `libgcc.a` contributes members, even though libc, libm and libstdc++ were all
  searched; an SBOM that listed the searched ones would be over-claiming.

The report carries the sha256 of the built image, so a document cannot drift
from the binary it describes, and that digest is also the SPDX document
namespace (reproducible builds make it a stable identity).

### Worked example

```console
$ cd examples/http_server && alloy build --board nucleo_f767zi && alloy sbom
```

| Component | Kind | Version | Licence |
|---|---|---|---|
| `http_server` | application | — | from `[project] license`, if declared |
| alloy | framework | 0.3.0 | MIT |
| alloy-devices | device-data | 0.3.0 | MIT |
| lwIP | vendored | 2.2.1 | BSD-3-Clause |
| libgcc | toolchain-runtime | 14.2.1 | `GPL-3.0-or-later WITH GCC-exception-3.1` |
| newlib (nano) | toolchain-runtime | 4.4.0 | *not asserted* — licence text path given |
| newlib syscall stubs | toolchain-runtime | 4.4.0 | *not asserted* |

Versions are read out of the trees themselves (`LWIP_VERSION_*`, `LFS_VERSION`,
the Monocypher vendoring note), so a vendor bump moves the report without an
edit. Where a tree records no version, the report says so rather than inventing
one.

### Where it refuses to guess

- **newlib** is a collection of licences, not one identifier. alloy reports the
  path to the licence text the toolchain ships and asserts nothing.
- **A source file belonging to no declared package is not dropped.** It becomes
  a component with an undeclared licence, pointing at the nearest licence file
  found by walking up the tree. `--strict` turns any undeclared licence into a
  non-zero exit — useful as a release gate once you have decided what your
  policy is for the toolchain runtime.
- **Scope is the firmware image.** alloy's Python build tooling (pyyaml,
  jsonschema, cryptography, …) runs on the build host, ships in no device and is
  deliberately out of scope. Use `uv export` against `tools/alloy` for that
  question — and say which one you mean when legal asks.

The repository also carries a static [`NOTICE`](https://github.com/Alloy-Embedded/alloy/blob/main/NOTICE)
covering every third-party tree in the source distribution, whether or not a
given image compiles it. A unit test fails if it stops matching the versions
read from the vendored trees.
