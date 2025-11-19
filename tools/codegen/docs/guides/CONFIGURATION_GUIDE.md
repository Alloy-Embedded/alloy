# Alloy CLI Configuration Guide

This guide explains how to configure the Alloy CLI using `.alloy.yaml` configuration files.

## Table of Contents

- [Overview](#overview)
- [Configuration Hierarchy](#configuration-hierarchy)
- [Configuration File Locations](#configuration-file-locations)
- [Configuration Options](#configuration-options)
- [Environment Variables](#environment-variables)
- [Common Workflows](#common-workflows)
- [Examples](#examples)

---

## Overview

Alloy CLI supports hierarchical configuration through YAML files and environment variables. This allows you to:

- Set global defaults for all projects (user config)
- Override settings per-project (project config)
- Temporarily override any setting via CLI arguments or environment variables

**Benefits**:
- ✅ No need to pass the same flags repeatedly
- ✅ Consistent settings across team members (project config in git)
- ✅ Personal preferences (user config in home directory)
- ✅ Easy to override when needed

---

## Configuration Hierarchy

Configuration values are merged in the following priority order (highest to lowest):

1. **CLI Arguments** (highest priority)
   ```bash
   alloy list mcus --vendor st  # Overrides all config files
   ```

2. **Environment Variables**
   ```bash
   export ALLOY_VERBOSE=1
   alloy list mcus  # Will be verbose
   ```

3. **Project Config** (`.alloy.yaml` in current directory)
   ```yaml
   discovery:
     default_vendor: st
   ```

4. **User Config** (`~/.config/alloy/config.yaml`)
   ```yaml
   general:
     editor: code
   ```

5. **System Config** (`/etc/alloy/config.yaml`)
   ```yaml
   paths:
     database: /usr/share/alloy/database
   ```

6. **Defaults** (lowest priority)
   - Built-in sensible defaults

---

## Configuration File Locations

### Project Config
**Path**: `.alloy.yaml` (in project root)
**Purpose**: Project-specific settings shared by team
**Recommended**: Commit to git

```yaml
project:
  name: my-project
  board: nucleo_f401re
```

### User Config
**Path**: `~/.config/alloy/config.yaml`
**Purpose**: Personal preferences
**Recommended**: Don't commit to git

```yaml
general:
  editor: vim
  verbose: true
```

### System Config
**Path**: `/etc/alloy/config.yaml`
**Purpose**: System-wide defaults
**Recommended**: Use for shared installations

```yaml
paths:
  database: /opt/alloy/database
```

---

## Configuration Options

### General Settings

```yaml
general:
  output_format: table  # table, json, yaml, compact
  color: true           # Enable colored output
  verbose: false        # Enable verbose logging
  editor: vim          # Editor for 'alloy config edit'
```

### Paths

```yaml
paths:
  database: ./database              # MCU/board metadata
  templates: ./database/templates   # Project templates
  svd: ./svd                       # SVD files
  output: ./generated              # Generated code output
```

**Notes**:
- Paths can be absolute or relative
- `~` expands to home directory
- Environment variables are supported: `${ALLOY_DATABASE_PATH:-./database}`

### Discovery Settings

```yaml
discovery:
  default_vendor: st        # Default vendor filter
  default_family: stm32f4   # Default MCU family
  show_deprecated: false    # Show deprecated MCUs/boards
  max_results: 50          # Maximum results in list commands
  default_sort: part_number # Default sort order
```

### Build Settings

```yaml
build:
  system: cmake                      # cmake or meson
  build_type: Debug                  # Debug, Release, RelWithDebInfo, MinSizeRel
  jobs: 0                           # Parallel jobs (0=auto)
  toolchain: null                    # Path to toolchain file
  cmake_generator: null              # CMake generator
```

### Validation Settings

```yaml
validation:
  strict: false                      # Enable strict validation mode
  syntax: true                       # Enable syntax validation
  semantic: true                     # Enable semantic validation
  compile: false                     # Enable compilation check
  clang_path: clang++               # Clang path for syntax validation
  gcc_arm_path: arm-none-eabi-gcc   # GCC ARM path for compilation
```

### Metadata Settings

```yaml
metadata:
  format: yaml           # yaml or json
  auto_format: true      # Auto-format on save
  include_comments: true # Include comments in generated metadata
```

### Project Settings

```yaml
project:
  name: my-project       # Project name
  board: nucleo_f401re   # Target board
  mcu: STM32F401RET6    # Or specify MCU directly
  peripherals:           # Configured peripherals
    - gpio
    - uart
    - i2c
  build_dir: build       # Build directory
```

---

## Environment Variables

All configuration options can be overridden via environment variables using the `ALLOY_` prefix:

### General
- `ALLOY_OUTPUT_FORMAT` → `general.output_format`
- `ALLOY_COLOR` → `general.color`
- `ALLOY_VERBOSE` → `general.verbose`
- `ALLOY_EDITOR` → `general.editor`

### Paths
- `ALLOY_DATABASE_PATH` → `paths.database`
- `ALLOY_TEMPLATES_PATH` → `paths.templates`
- `ALLOY_SVD_PATH` → `paths.svd`
- `ALLOY_OUTPUT_PATH` → `paths.output`

### Discovery
- `ALLOY_DEFAULT_VENDOR` → `discovery.default_vendor`
- `ALLOY_DEFAULT_FAMILY` → `discovery.default_family`
- `ALLOY_SHOW_DEPRECATED` → `discovery.show_deprecated`
- `ALLOY_MAX_RESULTS` → `discovery.max_results`

### Build
- `ALLOY_BUILD_SYSTEM` → `build.system`
- `ALLOY_BUILD_TYPE` → `build.build_type`
- `ALLOY_BUILD_JOBS` → `build.jobs`

### Validation
- `ALLOY_VALIDATION_STRICT` → `validation.strict`
- `ALLOY_CLANG_PATH` → `validation.clang_path`
- `ALLOY_GCC_ARM_PATH` → `validation.gcc_arm_path`

### Metadata
- `ALLOY_METADATA_FORMAT` → `metadata.format`

**Example**:
```bash
export ALLOY_VERBOSE=1
export ALLOY_DATABASE_PATH=/custom/path
alloy list mcus
```

---

## Common Workflows

### 1. Initialize New Project

```bash
# Create project directory
mkdir my-project && cd my-project

# Initialize project config
alloy config init

# Edit config
alloy config edit

# Set project board
alloy config set project.board nucleo_f401re
```

### 2. Set Personal Preferences

```bash
# Initialize user config
alloy config init --scope user

# Set your editor
alloy config set general.editor code --scope user

# Enable verbose output by default
alloy config set general.verbose true --scope user

# Set custom database path
alloy config set paths.database ~/alloy/database --scope user
```

### 3. View Current Configuration

```bash
# Show merged configuration
alloy config show

# Show only paths section
alloy config show --section paths

# Show user config only
alloy config show --scope user

# Show all config files
alloy config show --scope all
```

### 4. Temporary Overrides

```bash
# Use environment variable for one command
ALLOY_VERBOSE=1 alloy list mcus

# Use CLI argument
alloy list mcus --vendor st  # Overrides discovery.default_vendor
```

### 5. Team Configuration

**In git repository** (`.alloy.yaml`):
```yaml
project:
  name: team-project
  board: nucleo_f401re

discovery:
  default_vendor: st
  default_family: stm32f4
```

Commit this file so all team members get the same defaults.

---

## Examples

### Example 1: Embedded Developer Setup

**User config** (`~/.config/alloy/config.yaml`):
```yaml
general:
  editor: code
  verbose: true

paths:
  database: ~/embedded/alloy-db
  svd: ~/embedded/svd-files

build:
  jobs: 8
  build_type: Release

validation:
  gcc_arm_path: /opt/gcc-arm-none-eabi/bin/arm-none-eabi-gcc
```

### Example 2: STM32 Project

**Project config** (`.alloy.yaml`):
```yaml
project:
  name: stm32-usb-device
  board: nucleo_f401re
  peripherals:
    - gpio
    - uart
    - usb

discovery:
  default_vendor: st
  default_family: stm32f4

build:
  build_type: Debug
  cmake_generator: Ninja
```

### Example 3: Multi-Vendor Development

**Project config** (`.alloy.yaml`):
```yaml
project:
  name: multi-vendor-hal
  mcu: null  # No specific MCU, developing for multiple

discovery:
  show_deprecated: false
  max_results: 100

metadata:
  format: yaml
  include_comments: true
```

---

## CLI Commands Reference

### Initialize Config
```bash
alloy config init                    # Create .alloy.yaml in current dir
alloy config init --scope user       # Create user config
alloy config init --force            # Overwrite existing
```

### View Config
```bash
alloy config show                    # Show merged config
alloy config show --section paths   # Show specific section
alloy config show --scope user       # Show user config only
alloy config show --scope all        # Show all config files
```

### Set Values
```bash
alloy config set general.verbose true
alloy config set paths.database ./db --scope project
alloy config set discovery.default_vendor st --scope user
```

### Remove Values
```bash
alloy config unset general.verbose
alloy config unset paths.database --scope project
```

### Edit Config
```bash
alloy config edit                    # Edit user config
alloy config edit --scope project    # Edit project config
```

### Show Paths
```bash
alloy config path                    # Show all config file paths
alloy config path --scope user       # Show user config path
```

---

## Best Practices

1. **Use Project Config for Team Settings**
   - Commit `.alloy.yaml` to git
   - Include board, peripherals, build settings
   - Don't include personal preferences

2. **Use User Config for Personal Preferences**
   - Editor choice
   - Verbose output
   - Custom tool paths
   - Output format preferences

3. **Use Environment Variables for CI/CD**
   ```bash
   export ALLOY_BUILD_TYPE=Release
   export ALLOY_BUILD_JOBS=16
   alloy build compile
   ```

4. **Keep Configs Minimal**
   - Only set values that differ from defaults
   - Use comments to document why settings exist

5. **Version Control**
   - ✅ Commit: `.alloy.yaml` (project config)
   - ❌ Don't commit: User or system configs

---

## Troubleshooting

### Config Not Loading?

```bash
# Check which config files exist
alloy config path

# View merged configuration
alloy config show

# Enable verbose mode to see config loading
ALLOY_VERBOSE=1 alloy config show
```

### Value Not Taking Effect?

Check the hierarchy:
1. CLI arguments override everything
2. Environment variables override files
3. Project config overrides user config
4. User config overrides system config

```bash
# Debug by checking merged config
alloy config show --section general
```

### Invalid YAML Syntax?

```bash
# Edit config to fix syntax errors
alloy config edit

# Validate by trying to load
alloy config show
```

---

## See Also

- [YAML Metadata Guide](./YAML_METADATA_GUIDE.md)
- [CLI Commands Reference](../README.md)
- [Project Initialization Guide](./PROJECT_INIT_GUIDE.md)
