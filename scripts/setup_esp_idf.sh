#!/bin/bash
# Automatic ESP-IDF Setup for Alloy
# This script downloads, installs, and configures ESP-IDF seamlessly

set -e

echo "═══════════════════════════════════════════════════"
echo "  ESP-IDF Automatic Setup for Alloy"
echo "═══════════════════════════════════════════════════"
echo ""

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Configuration
ESP_IDF_VERSION="v5.1.2"  # Stable version
ESP_IDF_DIR="${PROJECT_ROOT}/external/esp-idf"

echo "📁 Project: $PROJECT_ROOT"
echo "📦 ESP-IDF Version: $ESP_IDF_VERSION"
echo "📂 Install Location: $ESP_IDF_DIR"
echo ""

# Check if already installed
if [ -f "${ESP_IDF_DIR}/export.sh" ]; then
    echo "✅ ESP-IDF already installed at: $ESP_IDF_DIR"
    echo ""
    echo "To update ESP-IDF:"
    echo "  cd $ESP_IDF_DIR"
    echo "  git pull"
    echo "  git submodule update --recursive"
    echo "  ./install.sh esp32"
    echo ""

    read -p "Reinstall anyway? (y/N): " reinstall
    if [[ ! "$reinstall" =~ ^[Yy]$ ]]; then
        echo ""
        echo "Skipping installation. To use ESP-IDF:"
        echo "  source $ESP_IDF_DIR/export.sh"
        echo "  ./build-esp32.sh"
        exit 0
    fi

    echo ""
    echo "⚠️  Removing existing ESP-IDF..."
    rm -rf "$ESP_IDF_DIR"
fi

# Create external directory
mkdir -p external
cd external

echo "📥 Step 1/4: Downloading ESP-IDF $ESP_IDF_VERSION..."
echo "   This may take 5-10 minutes depending on your internet"
echo ""

# Clone ESP-IDF
if [ -d "esp-idf" ]; then
    echo "   Removing old clone..."
    rm -rf esp-idf
fi

git clone --recursive --branch "$ESP_IDF_VERSION" \
    https://github.com/espressif/esp-idf.git

cd esp-idf

echo ""
echo "✅ Download complete!"
echo ""

# Install prerequisites (macOS specific)
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "📦 Step 2/4: Installing macOS prerequisites..."
    echo ""

    # Check Homebrew
    if ! command -v brew &> /dev/null; then
        echo "⚠️  Homebrew not found. Installing..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    # Install dependencies
    echo "   Installing dependencies via Homebrew..."
    brew install cmake ninja dfu-util python3 || true

    echo "✅ Prerequisites installed!"
    echo ""
fi

# Install ESP-IDF Python environment and toolchains
echo "🔧 Step 3/4: Installing ESP-IDF tools..."
echo "   This will download Xtensa toolchain and Python packages"
echo "   May take 10-15 minutes..."
echo ""

./install.sh esp32

echo ""
echo "✅ ESP-IDF tools installed!"
echo ""

# Setup environment
echo "⚙️  Step 4/4: Configuring environment..."
echo ""

# Add export to build script
BUILD_SCRIPT="${PROJECT_ROOT}/build-esp32.sh"
if [ -f "$BUILD_SCRIPT" ]; then
    # Check if already has ESP-IDF export
    if ! grep -q "source.*esp-idf/export.sh" "$BUILD_SCRIPT"; then
        echo "   Updating build-esp32.sh with ESP-IDF environment..."

        # Backup original
        cp "$BUILD_SCRIPT" "${BUILD_SCRIPT}.backup"

        # Add export at the beginning
        cat > "$BUILD_SCRIPT.new" << 'EOF'
#!/bin/bash
# Auto-generated ESP-IDF environment setup

# Determine project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Setup ESP-IDF environment
if [ -f "${PROJECT_ROOT}/external/esp-idf/export.sh" ]; then
    echo "🔧 Setting up ESP-IDF environment..."
    source "${PROJECT_ROOT}/external/esp-idf/export.sh"
else
    echo "❌ ESP-IDF not found. Run ./scripts/setup_esp_idf.sh"
    exit 1
fi

# Original build script continues below...
EOF

        # Append rest of original script (skip shebang)
        tail -n +2 "$BUILD_SCRIPT" >> "$BUILD_SCRIPT.new"

        # Replace original
        mv "$BUILD_SCRIPT.new" "$BUILD_SCRIPT"
        chmod +x "$BUILD_SCRIPT"

        echo "✅ build-esp32.sh updated!"
    else
        echo "✅ build-esp32.sh already configured"
    fi
fi

# Create activation script
ACTIVATE_SCRIPT="${PROJECT_ROOT}/activate_esp_idf.sh"
cat > "$ACTIVATE_SCRIPT" << EOF
#!/bin/bash
# Activate ESP-IDF environment for current shell session

ESP_IDF_DIR="${ESP_IDF_DIR}"

if [ -f "\${ESP_IDF_DIR}/export.sh" ]; then
    echo "🔧 Activating ESP-IDF environment..."
    source "\${ESP_IDF_DIR}/export.sh"
    echo "✅ ESP-IDF environment ready!"
    echo ""
    echo "ESP-IDF Path: \$IDF_PATH"
    echo "Xtensa GCC: \$(xtensa-esp32-elf-gcc --version | head -1)"
    echo ""
    echo "You can now build ESP32 projects:"
    echo "  ./build-esp32.sh"
    echo "  or"
    echo "  cmake -B build -DALLOY_BOARD=esp32_devkit ..."
else
    echo "❌ ESP-IDF not found at: \$ESP_IDF_DIR"
    echo "   Run: ./scripts/setup_esp_idf.sh"
    exit 1
fi
EOF

chmod +x "$ACTIVATE_SCRIPT"

echo ""
echo "═══════════════════════════════════════════════════"
echo "  ✅ ESP-IDF Setup Complete!"
echo "═══════════════════════════════════════════════════"
echo ""
echo "📋 Installation Summary:"
echo "   ESP-IDF Version: $ESP_IDF_VERSION"
echo "   Location: $ESP_IDF_DIR"
echo "   Size: $(du -sh "$ESP_IDF_DIR" | cut -f1)"
echo ""
echo "🚀 Quick Start:"
echo ""
echo "   Option 1 - Use build script (easiest):"
echo "     ./build-esp32.sh"
echo ""
echo "   Option 2 - Activate environment manually:"
echo "     source ./activate_esp_idf.sh"
echo "     cmake -B build -DALLOY_BOARD=esp32_devkit \\"
echo "         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake"
echo "     cmake --build build"
echo ""
echo "   Option 3 - One-liner per session:"
echo "     source $ESP_IDF_DIR/export.sh"
echo ""
echo "📖 Documentation:"
echo "   ESP-IDF: https://docs.espressif.com/projects/esp-idf/"
echo "   Alloy ESP32: docs/ESP32_QUICK_START.md"
echo ""
echo "═══════════════════════════════════════════════════"
