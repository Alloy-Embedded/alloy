# VS Code extension

The companion extension puts the whole workflow — scaffold, configure, build, flash, debug,
update — behind buttons, without hiding what it runs.

!!! info "The CLI is the brain"
    The extension holds **no domain logic**. Every fact it shows arrives through `alloy … --json`,
    and every action it takes is a CLI command you can run yourself. That is deliberate: a UI that
    reimplements the tool eventually disagrees with it.

## Install

The extension is not on the Marketplace yet. Build and install it from the
[`alloy-vscode`](https://github.com/Alloy-Embedded/alloy-vscode) repository:

```bash
npm install && npm run build && npx vsce package --no-dependencies
```

Then **Extensions → … → Install from VSIX**. It activates when a folder contains an `alloy.toml`.

It needs the CLI (≥ 0.1.0) on your `PATH`; for a source checkout, point `alloy.cliPath` at a
wrapper instead:

```json title=".vscode/settings.json"
{ "alloy.cliPath": "/path/to/alloy-wrapper" }
```

## Start a project

**Alloy: New Project** offers two paths:

=== "From a supported board"

    Pick vendor → board → name. You get the same scaffold `alloy new` produces: a portable
    blink-and-echo that already builds.

=== "Custom board — choose an MCU"

    Pick any chip in the device database. You get a clean, editable board under
    `boards/<name>/board.json` with the MCU and a boot-safe clock already set, and you fill in the
    pins yourself. This is the path for your own hardware.

## Configure the board visually

**Alloy: Configure Board (clock, pins)** opens a configurator in the style of a vendor pin
planner, except that every fact in it comes from the device database rather than from a
hand-maintained UI:

- **Pinout** — every pin the chip data knows, with the alternate functions actually routable on
  it. Click a pin, choose a function, give it a name; the name becomes a code alias.
- **System clock** — pick a profile, or enter a target frequency and let `alloy clock` solve the
  PLL for it.
- **Roles** — which of the framework's board roles this silicon can fill, and with what.
- **Problems** — validation runs on save and the results come back into the panel, so fixing a
  bad pin route is a loop instead of a reopen.

Two behaviours worth knowing:

!!! warning "Framework boards are read-only"
    Rewriting a curated board would change every project that uses it. Open one and you get a
    **Duplicate to edit** button, which clones it into your project with all its settings and
    switches you to the copy.

Saving does not close the panel — it writes `board.json`, re-runs validation, and shows you the
result of the edit you just made.

## Build, flash, debug

The side bar carries **Project**, **Memory**, **Libraries** and **Toolchains** views, and the
status bar shows the current board.

| Command | Runs |
| --- | --- |
| Alloy: Build / Flash / Run / Monitor / Clean | `alloy build` / `flash` / `run` / `monitor` / `clean` |
| Alloy: Pick Board | `alloy set-board` |
| Alloy: Debug | a Cortex-Debug session from `alloy debug-info` |
| Alloy: Generate Debug Configuration | writes `launch.json` |
| Alloy: Setup Environment / Install Missing Tools | `alloy setup` — in the terminal, visibly |

Builds are VS Code **tasks** of type `alloy` with a GCC problem matcher, so compile errors land in
the Problems panel and are clickable. IntelliSense needs no setup: `alloy build` emits
`compile_commands.json`.

The **Memory** view reports what the last build costs against the chip's real memories — flash and
RAM used, and whether the image fits its [update slot](firmware-update.md).

## Add a driver

The **Libraries** view browses the [driver registry](libraries.md) grouped by category. One click
vendors a driver into the project (`alloy lib add`) and wires the include path. Then
`#include <sht31.hpp>` and go.

## Update a device

**Alloy: Update Device (UART)** is the [firmware-update](firmware-update.md) path from the editor:
it picks a serial port, sends a packed image over the bootloader's update window, and reports what
the device answered. Behind it is exactly `alloy update` — the same client CI runs against
emulated hardware.

!!! note "The extension never downloads a toolchain by itself"
    Tool installation always runs `alloy setup` in a visible terminal. You see what is fetched,
    and the CLI still refuses any toolchain whose checksum is not pinned.
