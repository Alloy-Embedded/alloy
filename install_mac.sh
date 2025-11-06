#!/bin/bash
# ==============================================================================
# Alloy Framework - macOS Installation Script
# ==============================================================================
# This script installs all dependencies needed to build embedded projects
# for ARM Cortex-M microcontrollers on macOS.
#
# Supported platforms:
# - SAME70/SAMV71 (Atmel/Microchip)
# - SAMD21 (Atmel/Microchip)
# - STM32 (ST Microelectronics)
# - RP2040 (Raspberry Pi)
# - ESP32 (Espressif)
#
# Requirements:
# - macOS (Apple Silicon or Intel)
# - Homebrew package manager
# ==============================================================================

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print functions
print_header() {
    echo ""
    echo "================================================================================"
    echo -e "${BLUE}$1${NC}"
    echo "================================================================================"
    echo ""
}

print_step() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[i]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_header "Alloy Framework - macOS Installation"

print_info "Installation directory: $SCRIPT_DIR"
print_info "macOS version: $(sw_vers -productVersion)"
print_info "Architecture: $(uname -m)"

# ==============================================================================
# 1. Check for Homebrew
# ==============================================================================

print_header "Step 1/6: Checking Homebrew"

if command -v brew &> /dev/null; then
    print_step "Homebrew is already installed"
    brew --version
else
    print_warning "Homebrew not found. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    # Add Homebrew to PATH for Apple Silicon
    if [[ $(uname -m) == "arm64" ]]; then
        echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
        eval "$(/opt/homebrew/bin/brew shellenv)"
    fi

    print_step "Homebrew installed successfully"
fi

# Update Homebrew
print_info "Updating Homebrew..."
brew update

# ==============================================================================
# 2. Install ARM Toolchain
# ==============================================================================

print_header "Step 2/6: Installing ARM Toolchain"

if command -v arm-none-eabi-gcc &> /dev/null; then
    print_step "ARM toolchain is already installed"
    arm-none-eabi-gcc --version | head -1
else
    print_info "Installing ARM GCC toolchain..."
    brew install arm-none-eabi-gcc
    print_step "ARM toolchain installed successfully"
fi

# Verify ARM toolchain components
print_info "Verifying ARM toolchain components..."
for tool in arm-none-eabi-gcc arm-none-eabi-g++ arm-none-eabi-objcopy arm-none-eabi-size arm-none-eabi-gdb; do
    if command -v $tool &> /dev/null; then
        print_step "$tool: $(command -v $tool)"
    else
        print_error "$tool not found!"
        exit 1
    fi
done

# ==============================================================================
# 3. Install CMake
# ==============================================================================

print_header "Step 3/6: Installing CMake"

if command -v cmake &> /dev/null; then
    print_step "CMake is already installed"
    cmake --version | head -1
else
    print_info "Installing CMake..."
    brew install cmake
    print_step "CMake installed successfully"
fi

# ==============================================================================
# 4. Install Additional Build Tools
# ==============================================================================

print_header "Step 4/6: Installing Additional Build Tools"

TOOLS=(
    "ninja"      # Fast build system
    "ccache"     # Compiler cache for faster rebuilds
    "git"        # Version control
    "python3"    # For code generation scripts
)

for tool in "${TOOLS[@]}"; do
    if brew list $tool &> /dev/null; then
        print_step "$tool is already installed"
    else
        print_info "Installing $tool..."
        brew install $tool
        print_step "$tool installed successfully"
    fi
done

# ==============================================================================
# 5. Install Python Dependencies
# ==============================================================================

print_header "Step 5/6: Installing Python Dependencies"

print_info "Installing Python packages for code generation..."

# Check if pip is available
if ! command -v pip3 &> /dev/null; then
    print_error "pip3 not found. Please install Python 3"
    exit 1
fi

# Install required Python packages
PYTHON_PACKAGES=(
    "jinja2"        # Template engine for code generation
    "pyyaml"        # YAML parser for configuration
    "lxml"          # XML parser for SVD files
    "click"         # CLI framework
    "colorama"      # Colored terminal output
)

for package in "${PYTHON_PACKAGES[@]}"; do
    print_info "Installing $package..."
    pip3 install --user $package --quiet
done

print_step "Python dependencies installed"

# ==============================================================================
# 6. Install Flash/Debug Tools (Optional)
# ==============================================================================

print_header "Step 6/6: Installing Flash/Debug Tools (Optional)"

print_info "These tools are optional but recommended for flashing and debugging:"
echo ""
echo "  For SAME70/SAMV71/SAMD21:"
echo "    - OpenOCD (open source debugger)"
echo "    - SEGGER J-Link (commercial, but free for eval boards)"
echo ""
echo "  For STM32:"
echo "    - OpenOCD"
echo "    - ST-Link tools"
echo ""
echo "  For RP2040:"
echo "    - picotool"
echo "    - OpenOCD"
echo ""
echo "  For ESP32:"
echo "    - esptool.py (Python tool)"
echo ""

read -p "Install OpenOCD? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    brew install openocd
    print_step "OpenOCD installed"
fi

read -p "Install picotool (for RP2040)? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    brew install picotool
    print_step "picotool installed"
fi

read -p "Install esptool (for ESP32)? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    pip3 install --user esptool
    print_step "esptool installed"
fi

# ==============================================================================
# Installation Summary
# ==============================================================================

print_header "Installation Complete!"

echo "✅ Installed Components:"
echo ""
echo "  Build Tools:"
echo "    - ARM GCC Toolchain:  $(arm-none-eabi-gcc --version | head -1 | awk '{print $NF}')"
echo "    - CMake:              $(cmake --version | head -1 | awk '{print $NF}')"
echo "    - Ninja:              $(ninja --version)"
echo "    - ccache:             $(ccache --version | head -1 | awk '{print $NF}')"
echo ""
echo "  Python:"
echo "    - Python 3:           $(python3 --version | awk '{print $NF}')"
echo "    - pip3:               $(pip3 --version | awk '{print $2}')"
echo ""

if command -v openocd &> /dev/null; then
    echo "  Debug Tools:"
    echo "    - OpenOCD:            $(openocd --version 2>&1 | head -1 | awk '{print $4}')"
fi

if command -v picotool &> /dev/null; then
    echo "    - picotool:           $(picotool version | head -1)"
fi

if command -v esptool.py &> /dev/null; then
    echo "    - esptool:            $(esptool.py version | head -1)"
fi

echo ""
echo "================================================================================"
echo "🎉 Ready to build!"
echo "================================================================================"
echo ""
echo "Next steps:"
echo ""
echo "  1. Test SAME70 blink example:"
echo "     cd examples/same70_blink/build"
echo "     cmake .."
echo "     make"
echo ""
echo "  2. Regenerate all GPIO files:"
echo "     cd tools/codegen"
echo "     ./regenerate_all_gpio.sh"
echo ""
echo "  3. Read the documentation:"
echo "     - README.md (project overview)"
echo "     - tools/codegen/PIN_GPIO_ALL_COMPLETE.md (GPIO improvements)"
echo "     - examples/same70_blink/README.md (SAME70 example)"
echo ""
echo "================================================================================"
echo ""
