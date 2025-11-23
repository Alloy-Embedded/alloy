# Documentation Guide

This guide explains how MicroCore's automated documentation system works.

## Overview

MicroCore uses **Doxygen** to generate API documentation from source code comments. The documentation is automatically built and deployed to GitHub Pages on every commit to `main`.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  Source Code (src/**/*.hpp)                         │
│  + Doxygen Comments (/** ... */)                    │
└──────────────────┬──────────────────────────────────┘
                   │
                   ▼
          ┌────────────────────┐
          │   Doxyfile          │
          │   Configuration     │
          └─────────┬──────────┘
                    │
                    ▼
          ┌─────────────────────┐
          │   Doxygen            │
          │   Generator          │
          └─────────┬────────────┘
                    │
                    ▼
          ┌─────────────────────┐
          │   HTML Output        │
          │   (docs/api/html/)   │
          └─────────┬────────────┘
                    │
                    ▼
          ┌─────────────────────┐
          │   GitHub Pages       │
          │   (microcore.dev)    │
          └──────────────────────┘
```

## Local Documentation Build

### Prerequisites

```bash
# macOS
brew install doxygen graphviz

# Linux
sudo apt-get install doxygen graphviz

# Windows
choco install doxygen.install graphviz
```

### Build Documentation

```bash
# Generate HTML documentation
doxygen Doxyfile

# Output will be in: docs/api/html/index.html
open docs/api/html/index.html  # macOS
xdg-open docs/api/html/index.html  # Linux
```

### Using CMake

```bash
# Configure and build
cmake -B build
cmake --build build --target docs

# Open documentation
open docs/api/html/index.html
```

## Writing Documentation

### Basic Doxygen Comments

```cpp
/**
 * @brief Short description (one line)
 *
 * Detailed description can span multiple lines.
 * Use markdown for formatting.
 *
 * @param pin_num Pin number (0-31)
 * @param mode Pin mode (Input or Output)
 * @return Result<void, ErrorCode> Ok() on success
 *
 * @note Important implementation note
 * @warning Warning about usage
 *
 * Example:
 * @code
 * auto led = GpioPin<GPIOA_BASE, 5>{};
 * led.setDirection(PinDirection::Output);
 * led.set();
 * @endcode
 */
Result<void, ErrorCode> configure(uint8_t pin_num, PinMode mode);
```

### Documentation Tags

| Tag | Usage | Example |
|-----|-------|---------|
| `@brief` | One-line summary | `@brief Initialize GPIO pin` |
| `@param` | Parameter description | `@param pin_num Pin number` |
| `@return` | Return value | `@return true if success` |
| `@note` | Implementation note | `@note Thread-safe` |
| `@warning` | Important warning | `@warning Not atomic` |
| `@code` | Code example | `@code ... @endcode` |
| `@see` | Cross-reference | `@see GpioPin::set()` |
| `@tparam` | Template parameter | `@tparam T Pin type` |

### Markdown Support

Doxygen supports Markdown in documentation comments:

```cpp
/**
 * @brief Configure GPIO with advanced options
 *
 * ## Features
 * - **Zero overhead**: Compiles to single instruction
 * - **Type safe**: Compile-time pin validation
 * - **Atomic**: Safe from interrupts
 *
 * ### Example Usage
 * ```cpp
 * using Led = GpioPin<GPIOA_BASE, 5>;
 * Led led;
 * led.set();  // Turn LED on
 * ```
 *
 * See [GPIO Concepts](concepts.md#gpiopin) for details.
 */
```

## Automated Documentation (CI/CD)

### GitHub Actions Workflow

The `.github/workflows/documentation.yml` workflow:

1. **Triggers** on:
   - Push to `main` or `develop`
   - Pull requests affecting docs or source code
   - Manual dispatch

2. **Jobs**:
   - `build-docs`: Generates HTML documentation
   - `check-docs`: Checks for documentation warnings
   - `generate-changelog`: Auto-generates CHANGELOG.md
   - `link-check`: Validates all documentation links

3. **Deployment**:
   - Automatically deploys to GitHub Pages on `main` branch
   - Available at: `https://yourusername.github.io/corezero/`
   - Custom domain: `microcore.dev` (if configured)

### Setup GitHub Pages

1. **Enable GitHub Pages**:
   ```
   Repository → Settings → Pages
   Source: GitHub Actions
   ```

2. **Configure Custom Domain** (optional):
   ```
   Settings → Pages → Custom domain
   Enter: microcore.dev
   ```

