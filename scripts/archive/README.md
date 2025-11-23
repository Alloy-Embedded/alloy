# Archived Scripts

These scripts have been replaced by the unified `ucore` CLI and are kept here for reference only.

## Why These Were Archived

The MicroCore project previously had multiple ways to build and flash examples:
- CMake presets with `alloy.py`
- Board-specific shell scripts
- Manual CMake commands
- Different flash tools for each board

This created confusion and complexity. The new `ucore` CLI provides **one simple interface** for all boards and examples.

## Archived Files

### `alloy.py`
**Replaced by:** `../../ucore`

The old Python CLI that used CMake presets. The new `ucore` CLI is simpler and doesn't require understanding presets.

**Old:**
```bash
./alloy.py build nucleo-f401re-release
./alloy.py flash nucleo-f401re-release
```

**New:**
```bash
./ucore build nucleo_f401re blink
./ucore flash nucleo_f401re blink
```

### `build-and-flash.sh`
**Replaced by:** `./ucore flash <board> <example>`

Board-specific build and flash script. No longer needed.

### `flash_*.sh` (flash_blink_led.sh, flash_with_bossa.sh, flash_pico.sh)
**Replaced by:** `./ucore flash <board> <example>`

Board-specific flash scripts. The `ucore` CLI now auto-detects the correct flash method for each board.

### `same70-set-boot-flash.sh`
**Replaced by:** Built into `ucore` flash logic

Board-specific boot configuration. Now handled automatically.

### `test-all-builds.sh`
**Replaced by:** `../test-all-boards.sh`

Updated version that works with the new build system.

### `testing/` directory
**Replaced by:** `make test`, `make test-all`

Unit test scripts. Use the Makefile targets or CMake/CTest directly instead.

## Migration Guide

See [../../docs/CLI_MIGRATION.md](../../docs/CLI_MIGRATION.md) for complete migration guide.

## Quick Reference

| Task | Old Method | New Method |
|------|------------|------------|
| Build example | `./alloy.py build <preset>` | `./ucore build <board> <example>` |
| Flash | `./scripts/flash_blink_led.sh` | `./ucore flash <board> blink` |
| List boards | _Not available_ | `./ucore list boards` |
| List examples | _Not available_ | `./ucore list examples` |
| Test all boards | `./scripts/test-all-builds.sh` | `./scripts/test-all-boards.sh` |

## Need These Scripts?

If you have a specific use case that requires these archived scripts, please:
1. Open an issue explaining your use case
2. Consider if the `ucore` CLI can be extended to support it
3. These scripts may be removed in a future release

## Learn More

- **Quick Start:** [../../QUICKSTART.md](../../QUICKSTART.md)
- **CLI Migration:** [../../docs/CLI_MIGRATION.md](../../docs/CLI_MIGRATION.md)
- **Flash Troubleshooting:** [../../docs/FLASH_TROUBLESHOOTING.md](../../docs/FLASH_TROUBLESHOOTING.md)
