# alloy-embedded

The CLI for the [Alloy framework](https://github.com/Alloy-Embedded/alloy):
one portable C++23 app source, any supported microcontroller — STM32 (G0/F7),
SAM E70, RP2040, ESP32 — with zero `#ifdef`s.

```
uv tool install alloy-embedded      # installs the `alloy` command
alloy setup                          # verify/install toolchains (PATH-first)
alloy new blinky --board nucleo_g071rb
cd blinky && alloy run               # build + flash + serial monitor
```

The wheel embeds the framework (C++ runtime, board data) and depends on
`alloy-devices` (the curated register/chip database).

Verbs, by task:

- **start** — `new`, `boards`, `chips`, `set-board`, `setup`
- **build & run** — `build`, `flash`, `monitor`, `run`, `gen`, `clean`, `test`
- **inspect** — `size`, `matrix`, `frame-audit`, `svd`, `debug-info`
- **configure** — `board-info`, `board-validate`, `board-clone`, `chip-info`, `clock`
- **emulate** — `emulate` (headless Renode on a data-generated platform)
- **field update** — `keygen`, `image`, `ports`, `update` (A/B slots, trial boot with
  rollback, optional Ed25519-signed images)
- **libraries** — `lib list | search | info | add`

Most take `--json` for editor integration (versioned envelopes).

Docs: <https://alloy-embedded.github.io/alloy/> ·
VS Code extension: [alloy-vscode](https://github.com/Alloy-Embedded/alloy-vscode).
