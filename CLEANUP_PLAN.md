# Cleanup Plan - Old Build/Flash Scripts

Now that we have the unified `ucore` CLI, many old scripts are obsolete.

## 🗑️ Scripts to Remove (Obsolete)

These are replaced by `./ucore build/flash`:

### Root Directory
- ❌ **alloy.py** - Old CLI, replaced by `ucore`

### scripts/
- ❌ **build-and-flash.sh** - Replaced by `./ucore flash <board> <example>`
- ❌ **flash_blink_led.sh** - Replaced by `./ucore flash`
- ❌ **flash_with_bossa.sh** - Board-specific flash, replaced by `ucore` auto-detection
- ❌ **flash_pico.sh** - Board-specific flash, replaced by `ucore`
- ❌ **test-all-builds.sh** - Replaced by `./scripts/test-all-boards.sh`
- ❌ **same70-set-boot-flash.sh** - Board-specific, can integrate into `ucore` if needed

### scripts/testing/
- ❌ **validate_esp32_builds.sh** - ESP32 not currently supported
- ❌ **run_unit_tests.sh** - Use `make test` or CMake directly
- ❌ **run_all_tests.sh** - Use `make test-all`

## ✅ Scripts to Keep (Still Useful)

### Root Directory
- ✅ **setup-dev-env.sh** - Initial development environment setup
- ✅ **install_mac.sh** - Initial macOS setup

### scripts/
- ✅ **install-xpack-toolchain.sh** - ARM toolchain installation (needed before using `ucore`)
- ✅ **test-all-boards.sh** - Comprehensive board testing (can coexist with `ucore`)
- ✅ **install_drivers.sh** - Driver installation (ST-Link, etc.)
- ✅ **start_devcontainer.sh** - DevContainer support
- ✅ **setup_esp_idf.sh** - Future ESP32 support

## 🔄 Replacement Guide

| Old Method | New Method |
|------------|------------|
| `./alloy.py build preset` | `./ucore build <board> <example>` |
| `./alloy.py flash preset` | `./ucore flash <board> <example>` |
| `./scripts/build-and-flash.sh same70 blink` | `./ucore flash same70_xplained blink` |
| `./scripts/flash_blink_led.sh` | `./ucore flash <board> blink` |
| `./scripts/test-all-builds.sh` | `./scripts/test-all-boards.sh` |

## 📝 Action Items

1. **Move to archive:** Create `scripts/archive/` and move obsolete scripts there (don't delete yet, just in case)
2. **Update documentation:** Remove references to old scripts
3. **Add deprecation notice:** Add README in archive explaining why scripts were moved
4. **Test:** Ensure all examples can be built/flashed with `ucore` only

## 🎯 Final State

After cleanup, users should only need:

**Initial Setup:**
```bash
./setup-dev-env.sh                    # Dev environment
./scripts/install-xpack-toolchain.sh  # ARM toolchain
```

**Daily Development:**
```bash
./ucore list boards                   # Discover boards
./ucore list examples                 # Discover examples
./ucore build <board> <example>       # Build
./ucore flash <board> <example>       # Flash
```

**Testing:**
```bash
./scripts/test-all-boards.sh          # Test all boards
make test                             # Unit tests
```

That's it! Much simpler.