3. **Update DNS** (if using custom domain):
   ```
   Type: CNAME
   Name: microcore.dev
   Value: yourusername.github.io
   ```

## Documentation Structure

```
docs/
├── mainpage.md              # Landing page
├── getting_started.md       # Tutorial
├── adding_board.md          # Board creation guide
├── porting_platform.md      # Platform porting guide
├── concepts.md              # C++20 concepts reference
├── API_STABILITY.md         # Versioning policy
├── README.md                # Documentation index
├── doxygen-theme/
│   └── custom.css           # Custom styling
└── api/
    └── html/                # Generated documentation (git-ignored)
        └── index.html
```

## Customization

### Theme Customization

Edit `docs/doxygen-theme/custom.css` to customize:

- **Colors**: Modify CSS variables in `:root`
- **Typography**: Change font families and sizes
- **Layout**: Adjust spacing and widths
- **Dark mode**: Customize `@media (prefers-color-scheme: dark)`

Example color change:

```css
:root {
    --primary-color: #ff6600;  /* Change from blue to orange */
    --success: #00aa00;        /* Brighter green */
}
```

### Doxyfile Configuration

Key settings in `Doxyfile`:

```
PROJECT_NAME           = "MicroCore"
PROJECT_NUMBER         = "0.1.0"
OUTPUT_DIRECTORY       = docs/api
INPUT                  = src include docs
RECURSIVE              = YES
GENERATE_TREEVIEW      = YES      # Sidebar navigation
HTML_EXTRA_STYLESHEET  = docs/doxygen-theme/custom.css
HAVE_DOT               = YES      # Enable diagrams
```

## Best Practices

### 1. Document Public APIs First

Focus on documenting:
- ✅ Public classes and functions
- ✅ Template parameters
- ✅ Return values and error conditions
- ✅ Usage examples

Skip:
- ❌ Private implementation details
- ❌ Obvious getters/setters (unless special behavior)

### 2. Use Examples

Every non-trivial function should have a usage example:

```cpp
/**
 * @brief Toggle GPIO pin state
 *
 * Example:
 * @code
 * using Led = GpioPin<GPIOA_BASE, 5>;
 * auto led = Led{};
 * led.setDirection(PinDirection::Output);
 * led.toggle();  // Changes state
 * @endcode
 */
```

### 3. Cross-Reference Related APIs

```cpp
/**
 * @brief Set pin HIGH
 *
 * @see clear() to set pin LOW
 * @see toggle() to change state
 * @see write(bool) for conditional setting
 */
```

### 4. Document Performance Characteristics

```cpp
/**
 * @brief Atomic set operation
 *
 * @note Zero overhead - compiles to single STR instruction
 * @note Thread-safe - uses BSRR atomic register
 * @note Interrupt-safe - no read-modify-write
 */
```

### 5. Keep It Updated

- Update documentation when changing APIs
- Run `doxygen` locally before committing
- Check for warnings in CI logs

## Troubleshooting

### Documentation Not Generating

```bash
# Check Doxygen installation
doxygen --version

# Check for errors
doxygen Doxyfile 2>&1 | grep "error:"

# Verify output directory exists
ls -la docs/api/html/
```

### Warnings in CI

```bash
# Generate warnings locally
doxygen Doxyfile 2>&1 | tee warnings.txt

# Common warnings:
# - "Warning: no uniquely matching class member found"
#   → Fix: Check function signature matches declaration

# - "Warning: unable to resolve reference"
#   → Fix: Check @see references are correct
```

### GitHub Pages Not Updating

1. Check workflow status: `Actions` tab in GitHub
2. Verify Pages is enabled: `Settings → Pages`
3. Check deploy logs for errors
4. Try manual workflow dispatch

### Custom CSS Not Applied

```bash
# Verify CSS file exists
ls -la docs/doxygen-theme/custom.css

# Check Doxyfile setting
grep HTML_EXTRA_STYLESHEET Doxyfile

# Regenerate docs
rm -rf docs/api/html
doxygen Doxyfile
```

## Versioned Documentation

Future enhancement (Phase 4.1 complete):

```
docs/
├── v1.0/
│   └── html/
├── v1.1/
│   └── html/
└── latest/  → symlink to newest version
```

Implementation using git tags + workflows.

## Related Links

- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [Markdown in Doxygen](https://www.doxygen.nl/manual/markdown.html)
- [GitHub Pages Guide](https://docs.github.com/en/pages)
- [Keep a Changelog](https://keepachangelog.com/)

---

**Questions?** Open an issue or check the [Contributing Guide](../CONTRIBUTING.md).
