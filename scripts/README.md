# MicroCore Scripts

Essential scripts for MicroCore development.

## 🚀 Quick Start Scripts

These scripts help with initial setup and development environment configuration.

### `install-xpack-toolchain.sh`
**Purpose:** Install ARM GCC toolchain for embedded development

**When to use:** First time setup, before building any embedded examples

```bash
./scripts/install-xpack-toolchain.sh
```

**What it does:**
- Downloads xPack ARM toolchain (arm-none-eabi-gcc)
- Installs to `~/.local/xpack-arm-toolchain/`
- Includes newlib and full C/C++ standard library support

**After installation:**
```bash
export PATH="$HOME/.local/xpack-arm-toolchain/bin:$PATH"
```

### `install_drivers.sh`
**Purpose:** Install ST-Link and other debugging tools

**When to use:** First time setup on new machine

```bash
./scripts/install_drivers.sh
```

**What it does:**
- Installs ST-Link tools (st-flash, st-info)
- Installs OpenOCD for debugging
- Configures USB permissions (Linux)

### `test-all-boards.sh`
**Purpose:** Test building examples on all supported boards

**When to use:**
- Verify cross-platform compatibility
- CI/CD testing
- After making HAL changes

```bash
# Build only (no hardware needed)
./scripts/test-all-boards.sh

# Interactive flash mode (prompts for each board)
./scripts/test-all-boards.sh flash
```

**What it does:**
- Builds blink example for all 5 supported boards
- Reports success/failure and binary sizes
- Optional: Flash to connected hardware

**Supported boards:**
- nucleo_f401re (STM32F401RE)
- nucleo_f722ze (STM32F722ZE)
- nucleo_g071rb (STM32G071RB)
- nucleo_g0b1re (STM32G0B1RE)
- same70_xplained (ATSAME70Q21B)

## 🐳 DevContainer Scripts

### `start_devcontainer.sh`
**Purpose:** Start development in Docker container

**When to use:** If you prefer containerized development

```bash
./scripts/start_devcontainer.sh
```

**What it provides:**
- Pre-configured development environment
- All tools pre-installed
- Consistent across different host systems

## 🔮 Future Platform Scripts

### `setup_esp_idf.sh`
**Purpose:** Setup ESP32 development environment (ESP-IDF)

**Status:** Reserved for future ESP32 support

**When to use:** Not yet - ESP32 support coming in future release

## 📦 Archived Scripts

Old build/flash scripts have been moved to `archive/` directory. See [archive/README.md](archive/README.md) for details.

**Why archived?** Replaced by the unified `ucore` CLI:
```bash
./ucore build <board> <example>
./ucore flash <board> <example>
```

## 🎯 Common Workflows

### First Time Setup
```bash
# 1. Install ARM toolchain
./scripts/install-xpack-toolchain.sh

# 2. Install flash tools
./scripts/install_drivers.sh

# 3. Add to PATH
export PATH="$HOME/.local/xpack-arm-toolchain/bin:$PATH"
```

### Daily Development
```bash
# Use the ucore CLI (not scripts)
./ucore list boards
./ucore build nucleo_f401re blink
./ucore flash nucleo_f401re blink
```

### Testing/CI
```bash
# Test all boards
./scripts/test-all-boards.sh

# Unit tests
make test
```

## 📚 See Also

- **Main CLI:** [`../ucore`](../ucore) - Unified build and flash tool
- **Quick Start:** [../QUICKSTART.md](../QUICKSTART.md) - Complete getting started guide
- **CLI Migration:** [../docs/CLI_MIGRATION.md](../docs/CLI_MIGRATION.md) - Migrate from old scripts
- **Flash Troubleshooting:** [../docs/FLASH_TROUBLESHOOTING.md](../docs/FLASH_TROUBLESHOOTING.md) - Fix flash issues

## ❓ Questions?

**Q: Where are the build/flash scripts?**
A: Replaced by `./ucore` CLI. See [CLI Migration Guide](../docs/CLI_MIGRATION.md).

**Q: How do I flash my board?**
A: Use `./ucore flash <board> <example>`. See [QUICKSTART.md](../QUICKSTART.md).

**Q: Can I still use the old scripts?**
A: Yes, they're in `archive/` but the `ucore` CLI is recommended.
