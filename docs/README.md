# MicroCore Documentation

Welcome to the MicroCore Framework documentation!

## Quick Links

- **[API Reference](api/html/index.html)** - Complete API documentation (Doxygen)
- **[Getting Started](getting_started.md)** - Build your first application
- **[Adding a Board](adding_board.md)** - Create custom board configuration
- **[Porting Platform](porting_platform.md)** - Port to new MCU platform
- **[C++20 Concepts](concepts.md)** - Concept reference guide
- **[Board Configuration](BOARD_CONFIGURATION.md)** - YAML configuration guide
- **[Host Platform Testing](HOST_PLATFORM_TESTING.md)** - Unit testing guide
- **[API Stability](API_STABILITY.md)** - Versioning and compatibility

## Documentation Overview

### For Beginners

Start here if you're new to MicroCore:

1. [Getting Started](getting_started.md) - Install, build, and flash your first app
2. [Examples](../examples/) - Browse example projects
3. [Board Configuration](BOARD_CONFIGURATION.md) - Configure boards with YAML

### For Application Developers

Building applications with MicroCore:

- [API Reference](api/html/index.html) - Complete class/function documentation
- [C++20 Concepts](concepts.md) - Type-safe interfaces
- [Host Testing](HOST_PLATFORM_TESTING.md) - Test without hardware

### For Platform Developers

Adding support for new platforms:

- [Adding a Board](adding_board.md) - Create board configuration
- [Porting Platform](porting_platform.md) - Implement hardware policies
- [Contributing](../CONTRIBUTING.md) - Contribution guidelines

## Building Documentation

### Prerequisites

```bash
# macOS
brew install doxygen graphviz

# Linux
sudo apt-get install doxygen graphviz
```

### Generate

```bash
# From project root
doxygen Doxyfile

# Or use CMake
cmake -B build
cmake --build build --target docs
```

Output: `docs/api/html/index.html`

## Documentation Structure

```
docs/
├── README.md                    # This file
├── mainpage.md                  # Doxygen main page
├── getting_started.md           # Tutorial
├── adding_board.md              # Board guide
├── porting_platform.md          # Platform guide
├── concepts.md                  # C++20 concepts
├── BOARD_CONFIGURATION.md       # YAML guide
├── HOST_PLATFORM_TESTING.md     # Testing guide
├── API_STABILITY.md             # Versioning policy
├── api/                         # Generated docs
│   └── html/
│       └── index.html          # API reference
└── images/                      # Diagrams
```

## Viewing Locally

```bash
# Open in browser
open docs/api/html/index.html  # macOS
xdg-open docs/api/html/index.html  # Linux
```

## GitHub Pages

Documentation is automatically published to GitHub Pages on push to `main`.

**URL:** https://lgili.github.io/corezero/

## Contributing to Documentation

See [Contributing Guide](../CONTRIBUTING.md) for:
- Documentation style guide
- How to add new pages
- Doxygen comment format
- Cross-referencing

## License

Documentation is licensed under MIT, same as the code.

---

**Questions?** Open an issue on [GitHub](https://github.com/lgili/corezero/issues)
