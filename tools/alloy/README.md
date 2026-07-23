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
`alloy-devices` (the curated register/chip database). Verbs: `new`,
`boards [--json]`, `gen`, `build`, `flash`, `monitor`, `run`, `clean`,
`set-board`, `setup`, `debug-info [--json]`.

VS Code extension: [alloy-vscode](https://github.com/Alloy-Embedded/alloy-vscode).
